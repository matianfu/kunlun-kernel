/*
 * linux/drivers/power/ap_modem.c
 *
 * VIA CBP driver for Linux
 *
 * Copyright (C) 2009 VIA TELECOM Corporation, Inc.
 * Author: VIA TELECOM Corporation, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <linux/ctype.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <mach/board-kunlun.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/usb.h>
#include "../drivers/usb/core/usb.h"

#if defined(CONFIG_MACH_OMAP_KUNLUN)
#define GPIO_POWER_MDM      (126)
#define GPIO_RESET_MDM      (149)
#define GPIO_AP_WAKE_MDM    (127)
#define GPIO_MDM_WAKE_AP    (129)
#define GPIO_GPS_PWR_EN     (16)
#endif

#define POWER_HOLD_DELAY    (500) //ms
#define RESET_HOLD_DELAY    (100) //ms
#define WAKE_HOLD_DELAY	    (2) //ms

#define WAKE_LEVEL	(1)
#define SLEEP_LEVEL	(!(WAKE_LEVEL))

//#define VIA_AP_MODEM_DEBUG
#ifdef VIA_AP_MODEM_DEBUG
#undef dbg
#define dbg(format, arg...) printk("[AP_MODEM]: " format "\n" , ## arg)
#else
#undef dbg
#define dbg(format, arg...) do {} while (0)
#endif

#define VENDOR_ID   (0x15eb)
#define PRODUCT_ID  (0x0001)

#define RH_VENDOR_ID    (0x1d6b)
#define RH_PRODUCT_ID   (0x0001)

struct ap_modem{
    struct wake_lock modem_lock;
     struct wake_lock reset_lock;
    struct regulator *vsim;
    int wake_count;
    spinlock_t lock;
};
static struct ap_modem ap_mdm;
static struct ap_modem *pmdm = &ap_mdm;

/*Emulate modem detached by pull P0M1 when CBP jump*/
#define MDM_JUMP_DELAY	    (1100) //time for emulating detached by pull P0M1

/*Emulate modem detached by pull P0M1 when CBP reset*/
#define MDM_RESET_DELAY	(5500) //time for emulating detached by pull P0M1

#define MDM_WAKEUP_LOCK_TIME (3) // seconds that lock the console to susupend
#define MDM_RESET_LOCK_TIME (120) 
extern int in_dpm_suspend;

static int has_error_flag = 0;
static int usb_be_sleep(void)
{
    return (!!gpio_get_value(GPIO_AP_WAKE_MDM)) == SLEEP_LEVEL;
}

#if defined(CONFIG_USB_SUSPEND)
static DECLARE_WAIT_QUEUE_HEAD(mdm_pull_delay);

struct cp_usb_device {
    struct usb_device * cp_dev;
    struct mutex cp_mutex;
};

static struct cp_usb_device _cp_usb_dev = {
    .cp_dev = NULL,
};
void ap_bind_usb_device(struct usb_device *dev) {
    dbg("%s: enter\n", __func__);
    dbg("dev=0x%p, name=%s, devp=0x%p, name=%s\n",
        dev, kobject_name(&dev->dev.kobj), dev->parent, kobject_name(&dev->parent->dev.kobj));
    
    if(_cp_usb_dev.cp_dev)
    {
        printk(KERN_ERR "%s: already initialized\n", __func__);
        return;
    }
    mutex_init(&(_cp_usb_dev.cp_mutex));
    mutex_lock(&(_cp_usb_dev.cp_mutex));
    if (_cp_usb_dev.cp_dev && _cp_usb_dev.cp_dev!= dev)
        printk(KERN_ERR "cp usb dev(%p) has binded, overwrite with(%p).\n",
                _cp_usb_dev.cp_dev, dev);
    _cp_usb_dev.cp_dev = dev;
    mutex_unlock(&(_cp_usb_dev.cp_mutex));

    return;
}
EXPORT_SYMBOL_GPL(ap_bind_usb_device);

