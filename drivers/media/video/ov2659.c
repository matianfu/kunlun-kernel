/*
 * drivers/media/video/ov2659.c
 *
 * ov2659 sensor driver
 *
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 * modified :jhy
 */
#include <linux/i2c.h>
#include <linux/delay.h>
#include <media/v4l2-int-device.h>
#include <linux/i2c/twl.h>
#include <mach/gpio.h>
#include <media/ov2659.h>
#include "omap34xxcam.h"
#include "isp/isp.h"

#define OV2659_DRIVER_NAME  "ov2659"
#define MOD_NAME "OV2659: "

#define I2C_M_WR 0
#define OV2659_YUV_MODE 1

//#define OV2659_DEBUG 1

#ifdef OV2659_DEBUG
#define DPRINTK_OV(format, ...)				\
	printk(KERN_ERR "OV2659:%s: " format " ",__func__, ## __VA_ARGS__)
#else
#define DPRINTK_OV(format, ...)
#endif

/* Array of image sizes supported by OV2659.  These must be ordered from
 * smallest image size to largest.
 */
const static struct capture_size_ov OV2659_sizes[] = {
	/*SQCIF,video*/
	{128,96},
	/*QCIF,video*/
	{176,144},
	/* QVGA,capture & video */
	{320,240},
	/*CIF,video*/
	{352,288},
	/*VGA,capture & video*/
	{640,480},
	/*DVD-video NTSC,video*/
	{720,480},
	/*SVGA*,capture*/
	{800,600},
	/*capture*/
	{1024,768},
	/*capture*/
	{1280,960},
	/*UXGA*/
	{1600,1200},
};

// Normal
const static struct OV2659_reg OV2659_effect_off[]={
	{0x5001, 0x1f, 0xff},
	{0x507b, 0x04, 0xff},  //0x06 jhy change B&W occure
//	{0x507e, 0x30, 0xff},
//	{0x507f, 0x00, 0xff},
	{0xFFFF, 0xFF, 0xff}
};

//B&W
const static struct OV2659_reg OV2659_effect_mono[]={
	{0x5001, 0x1f, 0xff},
	{0x507b, 0x26, 0x20},
	{0xFFFF, 0xFF, 0xff}
};

//Negative
const static struct OV2659_reg OV2659_effect_negative[]={
	{0x5001, 0x1f, 0xff},
	{0x507b, 0x46, 0x40},
	{0xFFFF, 0xFF, 0xff}
};

//Sepia
const static struct OV2659_reg OV2659_effect_sepia[]={
	{0x5001, 0x1f, 0xff},
	{0x507b, 0x1e, 0x1e},
	{0x507e, 0x40, 0xff},
	{0x507f, 0xa0, 0xff},
	{0xffff, 0xff, 0xff}
};

//Bluish
const static struct OV2659_reg OV2659_effect_bluish[]={
	{0x5001, 0x1f, 0xff},
	{0x507b, 0x1e, 0x1e},
	{0x507e, 0xa0, 0xff},
	{0x507f, 0x40, 0xff},
	{0xffff, 0xff, 0xff}
};

//Greenish
const static struct OV2659_reg OV2659_effect_green[]={
	{0x5001, 0x1f, 0xff},
	{0x507b, 0x1e, 0x1e},
	{0x507e, 0x60, 0xff},
	{0x507f, 0x60, 0xff},
	{0xffff, 0xff, 0xff}
};

//Reddish
const static struct OV2659_reg OV2659_effect_reddish[]={
	{0x5001, 0x1f, 0xff},
	{0x507b, 0x1e, 0x1e},
	{0x507e, 0x80, 0xff},
	{0x507f, 0xc0, 0xff},
	{0xffff, 0xff, 0xff}
};


const static struct OV2659_reg OV2659_effect_yellowish[]={
	{0x5001, 0x1f, 0xff},
	{0x507b, 0x1e, 0x1e},
	{0x507e, 0x30, 0xff},
	{0x507f, 0x90, 0xff},
	{0xffff, 0xff, 0xff}
};

const static struct OV2659_reg* OV2659_effects[]={
	OV2659_effect_off,
	OV2659_effect_mono,
	OV2659_effect_negative,
	OV2659_effect_sepia,
	OV2659_effect_bluish,
	OV2659_effect_green,
	OV2659_effect_reddish,
	OV2659_effect_yellowish
};

//Auto
const static struct OV2659_reg OV2659_wb_auto[]={
	{0x3406,0x00, 0xff},
	{0xFFFF,0xFF, 0xff}
};

//Sunny
const static struct OV2659_reg OV2659_wb_sunny[]={
	{0x3406,0x01, 0xff},
	{0x3400,0x05, 0xff},
	{0x3401,0x5d, 0xff},
	{0x3402,0x04, 0xff},
	{0x3403,0x00, 0xff},
	{0x3404,0x04, 0xff},
	{0x3405,0xde, 0xff},
	{0xFFFF,0xFF, 0xff}
};

//Cloudy
const static struct OV2659_reg OV2659_wb_cloudy[]={
	{0x3406,0x01, 0xff},
	{0x3400,0x06, 0xff},
	{0x3401,0x1a, 0xff},
	{0x3402,0x04, 0xff},
	{0x3403,0x09, 0xff},
	{0x3404,0x04, 0xff},
	{0x3405,0x00, 0xff},
	{0xFFFF,0xFF, 0xff}
};

//Office
const static struct OV2659_reg OV2659_wb_office[]={
	{0x3406,0x01, 0xff},
	{0x3400,0x04, 0xff},
	{0x3401,0xe0, 0xff},
	{0x3402,0x04, 0xff},
	{0x3403,0x00, 0xff},
	{0x3404,0x05, 0xff},
	{0x3405,0xa0, 0xff},
	{0xFFFF,0xFF, 0xff}
};

//Home
const static struct OV2659_reg OV2659_wb_home[]={
	{0x3406,0x01, 0xff},
	{0x3400,0x04, 0xff},
	{0x3401,0x00, 0xff},
	{0x3402,0x04, 0xff},
	{0x3403,0x00, 0xff},
	{0x3404,0x06, 0xff},
	{0x3405,0x50, 0xff},
	{0xFFFF,0xFF, 0xff}
};

const static struct OV2659_reg* OV2659_whitebalance[]={
	OV2659_wb_auto,
	OV2659_wb_sunny,
	OV2659_wb_cloudy,
	OV2659_wb_office,
	OV2659_wb_home
};

//Brightness -3
const static struct OV2659_reg OV2659_brightness_0[]={
	{0x5001,0x1f, 0xff},
	{0x5082,0x30, 0xff},
	{0x5083,0x09, 0x08},
	{0xFFFF,0xFF, 0xff}
};

//Brightness -2
const static struct OV2659_reg OV2659_brightness_1[]={
	{0x5001,0x1f, 0xff},
	{0x5082,0x20, 0xff},
	{0x5083,0x09, 0x08},
	{0xFFFF,0xFF, 0xff}
};

//Brightness -1
const static struct OV2659_reg OV2659_brightness_2[]={
	{0x5001,0x1f, 0xff},
	{0x5082,0x10, 0xff},
	{0x5083,0x09, 0x08},
	{0xFFFF,0xFF, 0xff}
};

//Default Brightness
const static struct OV2659_reg OV2659_brightness_3[]={
	{0x5001,0x1f, 0xff},
	{0x5082,0x00, 0xff},
	{0x5083,0x01, 0x08},
	{0xFFFF,0xFF, 0xff}
};

//Brightness +1
const static struct OV2659_reg OV2659_brightness_4[]={
	{0x5001,0x1f, 0xff},
	{0x5082,0x10, 0xff},
	{0x5083,0x01, 0x08},
	{0xFFFF,0xFF, 0xff}
};

//Brightness +2
const static struct OV2659_reg OV2659_brightness_5[]={
	{0x5001,0x1f, 0xff},
	{0x5082,0x20, 0xff},
	{0x5083,0x01, 0x08},
	{0xFFFF,0xFF, 0xff}
};

//Brightness +3
const static struct OV2659_reg OV2659_brightness_6[]={
	{0x5001,0x1f, 0xff},
	{0x5082,0x30, 0xff},
	{0x5083,0x01, 0x08},
	{0xFFFF,0xFF, 0xff}
};

const static struct OV2659_reg* OV2659_brightness[]={
	OV2659_brightness_0,
	OV2659_brightness_1,
	OV2659_brightness_2,
	OV2659_brightness_3,
	OV2659_brightness_4,
	OV2659_brightness_5,
	OV2659_brightness_6
};

//Contrast +3
const static struct OV2659_reg OV2659_contrast_6[]={
	{0x5001,0x1f, 0xff},
	{0x5080,0x2c, 0xff},
	{0x5081,0x2c, 0xff},
	{0x5083,0x01, 0x04},
	{0xFFFF,0xFF, 0xff}
};

//Contrast +2
const static struct OV2659_reg OV2659_contrast_5[]={
	{0x5001,0x1f, 0xff},
	{0x5080,0x28, 0xff},
	{0x5081,0x28, 0xff},
	{0x5083,0x01, 0x04},
	{0xFFFF,0xFF, 0xff}
};

//Contrast +1
const static struct OV2659_reg OV2659_contrast_4[]={
	{0x5001,0x1f, 0xff},
	{0x5080,0x24, 0xff},
	{0x5081,0x24, 0xff},
	{0x5083,0x01, 0x04},
	{0xFFFF,0xFF, 0xff}
};

//Contrast default
const static struct OV2659_reg OV2659_contrast_3[]={
	{0x5001,0x1f, 0xff},
	{0x5080,0x20, 0xff},
	{0x5081,0x20, 0xff},
	{0x5083,0x01, 0x04},
	{0xFFFF,0xFF, 0xff}
};
//Contrast -1
const static struct OV2659_reg OV2659_contrast_2[]={
	{0x5001,0x1f, 0xff},
	{0x5080,0x1c, 0xff},
	{0x5081,0x1c, 0xff},
	{0x5083,0x01, 0x04},
	{0xFFFF,0xFF, 0xff}
};

//Contrast -2
const static struct OV2659_reg OV2659_contrast_1[]={
	{0x5001,0x1f, 0xff},
	{0x5080,0x18, 0xff},
	{0x5081,0x18, 0xff},
	{0x5083,0x01, 0x04},
	{0xFFFF,0xFF, 0xff}
};

//Contrast -3
const static struct OV2659_reg OV2659_contrast_0[]={
	{0x5001,0x1f, 0xff},
	{0x5080,0x14, 0xff},
	{0x5081,0x14, 0xff},
	{0x5083,0x01, 0x04},
	{0xFFFF,0xFF, 0xff}
};

const static struct OV2659_reg* OV2659_contrast[]={
	OV2659_contrast_0,
	OV2659_contrast_1,
	OV2659_contrast_2,
	OV2659_contrast_3,
	OV2659_contrast_4,
	OV2659_contrast_5,
	OV2659_contrast_6
};

//x-4
const static struct OV2659_reg OV2659_saturation_0[]={
	{0x5001,0x1f, 0xff},
	{0x507e,0x00, 0xff},
	{0x507f,0x00, 0xff},
	{0x507b,0x02, 0x02},
	{0x507a,0x11, 0x23},
	{0xFFFF,0xFF, 0xff}
};
//x-3
const static struct OV2659_reg OV2659_saturation_1[]={
	{0x5001,0x1f, 0xff},
	{0x507e,0x10, 0xff},
	{0x507f,0x10, 0xff},
	{0x507b,0x02, 0x02},
	{0x507a,0x11, 0x23},
	{0xFFFF,0xFF, 0xff}
};
//x-2
const static struct OV2659_reg OV2659_saturation_2[]={
	{0x5001,0x1f, 0xff},
	{0x507e,0x20, 0xff},
	{0x507f,0x20, 0xff},
	{0x507b,0x02, 0x02},
	{0x507a,0x11, 0x23},
	{0xFFFF,0xFF, 0xff}
};
//x-1
const static struct OV2659_reg OV2659_saturation_3[]={
	{0x5001,0x1f, 0xff},
	{0x507e,0x30, 0xff},
	{0x507f,0x30, 0xff},
	{0x507b,0x02, 0x02},
	{0x507a,0x11, 0x23},
	{0xFFFF,0xFF, 0xff}
};
//x-0
const static struct OV2659_reg OV2659_saturation_4[]={
	{0x5001,0x1f, 0xff},
	{0x507e,0x40, 0xff},
	{0x507f,0x40, 0xff},
	{0x507b,0x02, 0x02},
	{0x507a,0x11, 0x23},
	{0xFFFF,0xFF, 0xff}
};
//x+1
const static struct OV2659_reg OV2659_saturation_5[]={
	{0x5001,0x1f, 0xff},
	{0x507e,0x50, 0xff},
	{0x507f,0x50, 0xff},
	{0x507b,0x02, 0x02},
	{0x507a,0x11, 0x23},
	{0xFFFF,0xFF, 0xff}
};
//x+2
const static struct OV2659_reg OV2659_saturation_6[]={
	{0x5001,0x1f, 0xff},
	{0x507e,0x60, 0xff},
	{0x507f,0x60, 0xff},
	{0x507b,0x02, 0x02},
	{0x507a,0x11, 0x23},
	{0xFFFF,0xFF, 0xff}
};
//x+3
const static struct OV2659_reg OV2659_saturation_7[]={
	{0x5001,0x1f, 0xff},
	{0x507e,0x70, 0xff},
	{0x507f,0x70, 0xff},
	{0x507b,0x02, 0x02},
	{0x507a,0x11, 0x23},
	{0xFFFF,0xFF, 0xff}
};
//x+4
const static struct OV2659_reg OV2659_saturation_8[]={
	{0x5001,0x1f, 0xff},
	{0x507e,0x80, 0xff},
	{0x507f,0x80, 0xff},
	{0x507b,0x02, 0x02},
	{0x507a,0x11, 0x23},
	{0xFFFF,0xFF, 0xff}
};

const static struct OV2659_reg* OV2659_saturation[]={
	OV2659_saturation_0,
	OV2659_saturation_1,
	OV2659_saturation_2,
	OV2659_saturation_3,
	OV2659_saturation_4,
	OV2659_saturation_5,
	OV2659_saturation_6,
	OV2659_saturation_7,
	OV2659_saturation_8
};

//EV-5
const static struct OV2659_reg OV2659_exposure_0[]={
	{0x3a0f,0x10, 0xff},
	{0x3a10,0x08, 0xff},
	{0x3a1b,0x10, 0xff},
	{0x3a1e,0x08, 0xff},
	{0x3a11,0x20, 0xff},
	{0x3a1f,0x20, 0xff},
	{0xFFFF,0xFF, 0xff}
};

//EV-4
const static struct OV2659_reg OV2659_exposure_1[]={
	{0x3a0f,0x18, 0xff},
	{0x3a10,0x10, 0xff},
	{0x3a1b,0x18, 0xff},
	{0x3a1e,0x10, 0xff},
	{0x3a11,0x30, 0xff},
	{0x3a1f,0x10, 0xff},
	{0xFFFF,0xFF, 0xff}
};

//EV-3
const static struct OV2659_reg OV2659_exposure_2[]={
	{0x3a0f,0x20, 0xff},
	{0x3a10,0x10, 0xff},
	{0x3a1b,0x18, 0xff},
	{0x3a1e,0x10, 0xff},
	{0x3a11,0x30, 0xff},
	{0x3a1f,0x10, 0xff},
	{0xFFFF,0xFF, 0xff}
};

//EV-2
const static struct OV2659_reg OV2659_exposure_3[]={
	{0x3a0f,0x28, 0xff},
	{0x3a10,0x20, 0xff},
	{0x3a1b,0x51, 0xff},
	{0x3a1e,0x28, 0xff},
	{0x3a11,0x20, 0xff},
	{0x3a1f,0x10, 0xff},
	{0xFFFF,0xFF, 0xff}
};

//EV-1
const static struct OV2659_reg OV2659_exposure_4[]={
	{0x3a0f,0x30, 0xff},
	{0x3a10,0x28, 0xff},
	{0x3a1b,0x61, 0xff},
	{0x3a1e,0x30, 0xff},
	{0x3a11,0x28, 0xff},
	{0x3a1f,0x10, 0xff},
	{0xFFFF,0xFF, 0xff}
};

//default
const static struct OV2659_reg OV2659_exposure_5[]={
	{0x3a0f,0x38, 0xff},
	{0x3a10,0x30, 0xff},
	{0x3a1b,0x61, 0xff},
	{0x3a1e,0x38, 0xff},
	{0x3a11,0x30, 0xff},
	{0x3a1f,0x10, 0xff},
	{0xFFFF,0xFF, 0xff}
};

//EV+1
const static struct OV2659_reg OV2659_exposure_6[]={
	{0x3a0f,0x40, 0xff},
	{0x3a10,0x38, 0xff},
	{0x3a1b,0x71, 0xff},
	{0x3a1e,0x40, 0xff},
	{0x3a11,0x38, 0xff},
	{0x3a1f,0x10, 0xff},
	{0xFFFF,0xFF, 0xff}
};

//EV+2
const static struct OV2659_reg OV2659_exposure_7[]={
	{0x3a0f,0x48, 0xff},
	{0x3a10,0x40, 0xff},
	{0x3a1b,0x80, 0xff},
	{0x3a1e,0x48, 0xff},
	{0x3a11,0x40, 0xff},
	{0x3a1f,0x20, 0xff},
	{0xFFFF,0xFF, 0xff}
};

//EV+3
const static struct OV2659_reg OV2659_exposure_8[]={
	{0x3a0f,0x50, 0xff},
	{0x3a10,0x48, 0xff},
	{0x3a1b,0x90, 0xff},
	{0x3a1e,0x50, 0xff},
	{0x3a11,0x48, 0xff},
	{0x3a1f,0x20, 0xff},
	{0xFFFF,0xFF, 0xff}
};

//EV+4
const static struct OV2659_reg OV2659_exposure_9[]={
	{0x3a0f,0x58, 0xff},
	{0x3a10,0x50, 0xff},
	{0x3a1b,0x91, 0xff},
	{0x3a1e,0x58, 0xff},
	{0x3a11,0x50, 0xff},
	{0x3a1f,0x20, 0xff},
	{0xFFFF,0xFF, 0xff}
};

//EV+5
const static struct OV2659_reg OV2659_exposure_10[]={
	{0x3a0f,0x60, 0xff},
	{0x3a10,0x58, 0xff},
	{0x3a1b,0xa0, 0xff},
	{0x3a1e,0x60, 0xff},
	{0x3a11,0x58, 0xff},
	{0x3a1f,0x20, 0xff},
	{0xFFFF,0xFF, 0xff}
};

const static struct OV2659_reg* OV2659_exposure[]={
	OV2659_exposure_0,
	OV2659_exposure_1,
	OV2659_exposure_2,
	OV2659_exposure_3,
	OV2659_exposure_4,
	OV2659_exposure_5,
	OV2659_exposure_6,
	OV2659_exposure_7,
	OV2659_exposure_8,
	OV2659_exposure_9,
	OV2659_exposure_10
};

//Sharpness-2
const static struct OV2659_reg OV2659_sharpness_0[]={
	{0x506e, 0x44, 0xff},
	{0x5064, 0x08, 0xff},
	{0x5065, 0x10, 0xff},
	{0x5066, 0x08, 0xff},
	{0x5067, 0x00, 0xff},
	{0x506c, 0x08, 0xff},
	{0x506d, 0x10, 0xff},
	{0x506e, 0x04, 0xff},
	{0x506f, 0x06, 0xff},
	{0xFFFF, 0xFF, 0xff}
};

//Sharpness-1
const static struct OV2659_reg OV2659_sharpness_1[]={
	{0x506e, 0x44, 0xff},
	{0x5064, 0x08, 0xff},
	{0x5065, 0x10, 0xff},
	{0x5066, 0x0c, 0xff},
	{0x5067, 0x01, 0xff},
	{0x506c, 0x08, 0xff},
	{0x506d, 0x10, 0xff},
	{0x506e, 0x04, 0xff},
	{0x506f, 0x06, 0xff},
	{0xFFFF, 0xFF, 0xff}
};

//default
const static struct OV2659_reg OV2659_sharpness_2[]={
	{0x506e, 0x44, 0xff},
	{0x5064, 0x08, 0xff},
	{0x5065, 0x10, 0xff},
	{0x5066, 0x12, 0xff},
	{0x5067, 0x02, 0xff},
	{0x506c, 0x08, 0xff},
	{0x506d, 0x10, 0xff},
	{0x506e, 0x04, 0xff},
	{0x506f, 0x06, 0xff},
	{0xFFFF, 0xFF, 0xff}
};

//Sharpness+1
const static struct OV2659_reg OV2659_sharpness_3[]={
	{0x506e, 0x44, 0xff},
	{0x5064, 0x08, 0xff},
	{0x5065, 0x10, 0xff},
	{0x5066, 0x16, 0xff},
	{0x5067, 0x06, 0xff},
	{0x506c, 0x08, 0xff},
	{0x506d, 0x10, 0xff},
	{0x506e, 0x04, 0xff},
	{0x506f, 0x06, 0xff},
	{0xFFFF, 0xFF, 0xff}
};

//Sharpness+2
const static struct OV2659_reg OV2659_sharpness_4[]={
	{0x506e, 0x44, 0xff},
	{0x5064, 0x08, 0xff},
	{0x5065, 0x10, 0xff},
	{0x5066, 0x1c, 0xff},
	{0x5067, 0x0c, 0xff},
	{0x506c, 0x08, 0xff},
	{0x506d, 0x10, 0xff},
	{0x506e, 0x03, 0xff},
	{0x506f, 0x05, 0xff},
	{0xFFFF, 0xFF, 0xff}
};

const static struct OV2659_reg* OV2659_sharpness[]={
	OV2659_sharpness_0,
	OV2659_sharpness_1,
	OV2659_sharpness_2,
	OV2659_sharpness_3,
	OV2659_sharpness_4,
};

const static struct OV2659_reg OV2659_capture_uxgabase[]={
/*
	{0x5066, 0x1e, 0x00},
	{0x5067, 0x18, 0x00},
	{0x506a, 0x18, 0x00},
*/
	{0x3800, 0x00, 0xff},
	{0x3801, 0x00, 0xff},
	{0x3802, 0x00, 0xff},
	{0x3803, 0x00, 0xff},
	{0x3804, 0x06, 0xff},
	{0x3805, 0x5f, 0xff},
	{0x3806, 0x04, 0xff},
	{0x3807, 0xbb, 0xff},
	{0x3808, 0x06, 0xff},
	{0x3809, 0x40, 0xff},
	{0x380a, 0x04, 0xff},
	{0x380b, 0xb0, 0xff},
	{0x3811, 0x10, 0xff},
	{0x3813, 0x06, 0xff},
	{0x3814, 0x11, 0xff},
	{0x3815, 0x11, 0xff},

	{0x3820, 0x80, 0xff},
	{0x3821, 0x00, 0xff},
	{0x5002, 0x00, 0xff},
	{0x4608, 0x00, 0xff},
	{0x4609, 0x80, 0xff},

	{0x3623, 0x00, 0xff},
	{0x3634, 0x44, 0xff},
	{0x3701, 0x44, 0xff},
	{0x3702, 0x30, 0xff},
	{0x3703, 0x48, 0xff},
	{0x3704, 0x48, 0xff},
	{0x3705, 0x18, 0xff},
	{0x370a, 0x12, 0xff},

	{0x3003, 0x80, 0xff},
	{0x3004, 0x20, 0xff},
	{0x3005, 0x18, 0xff},
	{0x3006, 0x0d, 0xff},

	{0x380c, 0x07, 0xff},
	{0x380d, 0x9f, 0xff},
	{0x380e, 0x04, 0xff},
	{0x380f, 0xd0, 0xff},

	{0x3a02, 0x04, 0xff},
	{0x3a03, 0xd0, 0xff},
	{0x3a08, 0x00, 0xff},
	{0x3a09, 0x3d, 0xff},
	{0x3a0a, 0x00, 0xff},
	{0x3a0b, 0x3d, 0xff},
	{0x3a0d, 0x0e, 0xff},
	{0x3a0e, 0x0f, 0xff},
	{0x3a14, 0x04, 0xff},
	{0x3a15, 0xd0, 0xff},

/*OV2659 captrue settings
**resolution:UXGA(1600*1200),fps:5fps
*/
	{0xFFFF, 0xFF, 0xff}
};

const static struct OV2659_reg OV2659_common_svgabase[] = {
/* OV2659_SVGA_YUV 20 fps
* 24 MHz input clock
* 800*600 resolution
*/
	{0x0103,0x01, 0xff},
	{0x3000,0x0f, 0xff},
	{0x3001,0xff, 0xff},
	{0x3002,0xff, 0xff},
	{0x0100,0x01, 0xff},
	{0x3633,0x3d, 0xff},
	{0x3620,0x02, 0xff},
	{0x3631,0x11, 0xff},
	{0x3612,0x04, 0xff},
	{0x3630,0x20, 0xff},
	{0x4702,0x02, 0xff},
	{0x370c,0x34, 0xff},

	{0x4003,0x88, 0xff},

	{0x3800,0x00, 0xff},
	{0x3801,0x00, 0xff},
	{0x3802,0x00, 0xff},
	{0x3803,0x00, 0xff},
	{0x3804,0x06, 0xff},
	{0x3805,0x5f, 0xff},
	{0x3806,0x04, 0xff},
	{0x3807,0xb7, 0xff},
	{0x3808,0x03, 0xff},
	{0x3809,0x20, 0xff},
	{0x380a,0x02, 0xff},
	{0x380b,0x58, 0xff},
	{0x3811,0x08, 0xff},
	{0x3813,0x02, 0xff},
	{0x3814,0x31, 0xff},
	{0x3815,0x31, 0xff},

	{0x3820,0x81, 0xff},
	{0x3821,0x01, 0xff},
	{0x5002,0x10, 0xff},
	{0x4608,0x00, 0xff},
	{0x4609,0xa0, 0xff},

	{0x3623,0x00, 0xff},
	{0x3634,0x76, 0xff},
	{0x3701,0x44, 0xff},
	{0x3702,0x18, 0xff},
	{0x3703,0x24, 0xff},
	{0x3704,0x24, 0xff},
	{0x3705,0x0c, 0xff},
	{0x370a,0x52, 0xff},

	{0x3003,0x80, 0xff},
	{0x3004,0x10, 0xff},
	{0x3005,0x10, 0xff},
	{0x3006,0x0d, 0xff},

	{0x380c,0x05, 0xff},
	{0x380d,0x14, 0xff},
	{0x380e,0x02, 0xff},
	{0x380f,0x68, 0xff},

	{0x3a08,0x00, 0xff},
	{0x3a09,0x7b, 0xff},
	{0x3a0a,0x00, 0xff},
	{0x3a0b,0x7b, 0xff},
	{0x3a0d,0x04, 0xff},
	{0x3a0e,0x04, 0xff},

	{0x350c,0x00, 0xff},
	{0x350d,0x00, 0xff},
	{0x4300,0x30, 0xff},
	{0x5086,0x02, 0xff},
	{0x5000,0xfb, 0xff},
	{0x5001,0x1f, 0xff},
	{0x507e,0x34, 0x00},
	{0x507f,0x00, 0x00},
	{0x507c,0x80, 0x00},
	{0x507d,0x00, 0x00},
	{0x507b,0x06, 0x00},

	{0x5025,0x06, 0xff},
	{0x5026,0x0c, 0xff},
	{0x5027,0x19, 0xff},
	{0x5028,0x34, 0xff},
	{0x5029,0x4a, 0xff},
	{0x502a,0x5a, 0xff},
	{0x502b,0x67, 0xff},
	{0x502c,0x71, 0xff},
	{0x502d,0x7c, 0xff},
	{0x502e,0x8c, 0xff},
	{0x502f,0x9b, 0xff},
	{0x5030,0xa9, 0xff},
	{0x5031,0xc0, 0xff},
	{0x5032,0xd5, 0xff},
	{0x5033,0xe8, 0xff},
	{0x5034,0x20, 0xff},

	{0x5070,0x28, 0xff},
	{0x5071,0x48, 0xff},
	{0x5072,0x10, 0xff},

	{0x5073,0x1B, 0xff},  //1b  0x24  *3/4  caused by saturate disable
	{0x5074,0x90, 0xff},  //90  0xc0  0x75-0x73
	{0x5075,0xAB, 0xff},  //ab  0xe4  *3/4
	{0x5076,0xA8, 0xff},  //a8  0xe0  *3/4
	{0x5077,0xAB, 0xff},  //ab  0xe4
	{0x5078,0x03, 0xff},  //03  0x04

	{0x5079,0x98, 0xff},
	{0x507a,0x00, 0xff},

	{0x5035,0x6a, 0xff},
	{0x5036,0x11, 0xff},
	{0x5037,0x92, 0xff},
	{0x5038,0x21, 0xff},
	{0x5039,0xe1, 0xff},
	{0x503a,0x01, 0xff},
	{0x503c,0x10, 0xff},
	{0x503d,0x16, 0xff},
	{0x503e,0x08, 0xff},
	{0x503f,0x58, 0xff},
	{0x5040,0x62, 0xff},
	{0x5041,0x0e, 0xff},
	{0x5042,0x9c, 0xff},
	{0x5043,0x20, 0xff},
	{0x5044,0x30, 0xff},
	{0x5045,0x22, 0xff},
	{0x5046,0x32, 0xff},
	{0x5047,0xf8, 0xff},
	{0x5048,0x08, 0xff},
	{0x5049,0x70, 0xff},
	{0x504a,0xf0, 0xff},
	{0x504b,0xf0, 0xff},

	{0x5000,0xfb, 0xff},
	{0x500c,0x03, 0xff},
	{0x500d,0x20, 0xff},
	{0x500e,0x02, 0xff},
	{0x500f,0x5c, 0xff},
	{0x5010,0x5e, 0xff},
	{0x5011,0x18, 0xff},
	{0x5012,0x66, 0xff},
	{0x5013,0x03, 0xff},
	{0x5014,0x24, 0xff},
	{0x5015,0x02, 0xff},
	{0x5016,0x60, 0xff},
	{0x5017,0x55, 0xff},
	{0x5018,0x10, 0xff},
	{0x5019,0x66, 0xff},
	{0x501a,0x03, 0xff},
	{0x501b,0x20, 0xff},
	{0x501c,0x02, 0xff},
	{0x501d,0x64, 0xff},
	{0x501e,0x53, 0xff},
	{0x501f,0x10, 0xff},
	{0x5020,0x66, 0xff},

	{0x506e,0x44, 0xff},
	{0x5064,0x08, 0xff},
	{0x5065,0x10, 0xff},
	{0x5066,0x1e, 0xff},
	{0x5067,0x18, 0xff},
	{0x506c,0x08, 0xff},
	{0x506d,0x10, 0xff},
	{0x506f,0xa6, 0xff},
	{0x5068,0x08, 0xff},
	{0x5069,0x10, 0xff},
	{0x506a,0x18, 0xff},
	{0x506b,0x28, 0xff},
	{0x5084,0x0c, 0xff},
	{0x5085,0x3c, 0xff},
	{0x5005,0x80, 0xff},

	{0x3a0f,0x38, 0xff},
	{0x3a10,0x30, 0xff},
	{0x3a11,0x70, 0xff},
	{0x3a1b,0x38, 0xff},
	{0x3a1e,0x30, 0xff},
	{0x3a1f,0x20, 0xff},

	{0x5060,0x69, 0xff},
	{0x5061,0x7d, 0xff},
	{0x5062,0x7d, 0xff},
	{0x5063,0x69, 0xff},

	{0x3a19,0x38, 0xff},
	{0x4009,0x02, 0xff},

	{0x3503,0x00, 0xff},

	{0x3a00,0x7c, 0xff},
	{0x3a05,0xf0, 0xff},
	{0x3a14,0x04, 0xff},
	{0x3a15,0xd0, 0xff},
	{0x3a02,0x04, 0xff},
	{0x3a03,0xd0, 0xff},

	{0x3011,0x42, 0xff},

/* Banding filter for 15fps, VGA preview */
	{0x3a05, 0x30, 0xff},
	{0x3a00, 0x78, 0xff},
	{0x3a08, 0x00, 0xff},
	{0x3a09, 0x5c, 0xff},
	{0x3a0a, 0x00, 0xff},
	{0x3a0b, 0x4d, 0xff},
	{0x3a0e, 0x06, 0xff},
	{0x3a0d, 0x08, 0xff},

	{0xFFFF, 0xFF, 0xff}
};

const static struct OV2659_reg OV2659_Night_Auto[] = {
	{0x3a00,0x7c, 0xff},
	{0x3a05,0xf0, 0xff},
	{0x3a14,0x09, 0xff},
	{0x3a15,0xa0, 0xff},
	{0x3a02,0x09, 0xff},
	{0x3a03,0xa0, 0xff},

	{0xFFFF, 0xFF, 0xff}
};

const static struct OV2659_reg OV2659_15fps[] = {
	{0x3004, 0x00, 0xff},
	{0x3004, 0x20, 0xff},
	{0x3005, 0x18, 0xff},
	{0x3006, 0x0d, 0xff},
	{0xFFFF, 0xFF, 0xff}
};

const static struct OV2659_reg OV2659_7_5fps[] = {
	{0x3004, 0x00, 0xff},
	{0x3004, 0x20, 0xff},
	{0x3005, 0x24, 0xff},
	{0x3006, 0x0d, 0xff},
	{0xFFFF, 0xFF, 0xff}
};

const static struct OV2659_reg OV2659_svga_to_uxga[] = {
/* should not adjust these parameters
	{0x5066, 0x1e, 0xff},
	{0x5067, 0x18, 0xff},
	{0x506a, 0x18, 0xff},
*/
	{0x3800, 0x00, 0xff},
	{0x3801, 0x00, 0xff},
	{0x3802, 0x00, 0xff},
	{0x3803, 0x00, 0xff},
	{0x3804, 0x06, 0xff},
	{0x3805, 0x5f, 0xff},
	{0x3806, 0x04, 0xff},
	{0x3807, 0xbb, 0xff},
	{0x3808, 0x06, 0xff},
	{0x3809, 0x40, 0xff},
	{0x380a, 0x04, 0xff},
	{0x380b, 0xb0, 0xff},
	{0x3811, 0x10, 0xff},
	{0x3813, 0x06, 0xff},
	{0x3814, 0x11, 0xff},
	{0x3815, 0x11, 0xff},

	{0x3820, 0x80, 0xff},
	{0x3821, 0x00, 0xff},
	{0x5002, 0x00, 0xff},
	{0x4608, 0x00, 0xff},
	{0x4609, 0x80, 0xff},

	{0x3623, 0x00, 0xff},
	{0x3634, 0x44, 0xff},
	{0x3701, 0x44, 0xff},
	{0x3702, 0x30, 0xff},
	{0x3703, 0x48, 0xff},
	{0x3704, 0x48, 0xff},
	{0x3705, 0x18, 0xff},
	{0x370a, 0x12, 0xff},

	{0x3003, 0x80, 0xff},
	{0x3004, 0x20, 0xff},
	{0x3005, 0x18, 0xff},
	{0x3006, 0x0d, 0xff},

	{0x380c, 0x07, 0xff},
	{0x380d, 0x9f, 0xff},
	{0x380e, 0x04, 0xff},
	{0x380f, 0xd0, 0xff},

	{0x3a02, 0x04, 0xff},
	{0x3a03, 0xd0, 0xff},
	{0x3a08, 0x00, 0xff},
	{0x3a09, 0x3d, 0xff},
	{0x3a0a, 0x00, 0xff},
	{0x3a0b, 0x3d, 0xff},
	{0x3a0d, 0x0e, 0xff},
	{0x3a0e, 0x0f, 0xff},
	{0x3a14, 0x04, 0xff},
	{0x3a15, 0xd0, 0xff},

	{0xFFFF, 0xFF, 0xff}
};

/*YUYV 422 (16 bits /pixel)*/
const static struct OV2659_reg OV2659_yuv422[] = {
	{0xFFFF,0xFF, 0xff}
};

const static struct OV2659_reg OV2659_svga_to_sqcif[]={
	{0x3808, 0x00, 0xff},
	{0x3809, 0x80, 0xff},
	{0x380a, 0x00, 0xff},
	{0x380b, 0x60, 0xff},
	{0x380c, 0x05, 0xff},
	{0x380d, 0x14, 0xff},
	{0x380e, 0x02, 0xff},
	{0x380f, 0x68, 0xff},
	{0xFFFF, 0xFF, 0xff}
};

const static struct OV2659_reg OV2659_svga_to_vga[] = {
	{0x3808, 0x02, 0xff},
	{0x3809, 0x80, 0xff},
	{0x380a, 0x01, 0xff},
	{0x380b, 0xe0, 0xff},
	{0x380c, 0x05, 0xff},
	{0x380d, 0x14, 0xff},
	{0x380e, 0x02, 0xff},
	{0x380f, 0x68, 0xff},
	{0xFFFF, 0xFF, 0xff}
};

const static struct OV2659_reg OV2659_svga_to_cif[] = {
	{0x3808, 0x01, 0xff},
	{0x3809, 0x60, 0xff},
	{0x380a, 0x01, 0xff},
	{0x380b, 0x20, 0xff},
	{0x380c, 0x05, 0xff},
	{0x380d, 0x14, 0xff},
	{0x380e, 0x02, 0xff},
	{0x380f, 0x68, 0xff},
	{0xFFFF, 0xFF, 0xff}
};

const static struct OV2659_reg OV2659_svga_to_qvga[] = {
	{0x3808, 0x01, 0xff},
	{0x3809, 0x40, 0xff},
	{0x380a, 0x00, 0xff},
	{0x380b, 0xF0, 0xff},
	{0x380c, 0x05, 0xff},
	{0x380d, 0x14, 0xff},
	{0x380e, 0x02, 0xff},
	{0x380f, 0x68, 0xff},
	{0xFFFF, 0xFF, 0xff}
};

const static struct OV2659_reg OV2659_svga_to_qcif[] = {
	{0x3808, 0x00, 0xff},
	{0x3809, 0xb0, 0xff},
	{0x380a, 0x00, 0xff},
	{0x380b, 0x90, 0xff},
	{0x380c, 0x05, 0xff},
	{0x380d, 0x14, 0xff},
	{0x380e, 0x02, 0xff},
	{0x380f, 0x68, 0xff},
	{0xFFFF, 0xFF, 0xff}
};

/*SVGA(800*600) to dvd-video ntsc(720*480)*/
const static struct OV2659_reg OV2659_svga_to_dvd_ntsc[] = {
	{0x3808, 0x00, 0xff},
	{0x3809, 0xb0, 0xff},
	{0x380a, 0x00, 0xff},
	{0x380b, 0x90, 0xff},

	{0x380c, 0x05, 0xff},
	{0x380d, 0x14, 0xff},
	{0x380e, 0x01, 0xff},
	{0x380f, 0xf0, 0xff},
	{0xFFFF, 0xFF, 0xff}
};

/*SVGA(800*600) to SVGA,for consistence*/
const static struct OV2659_reg OV2659_svga_to_svga[] = {
	{0x3808, 0x03, 0xff},
	{0x3809, 0x20, 0xff},
	{0x380a, 0x02, 0xff},
	{0x380b, 0x58, 0xff},
	{0x380c, 0x05, 0xff},
	{0x380d, 0x14, 0xff},
	{0x380e, 0x02, 0xff},
	{0x380f, 0x68, 0xff},
	{0xFFFF, 0xFF, 0xff}
};

/*SVGA(800*600) to XGA(1024*768)*/
const static struct OV2659_reg OV2659_svga_to_xga[] = {
/* should not adjust these parameters
	{0x5066, 0x1e, 0xff},
	{0x5067, 0x18, 0xff},
	{0x506a, 0x18, 0xff},
*/
	{0x3800, 0x00, 0xff},
	{0x3801, 0xa0, 0xff},
	{0x3802, 0x00, 0xff},
	{0x3803, 0x50, 0xff},
	{0x3804, 0x05, 0xff},
	{0x3805, 0xbf, 0xff},
	{0x3806, 0x04, 0xff},
	{0x3807, 0x17, 0xff},
	{0x3808, 0x04, 0xff},
	{0x3809, 0x00, 0xff},
	{0x380a, 0x03, 0xff},
	{0x380b, 0x00, 0xff},
	{0x3811, 0x08, 0xff},
	{0x3813, 0x04, 0xff},
	{0x3814, 0x11, 0xff},
	{0x3815, 0x11, 0xff},

	{0x3820, 0x80, 0xff},
	{0x3821, 0x00, 0xff},
	{0x5002, 0x00, 0xff},
	{0x4608, 0x00, 0xff},
	{0x4609, 0x80, 0xff},

	{0x3623, 0x00, 0xff},
	{0x3634, 0x44, 0xff},
	{0x3701, 0x44, 0xff},
	{0x3702, 0x30, 0xff},
	{0x3703, 0x48, 0xff},
	{0x3704, 0x48, 0xff},
	{0x3705, 0x18, 0xff},
	{0x370a, 0x12, 0xff},

	{0x3003, 0x80, 0xff},
	{0x3004, 0x20, 0xff},
	{0x3005, 0x18, 0xff},
	{0x3006, 0x0d, 0xff},

	{0x380c, 0x07, 0xff},
	{0x380d, 0x9f, 0xff},
	{0x380e, 0x04, 0xff},
	{0x380f, 0xd0, 0xff},

	{0x3a02, 0x04, 0xff},
	{0x3a03, 0xd0, 0xff},
	{0x3a08, 0x00, 0xff},
	{0x3a09, 0x3d, 0xff},
	{0x3a0a, 0x00, 0xff},
	{0x3a0b, 0x3d, 0xff},
	{0x3a0d, 0x0e, 0xff},
	{0x3a0e, 0x0f, 0xff},
	{0x3a14, 0x04, 0xff},
	{0x3a15, 0xd0, 0xff},

	{0xFFFF, 0xFF, 0xff}
};

/*SVGA(800*600) to 1M(1280*960)*/
const static struct OV2659_reg OV2659_svga_to_1M[] = {
/*
	{0x5066, 0x1e, 0xff},
	{0x5067, 0x18, 0xff},
	{0x506a, 0x18, 0xff},
*/
	{0x3800, 0x00, 0xff},
	{0x3801, 0xa0, 0xff},
	{0x3802, 0x00, 0xff},
	{0x3803, 0x50, 0xff},
	{0x3804, 0x05, 0xff},
	{0x3805, 0xbf, 0xff},
	{0x3806, 0x04, 0xff},
	{0x3807, 0x17, 0xff},
	{0x3808, 0x05, 0xff},
	{0x3809, 0x00, 0xff},
	{0x380a, 0x03, 0xff},
	{0x380b, 0xc0, 0xff},
	{0x3811, 0x08, 0xff},
	{0x3813, 0x04, 0xff},
	{0x3814, 0x11, 0xff},
	{0x3815, 0x11, 0xff},

	{0x3820, 0x80, 0xff},
	{0x3821, 0x00, 0xff},
	{0x5002, 0x00, 0xff},
	{0x4608, 0x00, 0xff},
	{0x4609, 0x80, 0xff},

	{0x3623, 0x00, 0xff},
	{0x3634, 0x44, 0xff},
	{0x3701, 0x44, 0xff},
	{0x3702, 0x30, 0xff},
	{0x3703, 0x48, 0xff},
	{0x3704, 0x48, 0xff},
	{0x3705, 0x18, 0xff},
	{0x370a, 0x12, 0xff},

	{0x3003, 0x80, 0xff},
	{0x3004, 0x20, 0xff},
	{0x3005, 0x18, 0xff},
	{0x3006, 0x0d, 0xff},

	{0x380c, 0x07, 0xff},
	{0x380d, 0x9f, 0xff},
	{0x380e, 0x04, 0xff},
	{0x380f, 0xd0, 0xff},

	{0x3a02, 0x04, 0xff},
	{0x3a03, 0xd0, 0xff},
	{0x3a08, 0x00, 0xff},
	{0x3a09, 0x3d, 0xff},
	{0x3a0a, 0x00, 0xff},
	{0x3a0b, 0x3d, 0xff},
	{0x3a0d, 0x0e, 0xff},
	{0x3a0e, 0x0f, 0xff},
	{0x3a14, 0x04, 0xff},
	{0x3a15, 0xd0, 0xff},

	{0xFFFF, 0xFF, 0xff}
};

const static struct OV2659_reg* OV2659_svga_to_xxx[] ={
	OV2659_svga_to_sqcif,
	OV2659_svga_to_qcif,
	OV2659_svga_to_qvga,
	OV2659_svga_to_cif,
	OV2659_svga_to_vga,
	OV2659_svga_to_dvd_ntsc,
	OV2659_svga_to_svga,
	OV2659_svga_to_xga,
	OV2659_svga_to_1M,
	OV2659_svga_to_uxga
};

static struct OV2659_sensor OV2659;
static struct i2c_driver OV2659sensor_i2c_driver;
static unsigned long xclk_current = OV2659_XCLK_MIN;
static enum v4l2_power current_power_state;

static int OV2659_capture_mode = 0;
//static char OV2659_Sensor_Type = 0;

static int OV2659_capture2preview_mode = 0;
static int OV2659_exit_mode = 0;

/* List of image formats supported by OV2659sensor */
const static struct v4l2_fmtdesc OV2659_formats[] = {
#if defined(OV2659_RAW_MODE)
	{
		.description	= "RAW10",
		.pixelformat	= V4L2_PIX_FMT_SGRBG10,
	},
#elif defined(OV2659_YUV_MODE)
	{
		.description	= "YUYV,422",
		.pixelformat	= V4L2_PIX_FMT_UYVY,
	},
#else
	{
		/* Note:  V4L2 defines RGB565 as:
		 *
		 *	Byte 0				Byte 1
		 *	g2 g1 g0 r4 r3 r2 r1 r0		b4 b3 b2 b1 b0 g5 g4 g3
		 *
		 * We interpret RGB565 as:
		 *
		 *	Byte 0				Byte 1
		 *	g2 g1 g0 b4 b3 b2 b1 b0		r4 r3 r2 r1 r0 g5 g4 g3
		 */
		.description	= "RGB565, le",
		.pixelformat	= V4L2_PIX_FMT_RGB565,
	},
	{
		/* Note:  V4L2 defines RGB565X as:
		 *
		 *	Byte 0				Byte 1
		 *	b4 b3 b2 b1 b0 g5 g4 g3		g2 g1 g0 r4 r3 r2 r1 r0
		 *
		 * We interpret RGB565X as:
		 *
		 *	Byte 0				Byte 1
		 *	r4 r3 r2 r1 r0 g5 g4 g3		g2 g1 g0 b4 b3 b2 b1 b0
		 */
		.description	= "RGB565, be",
		.pixelformat	= V4L2_PIX_FMT_RGB565X,
	},
	{
		.description	= "YUYV (YUV 4:2:2), packed",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
	},
	{
		.description	= "UYVY, packed",
		.pixelformat	= V4L2_PIX_FMT_UYVY,
	},
	{
		/* Note:  V4L2 defines RGB555 as:
		 *
		 *	Byte 0				Byte 1
		 *	g2 g1 g0 r4 r3 r2 r1 r0		x  b4 b3 b2 b1 b0 g4 g3
		 *
		 * We interpret RGB555 as:
		 *
		 *	Byte 0				Byte 1
		 *	g2 g1 g0 b4 b3 b2 b1 b0		x  r4 r3 r2 r1 r0 g4 g3
		 */
		.description	= "RGB555, le",
		.pixelformat	= V4L2_PIX_FMT_RGB555,
	},
	{
		/* Note:  V4L2 defines RGB555X as:
		 *
		 *	Byte 0				Byte 1
		 *	x  b4 b3 b2 b1 b0 g4 g3		g2 g1 g0 r4 r3 r2 r1 r0
		 *
		 * We interpret RGB555X as:
		 *
		 *	Byte 0				Byte 1
		 *	x  r4 r3 r2 r1 r0 g4 g3		g2 g1 g0 b4 b3 b2 b1 b0
		 */
		.description	= "RGB555, be",
		.pixelformat	= V4L2_PIX_FMT_RGB555X,
	},
#endif
};

#define NUM_CAPTURE_FORMATS (sizeof(OV2659_formats) / sizeof(OV2659_formats[0]))

/*
 * struct vcontrol - Video controls
 * @v4l2_queryctrl: V4L2 VIDIOC_QUERYCTRL ioctl structure
 * @current_value: current value of this control
 */
static struct vcontrol {
	struct v4l2_queryctrl qc;
	int current_value;
} video_control[] = {
#ifdef OV2659_YUV_MODE
	{
		{
			.id = V4L2_CID_BRIGHTNESS,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Brightness",
			.minimum = OV2659_MIN_BRIGHT,
			.maximum = OV2659_MAX_BRIGHT,
			.step = OV2659_BRIGHT_STEP,
			.default_value = OV2659_DEF_BRIGHT,
		},
		.current_value = OV2659_DEF_BRIGHT,
	},
	{
		{
			.id = V4L2_CID_CONTRAST,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Contrast",
			.minimum = OV2659_MIN_CONTRAST,
			.maximum = OV2659_MAX_CONTRAST,
			.step = OV2659_CONTRAST_STEP,
			.default_value = OV2659_DEF_CONTRAST,
		},
		.current_value = OV2659_DEF_CONTRAST,
	},
	{
		{
			.id = V4L2_CID_PRIVATE_BASE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Color Effects",
			.minimum = OV2659_MIN_COLOR,
			.maximum = OV2659_MAX_COLOR,
			.step = OV2659_COLOR_STEP,
			.default_value = OV2659_DEF_COLOR,
		},
		.current_value = OV2659_DEF_COLOR,
	}
#endif
};

/*
 * find_vctrl - Finds the requested ID in the video control structure array
 * @id: ID of control to search the video control array.
 *
 * Returns the index of the requested ID from the control structure array
 */
static int find_vctrl(int id)
{
	int i = 0;

	if (id < V4L2_CID_BASE)
		return -EDOM;

	for (i = (ARRAY_SIZE(video_control) - 1); i >= 0; i--)
		if (video_control[i].qc.id == id)
			break;
	if (i < 0)
		i = -EINVAL;
	return i;
}

#if 1 /* defined(CONFIG_CAM_GPIO_I2C) */
/*used by GPIO I2C*/
static int  OV2659_SCL_GPIO = 23;
static int  OV2659_SDA_GPIO = 24;

#define OV2659_AddW	(OV2659_I2C_ADDR<< 1)
#define OV2659_AddR	(OV2659_AddW | 0x01)

#define SET_OV2659_I2C_CLK_OUTPUT	(gpio_direction_output(OV2659_SCL_GPIO, 1))
#define SET_OV2659_I2C_DATA_OUTPUT	(gpio_direction_output(OV2659_SDA_GPIO, 1))
#define SET_OV2659_I2C_DATA_INPUT	(gpio_direction_input(OV2659_SDA_GPIO))
#define SET_OV2659_I2C_CLK_HIGH		(gpio_set_value(OV2659_SCL_GPIO, 1))
#define SET_OV2659_I2C_CLK_LOW		(gpio_set_value(OV2659_SCL_GPIO, 0))
#define SET_OV2659_I2C_DATA_HIGH	(gpio_set_value(OV2659_SDA_GPIO, 1))
#define SET_OV2659_I2C_DATA_LOW		(gpio_set_value(OV2659_SDA_GPIO, 0))
#define GET_OV2659_I2C_DATA_BIT		(gpio_get_value(OV2659_SDA_GPIO))

#define SET_I2C_SCL	SET_OV2659_I2C_CLK_HIGH
#define CLR_I2C_SCL	SET_OV2659_I2C_CLK_LOW
#define  I2C_SCL_OUT	SET_OV2659_I2C_CLK_OUTPUT

#define SDA_OUT		SET_OV2659_I2C_DATA_OUTPUT
#define SET_SDA		SET_OV2659_I2C_DATA_HIGH
#define CLR_SDA		SET_OV2659_I2C_DATA_LOW
#define SDA_IN		SET_OV2659_I2C_DATA_INPUT

#define WR	0
#define RD	1
#define rGPBDAT   GET_OV2659_I2C_DATA_BIT

void __dly1us(void)
{
	unsigned int i, j;

	for(i=0;i<3;i++)
	{
		for(j=0;j<4;j++)
			udelay(1);
	}
}

void delay5us(void)
{
	__dly1us();
	__dly1us();
	__dly1us();
	__dly1us();
	__dly1us();
}

int gI2CInit_2(void)  //firt set the gpio when use them
{
	I2C_SCL_OUT;
	SDA_OUT;

	return 0;
}

int gI2CStart_2(void)
{
	SET_SDA;
	delay5us();

	SET_I2C_SCL;
	delay5us();

	CLR_SDA;
	delay5us();

	CLR_I2C_SCL;
	delay5us();

	return 0;
}

int gI2CStop_2(void)
{
	SDA_OUT;
	CLR_I2C_SCL;
	delay5us();

	CLR_SDA;
	delay5us();

	SET_I2C_SCL;

	delay5us();
	SET_SDA;
	delay5us();

	return 0;
}

int Ack_2(void)
{
	SDA_OUT;
	CLR_SDA;
	delay5us();
	SET_I2C_SCL;
	delay5us();
	CLR_I2C_SCL;
	delay5us();
	SET_SDA;
	return 0;
}

int NoAck_2(void)
{
	SDA_OUT;
	SET_SDA;
	delay5us();
	SET_I2C_SCL;
	delay5us();
	CLR_I2C_SCL;
	delay5us();
	return 0;
}

int TestAck_2(void)   //get 0 continue get 1 err
{
	SDA_IN;
	delay5us();
	SET_I2C_SCL;
	delay5us();
	if((rGPBDAT&0X01)==0x01)  //GPB8
		return 1;
	delay5us();
	CLR_I2C_SCL;
	delay5us();
	return 0;
}

int WritegI2C_2(u8 data)
{
	u8 i,temp;
	SDA_OUT;
	for(i=0;i<8;i++)
	{
		temp=data&0x80;
		if(temp==0x80)
			SET_SDA;
		else
			CLR_SDA;
		__dly1us();
		__dly1us();
		__dly1us();
		__dly1us();
		SET_I2C_SCL;
		__dly1us();
		__dly1us();
		__dly1us();
		__dly1us();
		__dly1us();
		__dly1us();
		CLR_I2C_SCL;
		__dly1us();
		__dly1us();
		__dly1us();
		__dly1us();
		data=data<<1;
	}
	return 0;
}

//use in atb  and solomm
u8 ReadgI2C_2(void)
{
	u8 i;
	u8 byte=0;

	SDA_IN;
	for(i=0;i<40;i++);  //NEED TO ADD
	for(i=0;i<8;i++)
	{
		SET_I2C_SCL;
		__dly1us();
		__dly1us();
		__dly1us();
		__dly1us();
		__dly1us();
		__dly1us();
		byte=byte<<1;

		if((rGPBDAT&0X01)==0X01)
		{
			byte=(byte|0x01);
		}
		__dly1us();
		CLR_I2C_SCL;
		__dly1us();
		__dly1us();
		__dly1us();
		__dly1us();
		__dly1us();
		__dly1us();
		__dly1us();
		__dly1us();

	}
	SDA_OUT;
	return (byte);
}

u8 ReadgI2CDatas_2(u8 DevAddr, u16 regAddr,u8 *pdata,u8 size)  //read bytes
{
	uint16_t i,j;

	gI2CInit_2();
	gI2CStart_2();
	WritegI2C_2(DevAddr<<1|WR);
	if(TestAck_2()!=0)
	{
		printk("Warning !!!!!!	if (!IIcGetAck())");
		return 0;
	}

	WritegI2C_2(regAddr>>8);
	if(TestAck_2()!=0)
	{
		printk("Warning !!!!!!	if (!IIcGetAck())");
		return 0;
	}
	WritegI2C_2(regAddr&0xff);
	if(TestAck_2()!=0)
	{
		printk("Warning !!!!!!	if (!IIcGetAck())");
		return 0;
	}

	SDA_OUT;
	for(i=0;i<30;i++);

	gI2CStop_2();
	gI2CStart_2();
	WritegI2C_2(DevAddr<<1|RD);
	if(TestAck_2()!=0)
	{
		printk("Warning !!!!!!	if (!IIcGetAck())");
		return 0;
	}

	for(i=0;i<size;i++)
	{
		if(i==size-1)
		{
			for(j=0;j<5000;j++);  //202us
			*pdata=ReadgI2C_2();
			//for(j=0;j<40;j++);	//must be used
			NoAck_2();

		}
		else
		{

	//		for(i=0;i<2000;i++);
			for(j=0;j<5000;j++);  //202us
			*pdata=ReadgI2C_2();
			//for(j=0;j<40;j++);	//must be used
			Ack_2();
			pdata++;
		}
	}

	gI2CStop_2();
	SDA_OUT;

	return 1;
}

u8 writeI2CDatas_2(u8 DevAddr, u16 regAddr,u8 cmd)  //write byte
{
	gI2CInit_2();
	gI2CStart_2();
	WritegI2C_2(DevAddr<<1|WR);
	if(TestAck_2()!=0)
	{
		printk("Warning !!!!!!	if (!IIcGetAck())");
		return 0;
	}

	WritegI2C_2(regAddr>>8);
	if(TestAck_2()!=0)
	{
		printk("Warning !!!!!!	if (!IIcGetAck())");
		return 0;
	}
	WritegI2C_2(regAddr&0xff);
	if(TestAck_2()!=0)
	{
		printk("Warning !!!!!!	if (!IIcGetAck())");
		return 0;
	}

	WritegI2C_2(cmd);
	if(TestAck_2()!=0)
	{
		printk("Warning !!!!!!	if (!IIcGetAck())");
		return 0;
	}

	gI2CStop_2();
	SDA_OUT;

	return 1;
}

static int OV2659_write_reg(struct i2c_client *client, u16 reg, u8 val)
{
	if (!writeI2CDatas_2(OV2659_I2C_ADDR,reg,val))
	{
		printk( "Fail to I2C write reg=0x%x cmd=0x%x.\n", reg, val);
		return -EIO;
	}
	else
	{
		return 0;
	}
}

static int OV2659_read_reg(struct i2c_client *client, u16 data_length, u16 reg,	u32 *val)
{
	if (!ReadgI2CDatas_2(OV2659_I2C_ADDR,reg,(u8*)val, data_length))
	{
		printk( "Fail to I2C read reg=0x%x length=%d.\n", reg, data_length);
		return -EIO;
	}
	else
	{
		return 0;
	}
}

#else

/*
 * Read a value from a register in OV2659 sensor device.
 * The value is returned in 'val'.
 * Returns zero if successful, or non-zero otherwise.
 */
static int OV2659_read_reg(struct i2c_client *client, u16 data_length, u16 reg, u8 *val)
{
	int err = 0;
	struct i2c_msg msg[1];
	unsigned char data[data_length + 2];

	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = I2C_M_WR;
	msg->len = 2;
	msg->buf = data;

	/* High byte goes out first */
	data[0] = (u8) (reg >> 8);
	data[1] = (u8) (reg & 0xff);

	dev_err(&client->dev, "test 1\n");

	err = i2c_transfer(client->adapter, msg, 1);
	dev_err(&client->dev, "test 2\n");
	if (err >= 0) {
		mdelay(3);
		msg->flags = I2C_M_RD;
		msg->len = data_length;
		err = i2c_transfer(client->adapter, msg, 1);
	}
	dev_err(&client->dev, "test 3\n");

	if (err >= 0) {
		memcpy(val,data,data_length);
		return 0;
	}
	dev_err(&client->dev, "read from offset 0x%x error %d", reg, err);
	return err;
}

/* Write a value to a register in OV2659 sensor device.
 * @client: i2c driver client structure.
 * @reg: Address of the register to read value from.
 * @val: Value to be written to a specific register.
 * Returns zero if successful, or non-zero otherwise.
 */
static int OV2659_write_reg(struct i2c_client *client, u16 reg, u8 val)
{
	int err = 0;
	struct i2c_msg msg[1];
	unsigned char data[3];
	int retries = 0;

	if (!client->adapter)
		return -ENODEV;
retry:
	msg->addr = client->addr;
	msg->flags = I2C_M_WR;
	msg->len = 3;
	msg->buf = data;

	/* high byte goes out first */
	data[0] = (u8) (reg >> 8);
	data[1] = (u8) (reg & 0xff);
	data[2] = val;

	err = i2c_transfer(client->adapter, msg, 1);
	udelay(50);

	if (err >= 0)
		return 0;

	if (retries <= 5) {
		DPRINTK_OV( "Retrying I2C... %d", retries);
		retries++;
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(msecs_to_jiffies(20));
		goto retry;
	}

	return err;
}
#endif

/*
 * Initialize a list of OV2659 registers.
 * The list of registers is terminated by the pair of values
 * {OV2659_REG_TERM, OV2659_VAL_TERM}.
 * @client: i2c driver client structure.
 * @reglist[]: List of address of the registers to write data.
 * Returns zero if successful, or non-zero otherwise.
 */
static int OV2659_write_regs(struct i2c_client *client,
			const struct OV2659_reg reglist[])
{
	int err = 0;
	const struct OV2659_reg *next = reglist;

	while (!((next->reg == OV2659_REG_TERM)
		&& (next->val == OV2659_VAL_TERM))) {
		if (next->reg == 0){
			mdelay(next->val);
		}
		else if (next->mask)
		{
			unsigned char val;
			if (next->mask == 0xff)
			{
				val = next->val;
			}
			else
			{
				OV2659_read_reg(client,1, next->reg, &val);
				val &= ~(next->mask);
				val |= (next->val & next->mask);
			}
			err = OV2659_write_reg(client, next->reg, val);
			if (err)
				return err;
		}
		next++;
	}
	return 0;
}

/* Find the best match for a requested image capture size.  The best match
 * is chosen as the nearest match that has the same number or fewer pixels
 * as the requested size, or the smallest image size if the requested size
 * has fewer pixels than the smallest image.
 */
static enum image_size_ov OV2659_find_size(unsigned int width, unsigned int height)
{
	enum image_size_ov isize;

	for (isize = 0; isize < ARRAY_SIZE(OV2659_sizes); isize++) {
		if ((OV2659_sizes[isize].height >= height) &&
			(OV2659_sizes[isize].width >= width)) {
			break;
		}
	}
	DPRINTK_OV("width = %d,height = %d,return %d\n",width,height,isize);
	return isize;
}

#if 0 //not be used temporally(ldq)
/**
 * OV2659_set_framerate
 **/
static int OV2659_set_framerate(struct i2c_client *client,
				struct v4l2_fract *fper,
				enum image_size_ov isize)
{
	u32 tempfps1, tempfps2;
	u8 clkval;
/*
	u32 origvts, newvts, templineperiod;
	u32 origvts_h, origvts_l, newvts_h, newvts_l;
*/
	int err = 0;

	/* FIXME: QXGA framerate setting forced to 15 FPS */
	if (isize == QXGA) {
		err = OV2659_write_reg(client, OV2659_PLL_1, 0x32);
		err = OV2659_write_reg(client, OV2659_PLL_2, 0x21);
		err = OV2659_write_reg(client, OV2659_PLL_3, 0x21);
		err = OV2659_write_reg(client, OV2659_CLK, 0x01);
		err = OV2659_write_reg(client, 0x304c, 0x81);
		return err;
	}

	tempfps1 = fper->denominator * 10000;
	tempfps1 /= fper->numerator;
	tempfps2 = fper->denominator / fper->numerator;
	if ((tempfps1 % 10000) != 0)
		tempfps2++;
	clkval = (u8)((30 / tempfps2) - 1);

	err = OV2659_write_reg(client, OV2659_CLK, clkval);
	/* RxPLL = 50d = 32h */
	err = OV2659_write_reg(client, OV2659_PLL_1, 0x32);
	/* RxPLL = 50d = 32h */
	err = OV2659_write_reg(client, OV2659_PLL_2,
					OV2659_PLL_2_BIT8DIV_4 |
					OV2659_PLL_2_INDIV_1_5);
	/*
	 * NOTE: Sergio's Fix for MIPI CLK timings, not suggested by OV
	 */
	err = OV2659_write_reg(client, OV2659_PLL_3, 0x21 +
							(clkval & 0xF));
	/* Setting DVP divisor value */
	err = OV2659_write_reg(client, 0x304c, 0x82);
/* FIXME: Time adjustment to add granularity to the available fps */
/*
	OV2659_read_reg(client, 1, OV2659_VTS_H, &origvts_h);
	OV2659_read_reg(client, 1, OV2659_VTS_L, &origvts_l);
	origvts = (u32)((origvts_h << 8) + origvts_l);
	templineperiod = 1000000 / (tempfps2 * origvts);
	newvts = 1000000 / (tempfps2 * templineperiod);
	newvts_h = (u8)((newvts & 0xFF00) >> 8);
	newvts_l = (u8)(newvts & 0xFF);
	err = OV2659_write_reg(client, OV2659_VTS_H, newvts_h);
	err = OV2659_write_reg(client, OV2659_VTS_L, newvts_l);
*/
	return err;
}

#endif

/*
 * Configure the OV2659 for a specified image size, pixel format, and frame
 * period.  xclk is the frequency (in Hz) of the xclk input to the OV2659.
 * fper is the frame period (in seconds) expressed as a fraction.
 * Returns zero if successful, or non-zero otherwise.
 * The actual frame period is returned in fper.
 */

static int OV2659_configure(struct v4l2_int_device *s)
{
	struct OV2659_sensor *sensor = s->priv;
	struct v4l2_pix_format *pix = &sensor->pix;
	struct i2c_client *client = sensor->i2c_client;
	enum image_size_ov isize = ARRAY_SIZE(OV2659_sizes) - 1;
	struct sensor_ext_params *extp = &(sensor->ext_params);

	int  err = 0;
	enum pixel_format_ov pfmt = YUV;

	switch (pix->pixelformat) {

	case V4L2_PIX_FMT_RGB565:
	case V4L2_PIX_FMT_RGB565X:
		pfmt = RGB565;
		break;

	case V4L2_PIX_FMT_RGB555:
	case V4L2_PIX_FMT_RGB555X:
		pfmt = RGB555;
		break;

	case V4L2_PIX_FMT_SGRBG10:
		pfmt = RAW10;
		break;

	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_UYVY:
	default:
		pfmt = YUV;
	}

	isize = OV2659_find_size(pix->width, pix->height);

	if(OV2659_capture_mode)
	{
		OV2659_write_reg(client,0x3406,0x01); //stop AWB
		OV2659_write_regs(client, OV2659_svga_to_xxx[isize]);
		//OV2659_write_regs(client, OV2659_Night_Auto);
		OV2659_capture_mode = 0;
		OV2659_capture2preview_mode = 1;
		// cam_win_change = 1;
	}
	else
	{
		OV2659_write_reg(client,0x3406,0x00); //enable AWB

		if(OV2659_capture2preview_mode)  //(ov5640_capture2preview_mode)
		{
			OV2659_write_regs(client, OV2659_common_svgabase);
			OV2659_write_regs(client, OV2659_svga_to_xxx[isize]);
			//OV2659_write_regs(client, OV2659_Night_Auto);
			OV2659_capture2preview_mode = 0;
			OV2659_capture_mode = 0;
		}
		else
		{
			OV2659_write_regs(client, OV2659_common_svgabase);
			OV2659_write_regs(client, OV2659_svga_to_xxx[isize]);
			// OV2659_write_regs(client, OV2659_Night_Auto);
		}

		if(isize <= DVD_NTSC)
		{
			OV2659_write_regs(client,OV2659_15fps);
		}
		else
		{
			OV2659_write_regs(client,OV2659_7_5fps);
		}
		/*YUYV output (16bits)*/
		OV2659_write_regs(client, OV2659_yuv422);

		OV2659_write_regs(client, OV2659_effects[extp->effect]);  //effect
		OV2659_write_regs(client, OV2659_whitebalance[extp->white_balance]);
		OV2659_write_regs(client, OV2659_brightness[extp->brightness]);
		OV2659_write_regs(client, OV2659_contrast[extp->contrast]);
	}

	DPRINTK_OV("pix->width = %d,pix->height = %d\n",pix->width,pix->height);
	DPRINTK_OV("format = %d,size = %d\n",pfmt,isize);

#if 0
	/* Configure image size and pixel format */
	err = OV2659_write_regs(client, OV2659_reg_init[pfmt][isize]);

	/* Setting of frame rate (OV suggested way) */
	err = OV2659_set_framerate(client, &sensor->timeperframe, isize);
#endif

	/* Store image size */
	sensor->width = pix->width;
	sensor->height = pix->height;

	sensor->crop_rect.left = 0;
	sensor->crop_rect.width = pix->width;
	sensor->crop_rect.top = 0;
	sensor->crop_rect.height = pix->height;

	return err;
}

/* Detect if an OV2659 is present, returns a negative error number if no
 * device is detected, or pidl as version number if a device is detected.
 */
static int OV2659_detect(struct i2c_client *client)
{
	u32 pidh = 0, pidl = 0;

	DPRINTK_OV("\n");

	if (!client)
		return -ENODEV;

	if (OV2659_read_reg(client, 1, OV2659_PIDH, &pidh))
		return -ENODEV;

	if (OV2659_read_reg(client, 1, OV2659_PIDL, &pidl))
		return -ENODEV;

	printk( "Detect(%02X,%02X)\n", pidh,pidl);

	if ((pidh == OV2659_PIDH_MAGIC) && (pidl == OV2659_PIDL_MAGIC)) {
		printk( "Detect success (%02X,%02X)\n", pidh,
									pidl);
		return pidl;
	}

	return -ENODEV;
}

/* To get the cropping capabilities of OV2659 sensor
 * Returns zero if successful, or non-zero otherwise.
 */
static int ioctl_cropcap(struct v4l2_int_device *s,
			struct v4l2_cropcap *cropcap)
{
	struct OV2659_sensor *sensor = s->priv;

	DPRINTK_OV("sensor->width = %ld,sensor->height = %ld\n",sensor->width,sensor->height);
	cropcap->bounds.top = 0;
	cropcap->bounds.left = 0;
	cropcap->bounds.width = sensor->width;
	cropcap->bounds.height = sensor->height;
	cropcap->defrect = cropcap->bounds;
	cropcap->pixelaspect.numerator = 1;
	cropcap->pixelaspect.denominator = 1;
	sensor->crop_rect = cropcap->defrect;
	return 0;
}

/* To get the current crop window for of OV2659 sensor
 * Returns zero if successful, or non-zero otherwise.
 */
static int ioctl_g_crop(struct v4l2_int_device *s, struct  v4l2_crop *crop)
{
	struct OV2659_sensor *sensor = s->priv;
	DPRINTK_OV("\n");
	crop->c = sensor->crop_rect;
	return 0;
}

/* To set the crop window for of OV2659 sensor
 * Returns zero if successful, or non-zero otherwise.
 */
static int ioctl_s_crop(struct v4l2_int_device *s, struct  v4l2_crop *crop)
{
#if 0
	struct OV2659_sensor *sensor = s->priv;
	/* FIXME: Temporary workaround for avoiding Zoom setting */
	/* struct i2c_client *client = sensor->i2c_client; */
	struct v4l2_rect *cur_rect;
	unsigned long *cur_width, *cur_height;
	int hstart, vstart, hsize, vsize, hsize_l, vsize_l, hsize_h, vsize_h;
	int hratio, vratio, zoomfactor, err = 0;

	DPRINTK_OV("\n");
	cur_rect = &sensor->crop_rect;
	cur_width = &sensor->width;
	cur_height = &sensor->height;

	if ((crop->c.left == cur_rect->left) &&
	(crop->c.width == cur_rect->width) &&
	(crop->c.top == cur_rect->top) &&
	(crop->c.height == cur_rect->height))
		return 0;

	/* out of range? then return the current crop rectangle */
	if ((crop->c.left + crop->c.width) > sensor->width ||
	(crop->c.top + crop->c.height) > sensor->height) {
		crop->c = *cur_rect;
		return 0;
	}

	if (sensor->isize == ARRAY_SIZE(OV2659_sizes) - 1)
		zoomfactor = 1;
	else
		zoomfactor = 2;

	hratio = (sensor->hsize * 1000) / sensor->width;
	vratio = (sensor->vsize * 1000) / sensor->height;
	hstart = (((crop->c.left * hratio + 500) / 1000) * zoomfactor) + 0x11d;
	vstart = (((crop->c.top * vratio + 500) / 1000) + 0x0a);
	hsize  = (crop->c.width * hratio + 500) / 1000;
	vsize  = (crop->c.height * vratio + 500) / 1000;

	if (vsize & 1)
		vsize--;
	if (hsize & 1)
		hsize--;

	/* Adjusting numbers to set register correctly */
	hsize_l = (0xFF & hsize);
	hsize_h = (0xF00 & hsize) >> 8;
	vsize_l = (0xFF & vsize);
	vsize_h = (0x700 & vsize) >> 4;

	if ((sensor->height > vsize) || (sensor->width > hsize))
		return -EINVAL;

	hsize = hsize * zoomfactor;
/*
	err = OV2659_write_reg(client, OV2659_DSP_CTRL_2, 0xEF);
	err = OV2659_write_reg(client, OV2659_SIZE_IN_MISC, (vsize_h |
								hsize_h));
	err = OV2659_write_reg(client, OV2659_HSIZE_IN_L, hsize_l);
	err = OV2659_write_reg(client, OV2659_VSIZE_IN_L, vsize_l);
	err = OV2659_write_reg(client, OV2659_HS_H, (hstart >> 8) & 0xFF);
	err = OV2659_write_reg(client, OV2659_HS_L, hstart & 0xFF);
	err = OV2659_write_reg(client, OV2659_VS_H, (vstart >> 8) & 0xFF);
	err = OV2659_write_reg(client, OV2659_VS_L, vstart & 0xFF);
	err = OV2659_write_reg(client, OV2659_HW_H, ((hsize) >> 8) & 0xFF);
	err = OV2659_write_reg(client, OV2659_HW_L, hsize & 0xFF);
	err = OV2659_write_reg(client, OV2659_VH_H, ((vsize) >> 8) & 0xFF);
	err = OV2659_write_reg(client, OV2659_VH_L, vsize & 0xFF);
*/
	if (err)
		return err;

	/* save back */
	*cur_rect = crop->c;

	/* Setting crop too fast can cause frame out-of-sync. */

	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_timeout(msecs_to_jiffies(20));
#endif
	return 0;
}

/*
 * ioctl_queryctrl - V4L2 sensor interface handler for VIDIOC_QUERYCTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @qc: standard V4L2 VIDIOC_QUERYCTRL ioctl structure
 *
 * If the requested control is supported, returns the control information
 * from the video_control[] array.  Otherwise, returns -EINVAL if the
 * control is not supported.
 */
static int ioctl_queryctrl(struct v4l2_int_device *s,
			struct v4l2_queryctrl *qc)
{
	int i;

	i = find_vctrl(qc->id);
	if (i == -EINVAL)
		qc->flags = V4L2_CTRL_FLAG_DISABLED;

	if (i < 0)
		return -EINVAL;

	*qc = video_control[i].qc;
	return 0;
}

/*
 * ioctl_g_ctrl - V4L2 sensor interface handler for VIDIOC_G_CTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @vc: standard V4L2 VIDIOC_G_CTRL ioctl structure
 *
 * If the requested control is supported, returns the control's current
 * value from the video_control[] array.  Otherwise, returns -EINVAL
 * if the control is not supported.
 */

static int ioctl_g_ctrl(struct v4l2_int_device *s,
			 struct v4l2_control *vc)
{
	struct vcontrol *lvc;
	int i;

	i = find_vctrl(vc->id);
	if (i < 0)
		return -EINVAL;
	lvc = &video_control[i];

	switch (vc->id) {
	case V4L2_CID_BRIGHTNESS:
		vc->value = lvc->current_value;
		break;
	case V4L2_CID_CONTRAST:
		vc->value = lvc->current_value;
		break;
	case V4L2_CID_PRIVATE_BASE:
		vc->value = lvc->current_value;
		break;
	}
	return 0;
}

/*
 * ioctl_s_ctrl - V4L2 sensor interface handler for VIDIOC_S_CTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @vc: standard V4L2 VIDIOC_S_CTRL ioctl structure
 *
 * If the requested control is supported, sets the control's current
 * value in HW (and updates the video_control[] array).  Otherwise,
 * returns -EINVAL if the control is not supported.
 */
static int ioctl_s_ctrl(struct v4l2_int_device *s,
			 struct v4l2_control *vc)
{
	int err = 0;
	struct OV2659_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	struct sensor_ext_params *extp = &(sensor->ext_params);
//	unsigned int reg507b = 0;
#if 0
	if(vc->id != V4L2_CID_CAPTURE)
	return 0;
#endif

	switch(vc->id){
	case V4L2_CID_CAPTURE:
		DPRINTK_OV("V4L2_CID_CAPTURE:value = %d\n",vc->value);
		OV2659_capture_mode = 1;
		OV2659_exit_mode = 0;
		OV2659_capture2preview_mode = 0;
		break;
	case V4L2_CID_EFFECT:
		DPRINTK_OV("V4L2_CID_EFFECT:value = %d\n",vc->value);
		if(vc->value < 0 || vc->value > ARRAY_SIZE(OV2659_effects))
			vc->value = OV2659_DEF_EFFECT;
		extp->effect = vc->value;
		if(current_power_state == V4L2_POWER_ON){
			err = OV2659_write_regs(client, OV2659_effects[vc->value]);
		}
		break;
	case V4L2_CID_SCENE:
		DPRINTK_OV("V4L2_CID_EFFECT:value = %d\n",vc->value);
		break;
	case V4L2_CID_SHARPNESS:
		if(vc->value < 0 || vc->value > ARRAY_SIZE(OV2659_sharpness))
			vc->value = OV2659_DEF_SHARPNESS;
		extp->sharpness= vc->value;
		if(current_power_state == V4L2_POWER_ON){
			err = OV2659_write_regs(client, OV2659_sharpness[vc->value]);
		}
		break;
	case V4L2_CID_SATURATION:
		if(vc->value < 0 || vc->value > ARRAY_SIZE(OV2659_saturation))
			vc->value = OV2659_DEF_SATURATION;
		extp->saturation= vc->value;
		if(current_power_state == V4L2_POWER_ON){
			err = OV2659_write_regs(client, OV2659_saturation[vc->value]);
		}
		break;
	case V4L2_CID_EXPOSURE:
		DPRINTK_OV("V4L2_CID_EXPOSURE:value = %d\n",vc->value);
		if(vc->value < 0 || vc->value > ARRAY_SIZE(OV2659_exposure))
			vc->value = OV2659_DEF_EXPOSURE;
		extp->exposure= vc->value;
		/*Add the force refresh operation when setting the exposure lower than before.
		The code below judges whether the exposure value to be set is lower than before.
		If it's true, will force refresh the exposure to make sure the exposure can be set correctly.
		The resVal represent current exposure value, vc->value represent new exposure value
		*/
		uint resVal = 0;
		OV2659_read_reg(client, 1, 0x3a0f, &resVal);

		if((vc->value == 3 && resVal >= 0x38)
		   || (vc->value == 5 && resVal >= 0x48)
		   || (vc->value == 7 && resVal >= 0x60)){
		  err = OV2659_write_regs(client, OV2659_exposure[1]);
		  mdelay(200);
		}

		if(current_power_state == V4L2_POWER_ON){
			err = OV2659_write_regs(client, OV2659_exposure[vc->value]);
		}
		break;

	case V4L2_CID_CONTRAST:
		DPRINTK_OV("V4L2_CID_CONTRAST:value = %d\n",vc->value);
		if(vc->value < 0 || vc->value > ARRAY_SIZE(OV2659_contrast))
			vc->value = OV2659_DEF_CONTRAST;
		extp->contrast = vc->value;
		if(current_power_state == V4L2_POWER_ON){
			err = OV2659_write_regs(client, OV2659_contrast[vc->value]);
		}
		break;
	case V4L2_CID_BRIGHTNESS:
		DPRINTK_OV("V4L2_CID_BRIGHTNESS:value = %d\n",vc->value);
		if(vc->value < 0 || vc->value > ARRAY_SIZE(OV2659_brightness))
			vc->value = OV2659_DEF_BRIGHT;
		extp->brightness = vc->value;
		if(current_power_state == V4L2_POWER_ON){
			err = OV2659_write_regs(client, OV2659_brightness[vc->value]);
		}
		break;
	case V4L2_CID_DO_WHITE_BALANCE:
		DPRINTK_OV("V4L2_CID_DO_WHITE_BALANCE:value = %d\n",vc->value);
		if(vc->value < 0 || vc->value > ARRAY_SIZE(OV2659_whitebalance))
			vc->value = OV2659_DEF_WB;
		extp->white_balance = vc->value;
		if(current_power_state == V4L2_POWER_ON){
			err = OV2659_write_regs(client, OV2659_whitebalance[vc->value]);
		}
		break;
	case V4L2_CID_BACKTOPREVIEW:
		DPRINTK_OV("V4L2_CID_BACKTOPREVIEW:value = %d\n",vc->value);
		OV2659_capture2preview_mode = 1;
		OV2659_capture_mode = 0;
		OV2659_exit_mode = 0;
		break;
	case V4L2_CID_EXIT:
		DPRINTK_OV("V4L2_CID_BACKTOPREVIEW:value = %d\n",vc->value);
		OV2659_exit_mode = 1;
		OV2659_capture2preview_mode = 0;
		OV2659_capture_mode = 0;
		break;
	default:
		break;
	}
	DPRINTK_OV("%s return %d\n",__func__,err);
	return err;
}

/*
 * ioctl_enum_fmt_cap - Implement the CAPTURE buffer VIDIOC_ENUM_FMT ioctl
 * @s: pointer to standard V4L2 device structure
 * @fmt: standard V4L2 VIDIOC_ENUM_FMT ioctl structure
 *
 * Implement the VIDIOC_ENUM_FMT ioctl for the CAPTURE buffer type.
 */
 static int ioctl_enum_fmt_cap(struct v4l2_int_device *s,
				struct v4l2_fmtdesc *fmt)
{
	int index = fmt->index;
	enum v4l2_buf_type type = fmt->type;

	memset(fmt, 0, sizeof(*fmt));
	fmt->index = index;
	fmt->type = type;
	DPRINTK_OV("index = %d,type = %d\n",index,type);
	switch (fmt->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		if (index >= NUM_CAPTURE_FORMATS)
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}

	fmt->flags = OV2659_formats[index].flags;
	strlcpy(fmt->description, OV2659_formats[index].description,
					sizeof(fmt->description));
	fmt->pixelformat = OV2659_formats[index].pixelformat;

	return 0;
}

/*
 * ioctl_try_fmt_cap - Implement the CAPTURE buffer VIDIOC_TRY_FMT ioctl
 * @s: pointer to standard V4L2 device structure
 * @f: pointer to standard V4L2 VIDIOC_TRY_FMT ioctl structure
 *
 * Implement the VIDIOC_TRY_FMT ioctl for the CAPTURE buffer type.  This
 * ioctl is used to negotiate the image capture size and pixel format
 * without actually making it take effect.
 */

static int ioctl_try_fmt_cap(struct v4l2_int_device *s,
			 struct v4l2_format *f)
{
	int ifmt;
	enum image_size_ov isize = ARRAY_SIZE(OV2659_sizes) - 1;
	struct v4l2_pix_format *pix = &f->fmt.pix;

	DPRINTK_OV("v4l2_pix_format(w:%d,h:%d)\n",pix->width,pix->height);

	if (pix->width > OV2659_sizes[isize].width)
		pix->width = OV2659_sizes[isize].width;
	if (pix->height > OV2659_sizes[isize].height)
		pix->height = OV2659_sizes[isize].height;

	isize = OV2659_find_size(pix->width, pix->height);
	pix->width = OV2659_sizes[isize].width;
	pix->height = OV2659_sizes[isize].height;

	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (pix->pixelformat == OV2659_formats[ifmt].pixelformat)
			break;
	}
	if (ifmt == NUM_CAPTURE_FORMATS)
		ifmt = 0;
	pix->pixelformat = OV2659_formats[ifmt].pixelformat;
	pix->field = V4L2_FIELD_NONE;
	pix->bytesperline = pix->width*2;
	pix->sizeimage = pix->bytesperline*pix->height;
	pix->priv = 0;
	switch (pix->pixelformat) {
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_UYVY:
	default:
		pix->colorspace = V4L2_COLORSPACE_JPEG;
		break;
	case V4L2_PIX_FMT_SGRBG10:
	case V4L2_PIX_FMT_RGB565:
	case V4L2_PIX_FMT_RGB565X:
	case V4L2_PIX_FMT_RGB555:
	case V4L2_PIX_FMT_RGB555X:
		pix->colorspace = V4L2_COLORSPACE_SRGB;
		break;
	}
	return 0;
}

