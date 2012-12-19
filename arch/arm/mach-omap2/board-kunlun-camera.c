/*
 * linux/arch/arm/mach-omap2/board-kunlun-camera.c
 *
 * Copyright (C) 2007 Texas Instruments
 *
 * Modified from mach-omap2/board-generic.c
 *
 * Initial code: Syed Mohammed Khasim
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mm.h>

#if defined(CONFIG_TWL4030_CORE) && defined(CONFIG_VIDEO_OMAP3)

#include <linux/i2c/twl.h>

#include <asm/io.h>

#include <mach/gpio.h>
#include <plat/omap-pm.h>

#include <linux/videodev2.h>

#if defined(CONFIG_VIDEO_HI253)
#include <media/hi253.h>
#endif

static int cam_inited;
#include <media/v4l2-int-device.h>
#include <../drivers/media/video/omap34xxcam.h>
#include <../drivers/media/video/isp/ispreg.h>
#define DEBUG_BASE		0x08000000

#define REG_SDP3430_FPGA_GPIO_2 (0x50)
#define FPGA_SPR_GPIO1_3v3	(0x1 << 14)
#define FPGA_GPIO6_DIR_CTRL	(0x1 << 6)

#define VAUX_2_8_V		0x09
#define VAUX_1_8_V		0x05
#define VAUX2_1_5_V     0x04
#define VAUX4_2_8_V     0x09

#define VAUX_DEV_GRP_P1		0x20
#define VAUX_DEV_GRP_NONE	0x00
#define VMMC2_1_8_V		0x06
#define VMMC2_1_5_V 		0x04

#define VMMC2_DEV_GRP_P1		0x20
#define VMMC2_DEV_GRP_NONE	0x00
#define CAMZOOM2_USE_XCLKB  	1

/* Sensor specific GPIO signals */
#define HI253_RESET_GPIO  	98
#define HI253_STANDBY_GPIO	110
#define BF3703_STANDBY_GPIO	180
#define BF3703_RESET_GPIO	179



//#if defined(CONFIG_VIDEO_BF3703)
#if 0
#include <media/bf3703.h>
#define BF3703_BIGGEST_FRAME_BYTE_SIZE	PAGE_ALIGN(1600 * 1200 * 2)

#define CAMKUNLUN_USE_XCLKB  	1

#define ISP_MCLK		216000000

//static int bf3703_powered = 0;
static struct omap34xxcam_sensor_config bf3703_hwc = {
    .sensor_isp  = 1,
    .capture_mem = BF3703_BIGGEST_FRAME_BYTE_SIZE * 2,
    .ival_default    = { 1, 30 },
};

static int bf3703_set_prv_data(struct v4l2_int_device *s,void *priv)
{
    struct omap34xxcam_hw_config *hwc = priv;

    hwc->u.sensor.sensor_isp = bf3703_hwc.sensor_isp;
    hwc->dev_index		= 1;
    hwc->dev_minor		= 4;
    hwc->dev_type		= OMAP34XXCAM_SLAVE_SENSOR;

    return 0;
}


static struct isp_interface_config bf3703_if_config = {
    .ccdc_par_ser 		= ISP_PARLL,
    .dataline_shift 	= 0x0,
    .hsvs_syncdetect 	= ISPCTRL_SYNC_DETECT_VSRISE,
    .strobe 		= 0x0,
    .prestrobe 		= 0x0,
    .shutter 		= 0x0,
    .wenlog 		= ISPCCDC_CFG_WENLOG_AND,
    .wait_hs_vs		= 2,
    .cam_mclk		= ISP_MCLK,
    .u.par.par_bridge = 2,
    .u.par.par_clk_pol = 0,
};


