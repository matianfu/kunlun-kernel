#ifndef LINUX_CAM_Hi253_H
#define LINUX_CAM_Hi253_H

#define HI253_I2C_NAME  "Hi253"

#define Hi253_I2C_ADDR		0x20
#define BF3703_I2C_ADDR		0x6E

#define V4L2_CID_EFFECT (V4L2_CID_PRIVATE_BASE + 1)
#define V4L2_CID_SCENE (V4L2_CID_PRIVATE_BASE + 2)
#define V4L2_CID_CAPTURE (V4L2_CID_PRIVATE_BASE + 3)
#define V4L2_CID_FLASH (V4L2_CID_PRIVATE_BASE + 4)
#define V4L2_CID_TYPE (V4L2_CID_PRIVATE_BASE + 5)
#define V4L2_CID_BACKTOPREVIEW (V4L2_CID_PRIVATE_BASE + 6)
#define V4L2_CID_EXIT (V4L2_CID_PRIVATE_BASE + 7)

/* Register initialization tables for Hi253 */
/* Terminating list entry for reg */
#define HI253_REG_TERM		0xFF
/* Terminating list entry for val */
#define __VAL_TERM		0xFF

#define __USE_XCLKA_	0
#define __USE_XCLKB_	1

#define __CSI2_VIRTUAL_ID	0x1

#define DEBUG_BASE		0x08000000

#define HI253_IMAGE_SENSOR_FULL_WIDTH	(1600)
#define HI253_IMAGE_SENSOR_FULL_HEIGHT	(1200)

#define HI253_IMAGE_SENSOR_PV_WIDTH   (800)
#define HI253_IMAGE_SENSOR_PV_HEIGHT  (600)

#define HI253_IMAGE_SENSOR_PV1_WIDTH   (800)
#define HI253_IMAGE_SENSOR_PV1_HEIGHT  (600)

#define HI253_IMAGE_SENSOR_2M_WIDTH	(1600)
#define HI253_IMAGE_SENSOR_2M_HEIGHT	(1200)

/* FPS Capabilities */
#define __MIN_FPS			5
#define __DEF_FPS			15
#define __MAX_FPS			30

/*brightness*/
#define __MIN_BRIGHT		1
#define __MAX_BRIGHT		5
#define __DEF_BRIGHT		3
#define __BRIGHT_STEP		1

/*contrast*/
#define __DEF_CONTRAST		3
#define __MIN_CONTRAST		1
#define __MAX_CONTRAST		5
#define __CONTRAST_STEP		1

/*saturation*/
#define __DEF_SATURATION	3
#define __MIN_SATURATION	1
#define __MAX_SATURATION	5
#define __SATURATION_STEP	1

/*exposure*/
#define __DEF_EXPOSURE		3
#define __MIN_EXPOSURE		1
#define __MAX_EXPOSURE		5
#define __EXPOSURE_STEP		1

/*sharpness*/
#define __DEF_SHARPNESS		3
#define __MIN_SHARPNESS		1
#define __MAX_SHARPNESS		5
#define __SHARPNESS_STEP	1

#define __DEF_COLOR		0
#define __MIN_COLOR		0
#define __MAX_COLOR		2
#define __COLOR_STEP		1

/*effect*/
#define __DEF_EFFECT		0
#define __MIN_EFFECT		0
#define __MAX_EFFECT		7
#define __EFFECT_STEP		1

/*white balance*/
#define __DEF_WB		0
#define __MIN_WB		1
#define __MAX_WB		4
#define __WB_STEP   		1

#define __BLACK_LEVEL_10BIT	16

#define SENSOR_DETECTED		1
#define SENSOR_NOT_DETECTED	0

/* NOTE: Set this as 0 for enabling SoC mode */
//#define OV2655_RAW_MODE	1
#define __YUV_MODE 1

/* High byte of product ID */
#define __PIDH_MAGIC	0x92
/* Low byte of product ID  */
#define __PIDL_MAGIC	0x56

/* High byte of product ID */
#define __PIDH_MAGIC_SUB 0x73
/* Low byte of product ID  */
#define __PIDL_MAGIC_SUB	0x03