/*
 * ioctl_s_fmt_cap - V4L2 sensor interface handler for VIDIOC_S_FMT ioctl
 * @s: pointer to standard V4L2 device structure
 * @f: pointer to standard V4L2 VIDIOC_S_FMT ioctl structure
 *
 * If the requested format is supported, configures the HW to use that
 * format, returns error code if format not supported or HW can't be
 * correctly configured.
 */
 static int ioctl_s_fmt_cap(struct v4l2_int_device *s,
				struct v4l2_format *f)
{
	struct OV2659_sensor *sensor = s->priv;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	int rval;

	DPRINTK_OV("pix.pixformat = %x \n",pix->pixelformat);
	DPRINTK_OV("pix.width = %d \n",pix->width);
	DPRINTK_OV("pix.height = %d \n",pix->height);

	rval = ioctl_try_fmt_cap(s, f);
	if (rval)
		return rval;

	sensor->pix = *pix;
	sensor->width = sensor->pix.width;
	sensor->height = sensor->pix.height;
	DPRINTK_OV("sensor->pix.pixformat = %x \n",sensor->pix.pixelformat);
	DPRINTK_OV("sensor->pix.width = %d \n",sensor->pix.width);
	DPRINTK_OV("sensor->pix.height = %d \n",sensor->pix.height);

