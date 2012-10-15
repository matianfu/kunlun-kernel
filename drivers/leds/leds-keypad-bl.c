/*
 * =====================================================================================
 *
 *       Filename:  leds-keypad-bl.c
 *
 *    Description:  LEDs GPIO driver
 *
 *        Version:  1.0
 *        Created:  05/08/2011 15:33:43 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  juelun guo (jlguo), juelun guo <jlguo@via-telecom.com>
 *        Company:  VIA TELECOM
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *yx 2011 add thie file for keypads backlight
 * =====================================================================================
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <mach/gpio.h>

#include <mach/hardware.h>
#include <plat/led.h>

#include <linux/i2c/twl.h>


#define TWL_LEDEN                   (0x00)
#define TWL_CODEC_MODE              (0x01)
#define TWL_VIBRATOR_CFG            (0x05)
#define TWL_VIBRA_CTL               (0x45)
#define TWL_VIBRA_SET               (0x46)

#define TWL_CODEC_MODE_CODECPDZ		(0x01 << 1)
#define TWL_VIBRA_CTL_VIBRA_EN		(0x01)
#define TWL_VIBRA_CTL_VIBRA_DIS    	(0x00)


static void keypad_set_led(struct led_classdev *led_cdev,
			    enum led_brightness value)
{
	struct omap_led_config *led_dev;

	led_dev = container_of(led_cdev, struct omap_led_config, cdev);
	//if(!(led_cdev->flags & LED_SUSPENDED)){
	if(value){
		led_dev->cdev.brightness = value;
		printk(" LED  cdev.brightness %d \n",led_dev->cdev.brightness);
              twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE, TWL_VIBRA_CTL_VIBRA_EN,
		TWL_VIBRA_CTL);
	}else{
		printk(" LED  value %d \n",value);
            twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE, TWL_VIBRA_CTL_VIBRA_DIS,
		TWL_VIBRA_CTL);
	}

}

static int keypad_led_hardware_init(void)
{
    int ret = 0;
    u8 reg_value;

    printk(" keypad_led_hardware_init \n");

    ret = twl_i2c_read_u8(TWL4030_MODULE_LED, &reg_value,
		TWL_LEDEN);

    reg_value &= 0xFC;
    ret = twl_i2c_write_u8(TWL4030_MODULE_LED, reg_value,
		TWL_LEDEN);

    ret = twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE, TWL_CODEC_MODE_CODECPDZ,
		TWL_CODEC_MODE);

    return ret;
}

static int keypad_led_probe(struct platform_device *dev)
{
	struct omap_led_platform_data *pdata = dev->dev.platform_data;
	struct omap_led_config *leds = pdata->leds;
	int i, ret = 0;

	keypad_led_hardware_init();

	for (i = 0; ret >= 0 && i < pdata->nr_leds; i++) {
		if (!leds[i].cdev.brightness_set)
			leds[i].cdev.brightness_set = keypad_set_led;

		leds[i].cdev.brightness = 0;
		leds[i].cdev.flags = 0;

		ret = led_classdev_register(&dev->dev, &leds[i].cdev);
		if( ret < 0){
		    printk(" led %d register fail\n",i);
		    led_classdev_unregister(&leds[i].cdev);
	        }
	}

	return ret;
}

static int keypad_led_remove(struct platform_device *dev)
{
	struct omap_led_platform_data *pdata = dev->dev.platform_data;
	struct omap_led_config *leds = pdata->leds;
	int i;

	for (i = 0; i < pdata->nr_leds; i++) {
		led_classdev_unregister(&leds[i].cdev);
	}

	return 0;
}

static int keypad_led_suspend(struct platform_device *dev, pm_message_t state)
{
	struct omap_led_platform_data *pdata = dev->dev.platform_data;
	struct omap_led_config *leds = pdata->leds;
	int i;

	for (i = 0; i < pdata->nr_leds; i++){
		//leds[i].cdev.flags |= LED_SUSPENDED;
		led_classdev_suspend(&leds[i].cdev);
	}
	return 0;
}

static int keypad_led_resume(struct platform_device *dev)
{
	struct omap_led_platform_data *pdata = dev->dev.platform_data;
	struct omap_led_config *leds = pdata->leds;
	int i;

	for (i = 0; i < pdata->nr_leds; i++){
		leds[i].cdev.flags &= ~LED_SUSPENDED;
		led_classdev_resume(&leds[i].cdev);
	}
	return 0;
}

static struct platform_driver keypad_led_driver = {
	.probe		= keypad_led_probe,
	.remove		= keypad_led_remove,
	.suspend	= keypad_led_suspend,
	.resume		= keypad_led_resume,
	.driver		= {
		.name		= "button-backlight",
		.owner		= THIS_MODULE,
	},
};

static int __init keypad_led_init(void)
{
	return platform_driver_register(&keypad_led_driver);
}

static void __exit keypad_led_exit(void)
{
	platform_driver_unregister(&keypad_led_driver);
}

module_init(keypad_led_init);
module_exit(keypad_led_exit);

MODULE_AUTHOR("juelun guo<jlguo@via-telecom.com>");
MODULE_DESCRIPTION("KEYPAD LED driver");
MODULE_LICENSE("GPL");
