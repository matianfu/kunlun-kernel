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
#include <plat/gpio.h>
#include <plat/omap-pm.h>
#include <linux/videodev2.h>

#if defined(CONFIG_VIDEO_HI253)
#include <media/hi253.h>
#endif
#if defined(CONFIG_VIDEO_BF3703)
#include <media/bf3703.h>
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
#define VAUX3_1_8_V	0x01 //1.8v add by jhy for cam IOVDD  2011.6.30

#define VAUX_DEV_GRP_P1		0x20
#define VAUX_DEV_GRP_NONE	0x00
#define VMMC2_1_8_V		0x06
#define VMMC2_1_5_V 		0x04

#define VMMC2_DEV_GRP_P1	0x20
#define VMMC2_DEV_GRP_NONE	0x00
#define CAMZOOM2_USE_XCLKB  	1

#define TWL4030_VAUX2_REMAP		0x1D
#define TWL4030_VAUX3_REMAP		0x21
#define TWL4030_VAUX4_REMAP		0x25

#define TWL4030_VAUX_REMAP_VALUE 0xEE

#define OV2659_USE_STANDBY 0

/* Sensor specific GPIO signals */
#define HI253_RESET_GPIO  	98
#define HI253_STANDBY_GPIO	110
#define BF3703_STANDBY_GPIO	180
#define BF3703_RESET_GPIO	179

#if defined(CONFIG_VIDEO_BF3703)
#include <media/bf3703.h>
#define BF3703_BIGGEST_FRAME_BYTE_SIZE	PAGE_ALIGN(1600 * 1200 * 2)

#define CAMKL9C_USE_XCLKB  	1

#define ISP_MCLK		216000000

static int bf3703_powered = 0;
static struct omap34xxcam_sensor_config bf3703_hwc = {
	.sensor_isp  = 1,
	.capture_mem = BF3703_BIGGEST_FRAME_BYTE_SIZE * 2,
	.ival_default    = { 1, 30 },
};

static int bf3703_set_prv_data(struct v4l2_int_device *s,void *priv)
{
	struct omap34xxcam_hw_config *hwc = priv;

	hwc->u.sensor.sensor_isp = bf3703_hwc.sensor_isp;
	hwc->dev_index		= 3;
	hwc->dev_minor		= 6;
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

	return isp_set_xclk(vdev->cam->isp, xclkfreq, CAMKL9C_USE_XCLKB);
}

struct bf3703_platform_data kl9c_bf3703_platform_data = {
	.power_set            = bf3703_power_set,
	.priv_data_set        = bf3703_set_prv_data,
	.set_xclk             = bf3703_set_xclk,
};

#endif

#if defined(CONFIG_VIDEO_HI253)
#include <media/hi253.h>
#define BIGGEST_FRAME_BYTE_SIZE	PAGE_ALIGN(1600 * 1200 * 2)

static int hi253_powered = 0;

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

struct platform_data kl9c_hi253_platform_data = {
	.power_set            = hi253_power_set,
	.priv_data_set        = hi253_set_prv_data,
	.set_xclk             = hi253_set_xclk,
};

#endif

/*add by jhy for  Cam_GC2015  2011.6.2 Beg */
#if defined(CONFIG_VIDEO_GC2015)
#include <media/gc2015.h>
#define GC2015_BIGGEST_FRAME_BYTE_SIZE	PAGE_ALIGN(1600 * 1200 * 2)

#define CAMKUNLUN_USE_XCLKA  	1

#define ISP_GC2015_MCLK		216000000

static struct omap34xxcam_sensor_config gc2015_hwc = {
	.sensor_isp  = 1,
	.capture_mem = GC2015_BIGGEST_FRAME_BYTE_SIZE * 2,
	.ival_default    = { 1, 30 },
};

static int gc2015_sensor_set_prv_data(struct v4l2_int_device *s,void *priv)
{
	struct omap34xxcam_hw_config *hwc = priv;

	hwc->u.sensor.sensor_isp = gc2015_hwc.sensor_isp;
	hwc->dev_index		= 2;
	hwc->dev_minor		= 5;
	hwc->dev_type		= OMAP34XXCAM_SLAVE_SENSOR;

	return 0;
}