	return 0;
}

/*
 * ioctl_g_fmt_cap - V4L2 sensor interface handler for ioctl_g_fmt_cap
 * @s: pointer to standard V4L2 device structure
 * @f: pointer to standard V4L2 v4l2_format structure
 *
 * Returns the sensor's current pixel format in the v4l2_format
 * parameter.
 */
static int ioctl_g_fmt_cap(struct v4l2_int_device *s,
				struct v4l2_format *f)
{
	struct OV2659_sensor *sensor = s->priv;
	f->fmt.pix = sensor->pix;
	DPRINTK_OV("f->fmt.pix.pixformat = %x,V4L2_PIX_FMT_YUYV = %x\n",f->fmt.pix.pixelformat,V4L2_PIX_FMT_YUYV);
	DPRINTK_OV("f->fmt.pix.width = %d \n",f->fmt.pix.width);
	DPRINTK_OV("f->fmt.pix.height = %d \n",f->fmt.pix.height);

	return 0;
}

/*
 * ioctl_g_parm - V4L2 sensor interface handler for VIDIOC_G_PARM ioctl
 * @s: pointer to standard V4L2 device structure
 * @a: pointer to standard V4L2 VIDIOC_G_PARM ioctl structure
 *
 * Returns the sensor's video CAPTURE parameters.
 */
