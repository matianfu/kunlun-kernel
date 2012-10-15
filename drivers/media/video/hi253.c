/*
 * drivers/media/video/Hi253.c
 *
 * Hi253 sensor driver
 *
 * Leveraged code from the ov2655.c
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 * modified :daqing
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <media/v4l2-int-device.h>
#include <linux/i2c/twl.h>
#include <mach/gpio.h>
#include <media/hi253.h>
#include "omap34xxcam.h"
#include "isp/isp.h"

#define I2C_M_WR 0
#define __YUV_MODE 1

//#define __DEBUG 1
//#undef __DEBUG

#ifdef __DEBUG
#define DPRINTK_OV(format, ...)				\
	printk(KERN_ERR " Hi253:%s: " format " ",__func__, ## __VA_ARGS__)
#else
#define DPRINTK_OV(format, ...)
#endif

/* Array of image sizes supported by OV2655.  These must be ordered from
 * smallest image size to largest.
 */
/*<"pref_camera_videoquality_entry_0">SQCIF 128x96*/
/*<"pref_camera_videoquality_entry_1">QCIF 176x144*/
/*<"pref_camera_videoquality_entry_2">CIF 352x288*/
/*<"pref_camera_videoquality_entry_3">QVGA 320x240*/
/*<"pref_camera_videoquality_entry_4">VGA 640x480*/
/*<"pref_camera_videoquality_entry_5">TV NTSC 704x480*/
/*<"pref_camera_videoquality_entry_6">TV PAL 704x576*/
/*<"pref_camera_videoquality_entry_7">DVD-Video NTSC 720x480*/
/*<"pref_camera_videoquality_entry_8">DVD-Video PAL 720x576*/
/*<"pref_camera_videoquality_entry_9">WVGA 800x480*/
/*<"pref_camera_videoquality_entry_10">1280x720*/

const static struct capture_size_hi253 Hi253_sizes[] = {
	/*QSVGA*/
	{320,240},
	/*SVGA*/
	{640,480},
	/*UXGA*/
	{1600,1200},
};