void ap_unbind_usb_device(void) {
    dbg("%s: enter\n", __func__);

    if(_cp_usb_dev.cp_dev)
    {
        mutex_lock(&(_cp_usb_dev.cp_mutex));
        _cp_usb_dev.cp_dev = NULL;
        mutex_unlock(&(_cp_usb_dev.cp_mutex));
        mutex_destroy(&(_cp_usb_dev.cp_mutex));
    }
    else
    {
        printk(KERN_ERR "%s: already destroy, do noting\n", __func__);
    }
    return;
}
EXPORT_SYMBOL_GPL(ap_unbind_usb_device);

/* added by sguan, for usb device wakeup */
static void usb_wakeup(struct work_struct *p)
{
    struct usb_device *udev = NULL;

    if(usb_be_sleep()){
        for(;;)
        {
            if(in_dpm_suspend) {
                has_error_flag = 1;
                schedule_timeout(msecs_to_jiffies(500));
            } else {
                printk(KERN_ERR "%s: in_dpm_suspend change, can wake RH\n", __func__);
                break;
            }
        }
        if(has_error_flag)
        {
            printk(KERN_ERR "%s: sguan--ERROR, usb_wakeup occur before dpm_resume!!!!!\n", __func__);
        }
        has_error_flag = 0;
        if(_cp_usb_dev.cp_dev == NULL)
        {
            printk(KERN_ERR "%s: Can't find cp usb devices, error!!\n", __func__);
            return;
        }
        mutex_lock(&(_cp_usb_dev.cp_mutex));
        udev = _cp_usb_dev.cp_dev;
        usb_get_dev(udev);
        if (!udev) {
            printk(KERN_ERR "%s - usb find device error\n", __func__);
            mutex_unlock(&(_cp_usb_dev.cp_mutex));
            return;
        }
        usb_lock_device(udev);
        usb_mark_last_busy(udev);
        if(usb_autoresume_device(udev) == 0)
        {
            usb_autosuspend_device(udev);
        }
        usb_unlock_device(udev);

        usb_put_dev(udev);
        mutex_unlock(&(_cp_usb_dev.cp_mutex));
    }
    return;
}

static DECLARE_WORK(usb_wakeup_work, usb_wakeup);
/* End of added by sguan */
#endif

void ap_wake_modem(void)
{
#ifdef CONFIG_BOOTCASE_VIATELECOM
    /*boot by charger*/
    if(!strncmp(get_bootcase(), "charger", strlen("charger"))){
        return ;
    }
#endif
    gpio_direction_output(GPIO_AP_WAKE_MDM, WAKE_LEVEL);
    //mdelay(WAKE_HOLD_DELAY);
}
EXPORT_SYMBOL_GPL(ap_wake_modem);

void ap_sleep_modem(void)
{
#ifdef CONFIG_BOOTCASE_VIATELECOM
    /*boot by charger*/
    if(!strncmp(get_bootcase(), "charger", strlen("charger"))){
        return ;
    }
#endif
    gpio_direction_output(GPIO_AP_WAKE_MDM, SLEEP_LEVEL);
    //mdelay(WAKE_HOLD_DELAY);
}
EXPORT_SYMBOL_GPL(ap_sleep_modem);

void ap_poweroff_modem(void)
{
     gpio_direction_output(GPIO_POWER_MDM, 0);
     gpio_direction_output(GPIO_RESET_MDM, 1);
     mdelay(RESET_HOLD_DELAY);
     gpio_direction_output(GPIO_RESET_MDM, 0);
     mdelay(1);
}
EXPORT_SYMBOL(ap_poweroff_modem);

ssize_t modem_power_show(
	struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int power = 0;
    int ret = 0;

    power = !!gpio_get_value(GPIO_POWER_MDM);
    if(power){
        ret += sprintf(buf + ret, "on\n");
    }else{
        ret += sprintf(buf + ret, "off\n");
    }
    printk(KERN_ERR "Just show the GPIO_POWER_MDM status, not indicate the power status.");
    return ret;
}