static struct isp_interface_config gc2015_if_config = {
	.ccdc_par_ser 		= ISP_PARLL,
	.dataline_shift 	= 0x0,
	.hsvs_syncdetect 	= ISPCTRL_SYNC_DETECT_VSRISE,
	.strobe 		= 0x0,
	.prestrobe 		= 0x0,
	.shutter 		= 0x0,
	.wenlog 		= ISPCCDC_CFG_WENLOG_AND,
	.wait_hs_vs		= 2,
	.cam_mclk		= ISP_GC2015_MCLK,
	.u.par.par_bridge = 2,
	.u.par.par_clk_pol = 0,
};

static int gc2015_sensor_power_set(struct v4l2_int_device *s, enum v4l2_power power)
{
	struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;
	struct isp_device *isp = dev_get_drvdata(vdev->cam->isp);

	static enum v4l2_power previous_power = V4L2_POWER_OFF;
	static struct pm_qos_request_list *qos_request;
	int err = 0;

	printk(KERN_INFO "previous_power = %d,gc2015_sensor_power_set(%d)\n",previous_power,power);

	switch (power) {
	case V4L2_POWER_ON:
		/* Through-put requirement:
		 * 3280 x 2464 x 2Bpp x 7.5fps x 3 memory ops = 355163 KByte/s
		 */
		omap_pm_set_min_bus_tput(vdev->cam->isp,
			 OCP_INITIATOR_AGENT, 664000);

		/* Hold a constraint to keep MPU in C1 */
		omap_pm_set_max_mpu_wakeup_lat(&qos_request, 12);

		isp_configure_interface(vdev->cam->isp,&gc2015_if_config);
		udelay(20);
//#ifndef KUNLUN_P0 jhy close according to cam poweron timing
		//gpio_direction_output(GC2015_STANDBY_GPIO, 0);
		//udelay(10);
//#endif
		// gpio_direction_output(GC2015_RESET_GPIO, 0);

		/* turn on analog & IO power jhy add VAUX3 Ctrl 2011.6.30*/

		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX3_1_8_V, TWL4030_VAUX3_DEDICATED);
		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_DEV_GRP_P1, TWL4030_VAUX3_DEV_GRP);

		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX2_1_5_V, TWL4030_VAUX2_DEDICATED);
		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_DEV_GRP_P1, TWL4030_VAUX2_DEV_GRP);

		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_2_8_V, TWL4030_VAUX4_DEDICATED);
		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_DEV_GRP_P1, TWL4030_VAUX4_DEV_GRP);
		udelay(100);

		printk(KERN_INFO "VAUX4 = 2.8V\n");
/*jhy add according to cam poweron timing beg 2011.6.30*/
#if 1 /*defined CONFIG_CAM_GPIO_I2C*/
		gpio_direction_output(CAM_SCL_GPIO, 1);
		gpio_direction_output(CAM_SDA_GPIO, 1);
#endif

		gpio_direction_output(GC2015_STANDBY_GPIO, 0); //standby
/*jhy add according to cam poweron timing end 2011.6.30*/
		/* have to put sensor to reset to guarantee detection */
		gpio_direction_output(GC2015_RESET_GPIO, 0);
		udelay(1500);
		/* nRESET is active LOW. set HIGH to release reset */
		gpio_direction_output(GC2015_RESET_GPIO, 1);

		printk(KERN_INFO "reset camera\n");
		break;
	case V4L2_POWER_OFF:
		printk(KERN_INFO "GC2015_sensor_power_set(OFF)\n");
	case V4L2_POWER_STANDBY:
		printk(KERN_INFO "GC2015_sensor_power_set(STANDBY)\n");
#ifndef KUNLUN_P0
		/*kunlun P0 can't use GC2015_STANDBY_GPIO*/
		gpio_direction_output(GC2015_STANDBY_GPIO, 0);  //1 change by jhy for IOVDD has been close so standby no use 2011.6.30
		udelay(20);
		gpio_direction_output(GC2015_RESET_GPIO, 0);
		udelay(20);
		//printk(KERN_DEBUG "GC2015_sensor_power_set(STANDBY)\n");
#endif
		/* Remove pm constraints */
		omap_pm_set_min_bus_tput(vdev->cam->isp, OCP_INITIATOR_AGENT, 0);
		omap_pm_set_max_mpu_wakeup_lat(&qos_request, -1);

