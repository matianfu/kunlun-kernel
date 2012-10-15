/*
 *  drivers/switch/switch_gpio.c
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
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
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/switch.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/i2c/twl.h>
//#define VIA_DEBUG 1
#ifdef VIA_DEBUG
#undef headset_debug
#define headset_debug(format, arg...) printk(format "\n" , ##arg)
#else
#undef headset_debug
#define headset_debug(format, arg...) do {} while (0)
#endif
struct gpio_switch_data {
	struct switch_dev sdev;
	struct input_dev *input;
	atomic_t btn_state;
	unsigned gpio;
	unsigned mute;
	unsigned cable_in1;
	unsigned cable_in2;
	const char *name_on;
	const char *name_off;
	const char *state_on;
	const char *state_off;
	int irq;
	int irq_btn;
	struct delayed_work work_hddetect;
	struct delayed_work work_hdbutton;
	struct delayed_work work_enable_button_irq;
};

/*the GPIO value according to the state*/
#ifdef CONFIG_MACH_OMAP_KUNLUN
#if defined(CONFIG_MACH_OMAP_KUNLUN_P2)
#define HD_LEVEL_IN	(1)
#elif defined(CONFIG_MACH_OMAP_KUNLUN_N710E)
#define HD_LEVEL_IN	(0)
#else
#define HD_LEVEL_IN	(1)
#endif
#else
#define HD_LEVEL_IN	(0)
#endif
#define HD_LEVEL_OUT	(!HD_LEVEL_IN)
#define BTN_LEVEL_DOWN	(1)
#define BTN_LEVEL_UP	(!BTN_LEVEL_DOWN)

#define HD_DET_DELAY	(300) //ms
#define BTN_DET_DELAY	(120)  //ms

static int hd_previous_state = 0;
static int hd_last_state = 0;
static int btn_last_state = 0;
static struct gpio_switch_data *g_switch_data;
static struct workqueue_struct *hd_work_queue;
static int headset_button_irq_num = 0;
static int headset_detect_irq_num = 0;
static int hddetect_from_resume = 0;
/*1: inserted, 0:Out*/
static int get_hd_state(unsigned gpio)
{
    int ret;

    ret = !!gpio_get_value(gpio);
    if(HD_LEVEL_IN == ret){
        ret = 1;
    }else{
        ret = 0;
    }

    return ret;
}

/*1: Down, 0:Up*/
static int get_btn_state(unsigned gpio)
{
    int ret;

    ret = !!gpio_get_value(gpio);
    if(BTN_LEVEL_DOWN == ret){
        ret = 1;
    }else{
        ret = 0;
    }

    return ret;
}

static void button_pressed(struct gpio_switch_data *data)
{
	headset_debug("[H2W] Button: Down\n");
	atomic_set(&data->btn_state, 1);
	input_report_key(data->input, KEY_MEDIA, 1);
	input_sync(data->input);
}

static void button_released(struct gpio_switch_data *data)
{
	headset_debug( "[H2W] Button: Up\n");
	atomic_set(&data->btn_state, 0);
	input_report_key(data->input, KEY_MEDIA, 0);
	input_sync(data->input);
}

static void headset_button_event(struct gpio_switch_data *data, int is_press)
{
	if (!is_press) {
	    if (atomic_read(&data->btn_state))
	         button_released(data);
	} else {
	    if ( !atomic_read(&data->btn_state))
		button_pressed(data);
	}
}