ssize_t modem_power_store(
	struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t n)
{
    int power;

    /* power the modem */
    if ( !strncmp(buf, "on", strlen("on"))) {
        power = 1;
    }else if(!strncmp(buf, "off", strlen("off"))){
        power = 0;
    }else{
        pr_info("%s: input %s is invalid.\n", __FUNCTION__, buf);
        goto end;
    }

    /* FIXME: AP must power on the GPS and switch the GPS control to CBP,
      * otherwise the CBP will reboot because the error commucation with GPS.
      */
    if(power){
        gpio_direction_output(GPIO_GPS_PWR_EN, 1);
        mdelay(20);
    }

    printk("AP_MODEM: Power %s modem.\n", power?"on":"off");
    ap_wake_modem();

    /*set power pin down-up-down when power off*/
    if(power){
        gpio_direction_output(GPIO_POWER_MDM, power);
        mdelay(POWER_HOLD_DELAY);
        gpio_direction_output(GPIO_POWER_MDM, 0); //When power on, gpio_power_mdm will keep 0, not indicate the power status
    }
    else{
        ap_poweroff_modem();
    }

end:
    return n;
}

ssize_t modem_reset_show(
	struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int reset = 0;
    int ret = 0;

    reset = !!gpio_get_value(GPIO_RESET_MDM);
    if(reset){
        ret += sprintf(buf + ret, "resetting\n");
    }else{
        ret += sprintf(buf + ret, "working\n");
    }

    return ret;
}

ssize_t modem_reset_store(
	struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t n)
{
    /* reset the modem */
    if ( !strncmp(buf, "reset", strlen("reset"))) {
         schedule_work(&usb_wakeup_work);
         wake_lock_timeout(&pmdm->reset_lock, MDM_RESET_LOCK_TIME * HZ);
         schedule_timeout(HZ);
         /*Power off the CP*/
         gpio_direction_output(GPIO_POWER_MDM, 0);
         gpio_direction_output(GPIO_RESET_MDM, 1);
         mdelay(RESET_HOLD_DELAY);
         gpio_direction_output(GPIO_RESET_MDM, 0);

         /*simulate Power Key pressed to boot CP*/
         gpio_direction_output(GPIO_POWER_MDM, 1);
         mdelay(POWER_HOLD_DELAY);
         gpio_direction_output(GPIO_POWER_MDM, 0);

         printk("Warnning: Reset modem\n");
    }

    return n;
}

ssize_t modem_status_show(
	struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int sleep = 0;
    int ret = 0;

    sleep = !!gpio_get_value(GPIO_MDM_WAKE_AP);
    if(sleep == SLEEP_LEVEL){
        ret += sprintf(buf + ret, "sleep\n");
    }else{
        ret += sprintf(buf + ret, "wake\n");
    }

    return ret;
}

ssize_t modem_status_store(
	struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t n)
{
    /*do nothing*/
    return n;
}

ssize_t modem_set_show(
	struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int sleep = 0;
    int ret = 0;

    sleep = !!gpio_get_value(GPIO_AP_WAKE_MDM);
    if(sleep == SLEEP_LEVEL){
        ret += sprintf(buf + ret, "sleep\n");
    }else{
        ret += sprintf(buf + ret, "wake\n");
    }

    return ret;
}

ssize_t modem_set_store(
	struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t n)
{
    if ( !strncmp(buf, "wake", strlen("wake"))) {
        ap_wake_modem();
    }else if( !strncmp(buf, "sleep", strlen("sleep"))){
        ap_sleep_modem();
    }else{
        dbg("Unknow command.\n");
    }

    return n;
}

/*FIXME: the GPIO_AP_WAKE_MDM maybe should be kept*/
static int ap_suspend(struct platform_device *pdev, pm_message_t state)
{
    ap_sleep_modem();
    return 0;
}

static int ap_resume(struct platform_device *pdev)
{
    int sleep;

    sleep = !!gpio_get_value(GPIO_MDM_WAKE_AP);
    dbg("%s: Modem request AP to be %s.\n", __FUNCTION__, sleep?"WAKEN":"SLEEP");

    ap_wake_modem();
    return 0;
}

/*the action to sleep or wake the modem would be done in OHCI driver*/
static struct platform_driver ap_modem_driver = {
	.driver.name = "ap_modem",
#if !defined(CONFIG_USB_SUSPEND)
	.suspend = ap_suspend,
	.resume = ap_resume,
#endif
};

