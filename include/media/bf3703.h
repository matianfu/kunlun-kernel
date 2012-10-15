/*
 * drivers/media/video/bf3703.h
 *
 * Register definitions for the BF3703 CameraChip.
 *
 * Author: Pallavi Kulkarni (ti.com)
 *
 * Copyright (C) 2008 Texas Instruments.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#ifndef BF3703_H
#define BF3703_H

#define BF3703_I2C_ADDR		0x6E

/* Register initialization tables for bf3703 */
/* Terminating list entry for reg */
#define BF3703_REG_TERM		0xFF
/* Terminating list entry for val */
#define BF3703_VAL_TERM		0xFF

#define BF3703_USE_XCLKA	0
#define BF3703_USE_XCLKB	1

#define BF3703_CSI2_VIRTUAL_ID	0x1

#define DEBUG_BASE		0x08000000

/* FPS Capabilities */
#define BF3703_MIN_FPS			5
#define BF3703_DEF_FPS			15
#define BF3703_MAX_FPS			30

/*brightness*/
#define BF3703_MIN_BRIGHT		1
#define BF3703_MAX_BRIGHT		5
#define BF3703_DEF_BRIGHT		3
#define BF3703_BRIGHT_STEP		1

/*contrast*/
#define BF3703_DEF_CONTRAST		3
#define BF3703_MIN_CONTRAST		1
#define BF3703_MAX_CONTRAST		5
#define BF3703_CONTRAST_STEP		1

/*saturation*/
#define BF3703_DEF_SATURATION	3
#define BF3703_MIN_SATURATION   1
#define BF3703_MAX_SATURATION	5
#define BF3703_SATURATION_STEP	1

/*exposure*/
#define BF3703_DEF_EXPOSURE		3
#define BF3703_MIN_EXPOSURE		1
#define BF3703_MAX_EXPOSURE		5
#define BF3703_EXPOSURE_STEP		1

/*sharpness*/
#define BF3703_DEF_SHARPNESS	3
#define BF3703_MIN_SHARPNESS	1
#define BF3703_MAX_SHARPNESS	5
#define BF3703_SHARPNESS_STEP	1

#define BF3703_DEF_COLOR		0
#define BF3703_MIN_COLOR		0
#define BF3703_MAX_COLOR		2
#define BF3703_COLOR_STEP		1

/*effect*/
#define BF3703_DEF_EFFECT		0
#define BF3703_MIN_EFFECT		0
#define BF3703_MAX_EFFECT		7
#define BF3703_EFFECT_STEP		1

/*white balance*/
#define BF3703_DEF_WB		0
#define BF3703_MIN_WB		1
#define BF3703_MAX_WB		4
#define BF3703_WB_STEP   	1

#define BF3703_BLACK_LEVEL_10BIT	16

#define SENSOR_DETECTED		1
#define SENSOR_NOT_DETECTED	0

/* NOTE: Set this as 0 for enabling SoC mode */
//#define BF3703_RAW_MODE	1
#define BF3703_YUV_MODE 1

/* High byte of product ID */
#define BF3703_PIDH_MAGIC	0x26
/* Low byte of product ID  */
#define BF3703_PIDL_MAGIC	0x56

/*REGISTERs address*/
#define BF3703_PIDH	  0xFC
#define BF3703_PIDL	  0xFD

/* XCLK Frequency in Hz*/
#define BF3703_XCLK_MIN		24000000
#define BF3703_XCLK_MAX		24000000

/* ------------------------------------------------------ */

/* define a structure for bf3703 register initialization values */
struct bf3703_reg {
	unsigned char reg;
	unsigned char val;
};

struct capture_size_bf3703 {
	unsigned long width;
	unsigned long height;
};

enum image_size_bf3703 {
	BF3703_QVGA,/*320*240*/
	BF3703_VGA/*640*480*/
};

enum pixel_format_bf3703 {
	BF3703_YUV,
	BF3703_RGB565,
	BF3703_RGB555,
	BF3703_RAW10
};

#define OV_NUM_PIXEL_FORMATS		4
#define OV_NUM_FPS			3

#define V4L2_CID_EFFECT (V4L2_CID_PRIVATE_BASE + 1)
#define V4L2_CID_SCENE (V4L2_CID_PRIVATE_BASE + 2)
#define V4L2_CID_CAPTURE (V4L2_CID_PRIVATE_BASE + 3)

typedef enum {
	BF3703_SCENE_MODE_AUTO = 0,
	BF3703_SCENE_MODE_SUNNY = 1,
	BF3703_SCENE_MODE_CLOUDY = 2,
	BF3703_SCENE_MODE_OFFICE = 3,
	BF3703_SCENE_MODE_HOME = 4,
	BF3703_SCENE_MODE_NIGHT = 5
}BF3703_SCENE_MODE;

typedef enum{
	BF3703_WHITE_BALANCE_AUTO = 0,
	BF3703_WHITE_BALANCE_DAYLIGHT = 1,
	BF3703_WHITE_BALANCE_CLOUDY = 2,
	BF3703_WHITE_BALANCE_INCANDESCENT = 3,
	BF3703_WHITE_BALANCE_FLUORESCENT = 4,
} BF3703_WHITE_BALANCE_MODE;

typedef enum {
	BF3703_EFFECT_NORMAL = 0,
	BF3703_EFFECT_MONO = 1,
	BF3703_EFFECT_NEGATIVE = 2,
	BF3703_EFFECT_SEPIA =3,
	BF3703_EFFECT_BLUISH = 4,
	BF3703_EFFECT_GREEN = 5,
	BF3703_EFFECT_REDDISH = 6,
	BF3703_EFFECT_YELLOWISH = 7
}BF3703_EFFECTS;

struct bf3703_ext_params{
	BF3703_EFFECTS effect;
	BF3703_WHITE_BALANCE_MODE white_balance;
	BF3703_SCENE_MODE scene_mode;
	int brightness;
	int contrast;
	int saturation;
	int exposure;
	int sharpness;
};

/**
 * struct bf3703_sensor - main structure for storage of sensor information
 * @pdata: access functions and data for platform level information
 * @v4l2_int_device: V4L2 device structure structure
 * @i2c_client: iic client device structure
 * @pix: V4L2 pixel format information structure
 * @timeperframe: time per frame expressed as V4L fraction
 * @isize: base image size
 * @ver: bf3703 chip version
 * @width: configured width
 * @height: configuredheight
 * @vsize: vertical size for the image
 * @hsize: horizontal size for the image
 * @crop_rect: crop rectangle specifying the left,top and width and height
 */
struct bf3703_sensor {
	const struct bf3703_platform_data *pdata;
	struct v4l2_int_device *v4l2_int_device;
	struct i2c_client *i2c_client;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	struct bf3703_ext_params ext_params;
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
 * struct bf3703_platform_data - platform data values and access functions
 * @power_set: Power state access function, zero is off, non-zero is on.
 * @ifparm: Interface parameters access function
 * @priv_data_set: device private data (pointer) access function
 * @set_xclk:set sensor input main clock function
 */
struct bf3703_platform_data {
	int (*power_set)(struct v4l2_int_device *s, enum v4l2_power power);
	int (*ifparm)(struct v4l2_ifparm *p);
	int (*priv_data_set)(struct v4l2_int_device *s, void *);
	u32 (*set_xclk)(struct v4l2_int_device *s, u32 xclkfreq);
	int (*cfg_interface_bridge)(u32);
};

#endif /* ifndef BF3703_H */
