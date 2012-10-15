/*
 * =====================================================================================
 *
 *       Filename:  leds-wudang.c
 *
 *    Description:  WUDANG - LEDs GPIO driver
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
 * =====================================================================================
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <mach/gpio.h>
#include <linux/leds.h>

#include <mach/hardware.h>
#include <plat/led.h>

static void wudang_set_led_gpio(struct led_classdev *led_cdev,
			    enum led_brightness value)
{
	struct omap_led_config *led_dev;

	led_dev = container_of(led_cdev, struct omap_led_config, cdev);
	if(!(led_cdev->flags & LED_SUSPENDED)){
		led_dev->cdev.brightness = value;
		//printk(" LED  cdev.brightness %d \n",led_dev->cdev.brightness);
		gpio_set_value(led_dev->gpio, led_dev->cdev.brightness);
	}else{		
		//printk(" LED  value %d \n",value);
		gpio_set_value(led_dev->gpio, value);
	}

}

static int wudang_led_probe(struct platform_device *dev)
{
	struct omap_led_platform_data *pdata = dev->dev.platform_data;
	struct omap_led_config *leds = pdata->leds;
	int i, ret = 0;

	for (i = 0; ret >= 0 && i < pdata->nr_leds; i++) {
		ret = gpio_request(leds[i].gpio, leds[i].cdev.name);
		if (ret < 0){
			printk(" gpio %d request fail\n",i);
			gpio_free(leds[i].gpio);
			break;
		}
		gpio_direction_output(leds[i].gpio, 0);
		if (!leds[i].cdev.brightness_set)
			leds[i].cdev.brightness_set = wudang_set_led_gpio;

		leds[i].cdev.brightness = 0;
		leds[i].cdev.flags = 0;
		ret = led_classdev_register(&dev->dev, &leds[i].cdev);	
		if( ret < 0){
		    printk(" led %d register fail\n",i);
		    led_classdev_unregister(&leds[i].cdev);	
		    gpio_free(leds[i].gpio);	
		}
	}

	return ret;
}

static int wudang_led_remove(struct platform_device *dev)
{
	struct omap_led_platform_data *pdata = dev->dev.platform_data;
	struct omap_led_config *leds = pdata->leds;
	int i;

	for (i = 0; i < pdata->nr_leds; i++) {
		led_classdev_unregister(&leds[i].cdev);
		gpio_free(leds[i].gpio);
	}

	return 0;
}

static int wudang_led_suspend(struct platform_device *dev, pm_message_t state)
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

static int wudang_led_resume(struct platform_device *dev)
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

static struct platform_driver wudang_led_driver = {
	.probe		= wudang_led_probe,
	.remove		= wudang_led_remove,
	.suspend	= wudang_led_suspend,
	.resume		= wudang_led_resume,
	.driver		= {
		.name		= "button-backlight",
		.owner		= THIS_MODULE,
	},
};

static int __init wudang_led_init(void)
{
	return platform_driver_register(&wudang_led_driver);
}

static void __exit wudang_led_exit(void)
{
 	platform_driver_unregister(&wudang_led_driver);
}

module_init(wudang_led_init);
module_exit(wudang_led_exit);

MODULE_AUTHOR("juelun guo<jlguo@via-telecom.com>");
MODULE_DESCRIPTION("WUDANG LED driver");
MODULE_LICENSE("GPL");