static struct platform_device ap_modem_device = {
	.name = "ap_modem",
};

/*
 * modem status interrupt handler
 */
extern void ohci_resume_rh_autostop(void);
static irqreturn_t modem_wake_ap_irq(int irq, void *data)
{
    int sleep;

    sleep = !!gpio_get_value(GPIO_MDM_WAKE_AP);
    dbg("Modem request AP to be %s.\n", sleep?"WAKEN":"SLEEP");
#if defined(CONFIG_USB_SUSPEND)
    ohci_resume_rh_autostop();
    if(usb_be_sleep()){
        schedule_work(&usb_wakeup_work);
    }
#endif
    /*skip pm suspend for a while*/
    wake_lock_timeout(&pmdm->modem_lock, MDM_WAKEUP_LOCK_TIME * HZ);

    return IRQ_HANDLED;
}

static int __init ap_modem_init(void)
{
    int ret = 0;

#ifdef CONFIG_BOOTCASE_VIATELECOM
    /*CP has not been powered on when boot by charger, then do nothing*/
    if(!strncmp(get_bootcase(), "charger", strlen("charger"))){
        return 0;
    }
#endif

#if 0
    /* GPIO output 3V from PBIAS, which supplied by VSIM */
    pmdm->vsim = regulator_get(NULL, "VSIM");
    if(pmdm->vsim == NULL){
        pr_err("%s:fail to get vsim regulator.\n", __FUNCTION__);
        goto err_reg_driver;
    }
    regulator_enable(pmdm->vsim);
#endif
    pmdm->wake_count = 0;
    spin_lock_init(&pmdm->lock);
    wake_lock_init(&pmdm->modem_lock, WAKE_LOCK_SUSPEND, "modem");
    wake_lock_init(&pmdm->reset_lock, WAKE_LOCK_SUSPEND, "modem_rst");

    ret = platform_device_register(&ap_modem_device);
    if (ret < 0) {
        pr_err("ap_modem_init: platform_device_register failed\n");
        goto err_reg_device;
    }
    ret = platform_driver_register(&ap_modem_driver);
    if (ret < 0) {
        pr_err("ap_modem_init: platform_driver_register failed\n");
        goto err_reg_driver;
    }

    gpio_direction_input(GPIO_MDM_WAKE_AP);
    modem_wake_ap_irq(gpio_to_irq(GPIO_MDM_WAKE_AP), NULL);

    /*CP will wake AP by plull up the pin */
    set_irq_type(gpio_to_irq(GPIO_MDM_WAKE_AP), IRQ_TYPE_EDGE_RISING);
    ret = request_irq(gpio_to_irq(GPIO_MDM_WAKE_AP), modem_wake_ap_irq, IRQF_TRIGGER_RISING, "Modem ready", NULL);
    if (ret < 0) {
        pr_err("%s: fail to request irq for GPIO_MDM_WAKE_AP\n", __FUNCTION__);
        goto err_req_irq;
    }

    /* Make sure that the level of PowerKey pin keep low 
     * after power on CBP to avoid key press event to CBP
     * ,which maybe prevent CBP deep sleep
     */
    gpio_direction_output(GPIO_POWER_MDM, 0);
    gpio_direction_output(GPIO_RESET_MDM, 0);
    gpio_direction_output(GPIO_AP_WAKE_MDM, WAKE_LEVEL);
    gpio_direction_output(GPIO_GPS_PWR_EN, 1);
    ap_wake_modem();

    dbg("AP MODEM INIT is ok.\n");
    return 0;

err_req_irq:
    platform_driver_unregister(&ap_modem_driver);
err_reg_driver:
    platform_device_unregister(&ap_modem_device);
err_reg_device:

    return ret;
}

static void  __exit ap_modem_exit(void)
{
    free_irq(gpio_to_irq(GPIO_MDM_WAKE_AP), NULL);
    wake_lock_destroy(&pmdm->modem_lock);
    platform_driver_unregister(&ap_modem_driver);
    platform_device_unregister(&ap_modem_device);
}

/** UGlee **/
// late_initcall(ap_modem_init);
// module_exit(ap_modem_exit);