static void headset_detect_work(struct work_struct *work)
{
	static int step = 0;
	int state;
	struct gpio_switch_data	*data =
		container_of(work, struct gpio_switch_data, work_hddetect.work);
	u8 value;

	switch(step){
		/*make the debounce*/
		case 0:
			//disable_irq(data->irq);
			step = 1;
			hd_last_state = get_hd_state(data->cable_in1);
			queue_delayed_work(hd_work_queue, &data->work_hddetect, msecs_to_jiffies(HD_DET_DELAY));
			break;

		/*check the debouce result*/
		case 1:
			step = 0;
			state = get_hd_state(data->cable_in1);
			headset_debug("%s: curr_hd = %d, last_hd = %d\n", __FUNCTION__, state, hd_last_state);
			if(state == hd_last_state) {                           //headset state stable
				if(state != hd_previous_state) {		//do nothing if state not changed after interrupt
					switch_set_state(&data->sdev, state);
					hd_previous_state = state;
					if(state){				//headset plug in
						twl_i2c_read_u8(0x01, &value, 0x04);
						value |= 0x04;
						twl_i2c_write_u8(0x01, value, 0x04);
						queue_delayed_work(hd_work_queue, &data->work_enable_button_irq, msecs_to_jiffies(2000));
						if(hddetect_from_resume == 1) {
							hddetect_from_resume = 0;
							gpio_direction_input(data->cable_in1); //enable headset detect, last time disabled when suspend
						}
					} else {				//headset plug out
						disable_irq(data->irq_btn);
						twl_i2c_read_u8(0x01, &value, 0x04);
						value &= 0xFB;
						twl_i2c_write_u8(0x01, value, 0x04);
						if(hddetect_from_resume == 1) {
							hddetect_from_resume = 0;
							gpio_direction_input(data->cable_in1); //enable headset detect, last time disabled when suspend
						}
						else {
							enable_irq(data->irq);
						}

					}
				} else {
					headset_debug("fake interrupt, igore!!\n");

					if(hddetect_from_resume == 1){
						hddetect_from_resume = 0;
						gpio_direction_input(data->cable_in1);
					}
					else {
						enable_irq(data->irq);
					}
				}
			}else{
				/* Check again, because the work maybe canncled after step 0
				 * when go to suspend. Then the detection would be failed
				 * during resume which start checking at step 1.
				*/
				queue_delayed_work(hd_work_queue, &data->work_hddetect, 0);
				headset_debug("%s: check again\n", __FUNCTION__);
			}
			break;

		default:
			printk("[H2W headset]: %s unknow case.\n", __FUNCTION__);
	}
}

static void enable_button_irq_work(struct work_struct *work)
{
	struct gpio_switch_data	*data =
		container_of(work, struct gpio_switch_data, work_enable_button_irq.work);

	if(hddetect_from_resume == 1) {
		hddetect_from_resume = 0;
	}
	else {
		enable_irq(data->irq);
	}
	enable_irq(data->irq_btn);
	headset_debug("enable_button_irq_work");
}
static void headset_button_work(struct work_struct *work)
{
	static int step = 0;
	int state;
	struct gpio_switch_data	*data =
		container_of(work, struct gpio_switch_data, work_hdbutton.work);

	switch(step){
		case 0:
			disable_irq(data->irq_btn);
			step = 1;
			btn_last_state = get_btn_state(data->cable_in2);
			queue_delayed_work(hd_work_queue, &data->work_hdbutton, msecs_to_jiffies(BTN_DET_DELAY));
			break;

		case 1:
			step = 0;
			state = get_hd_state(data->cable_in1);
			headset_debug("%s: curr_hd = %d, last_hd = %d\n", __FUNCTION__, state, hd_last_state);
			if(state){
				state = get_btn_state(data->cable_in2);
				headset_debug("%s: curr_btn = %d, last_btn = %d\n", __FUNCTION__, state, btn_last_state);
				if(state == btn_last_state){
					headset_button_event(data, state);
				}
			}
			enable_irq(data->irq_btn);
			break;

		default:
			printk("[H2W headset]: %s unknow case.\n", __FUNCTION__);
	}
}

/*  Warnning :
 *  The operations of TWL5030 GPIO are based on I2C, which may be scheduled.
 *  So we must not get the GPIO value for debouce in IRQ context
 */
static irqreturn_t hddetect_irq_handler(int irq, void *dev_id)
{
	struct gpio_switch_data *switch_data = (struct gpio_switch_data *)dev_id;
	disable_irq_nosync(switch_data->irq);
	headset_debug("[H2W] HD Irq: hd_state=%d\n", hd_last_state);
	queue_delayed_work(hd_work_queue, &switch_data->work_hddetect, 0);
	return IRQ_HANDLED;
}

/*  Warnning :
 *  The operations of TWL5030 GPIO are based on I2C, which may be scheduled.
 *  So we must not get the GPIO value for debouce in IRQ context
 */
static irqreturn_t hdbutton_irq_handler(int irq, void *dev_id)
{
	struct gpio_switch_data *switch_data = (struct gpio_switch_data *)dev_id;

	headset_debug("[H2W] BTN Irq: hd_state=%d\n", hd_last_state);
	if(hd_previous_state){
		queue_delayed_work(hd_work_queue, &switch_data->work_hdbutton, 0);
	}

	return IRQ_HANDLED;
}