static int bf3703_power_set(struct v4l2_int_device *s, enum v4l2_power power)
{
    struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;
    struct isp_device *isp = dev_get_drvdata(vdev->cam->isp);
    static enum v4l2_power previous_power = V4L2_POWER_OFF;
    static struct pm_qos_request_list *qos_request;
    int err = 0;

    printk(KERN_INFO "previous_power = %d,bf3073_sensor_power_set(%d)\n",previous_power,power);

    switch (power) {
        case V4L2_POWER_ON:
            /*
             * Through-put requirement:
             * Set max OCP freq for 3630 is 200 MHz through-put
             * is in KByte/s so 200000 KHz * 4 = 800000 KByte/s
             */
            omap_pm_set_min_bus_tput(vdev->cam->isp,
                    OCP_INITIATOR_AGENT, 800000);


            /* Hold a constraint to keep MPU in C1 */
            omap_pm_set_max_mpu_wakeup_lat(&qos_request, 12);

            isp_configure_interface(vdev->cam->isp,&bf3703_if_config);

            gpio_direction_output(HI253_STANDBY_GPIO, 1);
            gpio_direction_output(BF3703_STANDBY_GPIO, 0);

            twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
                    VAUX_2_8_V, TWL4030_VAUX4_DEDICATED);
            twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
                    VAUX_DEV_GRP_P1, TWL4030_VAUX4_DEV_GRP);
            udelay(100);
            printk(KERN_ERR "VAUX4 = 2.8V\n");

            gpio_direction_output(BF3703_RESET_GPIO, 1);
            udelay(10);
            /* have to put sensor to reset to guarantee detection */
            gpio_direction_output(BF3703_RESET_GPIO, 0);
            mdelay(100);
            /* nRESET is active LOW. set HIGH to release reset */
            gpio_direction_output(BF3703_RESET_GPIO, 1);
            printk(KERN_INFO "reset camera(BF3703)\n");
            break;
        case V4L2_POWER_OFF:
            printk(KERN_INFO "bf3703_sensor_power_set(OFF)\n");
        case V4L2_POWER_STANDBY:
            printk(KERN_INFO "bf3703_sensor_power_set(STANDBY)\n");

            gpio_direction_output(BF3703_STANDBY_GPIO, 1);
            printk(KERN_DEBUG "bf3703_sensor_power_set(STANDBY)\n");

            /* Remove pm constraints */
            omap_pm_set_min_bus_tput(vdev->cam->isp, OCP_INITIATOR_AGENT, 0);
            omap_pm_set_max_mpu_wakeup_lat(&qos_request, -1);

            /*twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
              VAUX_DEV_GRP_NONE, TWL4030_VAUX4_DEV_GRP);*/

            /* Make sure not to disable the MCLK twice in a row */
            if (previous_power == V4L2_POWER_ON){
                isp_disable_mclk(isp);
            }
            break;
    }

    /* Save powerstate to know what was before calling POWER_ON. */
    previous_power = power;
    return err;
}

static u32 bf3703_set_xclk(struct v4l2_int_device *s, u32 xclkfreq)
{
    struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;
    printk(KERN_INFO "sensor_set_xclkb(%d)\n",xclkfreq);

    return isp_set_xclk(vdev->cam->isp, xclkfreq, CAMKUNLUN_USE_XCLKB);
}



struct bf3703_platform_data kunlun_bf3703_platform_data = {
    .power_set            = bf3703_power_set,
    .priv_data_set        = bf3703_set_prv_data,
    .set_xclk             = bf3703_set_xclk,
};

#endif

// #if defined(CONFIG_VIDEO_HI253)
#if 0
#include <media/hi253.h>
#define BIGGEST_FRAME_BYTE_SIZE	PAGE_ALIGN(1600 * 1200 * 2)

//static int hi253_powered = 0;

#define ISP_MCLK		216000000

static struct omap34xxcam_sensor_config hi253_hwc = {
    .sensor_isp  = 1,
    .capture_mem = BIGGEST_FRAME_BYTE_SIZE * 2,
    .ival_default    = { 1, 30 },
};

static int hi253_set_prv_data(struct v4l2_int_device *s,void *priv,u8 type)
{
    struct omap34xxcam_hw_config *hwc = priv;

    hwc->u.sensor.sensor_isp = hi253_hwc.sensor_isp;
    hwc->dev_index		= 2;
    hwc->dev_minor		= 5;
    hwc->dev_type		= OMAP34XXCAM_SLAVE_SENSOR;

    return 0;
}


static struct isp_interface_config Hi253_if_config = {
    .ccdc_par_ser 		= ISP_PARLL,
    .dataline_shift 	= 0x0,
    .hsvs_syncdetect 	= ISPCTRL_SYNC_DETECT_VSRISE,
    .strobe 		= 0x0,
    .prestrobe 		= 0x0,
    .shutter 		= 0x0,
    .wenlog 		= ISPCCDC_CFG_WENLOG_AND,
    .wait_hs_vs		= 2,
    .cam_mclk		= ISP_MCLK,
    .u.par.par_bridge = 2,
    .u.par.par_clk_pol = 0,
};