static int ioctl_g_parm(struct v4l2_int_device *s,
			 struct v4l2_streamparm *a)
{
	struct OV2659_sensor *sensor = s->priv;
	struct v4l2_captureparm *cparm = &a->parm.capture;

	DPRINTK_OV("\n");

	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(a, 0, sizeof(*a));
	a->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	cparm->capability = V4L2_CAP_TIMEPERFRAME;
	cparm->timeperframe = sensor->timeperframe;

	return 0;
}

/*
 * ioctl_s_parm - V4L2 sensor interface handler for VIDIOC_S_PARM ioctl
 * @s: pointer to standard V4L2 device structure
 * @a: pointer to standard V4L2 VIDIOC_S_PARM ioctl structure
 *
 * Configures the sensor to use the input parameters, if possible.  If
 * not possible, reverts to the old parameters and returns the
 * appropriate error code.
 */
static int ioctl_s_parm(struct v4l2_int_device *s,
			 struct v4l2_streamparm *a)
{
	int rval = 0;
	struct OV2659_sensor *sensor = s->priv;
	struct v4l2_fract *timeperframe = &a->parm.capture.timeperframe;
	struct v4l2_fract timeperframe_old;
	int desired_fps;

	timeperframe_old = sensor->timeperframe;
	sensor->timeperframe = *timeperframe;