static ssize_t switch_gpio_print_state(struct switch_dev *sdev, char *buf)
{
	struct gpio_switch_data	*switch_data =
		container_of(sdev, struct gpio_switch_data, sdev);
	const char *state;
	if (switch_get_state(sdev))
		state = switch_data->state_on;
	else
		state = switch_data->state_off;

	if (state)
		return sprintf(buf, "%s\n", state);
	return -1;
}

static int gpio_switch_probe(struct platform_device *pdev)
{
	struct gpio_switch_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_switch_data *switch_data;
	int ret = 0;
	u8 value;
	if (!pdata)
		return -EBUSY;
	switch_data = kzalloc(sizeof(struct gpio_switch_data), GFP_KERNEL);
	if (!switch_data)
		return -ENOMEM;
	atomic_set(&switch_data->btn_state, 0);

	switch_data->sdev.name = pdata->name;
	//switch_data->gpio = pdata->gpio;
	switch_data->name_on = pdata->name_on;
	switch_data->name_off = pdata->name_off;
	switch_data->state_on = pdata->state_on;
	switch_data->state_off = pdata->state_off;
	switch_data->sdev.print_state = switch_gpio_print_state;
	switch_data->cable_in1=pdata->cable_in1;
	switch_data->cable_in2=pdata->cable_in2;
	switch_data->mute=pdata->mute;
	ret = switch_dev_register(&switch_data->sdev);
	if (ret < 0)
		goto err_switch_dev_register;
         headset_debug("gpio_switch_probe: line: %d\n",__LINE__);
	ret = gpio_request(switch_data->cable_in1, "headset_detect");
	if (ret < 0)
		goto err_request_gpio;

	ret = gpio_direction_input(switch_data->cable_in1);
	if (ret < 0)
		goto err_set_detectgpio_input;
	INIT_DELAYED_WORK(&switch_data->work_hddetect, headset_detect_work);
	INIT_DELAYED_WORK(&switch_data->work_hdbutton, headset_button_work);
	INIT_DELAYED_WORK(&switch_data->work_enable_button_irq, enable_button_irq_work);
	ret = gpio_request(switch_data->cable_in2, "headset_button");
	if (ret < 0)
		goto err_request_gpio;
	ret = gpio_direction_input(switch_data->cable_in2);
	if (ret < 0)
		goto err_set_buttongpio_input;
	switch_data->irq = gpio_to_irq(switch_data->cable_in1);
	if (switch_data->irq < 0) {
		ret = switch_data->irq;
		goto err_detect_detectirq_num_failed;
	}
	ret = request_irq(switch_data->irq, hddetect_irq_handler,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"headset_detect", switch_data);
	if (ret < 0)
		goto err_request_detectirq;


	switch_data->irq_btn= gpio_to_irq(switch_data->cable_in2);
	if (switch_data->irq_btn < 0) {
		ret = switch_data->irq_btn;
		goto err_detect_buttonirq_num_failed;
	}
	ret = request_irq(switch_data->irq_btn, hdbutton_irq_handler,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"headset_button", switch_data);
	if (ret < 0)
		goto err_request_buttonirq;
	headset_detect_irq_num = switch_data->irq;
	headset_button_irq_num = switch_data->irq_btn;
	/* Perform initial detection */
	hd_previous_state = get_hd_state(switch_data->cable_in1);
	switch_set_state(&switch_data->sdev, hd_previous_state);
	headset_debug("gpio_switch_probe: headset: %d\n", hd_previous_state);
	if(!hd_previous_state) {
		disable_irq(switch_data->irq_btn);
	} else { /*Pullup the MICBIAS_CTL to enable the button detection
		during the system booting while headset plug in!*/
		twl_i2c_read_u8(0x01, &value, 0x04);
		value |= 0x04;
		twl_i2c_write_u8(0x01, value, 0x04);
	}

	switch_data->input = input_allocate_device();
	if (!switch_data->input) {
		ret = -ENOMEM;
		goto err_request_input_dev;
	}

	switch_data->input->name = "h2w headset";
	__set_bit(EV_KEY, switch_data->input->evbit);
	__set_bit(KEY_MEDIA, switch_data->input->keybit);
	#if 0
	set_bit(KEY_NEXTSONG, switch_data->input->keybit);
	set_bit(KEY_PLAYPAUSE, switch_data->input->keybit);
	set_bit(KEY_PREVIOUSSONG, switch_data->input->keybit);
	set_bit(KEY_MUTE, switch_data->input->keybit);
	set_bit(KEY_VOLUMEUP, switch_data->input->keybit);
	set_bit(KEY_VOLUMEDOWN, switch_data->input->keybit);
	set_bit(KEY_END, switch_data->input->keybit);
	set_bit(KEY_SEND, switch_data->input->keybit);
	#endif
	ret = input_register_device(switch_data->input);
	if (ret < 0)
		goto err_register_input_dev;

	platform_set_drvdata(pdev, switch_data);
	g_switch_data = switch_data;
	hd_work_queue = create_singlethread_workqueue("headset");
	if (hd_work_queue == NULL) {
		ret = -ENOMEM;
		goto err_register_input_dev;
	}

	return 0;
err_register_input_dev:
	input_free_device(switch_data->input);
err_request_input_dev:
	free_irq(switch_data->irq_btn, 0);
err_request_detectirq:
err_detect_detectirq_num_failed:
err_set_detectgpio_input:
	gpio_free(switch_data->cable_in1);
err_request_buttonirq:
err_detect_buttonirq_num_failed:
err_set_buttongpio_input:
	gpio_free(switch_data->cable_in2);
err_request_gpio:
    switch_dev_unregister(&switch_data->sdev);
err_switch_dev_register:
	kfree(switch_data);

	return ret;
}