/*jhy add according to cam poweron timing beg 2011.6.30*/
#if 1 /*definded CONFIG_CAM_GPIO_I2C*/
		gpio_direction_output(CAM_SCL_GPIO, 0);
		gpio_direction_output(CAM_SDA_GPIO, 0);
#endif
/*jhy add according to cam poweron timing end 2011.6.30*/
		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_DEV_GRP_NONE, TWL4030_VAUX4_DEV_GRP);
		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_DEV_GRP_NONE, TWL4030_VAUX2_DEV_GRP);
		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_DEV_GRP_NONE, TWL4030_VAUX3_DEV_GRP);  //jhy add for Cam IOVDD ctrl  2011.6.30

		/* Make sure not to disable the MCLK twice in a row */
		if (previous_power == V4L2_POWER_ON)
			isp_disable_mclk(isp);
		break;
	}

	/* Save powerstate to know what was before calling POWER_ON. */
	previous_power = power;
	return err;
}

static u32 gc2015_sensor_set_xclk(struct v4l2_int_device *s, u32 xclkfreq)
{
	struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;

	return isp_set_xclk(vdev->cam->isp, xclkfreq, GC2015_USE_XCLKA);
}

struct GC2015_platform_data kunlun_gc2015_platform_data = {
	.power_set            = gc2015_sensor_power_set,
	.priv_data_set        = gc2015_sensor_set_prv_data,
	.set_xclk             = gc2015_sensor_set_xclk,
};

#endif
/*add by jhy for  Cam_GC2015  2011.6.2 End */

/*add by jhy for  Cam_OV2659  2011.7.13 Beg */
#if defined(CONFIG_VIDEO_OV2659)
#include <media/ov2659.h>
#define OV2659_BIGGEST_FRAME_BYTE_SIZE	PAGE_ALIGN(1600 * 1200 * 2)

#define CAMKUNLUN_USE_XCLKA  	1

#define ISP_OV2659_MCLK		216000000

static struct omap34xxcam_sensor_config ov2659_hwc = {
	.sensor_isp  = 1,
	.capture_mem = OV2659_BIGGEST_FRAME_BYTE_SIZE * 2,
	.ival_default    = { 1, 30 },
};

static int ov2659_sensor_set_prv_data(struct v4l2_int_device *s,void *priv)
{
	struct omap34xxcam_hw_config *hwc = priv;

	hwc->u.sensor.sensor_isp = ov2659_hwc.sensor_isp;
	hwc->dev_index		= 2;
	hwc->dev_minor		= 5;
	hwc->dev_type		= OMAP34XXCAM_SLAVE_SENSOR;

	return 0;
}

static struct isp_interface_config ov2659_if_config = {
	.ccdc_par_ser 		= ISP_PARLL,
	.dataline_shift 	= 0x0,
	.hsvs_syncdetect 	= ISPCTRL_SYNC_DETECT_VSRISE,
	.strobe 		= 0x0,
	.prestrobe 		= 0x0,
	.shutter 		= 0x0,
	.wenlog 		= ISPCCDC_CFG_WENLOG_AND,
	.wait_hs_vs		= 2,
	.cam_mclk		= ISP_OV2659_MCLK,
	.u.par.par_bridge = 2,
	.u.par.par_clk_pol = 0,
};

