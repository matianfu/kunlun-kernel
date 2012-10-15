/*
 * Freescale MMA7455 accelerometer sensor driver
 * mma7455.c
 *
 */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/mma7455.h>
#include <linux/miscdevice.h>

#define MMA7455_I2C_ADDR	0x1D
#define RES_BUFF_LEN		320
#define MAX_CAL_CNT		20
#define CAL_FUZZ		(3)
#define MMA_GLVL_2G		0x04
#define MMA_GLVL_4G		0x08
#define MMA_GLVL_8G		0x00

#define ATKBD_SPECIAL		248
#define LSB_G			64

#define DRV_VERSION		"0.7.4"
#define SET_CALIBRATE		0x20
#define GET_CALIBRATE		0x21
#define CMD_CALIBRATE		0x22

#define DEFAULT_BUF 		"OK:calibration done:0xf2 0x07 0x10 0x00 0xe6 0x07"

struct kobject *mma7455_obj;

/* enum for AXIS remapping */
enum {
	AXIS_DIR_X	= 0,
	AXIS_DIR_X_N,
	AXIS_DIR_Y,
	AXIS_DIR_Y_N,
	AXIS_DIR_Z,
	AXIS_DIR_Z_N,
};

/* enum for slider event */
enum {
	SLIDER_X_UP	= 0,
	SLIDER_X_DOWN	,
	SLIDER_Y_UP	,
	SLIDER_Y_DOWN	,
};

/* register enum for mma7455 registers */
enum {
	REG_XOUTL = 0x00,
	REG_XOUTH,
	REG_YOUTL,
	REG_YOUTH,
	REG_ZOUTL,
	REG_ZOUTH,
	REG_XOUT8,
	REG_YOUT8,
	REG_ZOUT8,
	REG_STATUS,
	REG_DETSRC,
	REG_TOUT,
	REG_RESERVED_0,
	REG_I2CAD,
	REG_USRINF,
	REG_WHOAMI,
	REG_XOFFL,
	REG_XOFFH,
	REG_YOFFL,
	REG_YOFFH,
	REG_ZOFFL,
	REG_ZOFFH,
	REG_MCTL,
	REG_INTRST,
	REG_CTL1,
	REG_CTL2,
	REG_LDTH,
	REG_PDTH,
	REG_PD,
	REG_LT,
	REG_TW,
	REG_REVERVED_1,
};

/* mode enum for mma7455 register MCTL bit 0,1 */
enum {
	MOD_STANDBY = 0,
	MOD_MEASURE,
	MOD_LEVEL_D,
	MOD_PULSE_D,
};

/* enum for mma7455 register CTL1 bit 1,2 (for interrupt detction) */
enum {
	INT_1L_2P = 0,
	INT_1P_2L,
	INT_1SP_2P,
};

/* mma7455 status */
struct mma7455_status {
	u8 mode;
	u8 ctl1;
	u8 ctl2;
	u8 axis_dir_x;		/* current X axis remap */
	u8 axis_dir_y;		/* current Y axis remap */
	u8 axis_dir_z;		/* current Z axis remap */
	u8 slider_key[4][4];	/* current setting for slider key map */
};

static int mma7455_suspend(struct i2c_client *client, pm_message_t state);
static int mma7455_resume(struct i2c_client *client);

static struct i2c_driver mma7455_driver;
static int mma7455_probe(struct i2c_client *client,const struct i2c_device_id *id);
static int mma7455_remove(struct i2c_client *client);
static int mma7455_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);

