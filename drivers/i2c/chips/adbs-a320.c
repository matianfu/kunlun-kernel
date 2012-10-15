#include <linux/version.h>
#include <linux/module.h>
#include <linux/earlysuspend.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <mach/gpio.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/i2c/adbs-a320.h>
#include <linux/hrtimer.h>
#include <linux/timer.h>

#define POWER_ENABLE	1
#define POWER_DISABLE	0
#define ADBS_A320_ID    (0x33)
#define MS_POOL_PERIOD   (300*1000*1000)  //us

static int adbs_a320_check_FPD(struct i2c_client *client);
static int adbs_a320_remove(struct i2c_client *client);
static int adbs_a320_probe(struct i2c_client *client,
                            const struct i2c_device_id *id);
static int adbs_a320_detect(struct i2c_client *client, int kind,
                            struct i2c_board_info *info);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void adbs_a320_early_suspend(struct early_suspend *h);
static void adbs_a320_late_resume(struct early_suspend *h);
#endif

static const struct i2c_device_id adbs_a320_id[] = {
    {"adbs_a320", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, adbs_a320_id);


I2C_CLIENT_INSMOD_1(adbs_a320);

static struct i2c_driver adbs_a320_driver = {
    .driver = {
        .name = "adbs_a320",
        .owner = THIS_MODULE,
    },
    .class          = I2C_CLASS_HWMON,
    .probe          = adbs_a320_probe,
    .remove         = adbs_a320_remove,
    .id_table       = adbs_a320_id,
    .detect         = adbs_a320_detect,
};

struct adbs_a320 {
    struct input_dev        *input;
    struct i2c_client       *dev_client;
    struct adbs_a320_platform_data  *dev_platform_data;
    struct hrtimer	    timer;
    struct work_struct      work;
    void (*dev_regs_init)(struct i2c_client *ms);
    void (*dev_set_sensitivity)(struct i2c_client *ms, u8 level);
    int (*dev_selftest)(struct i2c_client *ms);
    int (*dev_checkid)(struct i2c_client *ms);
    int (*dev_read_reg)(struct i2c_client *ms, int reg);
    int (*dev_write_reg)(struct i2c_client *ms, int reg, int val);
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend early_suspend;
#endif
    spinlock_t		lock;
    int             irq;
    int             power_state;
    char            phys[32];
    unsigned char   keycode[OFN_KEY_NUM];
};

u8 irq_run = 1;

static struct workqueue_struct *adbs_a320_wq;

/*Local Variables*/
static unsigned char finger_status = 0;
static int delta_x = 0, delta_y = 0;
static unsigned char key_step = 0;

static unsigned char min_step = 2; // 2
static unsigned char step_tilt = 13; // 15

static long disturbance = 0;
/* Sensitivity */
static u8 speed_level = LEVEL_MIDDLE;
static u8 key_event_times = 2;
static ROCKER_THRESHOLD step_thresh[4] = {
                        {12,12},
                        {60,60},
                        {80,80},
                        {100,100}};

static unsigned char ofn_keycode[OFN_KEY_NUM] = {
    KEY_DOWN, KEY_UP, KEY_RIGHT, KEY_LEFT
};

/*static int wait_irq_down(void)
{
    int ret = 1000;

    while(ret--)
    {
        if(irq_run)
            udelay(10);
        else
            return ret;
    }

    return ret;
}*/


static int get_motion_pin_state(struct adbs_a320 *ms)
{
    if(!ms->dev_platform_data) {
        printk("get_motion_pin_state err\n");
        return 0;
    }

    return !gpio_get_value(ms->dev_platform_data->gpio_motion);
}

/*******************************************************************************
 Send OFN key code to system
*******************************************************************************/
static void ofn_key_event(struct adbs_a320 *ms, int dir)
{
    int ofn_key = 0xff;
   // static int last_key = -1;

    switch (dir)
    {
    case OFN_KEY_UP:
        ofn_key = DEVICE_KEY_UP;
        //printk("derection: DEVICE_KEY_UP\n");
        break;

    case OFN_KEY_DOWN:
        ofn_key = DEVICE_KEY_DOWN;
        //printk("derection: OFN_KEY_DOWN\n");
        break;

    case OFN_KEY_LEFT:
        ofn_key = DEVICE_KEY_LEFT;
        //printk("derection: DEVICE_KEY_LEFT\n");
        break;

    case OFN_KEY_RIGHT:
        ofn_key = DEVICE_KEY_RIGHT;
        //printk("derection: DEVICE_KEY_RIGHT\n");
        break;
    }

    //if((ofn_key != 0xFF) && (last_key == ofn_key)){
    if(ofn_key != 0xFF){
        input_report_key(ms->input, ms->keycode[ofn_key], 1);
        input_sync(ms->input);
        input_report_key(ms->input, ms->keycode[ofn_key], 0);
        input_sync(ms->input);
    }

    //last_key =  ofn_key;
}

/*Clear DeltaX and DeltaY buffer*/
void OFN_Reset_Status(void)
{
	delta_x  = 0;
	delta_y  = 0;
	key_step = 0;
	disturbance = 0;
}

/*Process delta X and delta Y according to the various operation modes*/
static unsigned char OFN_Data_Process(int x,int y)
{
	unsigned char cur_dir = OFN_KEY_NULL;

	int abs_x = abs(x);
	int abs_y = abs(y);

	//printk(" abs_x %d, abs_y %d\n",abs_x,abs_y);
	if ((abs_x-abs_y > min_step) && (abs_x*10 > abs_y*step_tilt))
	{
		if (x > 0)
		{
			cur_dir = OFN_KEY_LEFT;
		}
		else
		{
			cur_dir = OFN_KEY_RIGHT;
		}
	}
	else if ((abs_y-abs_x > min_step) && (abs_y*10 > abs_x*step_tilt))
	{
		if (y > 0)
		{
			cur_dir = OFN_KEY_DOWN;
		}
		else
		{
			cur_dir = OFN_KEY_UP;
		}
	}

	return cur_dir;
}


static void adbs_a320_work(struct work_struct *work)
{
	struct adbs_a320 *ms = container_of(work, struct adbs_a320, work);
	// Check if there is a motion occurrence on the sensor
	unsigned char motion;

	motion = i2c_smbus_read_byte_data(ms->dev_client,A320_MOTION);

	// Motion overflow, delta Y and/or X buffer has overflowed since last report
	if (motion & A320_MOTION_OVF)
	{
		printk("motion overflow!\n");

		// Clear the MOT and OVF bits, Delta_X and Delta_Y registers
		i2c_smbus_write_byte_data(ms->dev_client,A320_MOTION, 0x00);
	}
	else if (motion & A320_MOTION_MOT)
	{
		unsigned char ux, uy;

		if(!get_motion_pin_state(ms)) {
			enable_irq(ms->irq);
			printk(" adbs_a320 no data\n");
			return;     // no data
		} else {
			// Read delta_x and delta_y for reporting
			ux = i2c_smbus_read_byte_data(ms->dev_client,A320_DELTAX);
			uy = i2c_smbus_read_byte_data(ms->dev_client,A320_DELTAY);

			if (adbs_a320_check_FPD(ms->dev_client))
			{
				printk("check_FPD finger_status: %d\n", finger_status);
				if(disturbance > 1000)
					OFN_Reset_Status();
				i2c_smbus_write_byte_data(ms->dev_client,A320_MOTION, 0x00);
				hrtimer_start(&ms->timer, ktime_set(0, 120*1000*1000), HRTIMER_MODE_REL);
				return ;
			}

			delta_x = ux;//(char)ux;
			delta_y = uy;//(char)uy;

			//printk("ux %d ,uy %d; delta_x %d,delta_y %d\n",ux,uy,delta_x,delta_y);
			// bit7 indicates the moving direction
			if (ux & 0x80)
			{
				 delta_x -= 256;
			}

			if (uy & 0x80)
			{
				if (uy != 0x80)
				{
					delta_y -= 256;
				}
				else
				{
					delta_y = 0;
				}
			}

			if (delta_x || delta_y)
			{

				if(((delta_x == 0) && (abs(delta_y) <= min_step))
						|| ((delta_y == 0) && (abs(delta_x) <= min_step)))
				{
					OFN_Reset_Status();
					i2c_smbus_write_byte_data(ms->dev_client,A320_MOTION, 0x00);
					hrtimer_start(&ms->timer, ktime_set(0, 120*1000*1000), HRTIMER_MODE_REL);
					return;
				}
				if ((abs(delta_x) > min_step) || (abs(delta_y) > min_step))
				{
					//finger_status = FINGER_ON;
					key_step = OFN_Data_Process(delta_x, delta_y);

					if (key_step && disturbance < 3)
					{
						ofn_key_event(ms, key_step);
						//key_step = 0;
						OFN_Reset_Status();
					}
					else
						OFN_Reset_Status();
				}
			 }
			 i2c_smbus_write_byte_data(ms->dev_client,A320_MOTION, 0x00);
		}
	}
	/*else if (finger_status)
	{
		finger_status = motion & A320_MOTION_GPIO;
		adbs_a320_check_FPD(ms->dev_client);
		if (finger_status == FINGER_OFF)
		{
			//printk("finger_status: %d\n", finger_status);
			OFN_Reset_Status();
		}
	}*/
	hrtimer_start(&ms->timer, ktime_set(0, 120*1000*1000), HRTIMER_MODE_REL);
}

/*This is the main firmware data collecting and processing part Central timer.*/
static enum hrtimer_restart adbs_a320_timer(struct hrtimer *handle)
{
	struct adbs_a320 *ms = container_of(handle, struct adbs_a320, timer);

	spin_lock(&ms->lock);
	queue_work(adbs_a320_wq, &ms->work);
	spin_unlock(&ms->lock);

	return HRTIMER_NORESTART;
}

static irqreturn_t adbs_a320_irq(int irq, void *dev_id)
{
    struct adbs_a320 *ms = dev_id;
    unsigned long flags;

    if(ms->power_state == POWER_DISABLE)
        return IRQ_HANDLED;

    irq_run = 1;

    spin_lock_irqsave(&ms->lock, flags);
#if 1
    if(likely(get_motion_pin_state(ms))) {
        disable_irq_nosync(ms->irq);
        //schedule_work(&ms->work);
        hrtimer_start(&ms->timer, ktime_set(0, MS_POOL_PERIOD),HRTIMER_MODE_REL);
    }

#else
    //schedule_work(&ms->work);
#endif

    spin_unlock_irqrestore(&ms->lock, flags);

    irq_run = 0;
    return IRQ_HANDLED;
}

/* read sensor data from ADBS-A320 */
static int a320_read_byte_data(struct i2c_client *client, int reg)
{
    int ret = 0;

/*    if(wait_irq_down() <= 0)
        return -ETIMEDOUT;*/

    ret = i2c_smbus_read_byte_data(client, reg);

    return ret;
}

/* write data to ADBS-A320 */
static int a320_write_byte_data(struct i2c_client *client, int reg, int val)
{
    int ret = 0;

 /*   if(wait_irq_down() <= 0)
        return -ETIMEDOUT;*/

    ret = i2c_smbus_write_byte_data(client, reg, (val & 0xFF));

    return ret;
}

/*******************************************************************************
  Check and negate FPD when finger is not present or when there is no motion
*******************************************************************************/
static int adbs_a320_check_FPD(struct i2c_client *client)
{
	unsigned char squal, upper, lower, pix_max, pix_avg, pix_min;
	unsigned short shutter;

    squal = i2c_smbus_read_byte_data(client, A320_SQUAL);
    upper = i2c_smbus_read_byte_data(client, A320_SHUTTERUPPER);
    lower = i2c_smbus_read_byte_data(client, A320_SHUTTERLOWER);
    pix_max = i2c_smbus_read_byte_data(client, A320_MAXIMUMPIXEL);
    pix_min = i2c_smbus_read_byte_data(client, A320_MINIMUMPIXEL);
    pix_avg = i2c_smbus_read_byte_data(client, A320_PIXELSUM);


	shutter = (upper << 8) | lower;

	//printk("shutter: %d, pix: %d, squal: %d,pix_max: %d,pix_min: %d,pix_avg: %d\n",
	//		shutter, pix_max-pix_min, squal,pix_max,pix_min,pix_avg);


	if ((shutter < 90) || (abs(pix_max - pix_min) > 226) || (squal <= 12))
	{
		disturbance++;
		return 1;
	}
	else if(disturbance)
		disturbance--;
	return 0;
}

int a320_checkid(struct i2c_client *client)
{
    u8 tmp[4];

	tmp[0] = i2c_smbus_read_byte_data(client, A320_PRODUCTID);
    tmp[1] = i2c_smbus_read_byte_data(client, A320_REVISIONID);
    tmp[2] = i2c_smbus_read_byte_data(client, A320_INVERSEPRODUCTID);
    tmp[3] = i2c_smbus_read_byte_data(client, A320_INVERSEREVISIONID);

	printk(KERN_INFO "device info [%02X:%02X:%02X:%02X] \n\n\n ",
                                        tmp[0], tmp[1], tmp[2], tmp[3]);

    if((tmp[0] != 0x83) || (tmp[1] != 0x01) || (tmp[2] != 0x7C) || (tmp[3] != 0xFE)) {
        dev_err(&client->dev,"read chip ID is not equal to [83:01:7C:FE]!\n");
		printk(KERN_INFO "read chip ID failed\n");
    }

    return 0;
}

int a320_selftest(struct i2c_client *client)
{
    u8 crc[4] = {0};
    int orient, i;

    orient = i2c_smbus_read_byte_data(client, A320_OFN_ORIENTATION_CTRL);

    //printf("orient: 0x%02X\n", orient);

    // soft reset
    i2c_smbus_write_byte_data(client, A320_SOFTRESET, 0x5A);

    mdelay(5);

    i2c_smbus_write_byte_data(client, A320_OFN_ENGINE, 0xF6);
    i2c_smbus_write_byte_data(client, A320_OFN_QUANTIZE_CTRL, 0xAA);
    i2c_smbus_write_byte_data(client, A320_OFN_SPEED_CONTROL, 0xC4);

    // enable system self test
    i2c_smbus_write_byte_data(client, A320_SELFTEST, 0x01);

    // wait more than 250ms
    mdelay(500);

    crc[0] = i2c_smbus_read_byte_data(client, A320_CRC0);
    crc[1] = i2c_smbus_read_byte_data(client, A320_CRC1);
    crc[2] = i2c_smbus_read_byte_data(client, A320_CRC2);
    crc[3] = i2c_smbus_read_byte_data(client, A320_CRC3);

    if(orient & 0x01)
    {
        if ((crc[0]!=0x36) || (crc[1]!=0x72) || (crc[2]!=0x7F) || (crc[3]!=0xD6))
		{
			printk("OFN Self Test Error\n");

            // output value
            for(i = 0; i < 4; i++)
            {
                printk("crc[%d] = 0x%02X\n", i, crc[i]);
            }

			return -EINVAL;
		}
    }
    else
    {
        if ((crc[0]!=0x33) || (crc[1]!=0x8E) || (crc[2]!=0x24) || (crc[3]!=0x6C))
		{
			printk("OFN Self Test Error\n");
            // output value
            for(i = 0; i < 4; i++)
            {
                printk("crc[%d] = 0x%02X\n", i, crc[i]);
            }

			return -EINVAL;
		}
    }

    printk("adbs-a320 device selftest succeed..\n");
    return 0;
}

void a320_regs_init(struct i2c_client *client)
{
    u8 motion, ux, uy;

    // Soft Reset. All settings will revert to default values
    i2c_smbus_write_byte_data(client, A320_SOFTRESET, 0x5A);
    mdelay(3);


    /*******************************************************************
      These registers are used to set several properties of the sensor
    *******************************************************************/
    // OFN_Engine
    i2c_smbus_write_byte_data(client, A320_OFN_ENGINE, 0xA4);

    // Resolution 500cpi, Wakeup 500cpi
    i2c_smbus_write_byte_data(client, A320_OFN_RESOLUTION, 0x12);

    // SPIntval: 16ms; Enable Low cpi; Disable High cpi
    i2c_smbus_write_byte_data(client, A320_OFN_SPEED_CONTROL, 0x0E);

    // Speed Switching Thresholds
    i2c_smbus_write_byte_data(client, A320_OFN_SPEED_ST12, 0x08);
    i2c_smbus_write_byte_data(client, A320_OFN_SPEED_ST21, 0x06);
    i2c_smbus_write_byte_data(client, A320_OFN_SPEED_ST23, 0x40);
    i2c_smbus_write_byte_data(client, A320_OFN_SPEED_ST32, 0x08);
    i2c_smbus_write_byte_data(client, A320_OFN_SPEED_ST34, 0x48);
    i2c_smbus_write_byte_data(client, A320_OFN_SPEED_ST43, 0x0A);
    i2c_smbus_write_byte_data(client, A320_OFN_SPEED_ST45, 0x50);
    i2c_smbus_write_byte_data(client, A320_OFN_SPEED_ST54, 0x48);

    // Assert / De-assert Thresholds
    i2c_smbus_write_byte_data(client, A320_OFN_AD_CTRL, 0xC4);
    i2c_smbus_write_byte_data(client, A320_OFN_AD_ATH_HIGH, 0x34);
    i2c_smbus_write_byte_data(client, A320_OFN_AD_DTH_HIGH, 0x40);//0x3C
    i2c_smbus_write_byte_data(client, A320_OFN_AD_ATH_LOW, 0x34);//0x18
    i2c_smbus_write_byte_data(client, A320_OFN_AD_DTH_LOW, 0x40);//0x20

    // Finger Presence Detect
    i2c_smbus_write_byte_data(client, A320_OFN_FPD_CTRL, 0x50);

    // XY Quantization
    i2c_smbus_write_byte_data(client, A320_OFN_QUANTIZE_CTRL, 0x99);
    i2c_smbus_write_byte_data(client, A320_OFN_XYQ_THRESH, 0x02);

    // if use burst mode, define MOTION_BURST
#ifdef MOTION_BURST
    i2c_smbus_write_byte_data(client, A320_IO_MODE, 0x10);
#endif

    // write to registers 0x02, 0x03 and 0x04
    motion = i2c_smbus_read_byte_data(client, A320_MOTION);
    ux = i2c_smbus_read_byte_data(client, A320_DELTAX);
    uy = i2c_smbus_read_byte_data(client, A320_DELTAY);

    // Set LED drive current to 13mA
    i2c_smbus_write_byte_data(client, A320_LED_CONTROL, 0x00);
}

void a320_set_sensitivity(struct i2c_client *client, u8 level)
{
	speed_level = level;

	switch (speed_level)
	{
	case 0: // auto
		step_thresh[0].x_set = 14;
		step_thresh[0].y_set = 14;
		step_thresh[1].x_set = 40;
		step_thresh[1].y_set = 40;
		step_thresh[2].x_set = 40;
		step_thresh[2].y_set = 40;
		step_thresh[3].x_set = 40;
		step_thresh[3].y_set = 40;
		key_event_times      = 2;

		i2c_smbus_write_byte_data(client, A320_OFN_ENGINE, 0xE4);
		i2c_smbus_write_byte_data(client, A320_OFN_RESOLUTION, 0x12);
		//i2c_smbus_write_byte_data(client, A320_OFN_RESOLUTION, 0x1b);
		i2c_smbus_write_byte_data(client, A320_OFN_SPEED_CONTROL, 0x0D);
		//i2c_smbus_write_byte_data(client, A320_OFN_SPEED_CONTROL, 0x05);
		break;

	case 1: // lower
		step_thresh[0].x_set = 22;
		step_thresh[0].y_set = 22;
		step_thresh[1].x_set = 180;
		step_thresh[1].y_set = 180;
		step_thresh[2].x_set = 200;
		step_thresh[2].y_set = 200;
		step_thresh[3].x_set = 220;
		step_thresh[3].y_set = 220;
		key_event_times      = 2;

		i2c_smbus_write_byte_data(client, A320_OFN_ENGINE, 0xA4);
		i2c_smbus_write_byte_data(client, A320_OFN_RESOLUTION, 0x12);
		break;

	case 2: // low
		step_thresh[0].x_set = 20;
		step_thresh[0].y_set = 20;
		step_thresh[1].x_set = 80;
		step_thresh[1].y_set = 80;
		step_thresh[2].x_set = 120;
		step_thresh[2].y_set = 120;
		step_thresh[3].x_set = 140;
		step_thresh[3].y_set = 140;
		key_event_times      = 2;

		i2c_smbus_write_byte_data(client, A320_OFN_ENGINE, 0xA4);
		i2c_smbus_write_byte_data(client, A320_OFN_RESOLUTION, 0x12);
		break;

	case 3: // middle
		step_thresh[0].x_set = 18;
		step_thresh[0].y_set = 18;
		step_thresh[1].x_set = 60;
		step_thresh[1].y_set = 60;
		step_thresh[2].x_set = 80;
		step_thresh[2].y_set = 80;
		step_thresh[3].x_set = 100;
		step_thresh[3].y_set = 100;
		key_event_times      = 2;

		i2c_smbus_write_byte_data(client, A320_OFN_ENGINE, 0xA4);
		i2c_smbus_write_byte_data(client, A320_OFN_RESOLUTION, 0x12);
		break;

	case 4: // high
		step_thresh[0].x_set = 15;
		step_thresh[0].y_set = 15;
		step_thresh[1].x_set = 30;
		step_thresh[1].y_set = 30;
		step_thresh[2].x_set = 60;
		step_thresh[2].y_set = 60;
		step_thresh[3].x_set = 80;
		step_thresh[3].y_set = 80;
		key_event_times      = 2;

		i2c_smbus_write_byte_data(client, A320_OFN_ENGINE, 0xA4);
		i2c_smbus_write_byte_data(client, A320_OFN_RESOLUTION, 0x12);
		break;

	case 5: // higher
		step_thresh[0].x_set = 12;
		step_thresh[0].y_set = 12;
		step_thresh[1].x_set = 22;
		step_thresh[1].y_set = 22;
		step_thresh[2].x_set = 25;
		step_thresh[2].y_set = 25;
		step_thresh[3].x_set = 30;
		step_thresh[3].y_set = 30;
		key_event_times      = 1;

		i2c_smbus_write_byte_data(client, A320_OFN_ENGINE, 0xA4);
		i2c_smbus_write_byte_data(client, A320_OFN_RESOLUTION, 0x12);
		break;

	default: // auto
		step_thresh[0].x_set = 14;
		step_thresh[0].y_set = 14;
		step_thresh[1].x_set = 40;
		step_thresh[1].y_set = 40;
		step_thresh[2].x_set = 40;
		step_thresh[2].y_set = 40;
		step_thresh[3].x_set = 40;
		step_thresh[3].y_set = 40;
		key_event_times      = 2;

		i2c_smbus_write_byte_data(client, A320_OFN_ENGINE, 0xE4);
		i2c_smbus_write_byte_data(client, A320_OFN_RESOLUTION, 0x12);
		i2c_smbus_write_byte_data(client, A320_OFN_SPEED_CONTROL, 0x0D);

        printk("unkonw config, used default.\n");
		break;
	}
}


static int adbs_a320_detect(struct i2c_client * client, int kind,
                            struct i2c_board_info * info)
{
    struct i2c_adapter *adapter = client->adapter;

    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE
                                     | I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
        {
            printk("adbs_a320_detect error.\n");
            return -ENODEV;
        }

        /* This is the place to detect whether the chip at the specified
           address really is a MMA7455 chip. However, there is no method known
           to detect whether an I2C chip is a MMA7455 or any other I2C chip. */

        strlcpy(info->type, "adbs_a320", I2C_NAME_SIZE);

        return 0;
}

static int adbs_a320_remove(struct i2c_client * client)
{
    struct adbs_a320 *ms = i2c_get_clientdata(client);
    struct adbs_a320_platform_data *pdata = client->dev.platform_data;

    unregister_early_suspend(&ms->early_suspend);

    cancel_work_sync(&ms->work);
    /* disable voltage */
    if (pdata != NULL && pdata->power_control != NULL) {
		if (pdata->power_control(POWER_DISABLE) != 0) {
			dev_dbg(&client->dev, "power disable failed\n");
		}
	}
    free_irq(ms->irq, ms);
    hrtimer_cancel(&ms->timer);
    input_unregister_device(ms->input);
    kfree(ms->input);
    kfree(ms);

    return 0;
}

static int adbs_a320_probe(struct i2c_client * client, const struct i2c_device_id * id)
{
    struct adbs_a320   *ms;
    struct adbs_a320_platform_data *pdata = client->dev.platform_data;
    struct input_dev *input;
    int i;
    int ret = 0;

    /* enable voltage */
	if (pdata != NULL && pdata->power_control != NULL) {
		ret = pdata->power_control(POWER_ENABLE);
		if (ret != 0) {
            dev_dbg(&client->dev, "power enable failed\n");
			return ret;
		}
	}


    if (!(ms = kzalloc(sizeof(struct adbs_a320), GFP_KERNEL))) {
        ret = -ENOMEM;
        goto err_star_init;
    }


    if (!(input = input_allocate_device())) {
        ret = -ENOMEM;
        goto err_kfree;
    }

    memcpy(ms->keycode, ofn_keycode, sizeof(ms->keycode));

    ms->dev_client = client;

    i2c_set_clientdata(client, ms);

    ms->input = input;
    ms->dev_platform_data = pdata;

    hrtimer_init(&ms->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    ms->timer.function = adbs_a320_timer;

    spin_lock_init(&ms->lock);

    ms->dev_regs_init = a320_regs_init;
    ms->dev_set_sensitivity = a320_set_sensitivity;
    ms->dev_selftest = a320_selftest;
    ms->dev_checkid = a320_checkid;
    ms->dev_read_reg = a320_read_byte_data;
    ms->dev_write_reg = a320_write_byte_data;

    snprintf(ms->phys, sizeof(ms->phys),
		 "%s/input0", dev_name(&client->dev));

    input->name = "ofn keypad";
    input->phys = ms->phys;
    input->id.bustype = BUS_I2C;
    input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);

    for(i = 0; i < ARRAY_SIZE(ms->keycode); i++)
        set_bit(ms->keycode[i], input->keybit);

    //__set_bit(KEY_POWER, ms->input->keybit);

    ret = input_register_device(input);

    if (ret)
        goto err_kfree;

    pdata->init_device_hw();
    ms->dev_checkid(client);
    ms->dev_selftest(client);
    ms->dev_regs_init(client);
    ms->dev_set_sensitivity(client,0);

    ms->irq = client->irq;

    ret = request_irq(ms->irq, adbs_a320_irq, IRQF_TRIGGER_FALLING,
            "adbs-a320 irq", ms);

    if (ret)
        goto err_free_irq;

    INIT_WORK(&ms->work, adbs_a320_work);

#ifdef CONFIG_HAS_EARLYSUSPEND
    ms->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    ms->early_suspend.suspend = adbs_a320_early_suspend;
    ms->early_suspend.resume = adbs_a320_late_resume;
    register_early_suspend(&ms->early_suspend);
#endif

    ms->power_state = POWER_ENABLE;
    return 0;
err_free_irq:
    free_irq(ms->irq, ms);
err_kfree:
    kfree(ms);
    input_free_device(input);
err_star_init:
    /* disable voltage */
	if (pdata != NULL && pdata->power_control != NULL) {
		if (pdata->power_control(POWER_DISABLE) != 0) {
			dev_dbg(&client->dev, "power disable failed\n");
		}
	}

    printk("someting err[%d]....\n", ret);
   return ret;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void adbs_a320_early_suspend(struct early_suspend *h)
{
	struct adbs_a320 *ms;
	ms = container_of(h, struct adbs_a320, early_suspend);
	ms->power_state = POWER_DISABLE;
	disable_irq(ms->irq);
	gpio_set_value(ADBS_A320_SHDN_GPIO, 1);
}

static void adbs_a320_late_resume(struct early_suspend *h)
{
	struct adbs_a320 *ms;
	ms = container_of(h, struct adbs_a320, early_suspend);
	ms->power_state = POWER_ENABLE;
	enable_irq(ms->irq);
	gpio_set_value(ADBS_A320_SHDN_GPIO, 0);
	mdelay(150);
}
#endif

static int __init init_adbs_a320(void)
{
    adbs_a320_wq = create_singlethread_workqueue("adbs_a320_wq");
    if (!adbs_a320_wq)
	return -ENOMEM;
    return i2c_add_driver(&adbs_a320_driver);
}

static void __exit exit_adbs_a320(void)
{
	i2c_del_driver(&adbs_a320_driver);
	if (adbs_a320_wq)
		destroy_workqueue(adbs_a320_wq);
}


module_init(init_adbs_a320);
//subsys_initcall(init_adbs_a320);
module_exit(exit_adbs_a320);


MODULE_AUTHOR("VIA Telecom, Inc.");
MODULE_DESCRIPTION("ADBS A320 OFN mouse sensor driver");
MODULE_LICENSE("GPL");