static int ov2659_sensor_power_set(struct v4l2_int_device *s, enum v4l2_power power)
{
	struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;
	struct isp_device *isp = dev_get_drvdata(vdev->cam->isp);

	static enum v4l2_power previous_power = V4L2_POWER_OFF;
	static struct pm_qos_request_list *qos_request;
	int err = 0;

	printk(KERN_INFO "previous_power = %d,ov2659_sensor_power_set(%d)\n",previous_power,power);

	switch (power) {
	case V4L2_POWER_ON:
		/* Through-put requirement:
		 * 3280 x 2464 x 2Bpp x 7.5fps x 3 memory ops = 355163 KByte/s
		 */
		omap_pm_set_min_bus_tput(vdev->cam->isp,
			 OCP_INITIATOR_AGENT, 664000);

		/* Hold a constraint to keep MPU in C1 */
		omap_pm_set_max_mpu_wakeup_lat(&qos_request, 12);

		isp_configure_interface(vdev->cam->isp,&ov2659_if_config);
		udelay(20);

#if OV2659_USE_STANDBY  //add standby mode for cam open speed up  jhy  2011.09.09

		if (previous_power == V4L2_POWER_OFF)
		{
			gpio_direction_output(OV2659_STANDBY_GPIO, 0); //standby
			gpio_direction_output(OV2659_RESET_GPIO, 1);

			/* turn on analog & IO power jhy add VAUX3 Ctrl 2011.6.30*/

			twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
				VAUX3_1_8_V, TWL4030_VAUX3_DEDICATED);
			twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
				VAUX_DEV_GRP_P1, TWL4030_VAUX3_DEV_GRP);
			twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
				TWL4030_VAUX_REMAP_VALUE, TWL4030_VAUX3_REMAP);

			twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
				VAUX2_1_5_V, TWL4030_VAUX2_DEDICATED);
			twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
				VAUX_DEV_GRP_P1, TWL4030_VAUX2_DEV_GRP);
			twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
				TWL4030_VAUX_REMAP_VALUE, TWL4030_VAUX2_REMAP);

			twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
				VAUX_2_8_V, TWL4030_VAUX4_DEDICATED);
			twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
				VAUX_DEV_GRP_P1, TWL4030_VAUX4_DEV_GRP);
			twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
				TWL4030_VAUX_REMAP_VALUE, TWL4030_VAUX4_REMAP);
			udelay(100);

			//printk(KERN_INFO "VAUX4 = 2.8V\n");

			/* have to put sensor to reset to guarantee detection */
			gpio_direction_output(OV2659_RESET_GPIO, 0);
			udelay(1500);
			/* nRESET is active LOW. set HIGH to release reset */
			gpio_direction_output(OV2659_RESET_GPIO, 1);
		}
		else
		{
			gpio_direction_output(OV2659_STANDBY_GPIO, 0);
			printk(KERN_INFO "camera(OV2659) exit standby\n");
		}
#else
		gpio_direction_output(OV2659_STANDBY_GPIO, 0); //standby
		gpio_direction_output(OV2659_RESET_GPIO, 1);

		/* turn on analog & IO power jhy add VAUX3 Ctrl 2011.6.30*/

		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX3_1_8_V, TWL4030_VAUX3_DEDICATED);
		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_DEV_GRP_P1, TWL4030_VAUX3_DEV_GRP);

		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX2_1_5_V, TWL4030_VAUX2_DEDICATED);
		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_DEV_GRP_P1, TWL4030_VAUX2_DEV_GRP);

		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_2_8_V, TWL4030_VAUX4_DEDICATED);
		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_DEV_GRP_P1, TWL4030_VAUX4_DEV_GRP);
		udelay(100);

		//printk(KERN_INFO "VAUX4 = 2.8V\n");

		/* have to put sensor to reset to guarantee detection */
		gpio_direction_output(OV2659_RESET_GPIO, 0);
		udelay(1500);
		/* nRESET is active LOW. set HIGH to release reset */
		gpio_direction_output(OV2659_RESET_GPIO, 1);
#endif
		//printk(KERN_INFO "reset camera\n");
		break;

	case V4L2_POWER_OFF:
		printk(KERN_INFO "OV2659_sensor_power_set(OFF)\n");
	case V4L2_POWER_STANDBY:
		/* Remove pm constraints */
		omap_pm_set_min_bus_tput(vdev->cam->isp, OCP_INITIATOR_AGENT, 0);
		omap_pm_set_max_mpu_wakeup_lat(&qos_request, -1);

#if OV2659_USE_STANDBY  // standby mode for cam  jhy 2011.09.09
		gpio_direction_output(OV2659_STANDBY_GPIO, 1);
#else
		gpio_direction_output(OV2659_STANDBY_GPIO, 0);
		gpio_direction_output(OV2659_RESET_GPIO, 0);

#if 1 /*definded CONFIG_CAM_GPIO_I2C */
		gpio_direction_output(CAM_SCL_GPIO, 0);
		gpio_direction_output(CAM_SDA_GPIO, 0);
	   #endif

		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_DEV_GRP_NONE, TWL4030_VAUX4_DEV_GRP);
		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_DEV_GRP_NONE, TWL4030_VAUX2_DEV_GRP);
		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_DEV_GRP_NONE, TWL4030_VAUX3_DEV_GRP);  //jhy add for Cam IOVDD ctrl  2011.6.30