static const struct i2c_device_id mma7455_id[] = {
	{"mma7455", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, mma7455_id);


static struct i2c_client 	*mma7455_client;
//static u8			mma_open = 0;
static u8			mma_cal	= 0;			/* in calibration 	*/
static int 			debug = 0;			/* debug flag 		*/
static u8			no_overwrite = 0; 		/* do not check overwrite bit */
static char			res_buff[RES_BUFF_LEN];		/* response buffer for control interface */
static char 			block_read;			/* i2c block read supported ? 	*/

#undef DBG
#define DBG(format, arg...) do { if (debug) printk(KERN_DEBUG "%s: " format "\n" , __FILE__ , ## arg); } while (0)

static unsigned short normal_i2c[] = {0x1D,I2C_CLIENT_END};

I2C_CLIENT_INSMOD_1(mma7455);


static struct i2c_driver mma7455_driver = {
	.driver 	= {
				.name = "mma7455",
				.owner = THIS_MODULE,
			},
	.class		= I2C_CLASS_HWMON,
	.probe		= mma7455_probe,
	.remove		= mma7455_remove,
	.id_table	= mma7455_id,
	.detect		= mma7455_detect,
	.suspend 	= mma7455_suspend,
	.resume 	= mma7455_resume,
};
#if defined(CONFIG_MACH_OMAP_VIATELECOM_WUDANG)
static struct mma7455_status mma_status = {
	.mode 		= 0,
	.ctl1 		= 0,
	.ctl2 		= 0,
	.axis_dir_x	= AXIS_DIR_Y_N,
	.axis_dir_y	= AXIS_DIR_X_N,
	.axis_dir_z	= AXIS_DIR_Z,
	.slider_key	= {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}
};
#elif defined(CONFIG_MACH_OMAP_VIATELECOM_KUNLUN_P2)
static struct mma7455_status mma_status = {
	.mode 		= 0,
	.ctl1 		= 0,
	.ctl2 		= 0,
	.axis_dir_x	= AXIS_DIR_X,
	.axis_dir_y	= AXIS_DIR_Y,
	.axis_dir_z	= AXIS_DIR_Z_N,
	.slider_key	= {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}
};
#else
static struct mma7455_status mma_status = {
	.mode 		= 0,
	.ctl1 		= 0,
	.ctl2 		= 0,
	.axis_dir_x	= AXIS_DIR_X,
	.axis_dir_y	= AXIS_DIR_Y,
	.axis_dir_z	= AXIS_DIR_Z_N,
	.slider_key	= {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}
};
#endif

enum {
	MMA_REG_R = 0,		/* read register 	*/
	MMA_REG_W,		/* write register 	*/
	MMA_DUMPREG,		/* dump registers	*/
	MMA_CALIBRATE,		/* calibration		*/
	MMA_DUMPOFF,		/* dump offset drift registers	*/
	MMA_LOADOFF,		/* load offset drift registers	*/
	MMA_REMAP_AXIS,		/* remap directiron of 3-axis	*/
	MMA_DUMPAXIS,		/* dump current axis mapping	*/
	MMA_SETGLVL,		/* set glvl			*/
	MMA_ENMOUSE,		/* enable mouse	emulation	*/
	MMA_DISMOUSE,		/* disable mouse emulation	*/
	MMA_SLIDER_KEY,		/* set slider key		*/
	MMA_DUMP_SLKEY,		/* dump slider key code setting	*/
	MMA_SLIDER_ASC,		/* set slider key in ascii 	*/
	MMA_DUMPCONF,		/* dump configuration setting 	*/
	MMA_VERSION,		/* show driver version		*/

	MMA_SET_MOD,
	MMA_SET_L_THR,
	MMA_SET_P_THR,
	MMA_SET_INTP,
	MMA_SET_INTB,
	MMA_SET_G,
	MMA_I2C_EABLE,
	MMA_OFF_X,
	MMA_OFF_Y,
	MMA_OFF_Z,
	MMA_SELF_TEST,
	MMA_SET_LDPL,
	MMA_SET_PDPL,
	MMA_SET_PDV,
	MMA_SET_LTV,
	MMA_SET_TW,
	MMA_CMD_MAX
};

/* read sensor data from mma7455 */
static int mma7455_read_data(short *x, short *y, short *z) {
	u8 	status;
	u8	tmp_data[7];

	status = i2c_smbus_read_byte_data(mma7455_client, REG_STATUS);
	if (!(status & 0x01)) {				/* data ready in measurement mode? */
		DBG("mma7455 data not ready\n");
		return -1;
	}

	switch ((mma_status.mode & 0x0c)) {
		case MMA_GLVL_4G :
		case MMA_GLVL_2G :
			if (block_read) {
				if (i2c_smbus_read_i2c_block_data(mma7455_client,REG_XOUT8,3,tmp_data) < 3) {
					dev_err(&mma7455_client->dev, "i2c block read failed\n");
					return -3;
				}
				*x = tmp_data[0];
				*y = tmp_data[1];
				*z = tmp_data[2];
			} else {
				*x = (signed char)i2c_smbus_read_byte_data(mma7455_client, REG_XOUT8);
				*y = (signed char)i2c_smbus_read_byte_data(mma7455_client, REG_YOUT8);
				*z = (signed char)i2c_smbus_read_byte_data(mma7455_client, REG_ZOUT8);
			}

			if (no_overwrite == 0) {
				status = i2c_smbus_read_byte_data(mma7455_client, REG_STATUS);
				if (status & 0x02) {	/* data is overwrite */
					DBG("%s : data is overwrite\n",__FUNCTION__);
					return -2;
				}
			}

			if ((mma_status.mode & 0x0c) == MMA_GLVL_4G) {
				*x = 2 * (*x);
				*y = 2 * (*y);
				*z = 2 * (*z);
			}
			break;
		case MMA_GLVL_8G :
			if (block_read) {
				if (i2c_smbus_read_i2c_block_data(mma7455_client,REG_XOUTL,7,tmp_data) < 7) {
					dev_err(&mma7455_client->dev, "i2c block read failed\n");
					return -3;
				}
				if (no_overwrite == 0) {
					status = tmp_data[6];
				}
			} else {
				tmp_data[0] = i2c_smbus_read_byte_data(mma7455_client, REG_XOUTL);
				tmp_data[1] = i2c_smbus_read_byte_data(mma7455_client, REG_XOUTH);
				tmp_data[2] = i2c_smbus_read_byte_data(mma7455_client, REG_YOUTL);
				tmp_data[3] = i2c_smbus_read_byte_data(mma7455_client, REG_YOUTH);
				tmp_data[4] = i2c_smbus_read_byte_data(mma7455_client, REG_ZOUTL);
				tmp_data[5] = i2c_smbus_read_byte_data(mma7455_client, REG_ZOUTH);
				if (no_overwrite == 0) {
					status = i2c_smbus_read_byte_data(mma7455_client, REG_STATUS);
				}
			}

			if (no_overwrite == 0) {
				if (status & 0x02) {	/* data is overwrite */
					DBG("%s : data is overwrite\n",__FUNCTION__);
					return -2;
				}
			}

			*x = tmp_data[0] | (tmp_data[1] << 8);
			*y = tmp_data[2] | (tmp_data[3] << 8);
			*z = tmp_data[4] | (tmp_data[5] << 8);

			if (*x & 0x0200) {	/* check for the bit 9 */
				*x |= 0xfc00;
			}

			if (*y & 0x0200) {	/* check for the bit 9 */
				*y |= 0xfc00;
			}

			if (*z & 0x0200) {	/* check for the bit 9 */
				*z |= 0xfc00;
			}
	}
	return 0;
}

/* do the axis translation */
static void remap_axis(short *rem_x,short *rem_y,short *rem_z,short x,short y,short z) {

	switch (mma_status.axis_dir_x) {
		case AXIS_DIR_X:
			*rem_x		= x;
			break;
		case AXIS_DIR_X_N:
			*rem_x		= -x;
			break;
		case AXIS_DIR_Y:
			*rem_x		= y;
			break;
		case AXIS_DIR_Y_N:
			*rem_x		= -y;
			break;
		case AXIS_DIR_Z:
			*rem_x		= z;
			break;
		case AXIS_DIR_Z_N:
			*rem_x		= -z;
			break;
	}

	switch (mma_status.axis_dir_y) {
		case AXIS_DIR_X:
			*rem_y		= x;
			break;
		case AXIS_DIR_X_N:
			*rem_y		= -x;
			break;
		case AXIS_DIR_Y:
			*rem_y		= y;
			break;
		case AXIS_DIR_Y_N:
			*rem_y		= -y;
			break;
		case AXIS_DIR_Z:
			*rem_y		= z;
			break;
		case AXIS_DIR_Z_N:
			*rem_y		= -z;
			break;
	}

	switch (mma_status.axis_dir_z) {
		case AXIS_DIR_X:
			*rem_z		= x;
			break;
		case AXIS_DIR_X_N:
			*rem_z		= -x;
			break;
		case AXIS_DIR_Y:
			*rem_z		= y;
			break;
		case AXIS_DIR_Y_N:
			*rem_z		= -y;
			break;
		case AXIS_DIR_Z:
			*rem_z		= z;
			break;
		case AXIS_DIR_Z_N:
			*rem_z		= -z;
			break;
	}
}


/* undo the axis translation */
static void unmap_axis(short *unm_x,short *unm_y,short *unm_z,short x,short y,short z) {

	switch (mma_status.axis_dir_x) {
		case AXIS_DIR_X:
			*unm_x		= x;
			break;
		case AXIS_DIR_X_N:
			*unm_x		= -x;
			break;
		case AXIS_DIR_Y:
			*unm_y		= x;
			break;
		case AXIS_DIR_Y_N:
			*unm_y		= -x;
			break;
		case AXIS_DIR_Z:
			*unm_z		= x;
			break;
		case AXIS_DIR_Z_N:
			*unm_z		= -x;
			break;
	}

	switch (mma_status.axis_dir_y) {
		case AXIS_DIR_X:
			*unm_x		= y;
			break;
		case AXIS_DIR_X_N:
			*unm_x		= -y;
			break;
		case AXIS_DIR_Y:
			*unm_y		= y;
			break;
		case AXIS_DIR_Y_N:
			*unm_y		= -y;
			break;
		case AXIS_DIR_Z:
			*unm_z		= y;
			break;
		case AXIS_DIR_Z_N:
			*unm_z		= -y;
			break;
	}

	switch (mma_status.axis_dir_z) {
		case AXIS_DIR_X:
			*unm_x		= z;
			break;
		case AXIS_DIR_X_N:
			*unm_x		= -z;
			break;
		case AXIS_DIR_Y:
			*unm_y		= z;
			break;
		case AXIS_DIR_Y_N:
			*unm_y		= -z;
			break;
		case AXIS_DIR_Z:
			*unm_z		= z;
			break;
		case AXIS_DIR_Z_N:
			*unm_z		= -z;
			break;
	}
}

/* do the calibration */
static void cmd_calibrate(void) {
	int 	reg,ret;
	short 	x,y,z;
	short 	off_x,off_y,off_z;
	short 	unm_x=0,unm_y=0,unm_z=0;
	u8 	x_done,y_done,z_done;
	short 	x_remap=0,y_remap=0,z_remap=0;

	int 	cal_cnt = 0;

	off_x  	= 0;
	off_y	= 0;
	off_z	= 0;
	x_done 	= 0;
	y_done 	= 0;
	z_done 	= 0;

	mma_cal = 1;
	sprintf(res_buff,"WAIT:calibration in progress\n");

re_read:
	printk("re_read start %d\n",cal_cnt);
	if (cal_cnt > MAX_CAL_CNT) {
		sprintf(res_buff,"FAIL:calibration failed\n");
		DBG("%s:Calibration too many times(%d)\n",__FUNCTION__,cal_cnt);
		mma_cal = 0;
		printk("calibration too many times \n");
		return;
	}

	if (mma7455_read_data(&x,&y,&z) != 0) {
		cal_cnt ++;
		goto re_read;
	}

	remap_axis(&x_remap,&y_remap,&z_remap,x,y,z);

	if (((x_remap<  CAL_FUZZ) && (x_remap > -CAL_FUZZ)) &&
		((y_remap <  CAL_FUZZ) && (y_remap > -CAL_FUZZ)) &&
		((z_remap <  (64 + CAL_FUZZ)) && (z_remap > (64 -CAL_FUZZ)))) {
		DBG("%s: calibration done, count %d\n",__FUNCTION__,cal_cnt);
		printk("calibration done ,count %d\n",cal_cnt);
		goto cal_done;
	}

	DBG("%s: calbration count %d\n",__FUNCTION__,cal_cnt);
	cal_cnt ++;
	/*off_x += -x_remap * 2;
	off_y += -y_remap * 2;
	off_z += (64 - z_remap) * 2;*/

	off_x += -x_remap;
	off_y += -y_remap;
	off_z += (64 - z_remap);

	unmap_axis(&unm_x,&unm_y,&unm_z,off_x,off_y,off_z);

	ret = i2c_smbus_write_byte_data(mma7455_client, REG_XOFFL, (unm_x & 0xff));
	ret = i2c_smbus_write_byte_data(mma7455_client, REG_XOFFH, ((unm_x >> 8)& 0x07));
	ret = i2c_smbus_write_byte_data(mma7455_client, REG_YOFFL, (unm_y & 0xff));
	ret = i2c_smbus_write_byte_data(mma7455_client, REG_YOFFH, ((unm_y >> 8)& 0x07));
	ret = i2c_smbus_write_byte_data(mma7455_client, REG_ZOFFL, (unm_z & 0xff));
	ret = i2c_smbus_write_byte_data(mma7455_client, REG_ZOFFH, ((unm_z >> 8)& 0x07));

	msleep(2);
	goto re_read;

cal_done:
	sprintf(res_buff,"OK:calibration done:");
	printk("calibration done\n");

	for (reg = REG_XOFFL; reg <= REG_ZOFFH; reg++) {
		ret = i2c_smbus_read_byte_data(mma7455_client, reg);
		sprintf(&(res_buff[strlen(res_buff)]),"0x%02x ",ret);
	}

	sprintf(&(res_buff[strlen(res_buff)]),"\n");
	printk("%s\n",res_buff);
	mma_cal = 0;
}

static int mma7455_set_calibrate(char *buff, int len)
{
        char buf[50],*p;
        int offx_l,offx_h,offy_l,offy_h,offz_l,offz_h,ret;
        memset(buf,0x00,sizeof(buf));
        memcpy(buf,buff,len);

        p = strstr(buf,"OK:calibration done");
        if(p == NULL){
                printk("wrong command could not valibrate");
                return -1;
        }

        memset(res_buff,0x00,sizeof(res_buff));
        memcpy(res_buff,buff,len);
        p = strstr(buff,"0x");
        offx_l = (*(p+2)>='a'?(*(p+2)-'a'+10):(*(p+2)-'0'))*16 + (*(p+3)>='a'?(*(p+3)-'a'+10):(*(p+3)-'0'));
        offx_h = (*(p+7)>='a'?(*(p+7)-'a'+10):(*(p+7)-'0'))*16 + (*(p+8)>='a'?(*(p+8)-'a'+10):(*(p+8)-'0'));
        offy_l = (*(p+12)>='a'?(*(p+12)-'a'+10):(*(p+12)-'0'))*16 + (*(p+13)>='a'?(*(p+13)-'a'+10):(*(p+13)-'0'));
        offy_h = (*(p+17)>='a'?(*(p+17)-'a'+10):(*(p+17)-'0'))*16 + (*(p+18)>='a'?(*(p+18)-'a'+10):(*(p+18)-'0'));
        offz_l = (*(p+22)>='a'?(*(p+22)-'a'+10):(*(p+22)-'0'))*16 + (*(p+23)>='a'?(*(p+23)-'a'+10):(*(p+23)-'0'));
        offz_h = (*(p+27)>='a'?(*(p+27)-'a'+10):(*(p+27)-'0'))*16 + (*(p+28)>='a'?(*(p+28)-'a'+10):(*(p+28)-'0'));

        printk("get the param is 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",offx_l,offx_h,offy_l,offy_h,offz_l,offz_h);


        ret = i2c_smbus_write_byte_data(mma7455_client, REG_XOFFL, offx_l);
        ret = i2c_smbus_write_byte_data(mma7455_client, REG_XOFFH, offx_h);
        ret = i2c_smbus_write_byte_data(mma7455_client, REG_YOFFL, offy_l);
        ret = i2c_smbus_write_byte_data(mma7455_client, REG_YOFFH, offy_h);
        ret = i2c_smbus_write_byte_data(mma7455_client, REG_ZOFFL, offz_l);
        ret = i2c_smbus_write_byte_data(mma7455_client, REG_ZOFFH, offz_h);

        printk("mma7455_write\n");
        return 1;
}


typedef enum {NOSLIDE, UPWARD, DOWNWARD} SLIDEOUT;

#define NOSLIDE		0
#define SLIDEUPWARD	1
#define SLIDEUPWARD2	2
#define SLIDEDOWNWARD 	3
#define SLIDEDOWNWARD2	4

#define MAXSLIDEWIDTH 	10
#define SLIDERAVCOUNT 	4

#define SLIDE_THR	16
#define SLIDE_THR2	(36)

struct slider {
	int 		wave[SLIDERAVCOUNT];	/* for long term average */
	unsigned char	wave_idx;
	short		ref;
	unsigned char 	dir;
	unsigned char	report;
	unsigned char 	mode;
	unsigned char	cnt;
};

static struct slider zsl_x,zsl_y,zsl_z;

static ssize_t mma7455_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	short 	X,Y,Z;
	int ret = 0;

	ret = mma7455_read_data(&X,&Y,&Z);

    if(ret != 0){
        DBG("mma7455 data read failed, ret = %d\n", ret);
        return 0;
    }

#if defined (CONFIG_MACH_OMAP_VIATELECOM_KL9C)
	DBG(" %s x=%d,y=%d,x=%d\n\n", __FUNCTION__,X, Y, Z);
    if(X < -20 && Y > -20)
    {
        X = X + 63;
        Y = Y - 65;
    }else if(X < 20 && Y < -20) {
        X = X + 63;
        Y = Y + 63;
    }else if(X > 20 && Y < 20) {
        X = X - 65;
        Y = Y + 63;
    }else if(X > -20 && Y > 20) {
        X = X - 65;
        Y = Y - 65;  }

	DBG(" %s x=%d,y=%d,x=%d\n\n", __FUNCTION__,X, Y, Z);

	return sprintf(buf, "%d,%d,%d\n", X, Y, Z);
#elif defined(CONFIG_MACH_OMAP_VIATELECOM_WUDANG)
    short   x_remap=0,y_remap=0,z_remap=0;

    remap_axis(&x_remap,&y_remap,&z_remap,X,Y,Z);

    return sprintf(buf, "%d,%d,%d\n", x_remap, y_remap, y_remap);
#else
    return sprintf(buf, "%d,%d,%d\n", X, Y, Z);
#endif

}

