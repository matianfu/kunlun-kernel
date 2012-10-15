/*
 * drivers/media/video/ov2659.h
 *
 * Register definitions for the OV2659 CameraChip.
 *
 * Author: Haier
 *
 * Copyright (C) 2011 Haier.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#ifndef OV2659_H
#define OV2659_H

#define CONFIG_CAM_GPIO_I2C

#ifdef CONFIG_CAM_GPIO_I2C
#define  CAM_SCL_GPIO     23
#define  CAM_SDA_GPIO     24
#endif

#define V4L2_CID_EFFECT (V4L2_CID_PRIVATE_BASE + 1)
#define V4L2_CID_SCENE (V4L2_CID_PRIVATE_BASE + 2)
#define V4L2_CID_CAPTURE (V4L2_CID_PRIVATE_BASE + 3)
#define V4L2_CID_FLASH (V4L2_CID_PRIVATE_BASE + 4)
#define V4L2_CID_TYPE (V4L2_CID_PRIVATE_BASE + 5)
#define V4L2_CID_BACKTOPREVIEW (V4L2_CID_PRIVATE_BASE + 6)
#define V4L2_CID_EXIT (V4L2_CID_PRIVATE_BASE + 7)

#define VAUX4_2_8_V		0x09
#define ENABLE_VAUX4_DEV_GRP    0xE0
#define VAUX4_DEV_GRP_P1		0x20
#define VAUX4_DEV_GRP_NONE	0x00

#define OV2659_I2C_ADDR		0x30

/* Register initialization tables for OV2659 */

/* Terminating list entry for reg */
#define OV2659_REG_TERM		0xFFFF
/* Terminating list entry for val */
#define OV2659_VAL_TERM		0xFF

#define OV2659_USE_XCLKA	0
#define OV2659_USE_XCLKB	1

#define OV2659_CSI2_VIRTUAL_ID	0x1

#define DEBUG_BASE		0x08000000

/* Sensor specific GPIO signals */
#define OV2659_RESET_GPIO  	98
#define OV2659_STANDBY_GPIO	110

/* FPS Capabilities */
#define OV2659_MIN_FPS			5
#define OV2659_DEF_FPS			15
#define OV2659_MAX_FPS			30

/*brightness*/
#define OV2659_MIN_BRIGHT		0
#define OV2659_MAX_BRIGHT		6
#define OV2659_DEF_BRIGHT		3
#define OV2659_BRIGHT_STEP		1

/*contrast*/
#define OV2659_DEF_CONTRAST		3
#define OV2659_MIN_CONTRAST		0
#define OV2659_MAX_CONTRAST		6
#define OV2659_CONTRAST_STEP	1

/*saturation*/
#define OV2659_DEF_SATURATION	4
#define OV2659_MIN_SATURATION   0
#define OV2659_MAX_SATURATION	8
#define OV2659_SATURATION_STEP	1

/*exposure*/
#define OV2659_DEF_EXPOSURE	5
#define OV2659_MIN_EXPOSURE	0
#define OV2659_MAX_EXPOSURE	10
#define OV2659_EXPOSURE_STEP	1

/*sharpness*/
#define OV2659_DEF_SHARPNESS	2
#define OV2659_MIN_SHARPNESS	0
#define OV2659_MAX_SHARPNESS	4
#define OV2659_SHARPNESS_STEP	1

#define OV2659_DEF_COLOR		0
#define OV2659_MIN_COLOR		0
#define OV2659_MAX_COLOR		2
#define OV2659_COLOR_STEP		1

/*effect*/
#define OV2659_DEF_EFFECT		0
#define OV2659_MIN_EFFECT		0
#define OV2659_MAX_EFFECT		7
#define OV2659_EFFECT_STEP		1

/*white balance*/
#define OV2659_DEF_WB  	    0
#define OV2659_MIN_WB	    0
#define OV2659_MAX_WB	    4
#define OV2659_WB_STEP	    1

#define OV2659_BLACK_LEVEL_10BIT	16

#define SENSOR_DETECTED		1
#define SENSOR_NOT_DETECTED	0

/* NOTE: Set this as 0 for enabling SoC mode */
//#define OV2659_RAW_MODE	1
#define OV2659_YUV_MODE 1

/* High byte of product ID */
#define OV2659_PIDH_MAGIC	0x26
/* Low byte of product ID  */
#define OV2659_PIDL_MAGIC	0x56

/*REGISTERs address*/
#define OV2659_PIDH     0x300A
#define OV2659_PIDL     0x300B

/* XCLK Frequency in Hz*/
#define OV2659_XCLK_MIN		24000000
#define OV2659_XCLK_MAX		24000000

/* ------------------------------------------------------ */

/* define a structure for OV2659 register initialization values */
struct OV2659_reg {
	unsigned short reg;
	unsigned char val;
	unsigned char mask;
};