static int hi253_power_set(struct v4l2_int_device *s, enum v4l2_power power,u8 type)
{

    struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;
    struct isp_device *isp = dev_get_drvdata(vdev->cam->isp);

    static enum v4l2_power previous_power = V4L2_POWER_OFF;
    static struct pm_qos_request_list *qos_request;
    int err = 0;

    printk(KERN_ERR "previous_power = %d,Hi253_sensor_power_set(%d)\n",previous_power,power);

    switch (power) {
        case V4L2_POWER_ON:
            /*
             * Through-put requirement:
             * Set max OCP freq for 3630 is 200 MHz through-put
             * is in KByte/s so 200000 KHz * 4 = 800000 KByte/s
             */
            omap_pm_set_min_bus_tput(vdev->cam->isp,
                    OCP_INITIATOR_AGENT, 800000);

            /* Hold a constraint to keep MPU in C1 */
            omap_pm_set_max_mpu_wakeup_lat(&qos_request, 12);

            isp_configure_interface(vdev->cam->isp,&Hi253_if_config);

            gpio_direction_output(BF3703_STANDBY_GPIO,1);

            gpio_direction_output(HI253_STANDBY_GPIO, 0);
            udelay(10);

            twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
                    VAUX_2_8_V, TWL4030_VAUX4_DEDICATED);
            twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
                    VAUX_DEV_GRP_P1, TWL4030_VAUX4_DEV_GRP);
            udelay(100);
            printk(KERN_ERR "VAUX4 = 2.8V\n");

            twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
                    VMMC2_1_5_V, TWL4030_VMMC2_DEDICATED);
            twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
                    VMMC2_DEV_GRP_P1, TWL4030_VMMC2_DEV_GRP);
            udelay(100);

            printk(KERN_ERR "VMMC2 = 1.5V\n");


            gpio_direction_output(HI253_RESET_GPIO, 1);
            mdelay(1);
            /* have to put sensor to reset to guarantee detection */
            gpio_direction_output(HI253_RESET_GPIO, 0);
            udelay(1500);
            /* nRESET is active LOW. set HIGH to release reset */
            gpio_direction_output(HI253_RESET_GPIO, 1);

            printk(KERN_ERR "reset camera(HI253)\n");
            break;

        case V4L2_POWER_OFF:
        case V4L2_POWER_STANDBY:
            printk(KERN_ERR "_sensor_power_set(OFF )\n");

            gpio_direction_output(HI253_STANDBY_GPIO, 1);
            printk(KERN_DEBUG "hi253_sensor_power_set(STANDBY)\n");

            /* Remove pm constraints */
            omap_pm_set_min_bus_tput(vdev->cam->isp, OCP_INITIATOR_AGENT, 0);
            omap_pm_set_max_mpu_wakeup_lat(&qos_request, -1);

            /*
               twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
               VAUX_DEV_GRP_NONE, TWL4030_VAUX4_DEV_GRP);
               twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
               VAUX_DEV_GRP_NONE, TWL4030_VMMC2_DEV_GRP);
               */


            /* Make sure not to disable the MCLK twice in a row */
            if (previous_power == V4L2_POWER_ON){
                isp_disable_mclk(isp);
            }
            break;

            //    printk(KERN_ERR "_sensor_power_set(STANDBY)\n");
            break;
    }

    /* Save powerstate to know what was before calling POWER_ON. */
    previous_power = power;
    return err;
}

static u32 hi253_set_xclk(struct v4l2_int_device *s, u32 xclkfreq,u32 cntclk)
{
    struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;
    isp_set_cntclk(vdev->cam->isp, cntclk);
    isp_set_xclk(vdev->cam->isp, xclkfreq, __USE_XCLKA_);
    return 0;

}



struct platform_data kunlun_hi253_platform_data = {
    .power_set            = hi253_power_set,
    .priv_data_set        = hi253_set_prv_data,
    .set_xclk             = hi253_set_xclk,

};

#endif

#if 0

/** UGlee commented out **/

void __init kunlun_cam_init(void)
{
    cam_inited = 0;

#if defined(CONFIG_VIDEO_HI253)
    /*OV2655_STANDBY_GPIO can't act as output pin on kunlun p0 */
    /* Request and configure gpio pins */
    if (gpio_request(HI253_STANDBY_GPIO, "hi253_standby_gpio") != 0) {
        printk(KERN_ERR "Could not request GPIO %d",
                HI253_STANDBY_GPIO);
    }
    /* set to output mode */
    gpio_direction_output(HI253_STANDBY_GPIO, 1);
#endif

#if defined(CONFIG_VIDEO_BF3703)
    if (gpio_request(BF3703_STANDBY_GPIO, "bf3703_standby_gpio") != 0) {
        printk(KERN_ERR "Could not request GPIO %d",
                BF3703_STANDBY_GPIO);
    }

    gpio_direction_output(BF3703_STANDBY_GPIO, 1);
#endif

    cam_inited = 1;
}

#endif

#endif