static struct kobj_attribute mma7455_attr =
	__ATTR(mma7455_data, 0644, mma7455_show, NULL);

static int mma7455_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) {
        struct i2c_adapter *adapter = client->adapter;

        if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE
                                     | I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
                return -ENODEV;

        /* This is the place to detect whether the chip at the specified
           address really is a MMA7455 chip. However, there is no method known
           to detect whether an I2C chip is a MMA7455 or any other I2C chip. */

        strlcpy(info->type, "mma7455", I2C_NAME_SIZE);

        return 0;
}

static int mma7455_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	struct mma7455_platform_data *pdata = client->dev.platform_data;
	int ret;

	/* enable voltage */
	if (pdata != NULL && pdata->power_control != NULL) {
		ret = pdata->power_control(POWER_ENABLE);
		if (ret != 0) {
			dev_dbg(&client->dev, "power enable failed\n");
			return ret;
		}
	}

	printk(KERN_INFO "try to detect mma7455 chip id %02x\n",client->addr);

	ret = i2c_smbus_read_byte_data(client, 0x0D);
	printk(KERN_INFO "ret=0x%x \n\n\n ", ret);

	if (MMA7455_I2C_ADDR != (0x7F & ret)) {	//compare the address value
		dev_err(&client->dev,"read chip ID 0x%x is not equal to 0x%x!\n", ret,MMA7455_I2C_ADDR);
		printk(KERN_INFO "read chip ID failed\n");
		ret = -EINVAL;
		goto err_power_off;
	}

	mma7455_client = client;

	memset(&zsl_x,0,sizeof(struct slider));
	memset(&zsl_y,0,sizeof(struct slider));
	memset(&zsl_z,0,sizeof(struct slider));
	memset(res_buff,0,RES_BUFF_LEN);

	/* enable gSensor mode 8g, measure */
	i2c_smbus_write_byte_data(client, REG_MCTL, MMA_GLVL_4G | 1);
	mma_status.mode	= MMA_GLVL_4G | 1;

	//cmd_calibrate();
	mma7455_set_calibrate(DEFAULT_BUF,strlen(DEFAULT_BUF));	//use the default data to calibrate the mma7455

	return 0;