/*REGISTERs address*/
#define __PIDH	 0x300A
#define __PIDL	 0x300B

/* XCLK Frequency in Hz*/
#define __XCLK_MIN		24000000
#define __XCLK_MAX		24000000

#define IMAGE_NORMAL 0
#define IMAGE_H_MIRROR 1
#define IMAGE_V_MIRROR 2
#define IMAGE_HV_MIRROR 3
/* ------------------------------------------------------ */

/* define a structure for Hi253 register initialization values */
struct Hi253_reg {
	unsigned char reg;
	unsigned char val;
};

struct capture_size_hi253{
	unsigned long width;
	unsigned long height;
};

enum image_size_hi253 {
	HI253_QVGA, /*320*240*/
	HI253_VGA,
	HI253_UXGA  /*1600*1200*/
};

enum pixel_format_hi253 {
	HI253_YUV,
	HI253_RGB565,
	HI253_RGB555,
	HI253_RAW10
};

#define OV_NUM_PIXEL_FORMATS		4
#define OV_NUM_FPS			3

typedef enum {
	HI253_SCENE_MODE_AUTO = 0,
	HI253_SCENE_MODE_SUNNY = 1,
	HI253_SCENE_MODE_CLOUDY = 2,
	HI253_SCENE_MODE_OFFICE = 3,
	HI253_SCENE_MODE_HOME = 4,
	HI253_SCENE_MODE_NIGHT = 5
}HI253_SCENE_MODE;

typedef enum{
	HI253_WHITE_BALANCE_AUTO = 0,
	HI253_WHITE_BALANCE_DAYLIGHT = 1,
	HI253_WHITE_BALANCE_CLOUDY = 2,
	HI253_WHITE_BALANCE_INCANDESCENT = 3,
	HI253_WHITE_BALANCE_FLUORESCENT = 4,
} HI253_WHITE_BALANCE_MODE;

typedef enum {
	HI253_EFFECT_NORMAL = 0,
	HI253_EFFECT_MONO = 1,
	HI253_EFFECT_NEGATIVE = 2,
	HI253_EFFECT_SEPIA =3,
	HI253_EFFECT_BLUISH = 4,
	HI253_EFFECT_GREEN = 5,
	HI253_EFFECT_REDDISH = 6,
	HI253_EFFECT_YELLOWISH = 7
}HI253_EFFECTS;

struct hi253_ext_params{
	HI253_EFFECTS effect;
	HI253_WHITE_BALANCE_MODE white_balance;
	HI253_SCENE_MODE scene_mode;
	int brightness;
	int contrast;
	int saturation;
	int exposure;
	int sharpness;
};

/**
 * struct Hi253_sensor - main structure for storage of sensor information
 * @pdata: access functions and data for platform level information
 * @v4l2_int_device: V4L2 device structure structure
 * @i2c_client: iic client device structure
 * @pix: V4L2 pixel format information structure
 * @timeperframe: time per frame expressed as V4L fraction
 * @isize: base image size
 * @ver: Hi253 chip version
 * @width: configured width
 * @height: configuredheight
 * @vsize: vertical size for the image
 * @hsize: horizontal size for the image
 * @crop_rect: crop rectangle specifying the left,top and width and height
 */
struct _sensor_chip_ {
	const struct platform_data *pdata;
	struct v4l2_int_device *v4l2_int_device;
	struct i2c_client *i2c_client;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	struct hi253_ext_params ext_params;
	int isize;
	u8 ver;
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
 * struct Hi253_platform_data - platform data values and access functions
 * @power_set: Power state access function, zero is off, non-zero is on.
 * @ifparm: Interface parameters access function
 * @priv_data_set: device private data (pointer) access function
 * @set_xclk:set sensor input main clock function
 */
struct platform_data {
	int (*power_set)(struct v4l2_int_device *s, enum v4l2_power power,u8 type);
	int (*ifparm)(struct v4l2_ifparm *p);
	int (*priv_data_set)(struct v4l2_int_device *s, void *,u8 type);
	u32 (*set_xclk)(struct v4l2_int_device *s, u32 xclkfreq,u32 cntclk);
	int (*cfg_interface_bridge)(u32);
};
#endif /* ifndef OV2655_H */
