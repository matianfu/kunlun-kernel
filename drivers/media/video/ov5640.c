#include <linux/i2c.h>
#include <linux/delay.h>
#include <media/v4l2-int-device.h>
#include <linux/i2c/twl.h>
#include <mach/gpio.h>
#include "omap34xxcam.h"
#include "isp/isp.h"


static int __init ov5640_probe(struct i2c_client *client, const struct i2c_device_id *id)
{



}


static int __exit ov5640_remove(struct i2c_client *client)
{



}

static const struct i2c_driver_id ov5640_id[] = {
	{"ov5640", 0},
	{},
};

static struct i2c_driver ov5640sensor_i2c_driver = {
	.driver = {
		.name  = "ov5640",
		.owner = THIS_MODULE, 
	},
	.probe  = ov5640_probe,
	.remove = ov5640_remove, 
	.id_table = ov5640_id,
};

static int __init ov5640sensor_init(void)
{
	int err;

	err = i2c_add_driver(&ov5640sensor_i2c_driver);
	if (err) {
		printk(KERN_ERR "Failed to register ov5640sensor\n");
		return err;
	}

	return 0;
}
late_initcall(ov5640sensor_init);


static void __exit ov5640sensor_cleanup(void)
{

	i2c_del_driver(&ov5640sensor_i2c_driver);
}
module_exit(ov5640sensor_cleanup);

MODULE_LICENSE("GPL");
MODUEL_DESCRIPTION("ov5640 camera sensor driver");