	desired_fps = timeperframe->denominator / timeperframe->numerator;
	if ((desired_fps < OV2659_MIN_FPS) || (desired_fps > OV2659_MAX_FPS))
		rval = -EINVAL;

	if (rval)
		sensor->timeperframe = timeperframe_old;
	else
		*timeperframe = sensor->timeperframe;

	DPRINTK_OV("frame rate = %d\n",desired_fps);

	return rval;
}

/*
 * ioctl_g_priv - V4L2 sensor interface handler for vidioc_int_g_priv_num
 * @s: pointer to standard V4L2 device structure
 * @p: void pointer to hold sensor's private data address
 *
 * Returns device's (sensor's) private data area address in p parameter
 */
static int ioctl_g_priv(struct v4l2_int_device *s, void *p)
{
	struct OV2659_sensor *sensor = s->priv;

	return sensor->pdata->priv_data_set(s,p);
}

static int __OV2659_power_off_standby(struct v4l2_int_device *s,
					enum v4l2_power on)
{
	struct OV2659_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int rval;

	sensor->pdata->set_xclk(s, 0);
	rval = sensor->pdata->power_set(s, on);
	if (rval < 0) {
		v4l_err(client, "Unable to set the power state: "
			OV2659_DRIVER_NAME " sensor\n");
		return rval;
	}