#endif
		/* Make sure not to disable the MCLK twice in a row */
		if (previous_power == V4L2_POWER_ON)
			isp_disable_mclk(isp);
		break;
	}
#if !OV2659_USE_STANDBY

	/* Save powerstate to know what was before calling POWER_ON. */
	previous_power = power;
#endif

	return err;
}

static u32 ov2659_sensor_set_xclk(struct v4l2_int_device *s, u32 xclkfreq)
{
	struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;

	return isp_set_xclk(vdev->cam->isp, xclkfreq, OV2659_USE_XCLKA);
}

struct OV2659_platform_data kunlun_ov2659_platform_data = {
	.power_set            = ov2659_sensor_power_set,
	.priv_data_set        = ov2659_sensor_set_prv_data,
	.set_xclk             = ov2659_sensor_set_xclk,
};

#endif
/*add by jhy for  Cam_OV2659  2011.7.13 End */

/*yx add N710E 3M cam 2011.06.09*/
#if defined(CONFIG_VIDEO_S5K5CA)
#include <media/S5K5CA.h>
#define S5K5CA_BIGGEST_FRAME_BYTE_SIZE  PAGE_ALIGN(2048 * 1536 * 2)

#define CAMKUNLUN_USE_XCLKA  	1

#define ISP_S5K5CA_MCLK		216000000 // 48000000

static struct omap34xxcam_sensor_config S5K5CA_hwc = {
	.sensor_isp  = 1,
	.capture_mem = S5K5CA_BIGGEST_FRAME_BYTE_SIZE * 2,
	.ival_default    = { 1, 30 },
};

static int S5K5CA_sensor_set_prv_data(struct v4l2_int_device *s,void *priv)
{
	struct omap34xxcam_hw_config *hwc = priv;

	hwc->u.sensor.sensor_isp = S5K5CA_hwc.sensor_isp;
	hwc->dev_index		= 2;
	hwc->dev_minor		= 5;
	hwc->dev_type		= OMAP34XXCAM_SLAVE_SENSOR;

	return 0;
}

static struct isp_interface_config S5K5CA_if_config = {
	.ccdc_par_ser 		= ISP_PARLL,
	.dataline_shift 	= 0x0,
	.hsvs_syncdetect 	= ISPCTRL_SYNC_DETECT_VSRISE,
	.strobe 		= 0x0,
	.prestrobe 		= 0x0,
	.shutter 		= 0x0,
	.wenlog 		= ISPCCDC_CFG_WENLOG_AND,
	.wait_hs_vs		= 2,
	.cam_mclk		= ISP_S5K5CA_MCLK,
	.u.par.par_bridge = 2,
	.u.par.par_clk_pol = 0,
};

static int S5K5CA_sensor_power_set(struct v4l2_int_device *s, enum v4l2_power power)
{
	struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;
	struct isp_device *isp = dev_get_drvdata(vdev->cam->isp);

	static enum v4l2_power previous_power = V4L2_POWER_OFF;
	static struct pm_qos_request_list *qos_request;
	int err = 0;

	printk(KERN_INFO "previous_power = %d,S5K5CA_sensor_power_set(%d)\n",previous_power,power);

	switch (power) {
	case V4L2_POWER_ON:
		/* Through-put requirement:
		 * 3280 x 2464 x 2Bpp x 7.5fps x 3 memory ops = 355163 KByte/s
		 */
		omap_pm_set_min_bus_tput(vdev->cam->isp,
		OCP_INITIATOR_AGENT, 664000); // 276480);

		/* Hold a constraint to keep MPU in C1 */
		omap_pm_set_max_mpu_wakeup_lat(&qos_request, 12);

		isp_configure_interface(vdev->cam->isp,&S5K5CA_if_config);

		/* turn on analog power */
		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX2_1_5_V, TWL4030_VAUX2_DEDICATED);
		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_DEV_GRP_P1, TWL4030_VAUX2_DEV_GRP);
		   udelay(600);
		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_2_8_V, TWL4030_VAUX4_DEDICATED);
		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_DEV_GRP_P1, TWL4030_VAUX4_DEV_GRP);
		udelay(100);
		printk(KERN_INFO "VAUX4 = 2.8V\n");

		gpio_direction_output(S5K5CA_RESET_GPIO, 0);
		printk(KERN_INFO "reset camera\n");
		udelay(20);

		gpio_direction_output(S5K5CA_STANDBY_GPIO, 1);
		udelay(20);

		gpio_direction_output(S5K5CA_RESET_GPIO, 1);
		//udelay(20);
		break;
	case V4L2_POWER_OFF:
		printk(KERN_INFO "S5K5CA_sensor_power_set(OFF)\n");
		break;
	case V4L2_POWER_STANDBY:
		printk(KERN_INFO "S5K5CA_sensor_power_set(STANDBY)\n");
		/* Remove pm constraints */

		gpio_direction_output(S5K5CA_STANDBY_GPIO, 0);
		mdelay(50);

		omap_pm_set_min_bus_tput(vdev->cam->isp, OCP_INITIATOR_AGENT, 0);
		omap_pm_set_max_mpu_wakeup_lat(&qos_request, -1);

		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_DEV_GRP_NONE, TWL4030_VAUX4_DEV_GRP);
		twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
			VAUX_DEV_GRP_NONE, TWL4030_VAUX2_DEV_GRP);

		/* Make sure not to disable the MCLK twice in a row */
		if (previous_power == V4L2_POWER_ON)
			isp_disable_mclk(isp);
		break;
	}

	/* Save powerstate to know what was before calling POWER_ON. */
	previous_power = power;
	return err;
}