static int __devexit gpio_switch_remove(struct platform_device *pdev)
{
	struct gpio_switch_data *switch_data = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&switch_data->work_hddetect);
	cancel_delayed_work_sync(&switch_data->work_hdbutton);
	gpio_free(switch_data->cable_in1);
	gpio_free(switch_data->cable_in2);
	switch_dev_unregister(&switch_data->sdev);
	kfree(switch_data);
	g_switch_data = NULL;
	return 0;
}

#ifdef CONFIG_PM
int gpio_switch_suspend(void)
{
	struct gpio_switch_data *switch_data = g_switch_data;
	disable_irq(switch_data->irq_btn);
	disable_irq(switch_data->irq);
	gpio_direction_output(switch_data->cable_in1,0);
	cancel_delayed_work_sync(&switch_data->work_hddetect);
	cancel_delayed_work_sync(&switch_data->work_hdbutton);
	headset_debug("%s: curr_hd = %d\n", __FUNCTION__,  get_hd_state(switch_data->cable_in1));

	return 0;
}

int gpio_switch_resume(void)
{
	struct gpio_switch_data *switch_data = g_switch_data;

	enable_irq(switch_data->irq);
	enable_irq(switch_data->irq_btn);
	headset_debug("%s: curr_hd = %d, last_hd = %d\n", __FUNCTION__, get_hd_state(switch_data->cable_in1), hd_previous_state);
	/*headset state maybe has been changed during Ap sleep, so chech headset state again after resume*/
	hddetect_from_resume = 1;
	queue_delayed_work(hd_work_queue, &switch_data->work_hddetect, msecs_to_jiffies(HD_DET_DELAY));

	return 0;
}
#else
int gpio_switch_suspend(void)
{
      return 0;
}
int gpio_switch_resume(void)
{
      return 0;
}
#endif /* CONFIG_PM */

EXPORT_SYMBOL_GPL(gpio_switch_suspend);
EXPORT_SYMBOL_GPL(gpio_switch_resume);

int enable_headset_button_irq(int enable)
{
	if(hd_previous_state) {
		headset_debug("enable_headset_button_irq enable = %d\n",enable);
		if(enable){
			enable_irq(headset_button_irq_num);
			enable_irq(headset_detect_irq_num);
		} else {
			disable_irq(headset_button_irq_num);
			disable_irq(headset_detect_irq_num);
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(enable_headset_button_irq);

static struct platform_driver gpio_switch_driver = {
	.probe		= gpio_switch_probe,
	.remove		= __devexit_p(gpio_switch_remove),
	.driver		= {
		.name	= "switch-gpio",
		.owner	= THIS_MODULE,
	},
};

static int __init gpio_switch_init(void)
{
	headset_debug("[H2W headset]: gpio_switch_init\n");
	return platform_driver_register(&gpio_switch_driver);
}

static void __exit gpio_switch_exit(void)
{
	platform_driver_unregister(&gpio_switch_driver);
}

#ifndef MODULE
late_initcall(gpio_switch_init);
#else
module_init(gpio_switch_init);
#endif
module_exit(gpio_switch_exit);

MODULE_AUTHOR("Mike Lockwood <lockwood@android.com>");
MODULE_DESCRIPTION("GPIO Switch driver");
MODULE_LICENSE("GPL");