	return 0;
}

static int OV2659_power_off(struct v4l2_int_device *s)
{
	return __OV2659_power_off_standby(s, V4L2_POWER_OFF);
}

static int OV2659_power_standby(struct v4l2_int_device *s)
{
	return __OV2659_power_off_standby(s, V4L2_POWER_STANDBY);
}

static int OV2659_power_on(struct v4l2_int_device *s)
{
	struct OV2659_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int rval;

	sensor->pdata->set_xclk(s, xclk_current);

	rval = sensor->pdata->power_set(s, V4L2_POWER_ON);
	if (rval < 0) {
		v4l_err(client, "Unable to set the power state: "
			OV2659_DRIVER_NAME " sensor\n");
		sensor->pdata->set_xclk(s, 0);
		return rval;
	}

	return 0;
}

/*
 * ioctl_s_power - V4L2 sensor interface handler for vidioc_int_s_power_num
 * @s: pointer to standard V4L2 device structure
 * @on: power state to which device is to be set
 *
 * Sets devices power state to requrested state, if possible.
 */
static int ioctl_s_power(struct v4l2_int_device *s, enum v4l2_power on)
{
	switch (on) {
		case V4L2_POWER_ON:
			if(!OV2659_capture_mode && !OV2659_capture2preview_mode)
				OV2659_power_on(s);
			/*kunlun p1 can't support standby*/
			OV2659_configure(s);
			break;
		case V4L2_POWER_OFF:
			OV2659_capture_mode = 0;
			OV2659_capture2preview_mode = 0;
			OV2659_power_off(s);
			break;
		case V4L2_POWER_STANDBY:
			if(!OV2659_capture_mode && !OV2659_capture2preview_mode)
			OV2659_power_standby(s);
			break;
	}
	current_power_state = on;

	return 0;
}