err_power_off:
	/* disable voltage */
	if (pdata != NULL && pdata->power_control != NULL) {
		if (pdata->power_control(POWER_DISABLE) != 0) {
			dev_dbg(&client->dev, "power disable failed\n");
		}
	}

	return ret;
}


static int mma7455_remove(struct i2c_client *client)
{
	struct mma7455_platform_data *pdata = client->dev.platform_data;

	kfree(i2c_get_clientdata(client));
	DBG("MMA7455 device detatched\n");

	/* disable voltage */
	if (pdata != NULL && pdata->power_control != NULL) {
		if (pdata->power_control(POWER_DISABLE) != 0) {
			dev_dbg(&client->dev, "power disable failed\n");
		}
	}

	return 0;
}

static int mma7455_suspend(struct i2c_client *client, pm_message_t state)
{
	mma_status.mode = i2c_smbus_read_byte_data(mma7455_client, REG_MCTL);
	i2c_smbus_write_byte_data(mma7455_client, REG_MCTL,mma_status.mode & ~0x3);
	DBG("MMA7455 suspended\n");
	return 0;
}

static int mma7455_resume(struct i2c_client *client)
{
	i2c_smbus_write_byte_data(mma7455_client, REG_MCTL, mma_status.mode);
	DBG("MMA7455 resumed\n");
	return 0;
}

