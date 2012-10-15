/*
 * drivers/input/touchscreen/ft5x0x_ts.c
 *
 * FocalTech ft5x0x TouchScreen driver.
 *
 * Copyright (c) 2010  Focal tech Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 *	note: only support mulititouch	Wenfs 2010-10-01
 * 		ShanChangjun 		2011.06		added virtual key
 */

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/ft5x0x_ts.h>
#include <linux/earlysuspend.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/slab.h>

static struct i2c_client *this_client;
struct kobject *ft5x0x_properties_kobj;

#define CONFIG_FT5X0X_MULTITOUCH 1

//#define FT5X0X_DEBUG
#ifdef FT5X0X_DEBUG
	#define FT5X0X_TSDBUG(x...)  printk(KERN_INFO "[ft5x0x] " x)
#else
	#define FT5X0X_TSDBUG(x...)  do{} while(0)
#endif

struct ts_event {
	u16	x1;
	u16	y1;
	u16	x2;
	u16	y2;
	u16	x3;
	u16	y3;
	u16	x4;
	u16	y4;
	u16	x5;
	u16	y5;
	u16	pressure;
	s16 touch_ID1;
	s16 touch_ID2;
	s16 touch_ID3;
	s16 touch_ID4;
	s16 touch_ID5;
	u8  touch_point;
};

struct ft5x0x_ts_data {
	struct input_dev	*input_dev;
	struct ts_event		event;
	struct work_struct 	pen_event_work;
	struct workqueue_struct *ts_workqueue;
	struct early_suspend	early_suspend;
	unsigned int irq;
};

static unsigned char ts_press=0;
static unsigned int old_x1=0;
static unsigned int old_y1=0;

static int ft5x0x_i2c_rxdata(char *rxdata, int length)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= rxdata,
		},
		{
			.addr	= this_client->addr,
			.flags	= I2C_M_RD,
			.len	= length,
			.buf	= rxdata,
		},
	};

	ret = i2c_transfer(this_client->adapter, msgs, 2);
	if (ret < 0){
		pr_err("msg %s i2c read error: %d\n", __func__, ret);
	}

	return ret;
}

static int ft5x0x_i2c_txdata(char *txdata, int length)
{
	int ret;

	struct i2c_msg msg[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= length,
			.buf	= txdata,
		},
	};

	ret = i2c_transfer(this_client->adapter, msg, 1);
	if (ret < 0){
		pr_err("%s i2c write error: %d\n", __func__, ret);
	}

	return ret;
}

static int ft5x0x_set_reg(u8 addr, u8 para)
{
	u8 buf[3];
	int ret = -1;

	buf[0] = addr;
	buf[1] = para;
	ret = ft5x0x_i2c_txdata(buf, 2);
	if (ret < 0) {
		pr_err("write reg failed! %#x ret: %d", buf[0], ret);
		return -1;
	}

	return 0;
}

static void ft5x0x_ts_release(void)
{
	struct ft5x0x_ts_data *data = i2c_get_clientdata(this_client);

#ifdef CONFIG_FT5X0X_MULTITOUCH
	input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
#else
	input_report_abs(data->input_dev, ABS_PRESSURE, 0);
	input_report_key(data->input_dev, BTN_TOUCH, 0);
#endif
	input_sync(data->input_dev);
}