/*
 * ioctl_init - V4L2 sensor interface handler for VIDIOC_INT_INIT
 * @s: pointer to standard V4L2 device structure
 *
 * Initialize the sensor device (call OV2659_configure())
 */
static int ioctl_init(struct v4l2_int_device *s)
{
	DPRINTK_OV("\n");
	return 0;
}

/**
 * ioctl_dev_exit - V4L2 sensor interface handler for vidioc_int_dev_exit_num
 * @s: pointer to standard V4L2 device structure
 *
 * Delinitialise the dev. at slave detach.  The complement of ioctl_dev_init.
 */
static int ioctl_dev_exit(struct v4l2_int_device *s)
{
	DPRINTK_OV("\n");
	return 0;
}

/**
 * ioctl_dev_init - V4L2 sensor interface handler for vidioc_int_dev_init_num
 * @s: pointer to standard V4L2 device structure
 *
 * Initialise the device when slave attaches to the master.  Returns 0 if
 * OV2659 device could be found, otherwise returns appropriate error.
 */
static int ioctl_dev_init(struct v4l2_int_device *s)
{
	struct OV2659_sensor *sensor = s->priv;
	struct i2c_client *c = sensor->i2c_client;
	int err;

	err = OV2659_power_on(s);
	if (err)
	{
		printk("\n OV2659  Power On Failed  \n");
		return -ENODEV;
	}

	err = OV2659_detect(c);
	if (err < 0) {
		DPRINTK_OV("Unable to detect " OV2659_DRIVER_NAME
								" sensor\n");
	/*
	 * Turn power off before leaving this function
	 * If not, CAM powerdomain will on
	*/
	OV2659_power_off(s);
		return err;
	}

	sensor->ver = err;
	DPRINTK_OV(" chip version 0x%02x detected\n",
								sensor->ver);
	err = OV2659_power_off(s);
	if(err)
	{
		printk("\n !!!!!!!!!OV2659  Power Offf   Failed  \n");
		return -ENODEV;
	}

	return 0;
}