static int mma7455_open(struct inode *inode, struct file *file)
{
	printk("mma7455 open\n");
	return 0;
}

static int mma7455_release(struct inode *inode, struct file *file)
{
	printk("mma7455 close\n");
	return 0;
}

static int mma7455_get_calibrate(char *buff)
{
	sprintf(buff,"%s",res_buff);
        return strlen(res_buff);

}

static int mma7455_ioctl(struct inode *inode, struct file *file, unsigned int cmd,char *buff,unsigned long len)
{
	int ret = -1;
	switch(cmd)
	{
	case SET_CALIBRATE:
		printk("set calibrate\n");
		ret = mma7455_set_calibrate(buff,strlen(buff));
		break;
	case GET_CALIBRATE:
		printk("get calibrate\n");
		ret = mma7455_get_calibrate(buff);
		break;
	case CMD_CALIBRATE:
		printk("cmd calibrate\n");
		cmd_calibrate();
		ret = 0;
		break;
	default:
		break;
	}
	return ret;
}

static struct file_operations mma7455_fops = {
	.owner = THIS_MODULE,
	.open = mma7455_open,
	.release = mma7455_release,
	.ioctl = mma7455_ioctl,
};

static struct miscdevice mma7455_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mma7455_data",
	.fops = &mma7455_fops,
};

static int __init init_mma7455(void)
{
/* register driver */
	int res;

	res = i2c_add_driver(&mma7455_driver);
	if (res < 0){
		printk(KERN_INFO "add mma7455 i2c driver failed\n");
		return -ENODEV;
	}

	res = misc_register(&mma7455_device);
	if(res < 0){
		printk(KERN_INFO "mma7455 register misc failed\n");
		return -ENODEV;
	}
	mma7455_obj = kobject_create_and_add("mma7455", NULL);
	if (!mma7455_obj)
		return -ENOMEM;
	res = sysfs_create_file(mma7455_obj, &mma7455_attr.attr);

	return (res);
}

static void __exit exit_mma7455(void)
{
	printk(KERN_INFO "remove mma7455 i2c driver.\n");
	return i2c_del_driver(&mma7455_driver);
}


module_init(init_mma7455);
module_exit(exit_mma7455);


MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MMA7455 sensor driver");
MODULE_LICENSE("GPL");