static int ft5x0x_read_data(void)
{
	struct ft5x0x_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;
//	u8 buf[13] = {0};
	u8 buf[32] = {0};
	int ret = -1;

#ifdef CONFIG_FT5X0X_MULTITOUCH
//	ret = ft5x0x_i2c_rxdata(buf, 13);
	ret = ft5x0x_i2c_rxdata(buf, 31);
#else
	ret = ft5x0x_i2c_rxdata(buf, 7);
#endif
	if (ret < 0) {
		printk("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return ret;
	}

	memset(event, 0, sizeof(struct ts_event));
//	event->touch_point = buf[2] & 0x03;// 0000 0011
	event->touch_point = buf[2] & 0x07;// 000 0111

	if (event->touch_point == 0) {
		if(ts_press==1){
			ft5x0x_ts_release();
			ts_press=0;
		}
		return 1;
	}

#ifdef CONFIG_FT5X0X_MULTITOUCH
	switch (event->touch_point) {
		case 5:
			event->x5 = (s16)(buf[0x1b] & 0x0F)<<8 | (s16)buf[0x1c];
			event->y5 = (s16)(buf[0x1d] & 0x0F)<<8 | (s16)buf[0x1e];
				event->touch_ID5=(s16)(buf[0x1D] & 0xF0)>>4;
		case 4:
			event->x4 = (s16)(buf[0x15] & 0x0F)<<8 | (s16)buf[0x16];
			event->y4 = (s16)(buf[0x17] & 0x0F)<<8 | (s16)buf[0x18];
				event->touch_ID4=(s16)(buf[0x17] & 0xF0)>>4;
		case 3:
			event->x3 = (s16)(buf[0x0f] & 0x0F)<<8 | (s16)buf[0x10];
			event->y3 = (s16)(buf[0x11] & 0x0F)<<8 | (s16)buf[0x12];
				event->touch_ID3=(s16)(buf[0x11] & 0xF0)>>4;
		case 2:
			event->x2 = (s16)(buf[9] & 0x0F)<<8 | (s16)buf[10];
			event->y2 = (s16)(buf[11] & 0x0F)<<8 | (s16)buf[12];
			event->touch_ID2=(s16)(buf[0x0b] & 0xF0)>>4;
		case 1:
			event->x1 = (s16)(buf[3] & 0x0F)<<8 | (s16)buf[4];
			event->y1 = (s16)(buf[5] & 0x0F)<<8 | (s16)buf[6];
			event->touch_ID1=(s16)(buf[0x05] & 0xF0)>>4;

			if((event->touch_point==1)&&(event->x1==old_x1)
				&&(event->y1==old_y1)){

				return -1;
			}
			break;

		default:
			printk("__Multitouch_pointNumErr\n");
			return -1;
	}
#else
	if (event->touch_point == 1) {
		event->x1 = (s16)(buf[3] & 0x0F)<<8 | (s16)buf[4];
		event->y1 = (s16)(buf[5] & 0x0F)<<8 | (s16)buf[6];
	}
#endif
	event->pressure = 200;

	FT5X0X_TSDBUG("points=%d (%d, %d), (%d, %d)\n",
		event->touch_point, event->x1, event->y1, event->x2, event->y2);

	return 0;
}

static void ft5x0x_report_value(void)
{
	struct ft5x0x_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;

#ifdef CONFIG_FT5X0X_MULTITOUCH
	switch(event->touch_point) {
		case 5:
			//input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID5);
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x5);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y5);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
			FT5X0X_TSDBUG("===x5 = %d,y5 = %d ====\n",event->x5,event->y5);
		case 4:
			//input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID4);
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x4);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y4);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
			FT5X0X_TSDBUG("===x4 = %d,y4 = %d ====\n",event->x4, event->y4);
		case 3:
			//input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID3);
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x3);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y3);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
			FT5X0X_TSDBUG("===x3 = %d,y3 = %d ====\n",event->x3, event->y3);
		case 2:
			//input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID2);
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x2);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y2);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
			FT5X0X_TSDBUG("===x2 = %d,y2 = %d ====\n",event->x2,event->y2);
		case 1:
			//input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID1);
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x1);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y1);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
			FT5X0X_TSDBUG("===x1 = %d,y1 = %d ====\n",event->x1,event->y1);
			input_report_key(data->input_dev, BTN_TOUCH, 1);
			old_x1 = event->x1;
			old_y1 = event->y1;
			ts_press = 1;
			break;
		default:
			printk("__No touch_point to report!\n");
			break;
	}
#else	/* CONFIG_FT5X0X_MULTITOUCH*/
	if (event->touch_point == 1) {
		input_report_abs(data->input_dev, ABS_X, event->x1);
		input_report_abs(data->input_dev, ABS_Y, event->y1);
		input_report_abs(data->input_dev, ABS_PRESSURE, event->pressure);
	}

	FT5X0X_TSDBUG("TP(%d,%d)\n",event->x1,event->y1);
	input_report_key(data->input_dev, BTN_TOUCH, 1);