/**
 * ioctl_enum_framesizes - V4L2 sensor if handler for vidioc_int_enum_framesizes
 * @s: pointer to standard V4L2 device structure
 * @frms: pointer to standard V4L2 framesizes enumeration structure
 *
 * Returns possible framesizes depending on choosen pixel format
 **/
static int ioctl_enum_framesizes(struct v4l2_int_device *s,
					struct v4l2_frmsizeenum *frms)
{
	int ifmt;

	DPRINTK_OV("index=%d\n",frms->index);
	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (frms->pixel_format == OV2659_formats[ifmt].pixelformat)
			break;
	}
	/* Is requested pixelformat not found on sensor? */
	if (ifmt == NUM_CAPTURE_FORMATS)
		return -EINVAL;

	/* Do we already reached all discrete framesizes? */
	if (frms->index >= ARRAY_SIZE(OV2659_sizes))
		return -EINVAL;

	frms->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	frms->discrete.width = OV2659_sizes[frms->index].width;
	frms->discrete.height = OV2659_sizes[frms->index].height;

	DPRINTK_OV("discrete(%d,%d)\n",frms->discrete.width,frms->discrete.height);
	return 0;
}

/*QCIF,QVGA,CIF,VGA,SVGA can support up to 30fps*/
const struct v4l2_fract OV2659_frameintervals[] = {
	{ .numerator = 1, .denominator = 15 },//SQCIF,128*96
	{ .numerator = 1, .denominator = 15 },//QCIF,176*144
	{ .numerator = 1, .denominator = 15 },//QVGA,320*240
	{ .numerator = 1, .denominator = 15 },//CIF,352*288
	{ .numerator = 1, .denominator = 15 },//VGA,680*480
	{ .numerator = 1, .denominator = 15 },//DVD-video NTSC,720*480
	{ .numerator = 2, .denominator = 15 },//SVGA,800*600
	{ .numerator = 2, .denominator = 15 },//XGA,1024*768
	{ .numerator = 2, .denominator = 15 },//1M,1280*960
	{ .numerator = 2, .denominator = 15 },//2M,UXGA
};

static int ioctl_enum_frameintervals(struct v4l2_int_device *s,
					struct v4l2_frmivalenum *frmi)
{
	int ifmt;
	int isize = ARRAY_SIZE(OV2659_frameintervals) - 1;

	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (frmi->pixel_format == OV2659_formats[ifmt].pixelformat)
			break;
	}

	DPRINTK_OV("frmi->width = %d,frmi->height = %d,frmi->index = %d,ifmt = %d\n", \
		frmi->width,frmi->height,frmi->index,ifmt);

	/* Is requested pixelformat not found on sensor? */
	if (ifmt == NUM_CAPTURE_FORMATS)
		return -EINVAL;

	/* Do we already reached all discrete framesizes? */
	if (frmi->index > isize)
		return -EINVAL;

	frmi->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	frmi->discrete.numerator =
		OV2659_frameintervals[frmi->index].numerator;
	frmi->discrete.denominator =
		OV2659_frameintervals[frmi->index].denominator;

	return 0;
}

static unsigned long ov2659_reg;

static ssize_t ov2659_reg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%x", (u32)ov2659_reg);
}

static ssize_t ov2659_reg_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	strict_strtoul(buf, 16, &ov2659_reg);
	return size;
}


static ssize_t ov2659_val_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned int val;
	struct i2c_client * client;

	val = 0;
	client = container_of(dev, struct i2c_client, dev);
	OV2659_read_reg(client, 1, ov2659_reg, &val);
	return snprintf(buf, PAGE_SIZE, "%x", val);
}

static ssize_t ov2659_val_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned long val;

	if (strict_strtoul(buf, 16, &val) == 0)
	{
		struct i2c_client * client;

		client = container_of(dev, struct i2c_client, dev);
		OV2659_write_reg(client, ov2659_reg, (u8)val);
	}
	return size;
}

static DEVICE_ATTR(reg, S_IRUGO | S_IWUSR, ov2659_reg_show, ov2659_reg_store);
static DEVICE_ATTR(val, S_IRUGO | S_IWUSR, ov2659_val_show, ov2659_val_store);

static struct v4l2_int_ioctl_desc OV2659_ioctl_desc[] = {
	{vidioc_int_enum_framesizes_num,
		(v4l2_int_ioctl_func *)ioctl_enum_framesizes},
	{vidioc_int_enum_frameintervals_num,
		(v4l2_int_ioctl_func *)ioctl_enum_frameintervals},
	{vidioc_int_dev_init_num,
		(v4l2_int_ioctl_func *)ioctl_dev_init},
	{vidioc_int_dev_exit_num,
		(v4l2_int_ioctl_func *)ioctl_dev_exit},
	{vidioc_int_s_power_num,
		(v4l2_int_ioctl_func *)ioctl_s_power},
	{vidioc_int_g_priv_num,
		(v4l2_int_ioctl_func *)ioctl_g_priv},
	{vidioc_int_init_num,
		(v4l2_int_ioctl_func *)ioctl_init},
	{vidioc_int_enum_fmt_cap_num,
		(v4l2_int_ioctl_func *)ioctl_enum_fmt_cap},
	{vidioc_int_try_fmt_cap_num,
		(v4l2_int_ioctl_func *)ioctl_try_fmt_cap},
	{vidioc_int_g_fmt_cap_num,
		(v4l2_int_ioctl_func *)ioctl_g_fmt_cap},
	{vidioc_int_s_fmt_cap_num,
		(v4l2_int_ioctl_func *)ioctl_s_fmt_cap},
	{vidioc_int_g_parm_num,
		(v4l2_int_ioctl_func *)ioctl_g_parm},
	{vidioc_int_s_parm_num,
		(v4l2_int_ioctl_func *)ioctl_s_parm},
	{vidioc_int_queryctrl_num,
		(v4l2_int_ioctl_func *)ioctl_queryctrl},
	{vidioc_int_g_ctrl_num,
		(v4l2_int_ioctl_func *)ioctl_g_ctrl},
	{vidioc_int_s_ctrl_num,
		(v4l2_int_ioctl_func *)ioctl_s_ctrl},
	{ vidioc_int_g_crop_num,
		(v4l2_int_ioctl_func *)ioctl_g_crop},
	{vidioc_int_s_crop_num,
		(v4l2_int_ioctl_func *)ioctl_s_crop},
	{ vidioc_int_cropcap_num,
		(v4l2_int_ioctl_func *)ioctl_cropcap},
};

static struct v4l2_int_slave OV2659_slave = {
	.ioctls		= OV2659_ioctl_desc,
	.num_ioctls	= ARRAY_SIZE(OV2659_ioctl_desc),
};

static struct v4l2_int_device OV2659_int_device = {
	.module	= THIS_MODULE,
	.name	= OV2659_DRIVER_NAME,
	.priv	= &OV2659,
	.type	= v4l2_int_type_slave,
	.u	= {
		.slave = &OV2659_slave,
	},
};

/*
 * OV2659_probe - sensor driver i2c probe handler
 * @client: i2c driver client device structure
 *
 * Register sensor as an i2c client device and V4L2
 * device.
 */
static int __init
OV2659_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct OV2659_sensor *sensor = &OV2659;
	int err;

	DPRINTK_OV("entering \n");

	if (i2c_get_clientdata(client))
		return -EBUSY;

	sensor->pdata = client->dev.platform_data;

	if (!sensor->pdata) {
		dev_err(&client->dev, "No platform data?\n");
		return -ENODEV;
	}

	sensor->v4l2_int_device = &OV2659_int_device;
	sensor->i2c_client = client;

	i2c_set_clientdata(client, sensor);

#if 1
	/* Make the default preview format 320*240 YUV */
	sensor->pix.width = OV2659_sizes[QVGA].width;
	sensor->pix.height = OV2659_sizes[QVGA].height;
#else
	sensor->pix.width = OV2659_sizes[UXGA].width;
	sensor->pix.height = OV2659_sizes[UXGA].height;
#endif

	sensor->pix.pixelformat = V4L2_PIX_FMT_YUYV;

	err = v4l2_int_device_register(sensor->v4l2_int_device);
	if (err)
		i2c_set_clientdata(client, NULL);

	if (device_create_file(&client->dev, &dev_attr_reg))
		printk("failed to create sysfs file reg\n");
	if (device_create_file(&client->dev, &dev_attr_val))
		printk("failed to create sysfs file val\n");

	DPRINTK_OV("exit \n");
	return 0;
}

/*
 * OV2659_remove - sensor driver i2c remove handler
 * @client: i2c driver client device structure
 *
 * Unregister sensor as an i2c client device and V4L2
 * device. Complement of OV2659_probe().
 */
static int __exit
OV2659_remove(struct i2c_client *client)
{
	struct OV2659_sensor *sensor = i2c_get_clientdata(client);

	if (!client->adapter)
		return -ENODEV;	/* our client isn't attached */

	v4l2_int_device_unregister(sensor->v4l2_int_device);
	i2c_set_clientdata(client, NULL);

	return 0;
}

static const struct i2c_device_id OV2659_id[] = {
	{ OV2659_DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, OV2659_id);

static struct i2c_driver OV2659sensor_i2c_driver = {
	.driver = {
		.name	= OV2659_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
	.probe	= OV2659_probe,
	.remove	= __exit_p(OV2659_remove),
	.id_table = OV2659_id,
};

static struct OV2659_sensor OV2659 = {
	.timeperframe = {
		.numerator = 1,
		.denominator = 15,
	},
	.ext_params =
	{
		.effect=SENSOR_EFFECT_NORMAL,
		.white_balance=SENSOR_WHITE_BALANCE_AUTO,
		.scene_mode=SENSOR_SCENE_MODE_AUTO,
		.brightness=OV2659_DEF_BRIGHT,
		.contrast=OV2659_DEF_CONTRAST,
		.saturation=OV2659_DEF_SATURATION,
		.exposure=OV2659_DEF_EXPOSURE,
		.sharpness=OV2659_DEF_SHARPNESS,
	},
	.state = SENSOR_NOT_DETECTED,
};

/*
 * OV2659sensor_init - sensor driver module_init handler
 *
 * Registers driver as an i2c client driver.  Returns 0 on success,
 * error code otherwise.
 */
static int __init OV2659sensor_init(void)
{
	int err;

	DPRINTK_OV("\n");
#if 1 /* defined(CONFIG_CAM_GPIO_I2C)*/
	gpio_request(OV2659_SCL_GPIO,"ov2659_i2c_scl");
	gpio_request(OV2659_SDA_GPIO,"ov2659_i2c_sda");
	gpio_request(110,"ov2659_pwdn");
	gpio_request(98,"ov2659_rst");
#endif
	err = i2c_add_driver(&OV2659sensor_i2c_driver);
	if (err) {
		printk(KERN_ERR "Failed to register" OV2659_DRIVER_NAME ".\n");
		return  err;
	}

	return 0;
}
late_initcall(OV2659sensor_init);

/*
 * OV2659sensor_cleanup - sensor driver module_exit handler
 *
 * Unregisters/deletes driver as an i2c client driver.
 * Complement of OV2659sensor_init.
 */
static void __exit OV2659sensor_cleanup(void)
{
	i2c_del_driver(&OV2659sensor_i2c_driver);
}
module_exit(OV2659sensor_cleanup);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("OV2659 camera sensor driver");