const static struct Hi253_reg Hi253_effect_off[]={//CAM_EFFECT_ENC_NORMAL
	{0x03, 0x10},
	{0x11, 0x03},
	{0x12, 0x30},
	{0x13, 0x02},
	{0x14, 0x00},
	{0x44, 0x80},
	{0x45, 0x80},
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_effect_mono[]={//CAM_EFFECT_ENC_GRAYSCALE
	{0x03, 0x10},
	{0x11, 0x03},
	{0x12, 0x33},
	{0x13, 0x02},
	{0x14, 0x00},
	{0x44, 0x80},
	{0x45, 0x80},
	{0xFF,0xFF},
};
const static struct Hi253_reg Hi253_effect_negative[]={//CAM_EFFECT_ENC_COLORINV
	{0x03, 0x10},
	{0x11, 0x03},
	{0x12, 0x08},
	{0x13, 0x02},
	{0x14, 0x00},
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_effect_sepia[]={//CAM_EFFECT_ENC_SEPIA
	{0x03, 0x10},
	{0x11, 0x03},
	{0x12, 0x33},
	{0x13, 0x02},
	{0x14, 0x00},
	{0x44, 0x70},
	{0x45, 0x98},
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_effect_bluish[]={//CAM_EFFECT_ENC_SEPIABLUE
	{0x03, 0x10},
	{0x11, 0x03},
	{0x12, 0x33},
	{0x13, 0x02},
	{0x14, 0x00},
	{0x44, 0xb0},
	{0x45, 0x40},
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_effect_green[]={//CAM_EFFECT_ENC_SEPIAGREEN
	{0x03, 0x10},
	{0x11, 0x03},
	{0x12, 0x33},
	{0x13, 0x02},
	{0x14, 0x00},
	{0x44, 0x30},
	{0x45, 0x50},
	{0xFF,0xFF},
};
const static struct Hi253_reg Hi253_effect_reddish[]={
	{0x03, 0x10},
	{0x11, 0x03},
	{0x12, 0x33},
	{0x13, 0x02},
	{0x44, 0x70},
	{0x45, 0xf8},
	{0xFF,0xFF},
};
const static struct Hi253_reg Hi253_effect_yellowish[]={
	{0x03, 0x10},
	{0x11, 0x03},
	{0x12, 0x30},
	{0x13, 0x02},
	{0x44, 0x80},
	{0x45, 0x80},
	{0xFF,0xFF},
};

const static struct Hi253_reg* Hi253_effects[]={
	Hi253_effect_off,
	Hi253_effect_mono,
	Hi253_effect_negative,
	Hi253_effect_sepia,
	Hi253_effect_bluish,
	Hi253_effect_green,
	Hi253_effect_reddish,
	Hi253_effect_yellowish
};

const static struct Hi253_reg Hi253_wb_auto[]={ //CAM_WB_AUTO:
	{0x03, 0x22},
	{0x11, 0x2e},
	//{0x80, 0x40},
	//{0x82, 0x3e},
	{0x83, 0x5e},
	{0x84, 0x1e},
	{0x85, 0x5e},
	{0x86, 0x22},
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_wb_sunny[]={//CAM_WB_DAYLIGHT
	{0x03, 0x22},
	{0x11, 0x28},
	{0x80, 0x59},
	{0x82, 0x29},
	{0x83, 0x60},
	{0x84, 0x50},
	{0x85, 0x2f},
	{0x86, 0x23},
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_wb_cloudy[]={//CAM_WB_CLOUD:
	{0x03, 0x22},
	{0x11, 0x28},
	{0x80, 0x71},
	{0x82, 0x2b},
	{0x83, 0x72},
	{0x84, 0x70},
	{0x85, 0x2b},
	{0x86, 0x28},
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_wb_office[]={//FLUORESCENT
	{0x03, 0x22},
	{0x11, 0x28},
	{0x80, 0x41},
	{0x82, 0x42},
	{0x83, 0x44},
	{0x84, 0x34},
	{0x85, 0x46},
	{0x86, 0x3a},
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_wb_home[]={//INCANDESCENCE
	{0x03, 0x22},
	{0x11, 0x28},
	{0x80, 0x29},
	{0x82, 0x54},
	{0x83, 0x2e},
	{0x84, 0x23},
	{0x85, 0x58},
	{0x86, 0x4f},
	{0xFF,0xFF}
};

const static struct Hi253_reg* Hi253_whitebalance[]={
	Hi253_wb_auto,
	Hi253_wb_sunny,
	Hi253_wb_cloudy,
	Hi253_wb_office,
	Hi253_wb_home
};

const static struct Hi253_reg Hi253_brightness_1[]={
	{0x03,0x10},
	{0x40,0xa0},//-2
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_brightness_2[]={
	{0x03,0x10},
	{0x40,0x90},//-1
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_brightness_3[]={
	{0x03,0x10},
	{0x40,0x80},//-0
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_brightness_4[]={
	{0x03,0x10},
	{0x40,0x10},//+1
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_brightness_5[]={
	{0x03,0x10},
	{0x40,0x20},//+2
	{0xFF,0xFF}
};

const static struct Hi253_reg* Hi253_brightness[]={
	Hi253_brightness_1,
	Hi253_brightness_2,
	Hi253_brightness_3,
	Hi253_brightness_4,
	Hi253_brightness_5
};

const static struct Hi253_reg Hi253_contrast_1[]={
	{0x03,0x10},//Page10
	{0x13,0x0a},
	{0x48,0x48},
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_contrast_2[]={
	{0x03,0x10},//Page10
	{0x13,0x0a},
	{0x48,0x68},
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_contrast_3[]={
	{0x03,0x10},//Page10
	{0x13,0x0a},
	{0x48,0x88},
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_contrast_4[]={
	{0x03,0x10},//Page10
	{0x13,0x0a},
	{0x48,0xa8},
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_contrast_5[]={
	{0x03,0x10},//Page10
	{0x13,0x0a},
	{0x48,0xc8},
	{0xFF,0xFF}
};

const static struct Hi253_reg* Hi253_contrast[]={
	Hi253_contrast_1,
	Hi253_contrast_2,
	Hi253_contrast_3,
	Hi253_contrast_4,
	Hi253_contrast_5
};

const static struct Hi253_reg Hi253_saturation_1[]={
	{0x03,0x10},//Page10
	{0x61, 0x3f}, //7e //8e //88 //80
	{0x62, 0x3c}, //7e //8e //88 //80
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_saturation_2[]={
	{0x03,0x10},//Page10
	{0x61, 0x5f}, //7e //8e //88 //80
	{0x62, 0x5c}, //7e //8e //88 //80
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_saturation_3[]={
	{0x03,0x10},//Page10
	{0x61, 0x7f}, //7e //8e //88 //80
	{0x62, 0x7c}, //7e //8e //88 //80
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_saturation_4[]={
	{0x03,0x10},//Page10
	{0x61, 0x9f}, //7e //8e //88 //80
	{0x62, 0x9c}, //7e //8e //88 //80
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_saturation_5[]={
	{0x03,0x10},//Page10
	{0x61, 0xbf}, //7e //8e //88 //80
	{0x62, 0xbc}, //7e //8e //88 //80
	{0xFF,0xFF}
};

/*
const static struct Hi253_reg* Hi253_saturation[]={
	Hi253_saturation_1,
	Hi253_saturation_2,
	Hi253_saturation_3,
	Hi253_saturation_4,
	Hi253_saturation_5
};
*/

const static struct Hi253_reg Hi253_exposure_1[]={
	{0x03, 0x10},
	{0x40, 0xa0},		//-2
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_exposure_2[]={
	{0x03, 0x10},
	{0x40, 0x90},		//-1
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_exposure_3[]={
	{0x03, 0x10},
	{0x40, 0x80},
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_exposure_4[]={
	{0x03, 0x10},
	{0x40, 0x10},		//+1
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_exposure_5[]={
	{0x03, 0x10},
	{0x40, 0x20},		//+2
	{0xFF,0xFF}
};

const static struct Hi253_reg* Hi253_exposure[]={
	Hi253_exposure_1,
	Hi253_exposure_2,
	Hi253_exposure_3,
	Hi253_exposure_4,
	Hi253_exposure_5
};

const static struct Hi253_reg Hi253_sharpness_auto[]={
	 {0xFF,0xFF}
};
const static struct Hi253_reg Hi253_sharpness_1[]={
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_sharpness_2[]={
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_sharpness_3[]={
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_sharpness_4[]={
	{0xFF,0xFF}
};
const static struct Hi253_reg Hi253_sharpness_5[]={
	{0xFF,0xFF}
};

/*
const static struct Hi253_reg* Hi253_sharpness[]={
	Hi253_sharpness_auto,
	Hi253_sharpness_1,
	Hi253_sharpness_2,
	Hi253_sharpness_3,
	Hi253_sharpness_4,
	Hi253_sharpness_5
};
*/

//Reseverd for night mode
#if 0
const static struct Hi253_reg Hi253_night_mode[] = {
	{0x03, 0x20},
	{0x10, 0x1c},

	{0x88, 0x09}, //EXP Max 5 fps
	{0x89, 0x24},
	{0x8a, 0x00},

	{0x03, 0x20},//page 20
	{0x10, 0x9c},//AE ON

	{0xff, 0xff}
};

const static struct Hi253_reg Hi253_night_mode_exit[] = {
	{0x03, 0x20},
	{0x10, 0x1c},

	{0x88, 0x04}, //EXP Max 10 fps
	{0x89, 0x92},
	{0x8a, 0x00},

	{0x03, 0x20},//page 20
	{0x10, 0x9c},//AE ON

	{0xff, 0xff}
};
#endif

const static struct Hi253_reg Hi253_uxga_capture[] = {
	{0x03, 0x00},//Page 00
	{0x10, 0x00},//Fullsize
	{0x11, 0x90},//Close fixed function

	/////////////////********Scaler Function Start********/////////////////
	{0x03, 0x18},
	{0x10, 0x00},//Disable Scaler Function
	/////////////////********Scaler Function End********/////////////////

	//////////////********Fix Frame Rate Function Start********///////
	//{0x03, 0x20}, //Page 20
	//{0x83, 0x08}, //EXP Normal 7.69 fps
	//{0x84, 0xe9},
	//{0x85, 0x80},
	//{0x86, 0x01}, //EXPMin 9615.38 fps
	//{0x87, 0xd4},
	//{0x88, 0x08}, //EXP Max 7.69 fps
	//{0x89, 0xe9},
	//{0x8a, 0x80},
	//{0x8B, 0xaf}, //EXP100
	//{0x8C, 0x80},
	//{0x8D, 0x92}, //EXP120
	//{0x8E, 0x40},
	//{0x91, 0x09}, //EXP Fix 7.50 fps
	//{0x92, 0x27},
	//{0x93, 0xa8},
	//{0x9c, 0x0c}, //EXP Limit 1373.63 fps
	//{0x9d, 0xcc},
	//{0x9e, 0x01}, //EXP Unit
	//{0x9f, 0xd4},

	{0xff, 0xff}
};

const static struct Hi253_reg Hi253_svga_to_qvga[] = {

	//{0x01, 0xf1},

	{0x03, 0x20},
	{0x10, 0x1c},

	{0x03, 0x00},//Page 00
	{0x10, 0x11},//Pre1+Sub1
	{0x11, 0x90},//Close fixed function

	/////////////////********Scaler Function Start********/////////////////
	{0x03, 0x18},//Page 18
	{0x12, 0x20},
	{0x10, 0x07},
	{0x11, 0x00},
	{0x20, 0x02},
	{0x21, 0x80},
	{0x22, 0x00},
	{0x23, 0xf0},
	{0x24, 0x00},
	{0x25, 0x06},
	{0x26, 0x00},
	{0x27, 0x00},
	{0x28, 0x02},
	{0x29, 0x86},
	{0x2a, 0x00},
	{0x2b, 0xf0},
	{0x2c, 0x14},
	{0x2d, 0x00},
	{0x2e, 0x14},
	{0x2f, 0x00},
	{0x30, 0x65},
	/////////////////********Scaler Function End********/////////////////

	///////////********Variable Frame Rate Function Start********////////
	{0x03, 0x20}, //Page 20

	{0x18, 0x38},

	// {0x83, 0x01}, //EXP Normal 33.33 fps
	// {0x84, 0x5f},
	// {0x85, 0x00},

	{0x86, 0x02}, //EXPMin 5859.38 fps
	{0x87, 0x00},

	{0x88, 0x04}, //EXP Max 10 fps
	{0x89, 0x92},
	{0x8a, 0x00},

	//{0x88, 0x09}, //EXP Max 5 fps
	//{0x89, 0x24},
	//{0x8a, 0x00},

	{0x8B, 0x75}, //EXP100
	{0x8C, 0x00},
	{0x8D, 0x61}, //EXP120
	{0x8E, 0x00},

	{0x9c, 0x0e}, //EXP Limit 837.05 fps
	{0x9d, 0x00},
	{0x9e, 0x02}, //EXP Unit
	{0x9f, 0x00},
	//////////////********Fix Frame Rate Function End********/////////

	{0x03, 0x20},//page 20
	{0x10, 0x9c},//AE ON

	//{0x01, 0xf0},

	{0x03, 0x20},//page 20
	{0x18, 0x30},

	{0xff, 0xff}
};

const static struct Hi253_reg Hi253_svga_to_vga[] = {

	//{0x01, 0xf1},

	{0x03, 0x20},
	{0x10, 0x1c},

	{0x03, 0x00},//Page 00
	{0x10, 0x11},//Pre1+Sub1
	{0x11, 0x90},//Close fixed function

	/////////////////********Scaler Function Start********/////////////////
	{0x03, 0x18},//Page 18
	{0x12, 0x20},
	{0x10, 0x07},
	{0x11, 0x00},
	{0x20, 0x05},
	{0x21, 0x00},
	{0x22, 0x01},
	{0x23, 0xe0},
	{0x24, 0x00},
	{0x25, 0x06},
	{0x26, 0x00},
	{0x27, 0x00},
	{0x28, 0x05},
	{0x29, 0x06},
	{0x2a, 0x01},
	{0x2b, 0xe0},
	{0x2c, 0x0a},
	{0x2d, 0x00},
	{0x2e, 0x0a},
	{0x2f, 0x00},
	{0x30, 0x45},

	/////////////////********Scaler Function End********/////////////////

	/////////********Variable Frame Rate Function Start********////////
	{0x03, 0x20}, //Page 20

	{0x18, 0x38},

	// {0x83, 0x01}, //EXP Normal 33.33 fps
	// {0x84, 0x5f},
	// {0x85, 0x00},

	{0x86, 0x02}, //EXPMin 5859.38 fps
	{0x87, 0x00},

	{0x88, 0x04}, //EXP Max 10 fps
	{0x89, 0x92},
	{0x8a, 0x00},

	{0x8B, 0x75}, //EXP100
	{0x8C, 0x00},
	{0x8D, 0x61}, //EXP120
	{0x8E, 0x00},

	{0x9c, 0x0e}, //EXP Limit 837.05 fps
	{0x9d, 0x00},
	{0x9e, 0x02}, //EXP Unit
	{0x9f, 0x00},
	///////////********Fix Frame Rate Function End********////////

	{0x03, 0x20},//page 20
	{0x10, 0x9c},//AE ON

	//{0x01, 0xf0},

	{0x03, 0x20},//page 20
	{0x18, 0x30},

	{0xff, 0xff}
};

const static struct Hi253_reg Hi253_svga_to_svga[] = {

	{0xff, 0xff}
};

/*
const static struct Hi253_reg* Hi253_svga_to_xxx[] ={
	Hi253_svga_to_qvga,
	Hi253_svga_to_vga,
	Hi253_svga_to_svga
};
*/

const static struct Hi253_reg Hi253_common_svgabase[] = {
	/////// Start Sleep ///////
	{0x01, 0xf9}, //sleep on
	{0x08, 0x0f}, //Hi-Z on
	{0x01, 0xf8}, //sleep off

	{0x03, 0x00}, // Dummy 750us START
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00}, // Dummy 750us END

	{0x0e, 0x03}, //PLL On
	{0x0e, 0x73}, //PLLx2

	{0x03, 0x00}, // Dummy 750us START
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00}, // Dummy 750us END

	{0x0e, 0x00}, //PLL off
	{0x01, 0xf1}, //sleep on
	{0x08, 0x00}, //Hi-Z off

	{0x01, 0xf3},
	{0x01, 0xf1},

	// PAGE 20
	{0x03, 0x20}, //page 20
	{0x10, 0x1c}, //ae off

	// PAGE 22
	{0x03, 0x22}, //page 22
	{0x10, 0x69}, //awb off

	//Initial Start
	/////// PAGE 0 START ///////
	{0x03, 0x00},
	{0x10, 0x11}, // Sub1/2_Preview2 Mode_H binning
	{0x11, 0x90},
	{0x12, 0x04},

	{0x0b, 0xaa}, // ESD Check Register
	{0x0c, 0xaa}, // ESD Check Register
	{0x0d, 0xaa}, // ESD Check Register

	{0x20, 0x00}, // Windowing start point Y
	{0x21, 0x04},
	{0x22, 0x00}, // Windowing start point X
	{0x23, 0x07},

	{0x24, 0x04},
	{0x25, 0xb0},
	{0x26, 0x06},
	{0x27, 0x40}, // WINROW END

	{0x40, 0x01}, //Hblank 408
	{0x41, 0x98},
	{0x42, 0x00}, //Vblank 20
	{0x43, 0x14},

	{0x45, 0x04},
	{0x46, 0x18},
	{0x47, 0xd8},

	//BLC
	{0x80, 0x2e},
	{0x81, 0x7e},
	{0x82, 0x90},
	{0x83, 0x00},
	{0x84, 0x0c},
	{0x85, 0x00},
	{0x90, 0x0a}, //BLC_TIME_TH_ON
	{0x91, 0x0a}, //BLC_TIME_TH_OFF
	{0x92, 0xd8}, //BLC_AG_TH_ON
	{0x93, 0xd0}, //BLC_AG_TH_OFF
	{0x94, 0x75},
	{0x95, 0x70},
	{0x96, 0xdc},
	{0x97, 0xfe},
	{0x98, 0x38},

	//OutDoor  BLC
	{0x99, 0x43},
	{0x9a, 0x43},
	{0x9b, 0x43},
	{0x9c, 0x43},

	//Dark BLC
	{0xa0, 0x00},
	{0xa2, 0x00},
	{0xa4, 0x00},
	{0xa6, 0x00},

	//Normal BLC
	{0xa8, 0x43},
	{0xaa, 0x43},
	{0xac, 0x43},
	{0xae, 0x43},

	/////// PAGE 2 START ///////
	{0x03, 0x02},
	{0x12, 0x03},
	{0x13, 0x03},
	{0x16, 0x00},
	{0x17, 0x8C},
	{0x18, 0x4c}, //Double_AG off
	{0x19, 0x00},
	{0x1a, 0x39}, //ADC400->560
	{0x1c, 0x09},
	{0x1d, 0x40},
	{0x1e, 0x30},
	{0x1f, 0x10},

	{0x20, 0x77},
	{0x21, 0xde},
	{0x22, 0xa7},
	{0x23, 0x30}, //CLAMP
	{0x27, 0x3c},
	{0x2b, 0x80},
	{0x2e, 0x11},
	{0x2f, 0xa1},
	{0x30, 0x05}, //For Hi-253 never no change 0x05

	{0x50, 0x20},
	{0x52, 0x01},
	{0x53, 0xc1}, //
	{0x55, 0x1c},
	{0x56, 0x11},
	{0x5d, 0xa2},
	{0x5e, 0x5a},

	{0x60, 0x87},
	{0x61, 0x99},
	{0x62, 0x88},
	{0x63, 0x97},
	{0x64, 0x88},
	{0x65, 0x97},

	{0x67, 0x0c},
	{0x68, 0x0c},
	{0x69, 0x0c},

	{0x72, 0x89},
	{0x73, 0x96},
	{0x74, 0x89},
	{0x75, 0x96},
	{0x76, 0x89},
	{0x77, 0x96},

	{0x7c, 0x85},
	{0x7d, 0xaf},
	{0x80, 0x01},
	{0x81, 0x7f},
	{0x82, 0x13},

	{0x83, 0x24},
	{0x84, 0x7d},
	{0x85, 0x81},
	{0x86, 0x7d},
	{0x87, 0x81},

	{0x92, 0x48},
	{0x93, 0x54},
	{0x94, 0x7d},
	{0x95, 0x81},
	{0x96, 0x7d},
	{0x97, 0x81},

	{0xa0, 0x02},
	{0xa1, 0x7b},
	{0xa2, 0x02},
	{0xa3, 0x7b},
	{0xa4, 0x7b},
	{0xa5, 0x02},
	{0xa6, 0x7b},
	{0xa7, 0x02},

	{0xa8, 0x85},
	{0xa9, 0x8c},
	{0xaa, 0x85},
	{0xab, 0x8c},
	{0xac, 0x10},
	{0xad, 0x16},
	{0xae, 0x10},
	{0xaf, 0x16},

	{0xb0, 0x99},
	{0xb1, 0xa3},
	{0xb2, 0xa4},
	{0xb3, 0xae},
	{0xb4, 0x9b},
	{0xb5, 0xa2},
	{0xb6, 0xa6},
	{0xb7, 0xac},
	{0xb8, 0x9b},
	{0xb9, 0x9f},
	{0xba, 0xa6},
	{0xbb, 0xaa},
	{0xbc, 0x9b},
	{0xbd, 0x9f},
	{0xbe, 0xa6},
	{0xbf, 0xaa},

	{0xc4, 0x2c},
	{0xc5, 0x43},
	{0xc6, 0x63},
	{0xc7, 0x79},

	{0xc8, 0x2d},
	{0xc9, 0x42},
	{0xca, 0x2d},
	{0xcb, 0x42},
	{0xcc, 0x64},
	{0xcd, 0x78},
	{0xce, 0x64},
	{0xcf, 0x78},

	{0xd0, 0x0a},
	{0xd1, 0x09},
	{0xd4, 0x0a}, //DCDC_TIME_TH_ON
	{0xd5, 0x0a}, //DCDC_TIME_TH_OFF
	{0xd6, 0x98}, //DCDC_AG_TH_ON
	{0xd7, 0x90}, //DCDC_AG_TH_OFF
	{0xe0, 0xc4},
	{0xe1, 0xc4},
	{0xe2, 0xc4},
	{0xe3, 0xc4},
	{0xe4, 0x00},
	{0xe8, 0x80},
	{0xe9, 0x40},
	{0xea, 0x7f},

	{0xf0, 0x01},  //   sram1_cfg
	{0xf1, 0x01},  //   sram2_cfg
	{0xf2, 0x01},  //   sram3_cfg
	{0xf3, 0x01},  //   sram4_cfg
	{0xf4, 0x01},  //   sram5_cfg

	/////// PAGE 3 ///////
	{0x03, 0x03},
	{0x10, 0x10},

	/////// PAGE 10 START ///////
	{0x03, 0x10},
	{0x10, 0x03}, // CrYCbY // For Demoset 0x03
	{0x12, 0x30},
	{0x13, 0x0a}, // contrast on
	{0x20, 0x00},

	{0x30, 0x00},
	{0x31, 0x00},
	{0x32, 0x00},
	{0x33, 0x00},

	{0x34, 0x30},
	{0x35, 0x00},
	{0x36, 0x00},
	{0x38, 0x00},
	{0x3e, 0x58},
	{0x3f, 0x00},

	{0x40, 0x80}, // YOFS
	{0x41, 0x00}, // DYOFS
	{0x48, 0x80}, // Contrast

	{0x60, 0x67},
	{0x61, 0x7c}, //7e //8e //88 //80
	{0x62, 0x7c}, //7e //8e //88 //80
	{0x63, 0x50}, //Double_AG 50->30
	{0x64, 0x41},

	{0x66, 0x42},
	{0x67, 0x20},

	{0x6a, 0x70}, //8a
	{0x6b, 0x84}, //74
	{0x6c, 0x80}, //7e //7a
	{0x6d, 0x80}, //8e

	//Don't touch//////////////////////////
	//{0x72, 0x84},
	//{0x76, 0x19},
	//{0x73, 0x70},
	//{0x74, 0x68},
	//{0x75, 0x60}, // white protection ON
	//{0x77, 0x0e}, //08 //0a
	//{0x78, 0x2a}, //20
	//{0x79, 0x08},
	////////////////////////////////////////

	/////// PAGE 11 START ///////
	{0x03, 0x11},
	{0x10, 0x7f},
	{0x11, 0x40},
	{0x12, 0x0a}, // Blue Max-Filter Delete
	{0x13, 0xbb},

	{0x26, 0x31}, // Double_AG 31->20
	{0x27, 0x34}, // Double_AG 34->22
	{0x28, 0x0f},
	{0x29, 0x10},
	{0x2b, 0x30},
	{0x2c, 0x32},

	//Out2 D-LPF th
	{0x30, 0x70},
	{0x31, 0x10},
	{0x32, 0x58},
	{0x33, 0x09},
	{0x34, 0x06},
	{0x35, 0x03},

	//Out1 D-LPF th
	{0x36, 0x70},
	{0x37, 0x18},
	{0x38, 0x58},
	{0x39, 0x09},
	{0x3a, 0x06},
	{0x3b, 0x03},

	//Indoor D-LPF th
	{0x3c, 0x80},
	{0x3d, 0x18},
	{0x3e, 0xa0}, //80
	{0x3f, 0x0c},
	{0x40, 0x09},
	{0x41, 0x06},

	{0x42, 0x80},
	{0x43, 0x18},
	{0x44, 0xa0}, //80
	{0x45, 0x12},
	{0x46, 0x10},
	{0x47, 0x10},

	{0x48, 0x90},
	{0x49, 0x40},
	{0x4a, 0x80},
	{0x4b, 0x13},
	{0x4c, 0x10},
	{0x4d, 0x11},

	{0x4e, 0x80},
	{0x4f, 0x30},
	{0x50, 0x80},
	{0x51, 0x13},
	{0x52, 0x10},
	{0x53, 0x13},

	{0x54, 0x11},
	{0x55, 0x17},
	{0x56, 0x20},
	{0x57, 0x01},
	{0x58, 0x00},
	{0x59, 0x00},

	{0x5a, 0x1f}, //18
	{0x5b, 0x00},
	{0x5c, 0x00},

	{0x60, 0x3f},
	{0x62, 0x60},
	{0x70, 0x06},

	/////// PAGE 12 START ///////
	{0x03, 0x12},
	{0x20, 0x0f},
	{0x21, 0x0f},

	{0x25, 0x00}, //0x30

	{0x28, 0x00},
	{0x29, 0x00},
	{0x2a, 0x00},

	{0x30, 0x50},
	{0x31, 0x18},
	{0x32, 0x32},
	{0x33, 0x40},
	{0x34, 0x50},
	{0x35, 0x70},
	{0x36, 0xa0},

	//Out2 th
	{0x40, 0xa0},
	{0x41, 0x40},
	{0x42, 0xa0},
	{0x43, 0x90},
	{0x44, 0x90},
	{0x45, 0x80},

	//Out1 th
	{0x46, 0xb0},
	{0x47, 0x55},
	{0x48, 0xa0},
	{0x49, 0x90},
	{0x4a, 0x90},
	{0x4b, 0x80},

	//Indoor th
	{0x4c, 0xb0},
	{0x4d, 0x40},
	{0x4e, 0x90},
	{0x4f, 0x90},
	{0x50, 0xa0},
	{0x51, 0x80},

	//Dark1 th
	{0x52, 0xb0},
	{0x53, 0x60},
	{0x54, 0xc0},
	{0x55, 0xc0},
	{0x56, 0xc0},
	{0x57, 0x80},

	//Dark2 th
	{0x58, 0x90},
	{0x59, 0x40},
	{0x5a, 0xd0},
	{0x5b, 0xd0},
	{0x5c, 0xe0},
	{0x5d, 0x80},

	//Dark3 th
	{0x5e, 0x88},
	{0x5f, 0x40},
	{0x60, 0xe0},
	{0x61, 0xe0},
	{0x62, 0xe0},
	{0x63, 0x80},

	{0x70, 0x15},
	{0x71, 0x01}, //Don't Touch register

	{0x72, 0x18},
	{0x73, 0x01}, //Don't Touch register
	{0x90, 0x5d}, //DPC
	{0x91, 0x88},
	{0x98, 0x7d},
	{0x99, 0x28},
	{0x9A, 0x14},
	{0x9B, 0xc8},
	{0x9C, 0x02},
	{0x9D, 0x1e},
	{0x9E, 0x28},
	{0x9F, 0x07},
	{0xA0, 0x32},
	{0xA4, 0x04},
	{0xA5, 0x0e},
	{0xA6, 0x0c},
	{0xA7, 0x04},
	{0xA8, 0x3c},

	{0xAA, 0x14},
	{0xAB, 0x11},
	{0xAC, 0x0f},
	{0xAD, 0x16},
	{0xAE, 0x15},
	{0xAF, 0x14},

	{0xB1, 0xaa},
	{0xB2, 0x96},
	{0xB3, 0x28},
	//{0xB6,read}, only//dpc_flat_thres
	//{0xB7,read}, only//dpc_grad_cnt
	{0xB8, 0x78},
	{0xB9, 0xa0},
	{0xBA, 0xb4},
	{0xBB, 0x14},
	{0xBC, 0x14},
	{0xBD, 0x14},
	{0xBE, 0x64},
	{0xBF, 0x64},
	{0xC0, 0x64},
	{0xC1, 0x64},
	{0xC2, 0x04},
	{0xC3, 0x03},
	{0xC4, 0x0c},
	{0xC5, 0x30},
	{0xC6, 0x2a},
	{0xD0, 0x0c}, //CI Option/CI DPC
	{0xD1, 0x80},
	{0xD2, 0x67},
	{0xD3, 0x00},
	{0xD4, 0x00},
	{0xD5, 0x02},
	{0xD6, 0xff},
	{0xD7, 0x18},

	/////// PAGE 13 START ///////
	{0x03, 0x13},
	//Edge
	{0x10, 0xcb},
	{0x11, 0x7b},
	{0x12, 0x07},
	{0x14, 0x00},

	{0x20, 0x15},
	{0x21, 0x13},
	{0x22, 0x33},
	{0x23, 0x05},
	{0x24, 0x09},

	{0x25, 0x0a},

	{0x26, 0x18},
	{0x27, 0x30},
	{0x29, 0x12},
	{0x2a, 0x50},

	//Low clip th
	{0x2b, 0x00}, //Out2 02
	{0x2c, 0x00}, //Out1 02 //01
	{0x25, 0x06},
	{0x2d, 0x0c},
	{0x2e, 0x12},
	{0x2f, 0x12},

	//Out2 Edge
	{0x50, 0x18}, //0x10 //0x16
	{0x51, 0x1c}, //0x14 //0x1a
	{0x52, 0x1a}, //0x12 //0x18
	{0x53, 0x14}, //0x0c //0x12
	{0x54, 0x17}, //0x0f //0x15
	{0x55, 0x14}, //0x0c //0x12

	//Out1 Edge		  //Edge
	{0x56, 0x18}, //0x10 //0x16
	{0x57, 0x1c}, //0x13 //0x1a
	{0x58, 0x1a}, //0x12 //0x18
	{0x59, 0x14}, //0x0c //0x12
	{0x5a, 0x17}, //0x0f //0x15
	{0x5b, 0x14}, //0x0c //0x12

	//Indoor Edge
	{0x5c, 0x0a},
	{0x5d, 0x0b},
	{0x5e, 0x0a},
	{0x5f, 0x08},
	{0x60, 0x09},
	{0x61, 0x08},

	//Dark1 Edge
	{0x62, 0x08},
	{0x63, 0x08},
	{0x64, 0x08},
	{0x65, 0x06},
	{0x66, 0x06},
	{0x67, 0x06},

	//Dark2 Edge
	{0x68, 0x07},
	{0x69, 0x07},
	{0x6a, 0x07},
	{0x6b, 0x05},
	{0x6c, 0x05},
	{0x6d, 0x05},

	//Dark3 Edge
	{0x6e, 0x07},
	{0x6f, 0x07},
	{0x70, 0x07},
	{0x71, 0x05},
	{0x72, 0x05},
	{0x73, 0x05},

	//2DY
	{0x80, 0xfd},
	{0x81, 0x1f},
	{0x82, 0x05},
	{0x83, 0x31},

	{0x90, 0x05},
	{0x91, 0x05},
	{0x92, 0x33},
	{0x93, 0x30},
	{0x94, 0x03},
	{0x95, 0x14},
	{0x97, 0x20},
	{0x99, 0x20},

	{0xa0, 0x01},
	{0xa1, 0x02},
	{0xa2, 0x01},
	{0xa3, 0x02},
	{0xa4, 0x05},
	{0xa5, 0x05},
	{0xa6, 0x07},
	{0xa7, 0x08},
	{0xa8, 0x07},
	{0xa9, 0x08},
	{0xaa, 0x07},
	{0xab, 0x08},

	//Out2
	{0xb0, 0x22},
	{0xb1, 0x2a},
	{0xb2, 0x28},
	{0xb3, 0x22},
	{0xb4, 0x2a},
	{0xb5, 0x28},

	//Out1
	{0xb6, 0x22},
	{0xb7, 0x2a},
	{0xb8, 0x28},
	{0xb9, 0x22},
	{0xba, 0x2a},
	{0xbb, 0x28},

	//Indoor
	{0xbc, 0x25},
	{0xbd, 0x2a},
	{0xbe, 0x27},
	{0xbf, 0x25},
	{0xc0, 0x2a},
	{0xc1, 0x27},

	//Dark1
	{0xc2, 0x1e},
	{0xc3, 0x24},
	{0xc4, 0x20},
	{0xc5, 0x1e},
	{0xc6, 0x24},
	{0xc7, 0x20},

	//Dark2
	{0xc8, 0x18},
	{0xc9, 0x20},
	{0xca, 0x1e},
	{0xcb, 0x18},
	{0xcc, 0x20},
	{0xcd, 0x1e},

	//Dark3
	{0xce, 0x18},
	{0xcf, 0x20},
	{0xd0, 0x1e},
	{0xd1, 0x18},
	{0xd2, 0x20},
	{0xd3, 0x1e},

	/////// PAGE 14 START ///////
	{0x03, 0x14},
	{0x10, 0x11},

	{0x14, 0x80}, // GX
	{0x15, 0x80}, // GY
	{0x16, 0x80}, // RX
	{0x17, 0x80}, // RY
	{0x18, 0x80}, // BX
	{0x19, 0x80}, // BY

	{0x20, 0x66}, //X 60 //a0
	{0x21, 0xa0}, //Y

	{0x22, 0xa7},
	{0x23, 0xaa},
	{0x24, 0xce},

	{0x30, 0xc8},
	{0x31, 0x2b},
	{0x32, 0x00},
	{0x33, 0x00},
	{0x34, 0x90},

	{0x40, 0x48}, //31
	{0x50, 0x34}, //23 //32
	{0x60, 0x29}, //1a //27
	{0x70, 0x34}, //23 //32

	/////// PAGE 15 START ///////
	{0x03, 0x15},
	{0x10, 0x0f},

	//Rstep H 16
	//Rstep L 14
	{0x14, 0x42}, //CMCOFSGH_Day //4c
	{0x15, 0x32}, //CMCOFSGM_CWF //3c
	{0x16, 0x24}, //CMCOFSGL_A //2e
	{0x17, 0x2f}, //CMC SIGN

	//CMC_Default_CWF
	{0x30, 0x8f},
	{0x31, 0x59},
	{0x32, 0x0a},
	{0x33, 0x15},
	{0x34, 0x5b},
	{0x35, 0x06},
	{0x36, 0x07},
	{0x37, 0x40},
	{0x38, 0x87}, //86

	//CMC OFS L_A
	{0x40, 0x92},
	{0x41, 0x1b},
	{0x42, 0x89},
	{0x43, 0x81},
	{0x44, 0x00},
	{0x45, 0x01},
	{0x46, 0x89},
	{0x47, 0x9e},
	{0x48, 0x28},
	//CMC POFS H_DAY
	{0x50, 0x02},
	{0x51, 0x82},
	{0x52, 0x00},
	{0x53, 0x07},
	{0x54, 0x11},
	{0x55, 0x98},
	{0x56, 0x00},
	{0x57, 0x0b},
	{0x58, 0x8b},

	{0x80, 0x03},
	{0x85, 0x40},
	{0x87, 0x02},
	{0x88, 0x00},
	{0x89, 0x00},
	{0x8a, 0x00},

	/////// PAGE 16 START ///////
	{0x03, 0x16},
	{0x10, 0x31},
	{0x18, 0x5e},// Double_AG 5e->37
	{0x19, 0x5d},// Double_AG 5e->36
	{0x1a, 0x0e},
	{0x1b, 0x01},
	{0x1c, 0xdc},
	{0x1d, 0xfe},

	//GMA Default
	{0x30, 0x00},
	{0x31, 0x0a},
	{0x32, 0x1f},
	{0x33, 0x33},
	{0x34, 0x53},
	{0x35, 0x6c},
	{0x36, 0x81},
	{0x37, 0x94},
	{0x38, 0xa4},
	{0x39, 0xb3},
	{0x3a, 0xc0},
	{0x3b, 0xcb},
	{0x3c, 0xd5},
	{0x3d, 0xde},
	{0x3e, 0xe6},
	{0x3f, 0xee},
	{0x40, 0xf5},
	{0x41, 0xfc},
	{0x42, 0xff},

	{0x50, 0x00},
	{0x51, 0x09},
	{0x52, 0x1f},
	{0x53, 0x37},
	{0x54, 0x5b},
	{0x55, 0x76},
	{0x56, 0x8d},
	{0x57, 0xa1},
	{0x58, 0xb2},
	{0x59, 0xbe},
	{0x5a, 0xc9},
	{0x5b, 0xd2},
	{0x5c, 0xdb},
	{0x5d, 0xe3},
	{0x5e, 0xeb},
	{0x5f, 0xf0},
	{0x60, 0xf5},
	{0x61, 0xf7},
	{0x62, 0xf8},

	{0x70, 0x00},
	{0x71, 0x08},
	{0x72, 0x17},
	{0x73, 0x2f},
	{0x74, 0x53},
	{0x75, 0x6c},
	{0x76, 0x81},
	{0x77, 0x94},
	{0x78, 0xa4},
	{0x79, 0xb3},
	{0x7a, 0xc0},
	{0x7b, 0xcb},
	{0x7c, 0xd5},
	{0x7d, 0xde},
	{0x7e, 0xe6},
	{0x7f, 0xee},
	{0x80, 0xf4},
	{0x81, 0xfa},
	{0x82, 0xff},

	/////// PAGE 17 START ///////
	{0x03, 0x17},
	{0x10, 0xf7},

	/////// PAGE 20 START ///////
	{0x03, 0x20},
	{0x11, 0x1c},
	{0x18, 0x30},
	{0x1a, 0x08},
	{0x20, 0x01}, //05_lowtemp Y Mean off
	{0x21, 0x30},
	{0x22, 0x10},
	{0x23, 0x00},
	{0x24, 0x00}, //Uniform Scene Off

	{0x28, 0xe7},
	{0x29, 0x0d}, //20100305 ad->0d
	{0x2a, 0xff},
	{0x2b, 0x04}, //f4->Adaptive off

	{0x2c, 0xc2},
	{0x2d, 0xcf},  //ff->AE Speed option
	{0x2e, 0x33},
	{0x30, 0x78}, //f8
	{0x32, 0x03},
	{0x33, 0x2e},
	{0x34, 0x30},
	{0x35, 0xd4},
	{0x36, 0xfe},
	{0x37, 0x32},
	{0x38, 0x04},

	{0x39, 0x22}, //AE_escapeC10
	{0x3a, 0xde}, //AE_escapeC11

	{0x3b, 0x22}, //AE_escapeC1
	{0x3c, 0xde}, //AE_escapeC2

	{0x50, 0x45},
	{0x51, 0x88},

	{0x56, 0x03},
	{0x57, 0xf7},
	{0x58, 0x14},
	{0x59, 0x88},
	{0x5a, 0x04},

	//New Weight For Samsung
	//{0x60, 0xaa},
	//{0x61, 0xaa},
	//{0x62, 0xaa},
	//{0x63, 0xaa},
	//{0x64, 0xaa},
	//{0x65, 0xaa},
	//{0x66, 0xab},
	//{0x67, 0xEa},
	//{0x68, 0xab},
	//{0x69, 0xEa},
	//{0x6a, 0xaa},
	//{0x6b, 0xaa},
	//{0x6c, 0xaa},
	//{0x6d, 0xaa},
	//{0x6e, 0xaa},
	//{0x6f, 0xaa},

	{0x60, 0x55}, // AEWGT1
	{0x61, 0x55}, // AEWGT2
	{0x62, 0x6a}, // AEWGT3
	{0x63, 0xa9}, // AEWGT4
	{0x64, 0x6a}, // AEWGT5
	{0x65, 0xa9}, // AEWGT6
	{0x66, 0x6a}, // AEWGT7
	{0x67, 0xa9}, // AEWGT8
	{0x68, 0x6b}, // AEWGT9
	{0x69, 0xe9}, // AEWGT10
	{0x6a, 0x6a}, // AEWGT11
	{0x6b, 0xa9}, // AEWGT12
	{0x6c, 0x6a}, // AEWGT13
	{0x6d, 0xa9}, // AEWGT14
	{0x6e, 0x55}, // AEWGT15
	{0x6f, 0x55}, // AEWGT16

	{0x70, 0x76}, //6e
	{0x71, 0x00}, //82(+8)->+0

	// haunting control
	{0x76, 0x43},
	{0x77, 0xe2}, //04
	{0x78, 0x23}, //Yth1
	{0x79, 0x42}, //Yth2
	{0x7a, 0x23}, //23
	{0x7b, 0x22}, //22
	{0x7d, 0x23},

	{0x83, 0x01}, //EXP Normal 33.33 fps
	{0x84, 0x5f},
	{0x85, 0x00},

	{0x86, 0x02}, //EXPMin 5859.38 fps
	{0x87, 0x00},

	{0x88, 0x04}, //EXP Max 10.00 fps
	{0x89, 0x92},
	{0x8a, 0x00},

	{0x8B, 0x75}, //EXP100
	{0x8C, 0x00},
	{0x8D, 0x61}, //EXP120
	{0x8E, 0x00},

	{0x9c, 0x18}, //EXP Limit 488.28 fps
	{0x9d, 0x00},
	{0x9e, 0x02}, //EXP Unit
	{0x9f, 0x00},

	//AE_Middle Time option
	//{0xa0, 0x03},
	//{0xa1, 0xa9},
	//{0xa2, 0x80},

	{0xb0, 0x18},
	{0xb1, 0x14}, //ADC 400->560
	{0xb2, 0xe0}, //d0
	{0xb3, 0x18},
	{0xb4, 0x1a},
	{0xb5, 0x44},
	{0xb6, 0x2f},
	{0xb7, 0x28},
	{0xb8, 0x25},
	{0xb9, 0x22},
	{0xba, 0x21},
	{0xbb, 0x20},
	{0xbc, 0x1f},
	{0xbd, 0x1f},

	//AE_Adaptive Time option
	//{0xc0, 0x10},
	//{0xc1, 0x2b},
	//{0xc2, 0x2b},
	//{0xc3, 0x2b},
	//{0xc4, 0x08},

	{0xc8, 0x80},
	{0xc9, 0x40},

	/////// PAGE 22 START ///////
	{0x03, 0x22},
	{0x10, 0xfd},
	{0x11, 0x2e},
	{0x19, 0x01}, // Low On //
	{0x20, 0x30},
	{0x21, 0x80},
	{0x24, 0x01},
	//{0x25, 0x00}, //7f New Lock Cond & New light stable

	{0x30, 0x80},
	{0x31, 0x80},
	{0x38, 0x11},
	{0x39, 0x34},

	{0x40, 0xf4},
	{0x41, 0x55}, //44
	{0x42, 0x33}, //43

	{0x43, 0xf6},
	{0x44, 0x55}, //44
	{0x45, 0x44}, //33

	{0x46, 0x00},
	{0x50, 0xb2},
	{0x51, 0x81},
	{0x52, 0x98},

	{0x80, 0x40}, //3e
	{0x81, 0x20},
	{0x82, 0x3e},

	{0x83, 0x5e}, //5e
	{0x84, 0x0e}, //24
	{0x85, 0x5e}, //54 //56 //5a
	{0x86, 0x22}, //24 //22

	{0x87, 0x40},
	{0x88, 0x30},
	{0x89, 0x37}, //38
	{0x8a, 0x28}, //2a

	{0x8b, 0x40}, //47
	{0x8c, 0x33},
	{0x8d, 0x39},
	{0x8e, 0x30}, //2c

	{0x8f, 0x53}, //4e
	{0x90, 0x52}, //4d
	{0x91, 0x51}, //4c
	{0x92, 0x4e}, //4a
	{0x93, 0x4a}, //46
	{0x94, 0x45},
	{0x95, 0x3d},
	{0x96, 0x31},
	{0x97, 0x28},
	{0x98, 0x24},
	{0x99, 0x20},
	{0x9a, 0x20},

	{0x9b, 0x88},
	{0x9c, 0x88},
	{0x9d, 0x48},
	{0x9e, 0x38},
	{0x9f, 0x30},

	{0xa0, 0x60},
	{0xa1, 0x34},
	{0xa2, 0x6f},
	{0xa3, 0xff},

	{0xa4, 0x14}, //1500fps
	{0xa5, 0x2c}, // 700fps
	{0xa6, 0xcf},

	{0xad, 0x40},
	{0xae, 0x4a},

	{0xaf, 0x28},  // low temp Rgain
	{0xb0, 0x26},  // low temp Rgain

	{0xb1, 0x00}, //0x20 -> 0x00 0405 modify
	{0xb4, 0xea},
	{0xb8, 0xa0}, //a2: b-2, R+2  //b4 B-3, R+4 lowtemp
	{0xb9, 0x00},
	// PAGE 20
	{0x03, 0x20}, //page 20
	{0x10, 0x9c}, //ae off

	// PAGE 22
	{0x03, 0x22}, //page 22
	{0x10, 0xe9}, //awb off

	// PAGE 0
	{0x03, 0x00},
	{0x0e, 0x03}, //PLL On
	{0x0e, 0x73}, //PLLx2

	{0x03, 0x00}, // Dummy 750us
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},

	{0x03, 0x00}, // Page 0
	{0x01, 0xf0}, // Sleep Off

	{0xff, 0xff}
};


static struct _sensor_chip_ Hi253;
static struct i2c_driver sensor_i2c_driver;
static unsigned long xclk_current = __XCLK_MIN;
static unsigned long cntclk = 2000000;
static enum v4l2_power current_power_state;

static int Hi253_capture_mode = 0;

static int Hi253_capture2preview_mode = 0;
//static int Hi253_exit_mode = 0;

static char Hi253_Sensor_Type = 0;

unsigned char HI253_Sleep_Mode;
unsigned char HI253_Init_Mode=0;
unsigned char HI253_HV_Mirror=0;
unsigned int HI253_pv_HI253_exposure_lines=0x0249f0,HI253_cp_HI253_exposure_lines=0;
/* List of image formats supported by OV2655sensor */
	const static struct v4l2_fmtdesc Hi253_formats[] = {
#if defined(__RAW_MODE)
	{
		.description	= "RAW10",
		.pixelformat	= V4L2_PIX_FMT_SGRBG10,
	},
#elif defined(__YUV_MODE)
	{
		.description	= "YUYV,422",
		.pixelformat	= V4L2_PIX_FMT_UYVY,
	},
#else
	{
		/* Note:  V4L2 defines RGB565 as:
		 *
		 *	Byte 0			  Byte 1
		 *	g2 g1 g0 r4 r3 r2 r1 r0	  b4 b3 b2 b1 b0 g5 g4 g3
		 *
		 * We interpret RGB565 as:
		 *
		 *	Byte 0			  Byte 1
		 *	g2 g1 g0 b4 b3 b2 b1 b0	  r4 r3 r2 r1 r0 g5 g4 g3
		 */
		.description	= "RGB565, le",
		.pixelformat	= V4L2_PIX_FMT_RGB565,
	},
	{
		/* Note:  V4L2 defines RGB565X as:
		 *
		 *	Byte 0			  Byte 1
		 *	b4 b3 b2 b1 b0 g5 g4 g3	  g2 g1 g0 r4 r3 r2 r1 r0
		 *
		 * We interpret RGB565X as:
		 *
		 *	Byte 0			  Byte 1
		 *	r4 r3 r2 r1 r0 g5 g4 g3	  g2 g1 g0 b4 b3 b2 b1 b0
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
		 *	Byte 0			  Byte 1
		 *	g2 g1 g0 r4 r3 r2 r1 r0	  x  b4 b3 b2 b1 b0 g4 g3
		 *
		 * We interpret RGB555 as:
		 *
		 *	Byte 0			  Byte 1
		 *	g2 g1 g0 b4 b3 b2 b1 b0	  x  r4 r3 r2 r1 r0 g4 g3
		 */
		.description	= "RGB555, le",
		.pixelformat	= V4L2_PIX_FMT_RGB555,
	},
	{
		/* Note:  V4L2 defines RGB555X as:
		 *
		 *	Byte 0			  Byte 1
		 *	x  b4 b3 b2 b1 b0 g4 g3	  g2 g1 g0 r4 r3 r2 r1 r0
		 *
		 * We interpret RGB555X as:
		 *
		 *	Byte 0			  Byte 1
		 *	x  r4 r3 r2 r1 r0 g4 g3	  g2 g1 g0 b4 b3 b2 b1 b0
		 */
		.description	= "RGB555, be",
		.pixelformat	= V4L2_PIX_FMT_RGB555X,
	},
#endif
};

#define NUM_CAPTURE_FORMATS (sizeof(Hi253_formats) / sizeof(Hi253_formats[0]))

/* register initialization tables for Hi253 */
#define __REG_TERM 0xFF	/* terminating list entry for reg */
#define __VAL_TERM 0xFF	/* terminating list entry for val */

/*
 * struct vcontrol - Video controls
 * @v4l2_queryctrl: V4L2 VIDIOC_QUERYCTRL ioctl structure
 * @current_value: current value of this control
 */
static struct vcontrol {
	struct v4l2_queryctrl qc;
	int current_value;
} video_control[] = {
#ifdef __YUV_MODE
	{
		{
			.id = V4L2_CID_BRIGHTNESS,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Brightness",
			.minimum = __MIN_BRIGHT,
			.maximum = __MAX_BRIGHT,
			.step = __BRIGHT_STEP,
			.default_value = __DEF_BRIGHT,
		},
		.current_value = __DEF_BRIGHT,
	},
	{
		{
			.id = V4L2_CID_CONTRAST,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Contrast",
			.minimum = __MIN_CONTRAST,
			.maximum = __MAX_CONTRAST,
			.step = __CONTRAST_STEP,
			.default_value = __DEF_CONTRAST,
		},
		.current_value = __DEF_CONTRAST,
	},
	{
		{
			.id = V4L2_CID_PRIVATE_BASE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Color Effects",
			.minimum = __MIN_COLOR,
			.maximum = __MAX_COLOR,
			.step = __COLOR_STEP,
			.default_value = __DEF_COLOR,
		},
		.current_value = __DEF_COLOR,
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

/*
 * Read a value from a register in Hi253 sensor device.
 * The value is returned in 'val'.
 * Returns zero if successful, or non-zero otherwise.
 */

static int Sensor_read_reg(struct i2c_client *client, u8 reg)
{

	int ret;
	client->addr=Hi253_I2C_ADDR;

	ret = i2c_smbus_read_byte_data(client, reg);
	return ret;
}
/* Write a value to a register in Hi253 sensor device.
 * @client: i2c driver client structure.
 * @reg: Address of the register to read value from.
 * @val: Value to be written to a specific register.
 * Returns zero if successful, or non-zero otherwise.
 */
static int Sensor_write_reg(struct i2c_client *client, u16 reg, u8 val)
{
	int err = 0;
	struct i2c_msg msg[1];
	unsigned char data[2];
	int retries = 0;

	if (!client->adapter)
		return -ENODEV;
retry:
	msg->addr =	Hi253_I2C_ADDR;

	msg->addr = client->addr;
	msg->flags = I2C_M_WR;
	msg->len = 2;
	msg->buf = data;

	/* high byte goes out first */
	data[0] = (u8) (reg & 0xff);
	data[1] = val;

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

/*
 * Initialize a list of Hi253 registers.
 * The list of registers is terminated by the pair of values
 * {__REG_TERM, __VAL_TERM}.
 * @client: i2c driver client structure.
 * @reglist[]: List of address of the registers to write data.
 * Returns zero if successful, or non-zero otherwise.
 */
static int Sensor_write_regs(struct i2c_client *client,
		const struct Hi253_reg reglist[])
{
	int err = 0;
	const struct Hi253_reg *next = reglist;

	while (!((next->reg == __REG_TERM)
				&& (next->val == __VAL_TERM))) {
		err = Sensor_write_reg(client, next->reg, next->val);
		udelay(1);
		//   if(next->reg == 0x0000){
		//	 mdelay(next->val);
		//  }
		if (err)
			return err;
		next++;
	}
	return 0;
}

/* Find the best match for a requested image capture size.  The best match
 * is chosen as the nearest match that has the same number or fewer pixels
 * as the requested size, or the smallest image size if the requested size
 * has fewer pixels than the smallest image.
 */
	static enum image_size_hi253
Hi253_find_size(unsigned int width, unsigned int height)
{

	enum image_size_hi253 isize;

	for (isize = 0; isize < ARRAY_SIZE(Hi253_sizes); isize++) {
		if ((Hi253_sizes[isize].height >= height) &&
				(Hi253_sizes[isize].width >= width)) {
			break;
		}
	}
	DPRINTK_OV("width = %d,height = %d,return %d\n",width,height,isize);
	return isize;
}

#if 0
static void HI253_Enter_Standby (struct i2c_client *client)
{
	Sensor_write_reg(client,0x03, 0x02);
	Sensor_write_reg(client,0x55, 0x10);
}
static void HI253_Exit_Standby (struct i2c_client *client)
{
	u8 regVal;

	Sensor_write_reg(client,0x03,0x00); //Select Page 0
	regVal = Sensor_read_reg(client, 0x01);
	regVal &= ~(0x1<<1);
	Sensor_write_reg(client,0x01,regVal);
	regVal |= (0x1<<1);
	Sensor_write_reg(client,0x01,regVal);
	regVal &= ~(0x1<<1);

	Sensor_write_reg(client,0x03,0x02);  //select page 2
	Sensor_write_reg(client,0x55,0x1c);
}
#endif
/*
 * Configure the Hi253 for a specified image size, pixel format, and frame
 * period.  xclk is the frequency (in Hz) of the xclk input to the _.
 * fper is the frame period (in seconds) expressed as a fraction.
 * Returns zero if successful, or non-zero otherwise.
 * The actual frame period is returned in fper.
 */
static int Hi253_configure(struct v4l2_int_device *s)
{
	struct _sensor_chip_ *sensor = s->priv;
	struct v4l2_pix_format *pix = &sensor->pix;
	struct i2c_client *client = sensor->i2c_client;
	enum image_size_hi253 isize = ARRAY_SIZE(Hi253_sizes) - 1;
	struct hi253_ext_params *extp = &(sensor->ext_params);

	int  err = 0;
	unsigned long t1, t2;
	enum pixel_format_hi253 pfmt = HI253_YUV;

	DPRINTK_OV("\n");
	switch (pix->pixelformat) {

		case V4L2_PIX_FMT_RGB565:
		case V4L2_PIX_FMT_RGB565X:
			pfmt = HI253_RGB565;
			break;

		case V4L2_PIX_FMT_RGB555:
		case V4L2_PIX_FMT_RGB555X:
			pfmt = HI253_RGB555;
			break;

		case V4L2_PIX_FMT_SGRBG10:
			pfmt = HI253_RAW10;
			break;

		case V4L2_PIX_FMT_YUYV:
		case V4L2_PIX_FMT_UYVY:
		default:
			pfmt = HI253_YUV;
	}


	isize = Hi253_find_size(pix->width, pix->height);

	if(Hi253_capture_mode){

		t1 = jiffies;
		DPRINTK_OV("prepare to capture, isize=%d\n", isize);

		if(isize == HI253_UXGA)
			Sensor_write_regs(client, Hi253_uxga_capture);

		t2 = jiffies;

		DPRINTK_OV("hi253:capture setting time t1[%d] t2[%d], delay=[%dus]\n", t1, t2, jiffies_to_usecs(t2-t1));
		Hi253_capture_mode = 0;
		Hi253_capture2preview_mode = 1;
	}
	else if(Hi253_capture2preview_mode)
	{

		DPRINTK_OV("back from capture, isize=%d\n",isize);

		if(isize == HI253_VGA)
			Sensor_write_regs(client, Hi253_svga_to_vga);
		else if(isize == HI253_QVGA)
			Sensor_write_regs(client, Hi253_svga_to_qvga);
		else
		{
			printk("ERROR:HI253 not supportted priview size\n");
		}
		Hi253_capture2preview_mode = 0;
		Hi253_capture_mode = 0;
	}
	else{
		DPRINTK_OV("prepare to preview, Hi253_exit_mode=%d, isize=%d\n", isize, Hi253_exit_mode);

		// t3 = jiffies;

		Sensor_write_regs(client, Hi253_common_svgabase);

		if(isize == HI253_VGA)
			Sensor_write_regs(client, Hi253_svga_to_vga);
		else if(isize == HI253_QVGA)
			Sensor_write_regs(client, Hi253_svga_to_qvga);
		else
		{
			printk("ERROR:HI253 not supportted priview size\n");
		}
	}

	Sensor_write_regs(client, Hi253_effects[extp->effect]);
	Sensor_write_regs(client, Hi253_whitebalance[extp->white_balance]);
	Sensor_write_regs(client, Hi253_brightness[extp->brightness - 1]);
	//Sensor_write_regs(client, Hi253_contrast[extp->contrast - 1]);

	//err = Sensor_write_regs(client, Hi253_exposure[extp->exposure - 1]);

	//DPRINTK_OV("preview setting time t3[%d] t4[%d], delay=[%dus]\n", t3, t4, jiffies_to_usecs(t4-t3));

	DPRINTK_OV("[Eric]effect[%d],wb[%d],brightness[%d],contrast[%d],exposure[%d]\n",
				extp->effect,
				extp->white_balance,
				extp->brightness - 1,
				extp->contrast - 1,
				extp->exposure - 1
				);


	DPRINTK_OV("pix->width = %d,pix->height = %d\n",pix->width,pix->height);
	DPRINTK_OV("format = %d,size = %d\n",pfmt,isize);


	/* Store image size */
	sensor->width = pix->width;
	sensor->height = pix->height;

	sensor->crop_rect.left = 0;
	sensor->crop_rect.width = pix->width;
	sensor->crop_rect.top = 0;
	sensor->crop_rect.height = pix->height;

	return err;
}


/* Detect if an Hi253 is present, returns a negative error number if no
 * device is detected, or pidl as version number if a device is detected.
 */
static u8 Hi253_detect(struct i2c_client *client)
{
	u8 pid;

	DPRINTK_OV("\n");

	if (!client)
		return -ENODEV;
	if( (pid=Sensor_read_reg(client, 0x04))<0)
		return -ENODEV;
	DPRINTK_OV( "Detect  0x%x\n", pid);

	if (pid== __PIDH_MAGIC)
	{
		DPRINTK_OV( "Detect success 0x%x\n", pid);
		return pid;
	}


	return -ENODEV;
}

/* To get the cropping capabilities of Hi253 sensor
 * Returns zero if successful, or non-zero otherwise.
 */
static int ioctl_cropcap(struct v4l2_int_device *s,
		struct v4l2_cropcap *cropcap)
{
	struct _sensor_chip_ *sensor = s->priv;

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

/* To get the current crop window for of Hi253 sensor
 * Returns zero if successful, or non-zero otherwise.
 */
static int ioctl_g_crop(struct v4l2_int_device *s, struct  v4l2_crop *crop)
{
	struct _sensor_chip_ *sensor = s->priv;
	DPRINTK_OV("\n");
	crop->c = sensor->crop_rect;
	return 0;
}

/* To set the crop window for of Hi253 sensor
 * Returns zero if successful, or non-zero otherwise.
 */
static int ioctl_s_crop(struct v4l2_int_device *s, struct  v4l2_crop *crop)
{
	DPRINTK_OV("\n");
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
	DPRINTK_OV("\n");
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
	DPRINTK_OV("\n");
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
	struct _sensor_chip_ *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	struct hi253_ext_params *extp = &(sensor->ext_params);

	switch(vc->id){
		case V4L2_CID_CAPTURE:
			DPRINTK_OV("V4L2_CID_CAPTURE:value = %d\n",vc->value);
			Hi253_capture_mode = 1;
			Hi253_capture2preview_mode = 0;
			break;
		case V4L2_CID_EFFECT:
			DPRINTK_OV("V4L2_CID_EFFECT:value = %d\n",vc->value);
			if(vc->value < 0 || vc->value > ARRAY_SIZE(Hi253_effects))
				vc->value = __DEF_EFFECT;
			extp->effect = vc->value;
			if(current_power_state == V4L2_POWER_ON){
				err = Sensor_write_regs(client, Hi253_effects[vc->value]);
			}
			break;
		case V4L2_CID_SCENE:
			DPRINTK_OV("V4L2_CID_EFFECT:value = %d\n",vc->value);
			break;
#if 0
		case V4L2_CID_SHARPNESS:
			if(vc->value < 0 || vc->value > ARRAY_SIZE(Hi253_sharpness))
				vc->value = __DEF_SHARPNESS;
			extp->sharpness= vc->value;
			if(current_power_state == V4L2_POWER_ON){
				err = Sensor_write_regs(client, Hi253_sharpness[vc->value]);
			}
			break;
		case V4L2_CID_SATURATION:
			if(vc->value < 0 || vc->value > ARRAY_SIZE(Hi253_saturation))
				vc->value = __DEF_SATURATION;
			extp->saturation= vc->value;
			if(current_power_state == V4L2_POWER_ON){
				err = Sensor_write_regs(client, Hi253_saturation[vc->value - 1]);
			}
			break;
#endif
		case V4L2_CID_EXPOSURE:
			DPRINTK_OV("V4L2_CID_EXPOSURE:value = %d\n",vc->value);
			if(vc->value < 0 || vc->value > ARRAY_SIZE(Hi253_exposure))
				vc->value = __DEF_EXPOSURE;
			extp->exposure= vc->value;
			if(current_power_state == V4L2_POWER_ON){
				//Sensor_write_reg(client,0x03, 0x10);
				//Sensor_write_reg(client,0x12, (Sensor_read_reg(client, 0x12) |0x10));
				err = Sensor_write_regs(client, Hi253_exposure[vc->value - 1]);
			}
			break;

		case V4L2_CID_CONTRAST:
			DPRINTK_OV("V4L2_CID_CONTRAST:value = %d\n",vc->value);
			if(vc->value < 0 || vc->value > ARRAY_SIZE(Hi253_contrast))
				vc->value = __DEF_CONTRAST;
			extp->contrast = vc->value;
			if(current_power_state == V4L2_POWER_ON){
				err = Sensor_write_regs(client, Hi253_contrast[vc->value - 1]);
			}
			break;
		case V4L2_CID_BRIGHTNESS:
			DPRINTK_OV("V4L2_CID_BRIGHTNESS:value = %d\n",vc->value);
			if(vc->value < 0 || vc->value > ARRAY_SIZE(Hi253_brightness))
				vc->value = __DEF_BRIGHT;
			extp->brightness = vc->value;
			if(current_power_state == V4L2_POWER_ON){
				err = Sensor_write_regs(client, Hi253_brightness[vc->value - 1]);
			}
			break;
		case V4L2_CID_DO_WHITE_BALANCE:
			DPRINTK_OV("V4L2_CID_DO_WHITE_BALANCE:value = %d\n",vc->value);
			if(vc->value < 0 || vc->value > ARRAY_SIZE(Hi253_whitebalance))
				vc->value = __DEF_WB;
			extp->white_balance = vc->value;
			if(current_power_state == V4L2_POWER_ON){
				err = Sensor_write_regs(client, Hi253_whitebalance[vc->value]);
			}
			break;
		case V4L2_CID_TYPE:

			break;
		case V4L2_CID_FLASH:
			break;
		case V4L2_CID_BACKTOPREVIEW:
			DPRINTK_OV("V4L2_CID_BACKTOPREVIEW:value = %d\n",vc->value);
			Hi253_capture2preview_mode = 1;
			Hi253_capture_mode = 0;
			break;
		case V4L2_CID_EXIT:
			DPRINTK_OV("V4L2_CID_BACKTOPREVIEW:value = %d\n",vc->value);
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

	fmt->flags = Hi253_formats[index].flags;
	strlcpy(fmt->description, Hi253_formats[index].description,
			sizeof(fmt->description));
	fmt->pixelformat = Hi253_formats[index].pixelformat;

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
	enum image_size_hi253 isize = ARRAY_SIZE(Hi253_sizes) - 1;
	struct v4l2_pix_format *pix = &f->fmt.pix;


	DPRINTK_OV("v4l2_pix_format(w:%d,h:%d)\n",pix->width,pix->height);

	if (pix->width > Hi253_sizes[isize].width)
		pix->width = Hi253_sizes[isize].width;
	if (pix->height > Hi253_sizes[isize].height)
		pix->height = Hi253_sizes[isize].height;

	isize = Hi253_find_size(pix->width, pix->height);
	pix->width = Hi253_sizes[isize].width;
	pix->height = Hi253_sizes[isize].height;

	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (pix->pixelformat == Hi253_formats[ifmt].pixelformat)
			break;
	}
	if (ifmt == NUM_CAPTURE_FORMATS)
		ifmt = 0;
	pix->pixelformat = Hi253_formats[ifmt].pixelformat;
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
	struct _sensor_chip_ *sensor = s->priv;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	int rval;

	//   DPRINTK_OV("pix.pixformat = %x \n",pix->pixelformat);
	//  DPRINTK_OV("pix.width = %d \n",pix->width);
	// DPRINTK_OV("pix.height = %d \n",pix->height);

	rval = ioctl_try_fmt_cap(s, f);
	if (rval)
		return rval;

	sensor->pix = *pix;
	sensor->width = sensor->pix.width;
	sensor->height = sensor->pix.height;
	//  DPRINTK_OV("sensor->pix.pixformat = %x \n",sensor->pix.pixelformat);
	//  DPRINTK_OV("sensor->pix.width = %d \n",sensor->pix.width);
	//  DPRINTK_OV("sensor->pix.height = %d \n",sensor->pix.height);

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
	struct _sensor_chip_ *sensor = s->priv;
	f->fmt.pix = sensor->pix;
	//DPRINTK_OV("f->fmt.pix.pixformat = %x,V4L2_PIX_FMT_YUYV = %x\n",f->fmt.pix.pixelformat,V4L2_PIX_FMT_YUYV);
	//DPRINTK_OV("f->fmt.pix.width = %d \n",f->fmt.pix.width);
	//DPRINTK_OV("f->fmt.pix.height = %d \n",f->fmt.pix.height);

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
	struct _sensor_chip_ *sensor = s->priv;
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
	struct _sensor_chip_ *sensor = s->priv;
	struct v4l2_fract *timeperframe = &a->parm.capture.timeperframe;
	struct v4l2_fract timeperframe_old;
	int desired_fps;
	timeperframe_old = sensor->timeperframe;
	sensor->timeperframe = *timeperframe;

	desired_fps = timeperframe->denominator / timeperframe->numerator;
	if ((desired_fps < __MIN_FPS) || (desired_fps > __MAX_FPS))
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
	struct _sensor_chip_ *sensor = s->priv;

	return sensor->pdata->priv_data_set(s,p,Hi253_Sensor_Type);
}



static int __Hi253_power_off_standby(struct v4l2_int_device *s,
		enum v4l2_power on)
{
	struct _sensor_chip_ *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int rval;


	sensor->pdata->set_xclk(s, 0,0);

	rval = sensor->pdata->power_set(s, on,Hi253_Sensor_Type);

	if (rval < 0) {
		v4l_err(client, "Unable to set the power state: "
				HI253_I2C_NAME " sensor\n");
		return rval;
	}

	return 0;
}

static int Hi253_power_off(struct v4l2_int_device *s)
{
	return __Hi253_power_off_standby(s, V4L2_POWER_OFF);
}

static int Hi253_power_standby(struct v4l2_int_device *s)
{
	return __Hi253_power_off_standby(s, V4L2_POWER_STANDBY);
}



static int Hi253_power_on(struct v4l2_int_device *s)
{
	struct _sensor_chip_ *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int rval;

	sensor->pdata->set_xclk(s, xclk_current,cntclk);

	rval = sensor->pdata->power_set(s, V4L2_POWER_ON,Hi253_Sensor_Type);
	if (rval < 0) {
		v4l_err(client, "Unable to set the power state: "
				HI253_I2C_NAME " sensor\n");
		sensor->pdata->set_xclk(s, 0,0);
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
			if(!Hi253_capture_mode && !Hi253_capture2preview_mode)
				Hi253_power_on(s);
			/*kunlun p1 can't support standby*/
			Hi253_configure(s);
			break;
		case V4L2_POWER_OFF:
				Hi253_capture_mode = 0;
				Hi253_capture2preview_mode = 0;
				Hi253_power_off(s);
			break;
		case V4L2_POWER_STANDBY:
			if(!Hi253_capture_mode && !Hi253_capture2preview_mode)
				Hi253_power_standby(s);
			break;
	}


	DPRINTK_OV("ori=%d, current=%d\n",current_power_state, on);

	current_power_state = on;

	return 0;
}


/*
 * ioctl_init - V4L2 sensor interface handler for VIDIOC_INT_INIT
 * @s: pointer to standard V4L2 device structure
 *
 * Initialize the sensor device (call Hi253_configure())
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
 * Hi253 device could be found, otherwise returns appropriate error.
 */
static int ioctl_dev_init(struct v4l2_int_device *s)
{
	struct _sensor_chip_ *sensor = s->priv;
	struct i2c_client *c = sensor->i2c_client;
	u8 err;

	err = Hi253_power_on(s);
	if (err)
		return -ENODEV;

	err = Hi253_detect(c);
	if (err < 0) {
		DPRINTK_OV("Unable to detect " HI253_I2C_NAME
				" sensor\n");
		/*
		 * Turn power off before leaving this function
		 * If not, CAM powerdomain will on
		 */
		Hi253_power_off(s);
		return err;
	}

	sensor->ver = err;
	DPRINTK_OV(" chip version 0x%x detected\n",
			sensor->ver);
	err = Hi253_power_off(s);
	if(err)
	{
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

	//  DPRINTK_OV("index=%d\n",frms->index);
	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (frms->pixel_format == Hi253_formats[ifmt].pixelformat)
			break;
	}
	/* Is requested pixelformat not found on sensor? */
	if (ifmt == NUM_CAPTURE_FORMATS)
		return -EINVAL;

	/* Do we already reached all discrete framesizes? */
	if (frms->index >= ARRAY_SIZE(Hi253_sizes))
		return -EINVAL;

	frms->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	frms->discrete.width = Hi253_sizes[frms->index].width;
	frms->discrete.height = Hi253_sizes[frms->index].height;

	//DPRINTK_OV("discrete(%d,%d)\n",frms->discrete.width,frms->discrete.height);
	return 0;
}

/*QCIF,QVGA,CIF,VGA,SVGA can support up to 30fps*/
const struct v4l2_fract Hi253_frameintervals[] = {
	{ .numerator = 1, .denominator = 15 },
	{ .numerator = 1, .denominator = 30 },
	{ .numerator = 1, .denominator = 30 },
};

static int ioctl_enum_frameintervals(struct v4l2_int_device *s,
		struct v4l2_frmivalenum *frmi)
{
	int ifmt;
	int isize = ARRAY_SIZE(Hi253_frameintervals) - 1;

	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (frmi->pixel_format == Hi253_formats[ifmt].pixelformat)
			break;
	}

	//	DPRINTK_OV("frmi->width = %d,frmi->height = %d,frmi->index = %d,ifmt = %d\n",
	//			frmi->width,frmi->height,frmi->index,ifmt);

	/* Is requested pixelformat not found on sensor? */
	if (ifmt == NUM_CAPTURE_FORMATS)
		return -EINVAL;

	/* Do we already reached all discrete framesizes? */
	if (frmi->index > isize)
		return -EINVAL;


	frmi->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	frmi->discrete.numerator =
		Hi253_frameintervals[frmi->index].numerator;
	frmi->discrete.denominator =
		Hi253_frameintervals[frmi->index].denominator;

	return 0;

}

static struct v4l2_int_ioctl_desc Hi253_ioctl_desc[] = {
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

static struct v4l2_int_slave Hi253_slave = {
	.ioctls		= Hi253_ioctl_desc,
	.num_ioctls	= ARRAY_SIZE(Hi253_ioctl_desc),
};

static struct v4l2_int_device Hi253_int_device = {
	.module	= THIS_MODULE,
	.name	= HI253_I2C_NAME,
	.priv	= &Hi253,
	.type	= v4l2_int_type_slave,
	.u	= {
		.slave = &Hi253_slave,
	},
};

/*
 * Hi253_probe - sensor driver i2c probe handler
 * @client: i2c driver client device structure
 *
 * Register sensor as an i2c client device and V4L2
 * device.
 */
	static int __init
Hi253_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct _sensor_chip_ *sensor = &Hi253;
	int err;

	DPRINTK_OV("entering \n");

	if (i2c_get_clientdata(client))
		return -EBUSY;


	sensor->pdata = client->dev.platform_data;

	if (!sensor->pdata) {
		dev_err(&client->dev, "No platform data?\n");
		return -ENODEV;
	}

	sensor->v4l2_int_device = &Hi253_int_device;
	sensor->i2c_client = client;

	i2c_set_clientdata(client, sensor);

#if 1
	/* Make the default preview format 320*240 YUV */
	sensor->pix.width = Hi253_sizes[HI253_QVGA].width;
	sensor->pix.height = Hi253_sizes[HI253_QVGA].height;
#else
	sensor->pix.width = Hi253_sizes[HI253_UXGA].width;
	sensor->pix.height = Hi253_sizes[HI253_UXGA].height;
#endif

	sensor->pix.pixelformat = V4L2_PIX_FMT_YUYV;

	err = v4l2_int_device_register(sensor->v4l2_int_device);
	if (err)
		i2c_set_clientdata(client, NULL);

	DPRINTK_OV("exit \n");
	return 0;
}

/*
 * Hi253_remove - sensor driver i2c remove handler
 * @client: i2c driver client device structure
 *
 * Unregister sensor as an i2c client device and V4L2
 * device. Complement of Hi253_probe().
 */
	static int __exit
Hi253_remove(struct i2c_client *client)
{
	struct _sensor_chip_ *sensor = i2c_get_clientdata(client);

	if (!client->adapter)
		return -ENODEV;	/* our client isn't attached */

	v4l2_int_device_unregister(sensor->v4l2_int_device);
	i2c_set_clientdata(client, NULL);

	return 0;
}

static const struct i2c_device_id Hi253_id[] = {
	{ HI253_I2C_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, Hi253_id);

static struct i2c_driver sensor_i2c_driver = {
	.driver = {
		.name	= HI253_I2C_NAME,
		.owner = THIS_MODULE,
	},
	.probe	= Hi253_probe,
	.remove	= __exit_p(Hi253_remove),
	.id_table = Hi253_id,
};

static struct _sensor_chip_ Hi253 = {
	.timeperframe = {
		.numerator = 1,
		.denominator = 15,
	},
	.state = SENSOR_NOT_DETECTED,
};

/*
 * Hi253sensor_init - sensor driver module_init handler
 *
 * Registers driver as an i2c client driver.  Returns 0 on success,
 * error code otherwise.
 */
static int __init Hi253sensor_init(void)
{
	int err;

	DPRINTK_OV("\n");
	err = i2c_add_driver(&sensor_i2c_driver);
	HI253_Init_Mode=1;
	if (err) {
		printk(KERN_ERR "Failed to register" HI253_I2C_NAME ".\n");
		return  err;
	}
	return 0;
}
late_initcall(Hi253sensor_init);

/*
 * Hi253sensor_cleanup - sensor driver module_exit handler
 *
 * Unregisters/deletes driver as an i2c client driver.
 * Complement of Hi253sensor_init.
 */
static void __exit Hi253sensor_cleanup(void)
{
	i2c_del_driver(&sensor_i2c_driver);
	HI253_Init_Mode=1;
}
module_exit(Hi253sensor_cleanup);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("_ camera sensor driver");