#endif	/* CONFIG_FT5X0X_MULTITOUCH*/
	input_sync(data->input_dev);

	dev_dbg(&this_client->dev, "%s: 1:%d %d 2:%d %d \n", __func__,
		event->x1, event->y1, event->x2, event->y2);
}	/*end ft5x0x_report_value*/

static void ft5x0x_ts_pen_irq_work(struct work_struct *work)
{
	int ret = -1;

	ret = ft5x0x_read_data();
	if (ret == 0) {
		ft5x0x_report_value();
	}

	enable_irq(this_client->irq);
}

static irqreturn_t ft5x0x_ts_interrupt(int irq, void *dev_id)
{
	struct ft5x0x_ts_data *ft5x0x_ts = dev_id;

	disable_irq_nosync(this_client->irq);

	FT5X0X_TSDBUG("==ft5x0x_ts_interrupt==\n");
	if (!work_pending(&ft5x0x_ts->pen_event_work)) {
		queue_work(ft5x0x_ts->ts_workqueue, &ft5x0x_ts->pen_event_work);
	}

	return IRQ_HANDLED;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ft5x0x_ts_suspend(struct early_suspend *handler)
{
	struct ft5x0x_ts_data *ts;
	ts =  container_of(handler, struct ft5x0x_ts_data, early_suspend);

	FT5X0X_TSDBUG("==ft5x0x_ts_suspend=\n");
	disable_irq(this_client->irq);
//	cancel_work_sync(&ts->pen_event_work);
//	flush_workqueue(ts->ts_workqueue);
	// ==set sleep mode ==,
	ft5x0x_set_reg(FT5X0X_REG_PMODE, PMODE_HIBERNATE);
}

static void ft5x0x_ts_reset(void)
{
	// wake the mode
	gpio_direction_output(FT5X0X_RST_GPIO, 1);
	gpio_set_value(FT5X0X_RST_GPIO,0);
	msleep(50);
	gpio_set_value(FT5X0X_RST_GPIO,1);
	mdelay(10);
	gpio_set_value(FT5X0X_RST_GPIO,0);
	mdelay(100);
	gpio_set_value(FT5X0X_RST_GPIO,1);
	msleep(100);
}

static void ft5x0x_ts_enable(void)
{
	ft5x0x_set_reg(0x88, 0x05); //5, 6,7,8;Period Active[3-14]
	ft5x0x_set_reg(0x80, 30);//touching detect threshold
	msleep(50);

	return;
}

static void ft5x0x_ts_resume(struct early_suspend *handler)
{
	FT5X0X_TSDBUG("==ft5x0x_ts_resume=\n");

	ft5x0x_ts_reset();
	ft5x0x_ts_enable();

	enable_irq(this_client->irq);
}
#endif  //CONFIG_HAS_EARLYSUSPEND

/*add 4 virtual Keys begin*/
static ssize_t ft5x0x_virtual_keys_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf)
{

	printk("__ft5x0x_virtual_keys_show\n");
	return sprintf(buf,
		__stringify(EV_KEY) ":" __stringify(KEY_MENU)   ":40:515:40:30"
	   ":"  __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":120:515:40:30"
	   ":"  __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":200:515:40:30"
	   ":"  __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":280:515:40:30"
	   "\n");
}

static struct kobj_attribute ft5x0x_virtual_keys_attr =
	__ATTR(virtualkeys.ft5x0x_Touchscreen, S_IRUGO, ft5x0x_virtual_keys_show, NULL);

static struct attribute *ft5x0x_properties_attrs[] = {
	&ft5x0x_virtual_keys_attr.attr,
	NULL
};

static struct attribute_group ft5x0x_properties_attr_group = {
	.attrs = ft5x0x_properties_attrs,
};