struct capture_size_ov {
	unsigned long width;
	unsigned long height;
};

enum image_size_ov {
	SQCIF, /*128*96*/
	QCIF, /*176*144*/
	QVGA, /*320*240*/
	CIF, /*352*288*/
	VGA, /*640*480*/
	SVGA, /*800*600*/
	DVD_NTSC, /*720*480*/
	XGA, /*1024*768*/
	ONE_M, /*1280*960*/
	UXGA  /*1600*1200*/
};

enum pixel_format_ov {
	YUV,
	RGB565,
	RGB555,
	RAW10
};

#define OV_NUM_PIXEL_FORMATS		4
#define OV_NUM_FPS			3

typedef enum
{
	EXPOSURE_MODE_EXP_AUTO,
	EXPOSURE_MODE_EXP_MACRO,
	EXPOSURE_MODE_EXP_PORTRAIT,
	EXPOSURE_MODE_EXP_LANDSCAPE,
	EXPOSURE_MODE_EXP_SPORTS,
	EXPOSURE_MODE_EXP_NIGHT,
	EXPOSURE_MODE_EXP_NIGHT_PORTRAIT,
	EXPOSURE_MODE_EXP_BACKLIGHTING,
	EXPOSURE_MODE_EXP_MANUAL
} EXPOSURE_MODE_VALUES;

typedef enum {
	SENSOR_SCENE_MODE_AUTO = 0,
	SENSOR_SCENE_MODE_SUNNY = 1,
	SENSOR_SCENE_MODE_CLOUDY = 2,
	SENSOR_SCENE_MODE_OFFICE = 3,
	SENSOR_SCENE_MODE_HOME = 4,
	SENSOR_SCENE_MODE_NIGHT = 5
}SENSOR_SCENE_MODE;

typedef enum{
	SENSOR_WHITE_BALANCE_AUTO = 0,
	SENSOR_WHITE_BALANCE_DAYLIGHT = 1,
	SENSOR_WHITE_BALANCE_CLOUDY = 2,
	SENSOR_WHITE_BALANCE_INCANDESCENT = 3,
	SENSOR_WHITE_BALANCE_FLUORESCENT = 4,
} SENSOR_WHITE_BALANCE_MODE;

typedef enum {
	SENSOR_EFFECT_NORMAL = 0,
	SENSOR_EFFECT_MONO = 1,
	SENSOR_EFFECT_NEGATIVE = 2,
	SENSOR_EFFECT_SEPIA =3,
	SENSOR_EFFECT_BLUISH = 4,
	SENSOR_EFFECT_GREEN = 5,
	SENSOR_EFFECT_REDDISH = 6,
	SENSOR_EFFECT_YELLOWISH = 7
}SENSOR_EFFECTS;

struct sensor_ext_params{
	SENSOR_EFFECTS effect;
	SENSOR_WHITE_BALANCE_MODE white_balance;
	SENSOR_SCENE_MODE scene_mode;
	int brightness;
	int contrast;
	int saturation;
	int exposure;
	int sharpness;
};

/**
 * struct OV2659_sensor - main structure for storage of sensor information
 * @pdata: access functions and data for platform level information
 * @v4l2_int_device: V4L2 device structure structure
 * @i2c_client: iic client device structure
 * @pix: V4L2 pixel format information structure
 * @timeperframe: time per frame expressed as V4L fraction
 * @isize: base image size
 * @ver: OV2659 chip version
 * @width: configured width
 * @height: configuredheight
 * @vsize: vertical size for the image
 * @hsize: horizontal size for the image
 * @crop_rect: crop rectangle specifying the left,top and width and height
 */
struct OV2659_sensor {
	const struct OV2659_platform_data *pdata;
	struct v4l2_int_device *v4l2_int_device;
	struct i2c_client *i2c_client;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	struct sensor_ext_params ext_params;
	int isize;
	int ver;
	int fps;
	unsigned long width;
	unsigned long height;
	unsigned long vsize;
	unsigned long hsize;
	struct v4l2_rect crop_rect;
	int state;
};

struct v4l2_ifparm;
enum v4l2_power;

/**
 * struct OV2659_platform_data - platform data values and access functions
 * @power_set: Power state access function, zero is off, non-zero is on.
 * @ifparm: Interface parameters access function
 * @priv_data_set: device private data (pointer) access function
 * @set_xclk:set sensor input main clock function
 */
struct OV2659_platform_data {
	int (*power_set)(struct v4l2_int_device *s, enum v4l2_power power);
	int (*ifparm)(struct v4l2_ifparm *p);
	int (*priv_data_set)(struct v4l2_int_device *s, void *);
	u32 (*set_xclk)(struct v4l2_int_device *s, u32 xclkfreq);
	int (*cfg_interface_bridge)(u32);
};

#endif /* ifndef OV2659_H */