static u32 S5K5CA_sensor_set_xclk(struct v4l2_int_device *s, u32 xclkfreq)
{
	struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;

	return isp_set_xclk(vdev->cam->isp, xclkfreq, S5K5CA_USE_XCLKA);
}

struct S5K5CA_platform_data  kunlun_S5K5CA_platform_data = {
	.power_set            = S5K5CA_sensor_power_set,
	.priv_data_set        = S5K5CA_sensor_set_prv_data,
	.set_xclk             = S5K5CA_sensor_set_xclk,
};

#endif

void __init kl9c_cam_init(void)
{
	cam_inited = 0;
printk("yzt: %s called\n",__func__);
#if defined(CONFIG_VIDEO_HI253)
	/*OV2655_STANDBY_GPIO can't act as output pin on kunlun p0 */
	/* Request and configure gpio pins */
printk("yzt: %s called for hi253\n",__func__);
	if (gpio_request(HI253_STANDBY_GPIO, "hi253_standby_gpio") != 0) {
		printk(KERN_ERR "Could not request GPIO %d",
			HI253_STANDBY_GPIO);
		return;
	}
	/* set to output mode */
	gpio_direction_output(HI253_STANDBY_GPIO, 1);
#endif
#if defined(CONFIG_VIDEO_BF3703)
	if (gpio_request(BF3703_STANDBY_GPIO, "bf3703_standby_gpio") != 0) {
		printk(KERN_ERR "Could not request GPIO %d",
			BF3703_STANDBY_GPIO);
		return;
	}
	gpio_direction_output(BF3703_STANDBY_GPIO, 1);
#endif

#if defined (CONFIG_VIDEO_GC2015)
	if (gpio_request(GC2015_STANDBY_GPIO, "ov3640_standby_gpio") != 0) {
		printk(KERN_ERR "Could not request GPIO %d",
			GC2015_STANDBY_GPIO);
		return;
	}

	/* set to output mode */
	gpio_direction_output(GC2015_STANDBY_GPIO, true);
#endif

#if defined(CONFIG_VIDEO_OV2659) // jhy for OV2659  2011.07.13
	if (gpio_request(OV2659_STANDBY_GPIO, "ov3640_standby_gpio") != 0) {
		printk(KERN_ERR "Could not request GPIO %d",
			OV2659_STANDBY_GPIO);
		return;
	}
#endif

#if defined(CONFIG_VIDEO_S5K5CA) // yx add 2011.06.09 N710E 3.0M CAM
	if (gpio_request(S5K5CA_STANDBY_GPIO, "ov3640_standby_gpio") != 0) {
		printk(KERN_ERR "Could not request GPIO %d",
			S5K5CA_STANDBY_GPIO);
		return;
	}

	/* set to output mode */
	// gpio_direction_output(S5K5CA_STANDBY_GPIO, true);// yx close
#endif
	cam_inited = 1;
}
#endif