static int ft5x0x_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ft5x0x_ts_data *ft5x0x_ts;
	struct input_dev *input_dev;
	int err = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	ft5x0x_ts = kzalloc(sizeof(*ft5x0x_ts), GFP_KERNEL);
	if (!ft5x0x_ts)	{
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}


	i2c_set_clientdata(client, ft5x0x_ts);

	INIT_WORK(&ft5x0x_ts->pen_event_work, ft5x0x_ts_pen_irq_work);
	ft5x0x_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
	if (!ft5x0x_ts->ts_workqueue) {
		err = -ESRCH;
		goto exit_create_singlethread;
	}

	gpio_direction_input(FT5X0X_PENDOWN_GPIO);

	err = request_irq(client->irq, ft5x0x_ts_interrupt,
		IRQF_TRIGGER_FALLING, FT5X0X_NAME, ft5x0x_ts);

	if (err < 0) {
		dev_err(&client->dev, "ft5x0x_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}

	disable_irq(client->irq);

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}

	ft5x0x_ts->input_dev = input_dev;

#ifdef CONFIG_FT5X0X_MULTITOUCH
	set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);

	input_set_abs_params(input_dev,
			     ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);
#else
	set_bit(ABS_X, input_dev->absbit);
	set_bit(ABS_Y, input_dev->absbit);
	set_bit(ABS_PRESSURE, input_dev->absbit);
	set_bit(BTN_TOUCH, input_dev->keybit);

	input_set_abs_params(input_dev, ABS_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, PRESS_MAX, 0 , 0);
#endif

	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);

	input_dev->name		= FT5X0X_NAME;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;

	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
		"ft5x0x_ts_probe: failed to register input device: %s\n",
		dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	ft5x0x_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ft5x0x_ts->early_suspend.suspend = ft5x0x_ts_suspend;
	ft5x0x_ts->early_suspend.resume	= ft5x0x_ts_resume;
	register_early_suspend(&ft5x0x_ts->early_suspend);
#endif

	this_client = client;

	ft5x0x_ts_reset();
	ft5x0x_ts_enable();

	enable_irq(client->irq);

	//add virtual key start
	ft5x0x_properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (ft5x0x_properties_kobj){
		err = sysfs_create_group(ft5x0x_properties_kobj,
			&ft5x0x_properties_attr_group);
	}

	if (!ft5x0x_properties_kobj || err){
		FT5X0X_TSDBUG("failed to create ft5x0x_board_properties\n");
	}

	if (err)
		goto exit_input_register_device_failed;
	//virtual key end

	return 0;

exit_input_register_device_failed:
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
	free_irq(client->irq, ft5x0x_ts);
exit_irq_request_failed:
	cancel_work_sync(&ft5x0x_ts->pen_event_work);
	destroy_workqueue(ft5x0x_ts->ts_workqueue);
exit_create_singlethread:
	i2c_set_clientdata(client, NULL);
	kfree(ft5x0x_ts);
exit_alloc_data_failed:
exit_check_functionality_failed:
	printk("__ft5x0x_ts_probe error!\n");
	return err;
}

static int __devexit ft5x0x_ts_remove(struct i2c_client *client)
{
	struct ft5x0x_ts_data *ft5x0x_ts = i2c_get_clientdata(client);

	unregister_early_suspend(&ft5x0x_ts->early_suspend);
	free_irq(client->irq, ft5x0x_ts);
	input_unregister_device(ft5x0x_ts->input_dev);
	kfree(ft5x0x_ts);
	cancel_work_sync(&ft5x0x_ts->pen_event_work);
	destroy_workqueue(ft5x0x_ts->ts_workqueue);
	i2c_set_clientdata(client, NULL);

	return 0;
}

static const struct i2c_device_id ft5x0x_ts_id[] = {
	{ FT5X0X_I2C_NAME, 0 },{ }
};
MODULE_DEVICE_TABLE(i2c, ft5x0x_ts_id);

static struct i2c_driver ft5x0x_ts_driver = {
	.probe	= ft5x0x_ts_probe,
	.remove	= __devexit_p(ft5x0x_ts_remove),
	.id_table	= ft5x0x_ts_id,
	.driver	= {
		.name = FT5X0X_I2C_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init ft5x0x_ts_init(void)
{
	return i2c_add_driver(&ft5x0x_ts_driver);
}

static void __exit ft5x0x_ts_exit(void)
{
	i2c_del_driver(&ft5x0x_ts_driver);
}

module_init(ft5x0x_ts_init);
module_exit(ft5x0x_ts_exit);

MODULE_AUTHOR("<wenfs@Focaltech-systems.com>");
MODULE_DESCRIPTION("FocalTech ft5x0x TouchScreen driver");
MODULE_LICENSE("GPL");

