/*
 * drivers/media/video/bf3703.c
 *
 * bf3703 sensor driver
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
#include <media/bf3703.h>
#include "omap34xxcam.h"
#include "isp/isp.h"

#define BF3703_DRIVER_NAME  "bf3703"

#define I2C_M_WR 0
#define BF3703_YUV_MODE 1

//#define BF3703_DEBUG 1

#ifdef BF3703_DEBUG
#define DPRINTK_BYD(format, ...)				\
	printk(KERN_ERR "BF3703:%s: " format " ",__func__, ## __VA_ARGS__)
#else
#define DPRINTK_BYD(format, ...)
#endif


/* Array of image sizes supported by BF3703.  These must be ordered from
 * smallest image size to largest.
 */
const static struct capture_size_bf3703 bf3703_sizes[] = {
	/*QVGA*/
	{320,240},
	/*VGA,capture & video*/
	{640,480},
};

const static struct bf3703_reg bf3703_effect_off[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_effect_mono[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_effect_negative[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_effect_sepia[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_effect_bluish[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_effect_green[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_effect_reddish[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_effect_yellowish[]={
	{0xff,0xff},
};

const static struct bf3703_reg* bf3703_effects[]={
	bf3703_effect_off,
	bf3703_effect_mono,
	bf3703_effect_negative,
	bf3703_effect_sepia,
	bf3703_effect_bluish,
	bf3703_effect_green,
	bf3703_effect_reddish,
	bf3703_effect_yellowish
};

const static struct bf3703_reg bf3703_wb_auto[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_wb_sunny[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_wb_cloudy[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_wb_office[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_wb_home[]={
	{0xff,0xff},
};

const static struct bf3703_reg* bf3703_whitebalance[]={
	bf3703_wb_auto,
	bf3703_wb_sunny,
	bf3703_wb_cloudy,
	bf3703_wb_office,
	bf3703_wb_home
};

const static struct bf3703_reg bf3703_brightness_1[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_brightness_2[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_brightness_3[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_brightness_4[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_brightness_5[]={
	{0xff,0xff},
};

const static struct bf3703_reg* bf3703_brightness[]={
	bf3703_brightness_1,
	bf3703_brightness_2,
	bf3703_brightness_3,
	bf3703_brightness_4,
	bf3703_brightness_5
};

const static struct bf3703_reg bf3703_contrast_1[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_contrast_2[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_contrast_3[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_contrast_4[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_contrast_5[]={
	{0xff,0xff},
};

const static struct bf3703_reg* bf3703_contrast[]={
	bf3703_contrast_1,
	bf3703_contrast_2,
	bf3703_contrast_3,
	bf3703_contrast_4,
	bf3703_contrast_5
};

const static struct bf3703_reg bf3703_saturation_1[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_saturation_2[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_saturation_3[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_saturation_4[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_saturation_5[]={
	{0xff,0xff},
};

/*
const static struct bf3703_reg* bf3703_saturation[]={
	bf3703_saturation_1,
	bf3703_saturation_2,
	bf3703_saturation_3,
	bf3703_saturation_4,
	bf3703_saturation_5
};
*/

const static struct bf3703_reg bf3703_exposure_1[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_exposure_2[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_exposure_3[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_exposure_4[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_exposure_5[]={
	{0xff,0xff},
};

const static struct bf3703_reg* bf3703_exposure[]={
	bf3703_exposure_1,
	bf3703_exposure_2,
	bf3703_exposure_3,
	bf3703_exposure_4,
	bf3703_exposure_5
};

const static struct bf3703_reg bf3703_sharpness_auto[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_sharpness_1[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_sharpness_2[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_sharpness_3[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_sharpness_4[]={
	{0xff,0xff},
};
const static struct bf3703_reg bf3703_sharpness_5[]={
	{0xff,0xff},
};

/*
const static struct bf3703_reg* bf3703_sharpness[]={
	bf3703_sharpness_auto,
	bf3703_sharpness_1,
	bf3703_sharpness_2,
	bf3703_sharpness_3,
	bf3703_sharpness_4,
	bf3703_sharpness_5
};
*/

const static struct bf3703_reg bf3703_qvga[]={
#if 0
	{0x17,0x28},
	{0x18,0x78},
	{0x19,0x1E},
	{0x1A,0x5a},
	{0x03,0x00},
#endif
	{0x11,0x90},
	{0x12,0x10},
	{0xff,0xff}
};

const static struct bf3703_reg bf3703_vga[]={
#if 0
	{0x17,0x00},
	{0x18,0xa0},
	{0x19,0x00},
	{0x1A,0x78},
	{0x03,0x00},
#endif
	{0x11,0x90},
	{0x12,0x00},
	{0xff,0xff}
};

const static struct bf3703_reg* bf3703_vga_to_xxx[] ={
	bf3703_qvga,
	bf3703_vga
};

const static struct bf3703_reg bf3703_common_vgabase[] = {
	/*VGA 640 * 480*/
	{0x11,0x80},
	{0x12,0x00},
	{0x09,0x00},
	{0x13,0x00},
	{0x01,0x13},
	{0x02,0x21},
	{0x8c,0x02},//01 :devided by 2  02 :devided by 1
	{0x8d,0x64},//32: devided by 2  64 :devided by 1
	{0x87,0x18},
	{0x13,0x07},

	//POLARITY of Signal
	{0x15,0x41},
	{0x3a,0x03},

	//black level ,¶ÔÉÏµçÆ«ÂÌÓÐ¸ÄÉÆ,Èç¹ûÐèÒªÇëÊ¹ÓÃ

	{0x05,0x1f},
	{0x06,0x60},
	{0x14,0x1f},
	{0x27,0x03},
	{0x06,0xe0},

	//lens shading
	{0x35,0x68},
	{0x65,0x68},
	{0x66,0x62},
	{0x36,0x05},
	{0x37,0xf6},
	{0x38,0x46},
	{0x9b,0xf6},
	{0x9c,0x46},
	{0xbc,0x01},
	{0xbd,0xf6},
	{0xbe,0x46},

	//AE
	{0x82,0x14},
	{0x83,0x23},
	{0x9a,0x23},//the same as 0x83
	{0x84,0x1a},
	{0x85,0x20},
	{0x89,0x04},//02 :devided by 2  04 :devided by 1
	{0x8a,0x08},//04: devided by 2  05 :devided by 1
	{0x86,0x28},
	{0x96,0xa6},//AE speed
	{0x97,0x0c},//AE speed
	{0x98,0x18},//AE speed
	//AE target
	{0x24,0x78},
	{0x25,0x88},
	{0x94,0x0a},//INT_OPEN
	{0x80,0x55},

	//denoise
	{0x70,0x6f},//denoise
	{0x72,0x4f},//denoise
	{0x73,0x2f},//denoise
	{0x74,0x27},//denoise
	{0x7a,0x4e},//denoise in  low light,0x8e\0x4e\0x0e
	{0x7b,0x28},//the same as 0x86

	//black level
	{0X1F,0x20},//G target
	{0X22,0x20},//R target
	{0X26,0x20},//B target
	//Ä£Äâ²¿·Ö²ÎÊý
	{0X16,0x00},//Èç¹û¾õµÃºÚÉ«ÎïÌå²»¹»ºÚ£¬ÓÐµãÆ«ºì£¬½«0x16Ð´Îª0x03»áÓÐµã¸ÄÉÆ
	{0xbb,0x20},  // deglitch  ¶ÔxclkÕûÐÎ
	{0xeb,0x30},
	{0xf5,0x21},
	{0xe1,0x3c},
	{0xbb,0x20},
	{0X2f,0X66},
	{0x06,0xe0},

	//anti black sun spot
	{0x61,0xd3},//0x61[3]=0 black sun disable
	{0x79,0x48},//0x79[7]=0 black sun disable

	//Gamma

	{0x3b,0x60},//auto gamma offset adjust in  low light
	{0x3c,0x20},//auto gamma offset adjust in  low light
	{0x56,0x40},
	{0x39,0x80},
	//gamma1
	{0x3f,0xb0},
	{0X40,0X88},
	{0X41,0X74},
	{0X42,0X5E},
	{0X43,0X4c},
	{0X44,0X44},
	{0X45,0X3E},
	{0X46,0X39},
	{0X47,0X35},
	{0X48,0X31},
	{0X49,0X2E},
	{0X4b,0X2B},
	{0X4c,0X29},
	{0X4e,0X25},
	{0X4f,0X22},
	{0X50,0X1F},

	/*gamma2
	{0x3f,0xb0},
	{0X40,0X9b},
	{0X41,0X88},
	{0X42,0X6e},
	{0X43,0X59},
	{0X44,0X4d},
	{0X45,0X45},
	{0X46,0X3e},
	{0X47,0X39},
	{0X48,0X35},
	{0X49,0X31},
	{0X4b,0X2e},
	{0X4c,0X2b},
	{0X4e,0X26},
	{0X4f,0X23},
	{0X50,0X1F},
	*/
	/*gamma3
	{0X3f,0Xb0},
	{0X40,0X60},
	{0X41,0X60},
	{0X42,0X66},
	{0X43,0X57},
	{0X44,0X4c},
	{0X45,0X43},
	{0X46,0X3c},
	{0X47,0X37},
	{0X48,0X33},
	{0X49,0X2f},
	{0X4b,0X2c},
	{0X4c,0X29},
	{0X4e,0X25},
	{0X4f,0X22},
	{0X50,0X20},

	//gamma 4	low noise
	{0X3f,0Xa8},
	{0X40,0X48},
	{0X41,0X54},
	{0X42,0X4E},
	{0X43,0X44},
	{0X44,0X3E},
	{0X45,0X39},
	{0X46,0X34},
	{0X47,0X30},
	{0X48,0X2D},
	{0X49,0X2A},
	{0X4b,0X28},
	{0X4c,0X26},
	{0X4e,0X22},
	{0X4f,0X20},
	{0X50,0X1E},
	*/

	//color matrix
	{0x51,0x08},
	{0x52,0x3a},
	{0x53,0x32},
	{0x54,0x12},
	{0x57,0x7f},
	{0x58,0x6d},
	{0x59,0x50},
	{0x5a,0x5d},
	{0x5b,0x0d},
	{0x5D,0x95},
	{0x5C,0x0e},

	/*
	{0x51,0x00},
	{0x52,0x15},
	{0x53,0x15},
	{0x54,0x12},
	{0x57,0x7d},
	{0x58,0x6a},
	{0x59,0x5c},
	{0x5a,0x87},
	{0x5b,0x2b},
	{0x5D,0x95},
	{0x5C,0x0e},
	//

	//
	{0x51,0x0d},
	{0x52,0x2b},
	{0x53,0x1e},
	{0x54,0x15},
	{0x57,0x92},
	{0x58,0x7d},
	{0x59,0x5f},
	{0x5a,0x74},
	{0x5b,0x15},
	{0x5c,0x0e},
	{0x5d,0x95},//0x5c[3:0] low light color coefficient£¬smaller ,lower noise

	//
	{0x51,0x08},
	{0x52,0x0E},
	{0x53,0x06},
	{0x54,0x12},
	{0x57,0x82},
	{0x58,0x70},
	{0x59,0x5C},
	{0x5a,0x77},
	{0x5b,0x1B},
	{0x5c,0x0e},//0x5c[3:0] low light color coefficient£¬smaller ,lower noise
	{0x5d,0x95},

	//color
	{0x51,0x03},
	{0x52,0x0d},
	{0x53,0x0b},
	{0x54,0x14},
	{0x57,0x59},
	{0x58,0x45},
	{0x59,0x41},
	{0x5a,0x5f},
	{0x5b,0x1e},
	{0x5c,0x0e},//0x5c[3:0] low light color coefficient£¬smaller ,lower noise
	{0x5d,0x95},
	*/

	{0x60,0x20},//color open in low light
	//AWB
	{0x6a,0x81},
	{0x23,0x77},//Green gain
	{0xa0,0x03},

	{0xa1,0X31},
	{0xa2,0X0e},
	{0xa3,0X26},
	{0xa4,0X0d},
	{0xa5,0x26},
	{0xa6,0x06},
	{0xa7,0x80},//BLUE Target
	{0xa8,0x7c},//RED Target
	{0xa9,0x28},
	{0xaa,0x28},
	{0xab,0x28},
	{0xac,0x3c},
	{0xad,0xf0},
	{0xc8,0x18},
	{0xc9,0x20},
	{0xca,0x17},
	{0xcb,0x1f},
	{0xaf,0x00},
	{0xc5,0x18},
	{0xc6,0x00},
	{0xc7,0x20},
	{0xae,0x83},
	{0xcc,0x3c},
	{0xcd,0x90},
	{0xee,0x4c},// P_TH

	// color saturation
	{0xb0,0xd0},
	{0xb1,0xc0},
	{0xb2,0xa8},
	{0xb3,0x8a},

	//anti webcamera banding
	{0x9d,0x4c},

	//switch direction
	{0x1e,0x10},//00:normal  10:IMAGE_V_MIRROR   20:IMAGE_H_MIRROR  30:IMAGE_HV_MIRROR

	{0x11,0x90},//MCLK = 2PCLK
	{0x15,0x00},//HSYNC->HREF
	{0x3a,0x00},//HSYNC->HREF
	{0x13,0x07},// Turn ON AEC/AGC/AWB
	{0xFF,0xFF}
};

const static struct bf3703_reg bf3703_15fps[] = {
	{0xff,0xff},
	//adjust to 15fps
};

const static struct bf3703_reg bf3703_7_5fps[] = {
	{0xff,0xff},
	//adjust to 7.5fps
};

const static struct bf3703_reg bf3703_svga_to_uxga[] = {
	{0xff,0xff},
};

/*YUYV 422 (16 bits /pixel)*/
const static struct bf3703_reg bf3703_yuv422[] = {
	{0x12,0x00},
	/*YUV422(Reg 0x3a)
	*0x00:YUYV
	*0x01:YVYU
	*0x02:UYVY
	*0x03:VYUY
	*/
	{0x3a,0x00},
	{0xFF,0xFF}
};

const static struct bf3703_reg bf3703_svga_to_sqcif[]={
	{0xff,0xff},
};

const static struct bf3703_reg bf3703_svga_to_vga[] = {{0xff,0xff},};

const static struct bf3703_reg bf3703_svga_to_cif[] = {{0xff,0xff},};

const static struct bf3703_reg bf3703_svga_to_qvga[] = {{0xff,0xff},};

const static struct bf3703_reg bf3703_svga_to_qcif[] = {{0xff,0xff},};

/*SVGA(800*600) to dvd-video ntsc(720*480)*/
const static struct bf3703_reg bf3703_svga_to_dvd_ntsc[] = {{0xff,0xff},};

/*SVGA(800*600) to SVGA,for consistence*/
const static struct bf3703_reg bf3703_svga_to_svga[] = {{0xff,0xff},};

/*SVGA(800*600) to XGA(1024*768)*/
const static struct bf3703_reg bf3703_svga_to_xga[] = {{0xff,0xff},};

/*SVGA(800*600) to 1M(1280*960)*/
const static struct bf3703_reg bf3703_svga_to_1M[] = {{0xff,0xff},};

/*
const static struct bf3703_reg* bf3703_svga_to_xxx[] ={
	bf3703_svga_to_sqcif,
	bf3703_svga_to_qcif,
	bf3703_svga_to_qvga,
	bf3703_svga_to_cif,
	bf3703_svga_to_vga,
	bf3703_svga_to_dvd_ntsc,
	bf3703_svga_to_svga,
	bf3703_svga_to_xga,
	bf3703_svga_to_1M,
	bf3703_svga_to_uxga
};
*/

static struct bf3703_sensor bf3703;
static struct i2c_driver bf3703sensor_i2c_driver;
static unsigned long xclk_current = BF3703_XCLK_MIN;
static enum v4l2_power current_power_state;

static int bf3703_capture_mode = 0;


/* List of image formats supported by BF3703sensor */
const static struct v4l2_fmtdesc bf3703_formats[] = {
#if defined(BF3703_RAW_MODE)
	{
		.description	= "RAW10",
		.pixelformat	= V4L2_PIX_FMT_SGRBG10,
	},
#elif defined(BF3703_YUV_MODE)
	{
		.description	= "YUYV,422",
		.pixelformat	= V4L2_PIX_FMT_UYVY,
	},
#else

	{
		.description	= "YUYV (YUV 4:2:2), packed",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
	},
	{
		.description	= "UYVY, packed",
		.pixelformat	= V4L2_PIX_FMT_UYVY,
	},
#endif
};

#define NUM_CAPTURE_FORMATS (sizeof(bf3703_formats) / sizeof(bf3703_formats[0]))




/*
 * struct vcontrol - Video controls
 * @v4l2_queryctrl: V4L2 VIDIOC_QUERYCTRL ioctl structure
 * @current_value: current value of this control
 */
static struct vcontrol {
	struct v4l2_queryctrl qc;
	int current_value;
} video_control[] = {
#ifdef BF3703_YUV_MODE
	{
		{
		.id = V4L2_CID_BRIGHTNESS,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Brightness",
		.minimum = BF3703_MIN_BRIGHT,
		.maximum = BF3703_MAX_BRIGHT,
		.step = BF3703_BRIGHT_STEP,
		.default_value = BF3703_DEF_BRIGHT,
		},
	.current_value = BF3703_DEF_BRIGHT,
	},
	{
		{
		.id = V4L2_CID_CONTRAST,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Contrast",
		.minimum = BF3703_MIN_CONTRAST,
		.maximum = BF3703_MAX_CONTRAST,
		.step = BF3703_CONTRAST_STEP,
		.default_value = BF3703_DEF_CONTRAST,
		},
	.current_value = BF3703_DEF_CONTRAST,
	},
	{
		{
		.id = V4L2_CID_PRIVATE_BASE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Color Effects",
		.minimum = BF3703_MIN_COLOR,
		.maximum = BF3703_MAX_COLOR,
		.step = BF3703_COLOR_STEP,
		.default_value = BF3703_DEF_COLOR,
		},
	.current_value = BF3703_DEF_COLOR,
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
 * Read a value from a register in bf3703 sensor device.
 * The value is returned in 'val'.
 * Returns zero if successful, or non-zero otherwise.
 */
/*
 * Read a value from a register in Hi253 sensor device.
 * The value is returned in 'val'.
 * Returns zero if successful, or non-zero otherwise.
 */

static int bf3703_read_reg(struct i2c_client *client, u8 reg)
{

	int ret;

	client->addr= BF3703_I2C_ADDR;

	ret = i2c_smbus_read_byte_data(client, reg);
	return ret;
}


/* Write a value to a register in bf3703 sensor device.
 * @client: i2c driver client structure.
 * @reg: Address of the register to read value from.
 * @val: Value to be written to a specific register.
 * Returns zero if successful, or non-zero otherwise.
 */
static int bf3703_write_reg(struct i2c_client *client, u16 reg, u8 val)
{
	int err = 0;
	struct i2c_msg msg[1];
	unsigned char data[2];
	int retries = 0;

	if (!client->adapter)
		return -ENODEV;
retry:

	  msg->addr = BF3703_I2C_ADDR;

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
		DPRINTK_BYD( "Retrying I2C(%#x,%#x)... %d", reg,val,retries);
		retries++;
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(msecs_to_jiffies(20));
		goto retry;
	}

	return err;
}

/*
 * Initialize a list of bf3703 registers.
 * The list of registers is terminated by the pair of values
 * {BF3703_REG_TERM, BF3703_VAL_TERM}.
 * @client: i2c driver client structure.
 * @reglist[]: List of address of the registers to write data.
 * Returns zero if successful, or non-zero otherwise.
 */
static int bf3703_write_regs(struct i2c_client *client,
					const struct bf3703_reg reglist[])
{
	int err = 0;
	const struct bf3703_reg *next = reglist;

	while (!((next->reg == BF3703_REG_TERM)
		&& (next->val == BF3703_VAL_TERM))) {
		err = bf3703_write_reg(client, next->reg, next->val);
		//udelay(100);
		if(next->reg == 0x0000){
			mdelay(next->val);
		}
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
static enum image_size_bf3703
bf3703_find_size(unsigned int width, unsigned int height)
{

	enum image_size_bf3703 isize;

	for (isize = 0; isize < ARRAY_SIZE(bf3703_sizes); isize++) {
		if ((bf3703_sizes[isize].height >= height) &&
			(bf3703_sizes[isize].width >= width)) {
			break;
		}
	}
	DPRINTK_BYD("width = %d,height = %d,return %d\n",width,height,isize);
	return isize;
}


/* Detect if an bf3703 is present, returns a negative error number if no
 * device is detected, or pidl as version number if a device is detected.
 */
static int bf3703_detect(struct i2c_client *client)
{
	u8 pidl = 0,pidh = 0;
	DPRINTK_BYD("\n");

	if (!client)
		return -ENODEV;

	if ((pidh = bf3703_read_reg(client,BF3703_PIDH)) < 0)
		return -ENODEV;

	if ((pidl = bf3703_read_reg(client,BF3703_PIDL)) < 0)
		return -ENODEV;

	DPRINTK_BYD( "Detect(%02X,%02X)\n", pidh,pidl);

	return pidl;
}

/*
 * Configure the bf3703 for a specified image size, pixel format, and frame
 * period.  xclk is the frequency (in Hz) of the xclk input to the BF3703.
 * fper is the frame period (in seconds) expressed as a fraction.
 * Returns zero if successful, or non-zero otherwise.
 * The actual frame period is returned in fper.
 */
static int bf3703_configure(struct v4l2_int_device *s)
{
	struct bf3703_sensor *sensor = s->priv;
	struct v4l2_pix_format *pix = &sensor->pix;
	struct i2c_client *client = sensor->i2c_client;
	enum image_size_bf3703 isize = ARRAY_SIZE(bf3703_sizes) - 1;

	int  err = 0;
	enum pixel_format_bf3703 pfmt = BF3703_YUV;

	switch (pix->pixelformat) {
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_UYVY:
	default:
		pfmt = BF3703_YUV;
	}

	isize = bf3703_find_size(pix->width, pix->height);

	if(bf3703_capture_mode){
		bf3703_write_regs(client, bf3703_vga_to_xxx[isize]);
		bf3703_capture_mode = 0;
	}else{
		bf3703_write_regs(client, bf3703_common_vgabase);
		bf3703_write_regs(client, bf3703_vga_to_xxx[isize]);
		/*YUYV output (16bits)*/
		bf3703_write_regs(client, bf3703_yuv422);
	}

	DPRINTK_BYD("pix->width = %d,pix->height = %d\n",pix->width,pix->height);
	DPRINTK_BYD("format = %d,size = %d\n",pfmt,isize);

	/* Store image size */
	sensor->width = pix->width;
	sensor->height = pix->height;

	sensor->crop_rect.left = 0;
	sensor->crop_rect.width = pix->width;
	sensor->crop_rect.top = 0;
	sensor->crop_rect.height = pix->height;

	return err;
}

/* To get the cropping capabilities of bf3703 sensor
 * Returns zero if successful, or non-zero otherwise.
 */
static int ioctl_cropcap(struct v4l2_int_device *s,
						struct v4l2_cropcap *cropcap)
{
	struct bf3703_sensor *sensor = s->priv;

	DPRINTK_BYD("sensor->width = %ld,sensor->height = %ld\n",sensor->width,sensor->height);
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

/* To get the current crop window for of bf3703 sensor
 * Returns zero if successful, or non-zero otherwise.
 */
static int ioctl_g_crop(struct v4l2_int_device *s, struct  v4l2_crop *crop)
{
	struct bf3703_sensor *sensor = s->priv;
	   DPRINTK_BYD("\n");
	crop->c = sensor->crop_rect;
	return 0;
}

/* To set the crop window for of bf3703 sensor
 * Returns zero if successful, or non-zero otherwise.
 */
static int ioctl_s_crop(struct v4l2_int_device *s, struct  v4l2_crop *crop)
{
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
	struct bf3703_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	struct bf3703_ext_params *extp = &(sensor->ext_params);
//	unsigned int reg3306 = 0;
	return 0;
#if 0
	if(vc->id != V4L2_CID_CAPTURE)
		return 0;
#endif

	switch(vc->id){
		case V4L2_CID_CAPTURE:
			DPRINTK_BYD("V4L2_CID_CAPTURE:value = %d\n",vc->value);
			bf3703_capture_mode = 1;
			break;
		case V4L2_CID_EFFECT:
			DPRINTK_BYD("V4L2_CID_EFFECT:value = %d\n",vc->value);
			if(vc->value < 0 || vc->value > ARRAY_SIZE(bf3703_effects))
				vc->value = BF3703_DEF_EFFECT;
			 extp->effect = vc->value;
			if(current_power_state == V4L2_POWER_ON){
				err = bf3703_write_regs(client, bf3703_effects[vc->value]);
				}
			break;
		case V4L2_CID_SCENE:
			DPRINTK_BYD("V4L2_CID_EFFECT:value = %d\n",vc->value);
			break;
		case V4L2_CID_EXPOSURE:
			DPRINTK_BYD("V4L2_CID_EXPOSURE:value = %d\n",vc->value);
			if(vc->value < 0 || vc->value > ARRAY_SIZE(bf3703_exposure))
				vc->value = BF3703_DEF_EXPOSURE;
			extp->exposure= vc->value;
				if(current_power_state == V4L2_POWER_ON){
				err = bf3703_write_regs(client, bf3703_exposure[vc->value - 1]);
				}
			break;
		case V4L2_CID_CONTRAST:
			DPRINTK_BYD("V4L2_CID_CONTRAST:value = %d\n",vc->value);
			if(vc->value < 0 || vc->value > ARRAY_SIZE(bf3703_contrast))
				vc->value = BF3703_DEF_CONTRAST;
			extp->contrast = vc->value;
				if(current_power_state == V4L2_POWER_ON){
				err = bf3703_write_regs(client, bf3703_contrast[vc->value - 1]);
				}
			break;
		case V4L2_CID_BRIGHTNESS:
			DPRINTK_BYD("V4L2_CID_BRIGHTNESS:value = %d\n",vc->value);
			if(vc->value < 0 || vc->value > ARRAY_SIZE(bf3703_brightness))
				vc->value = BF3703_DEF_BRIGHT;
			 extp->brightness = vc->value;
				if(current_power_state == V4L2_POWER_ON){
				err = bf3703_write_regs(client, bf3703_brightness[vc->value - 1]);
			}
			break;
		case V4L2_CID_DO_WHITE_BALANCE:
			DPRINTK_BYD("V4L2_CID_DO_WHITE_BALANCE:value = %d\n",vc->value);
			if(vc->value < 0 || vc->value > ARRAY_SIZE(bf3703_whitebalance)){}

		  break;
	}
	DPRINTK_BYD("%s return %d\n",__func__,err);
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
	DPRINTK_BYD("index = %d,type = %d\n",index,type);
	switch (fmt->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		if (index >= NUM_CAPTURE_FORMATS)
			return -EINVAL;
	break;
	default:
		return -EINVAL;
	}

	fmt->flags = bf3703_formats[index].flags;
	strlcpy(fmt->description, bf3703_formats[index].description,
		sizeof(fmt->description));
	fmt->pixelformat = bf3703_formats[index].pixelformat;

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
	enum image_size_bf3703 isize = ARRAY_SIZE(bf3703_sizes) - 1;
	struct v4l2_pix_format *pix = &f->fmt.pix;

	DPRINTK_BYD("v4l2_pix_format(w:%d,h:%d)\n",pix->width,pix->height);

	if (pix->width > bf3703_sizes[isize].width)
		pix->width = bf3703_sizes[isize].width;
	if (pix->height > bf3703_sizes[isize].height)
		pix->height = bf3703_sizes[isize].height;

	isize = bf3703_find_size(pix->width, pix->height);
	pix->width = bf3703_sizes[isize].width;
	pix->height = bf3703_sizes[isize].height;

	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (pix->pixelformat == bf3703_formats[ifmt].pixelformat)
			break;
	}
	if (ifmt == NUM_CAPTURE_FORMATS)
		ifmt = 0;
	pix->pixelformat = bf3703_formats[ifmt].pixelformat;
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
	struct bf3703_sensor *sensor = s->priv;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	int rval;

	DPRINTK_BYD("pix.pixformat = %x \n",pix->pixelformat);
	DPRINTK_BYD("pix.width = %d \n",pix->width);
	DPRINTK_BYD("pix.height = %d \n",pix->height);

	rval = ioctl_try_fmt_cap(s, f);
	if (rval)
		return rval;

	sensor->pix = *pix;
	sensor->width = sensor->pix.width;
	sensor->height = sensor->pix.height;
	DPRINTK_BYD("sensor->pix.pixformat = %x \n",sensor->pix.pixelformat);
	DPRINTK_BYD("sensor->pix.width = %d \n",sensor->pix.width);
	DPRINTK_BYD("sensor->pix.height = %d \n",sensor->pix.height);

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
	struct bf3703_sensor *sensor = s->priv;
	f->fmt.pix = sensor->pix;
	DPRINTK_BYD("f->fmt.pix.pixformat = %x,V4L2_PIX_FMT_YUYV = %x\n",f->fmt.pix.pixelformat,V4L2_PIX_FMT_YUYV);
	DPRINTK_BYD("f->fmt.pix.width = %d \n",f->fmt.pix.width);
	DPRINTK_BYD("f->fmt.pix.height = %d \n",f->fmt.pix.height);

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
	struct bf3703_sensor *sensor = s->priv;
	struct v4l2_captureparm *cparm = &a->parm.capture;

	DPRINTK_BYD("\n");

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
	struct bf3703_sensor *sensor = s->priv;
	struct v4l2_fract *timeperframe = &a->parm.capture.timeperframe;
	struct v4l2_fract timeperframe_old;
	int desired_fps;
	timeperframe_old = sensor->timeperframe;
	sensor->timeperframe = *timeperframe;

	desired_fps = timeperframe->denominator / timeperframe->numerator;
	if ((desired_fps < BF3703_MIN_FPS) || (desired_fps > BF3703_MAX_FPS))
		rval = -EINVAL;

	if (rval)
		sensor->timeperframe = timeperframe_old;
	else
		*timeperframe = sensor->timeperframe;

	DPRINTK_BYD("frame rate = %d\n",desired_fps);

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
	struct bf3703_sensor *sensor = s->priv;

	return sensor->pdata->priv_data_set(s,p);
}



static int __bf3703_power_off_standby(struct v4l2_int_device *s,
					  enum v4l2_power on)
{
	struct bf3703_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int rval;

	rval = sensor->pdata->power_set(s, on);
	if (rval < 0) {
		v4l_err(client, "Unable to set the power state: "
			BF3703_DRIVER_NAME " sensor\n");
		return rval;
	}

	sensor->pdata->set_xclk(s, 0);
	return 0;
}

static int bf3703_power_off(struct v4l2_int_device *s)
{
	return __bf3703_power_off_standby(s, V4L2_POWER_OFF);
}

static int bf3703_power_standby(struct v4l2_int_device *s)
{
	return __bf3703_power_off_standby(s, V4L2_POWER_STANDBY);
}



static int bf3703_power_on(struct v4l2_int_device *s)
{
	struct bf3703_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int rval;

	sensor->pdata->set_xclk(s, xclk_current);

	rval = sensor->pdata->power_set(s, V4L2_POWER_ON);
	if (rval < 0) {
		v4l_err(client, "Unable to set the power state: "
			BF3703_DRIVER_NAME " sensor\n");
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
		if(!bf3703_capture_mode)
			bf3703_power_on(s);
		/*kunlun p1 can't support standby*/
		bf3703_configure(s);
		break;
	case V4L2_POWER_OFF:
		if(!bf3703_capture_mode)
			bf3703_power_off(s);
		break;
	case V4L2_POWER_STANDBY:
		if(!bf3703_capture_mode)
			bf3703_power_standby(s);
		break;
	}
	current_power_state = on;

	return 0;
}

/*
 * ioctl_init - V4L2 sensor interface handler for VIDIOC_INT_INIT
 * @s: pointer to standard V4L2 device structure
 *
 * Initialize the sensor device (call bf3703_configure())
 */
static int ioctl_init(struct v4l2_int_device *s)
{
	DPRINTK_BYD("\n");
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

	DPRINTK_BYD("\n");
	return 0;
}

/**
 * ioctl_dev_init - V4L2 sensor interface handler for vidioc_int_dev_init_num
 * @s: pointer to standard V4L2 device structure
 *
 * Initialise the device when slave attaches to the master.  Returns 0 if
 * bf3703 device could be found, otherwise returns appropriate error.
 */
static int ioctl_dev_init(struct v4l2_int_device *s)
{
	struct bf3703_sensor *sensor = s->priv;
	struct i2c_client *c = sensor->i2c_client;
	int err;

	err = bf3703_power_on(s);
	if (err)
		return -ENODEV;

	err = bf3703_detect(c);
	if (err < 0) {
		DPRINTK_BYD("Unable to detect " BF3703_DRIVER_NAME
								" sensor\n");
		/*
		 * Turn power off before leaving this function
		 * If not, CAM powerdomain will on
		*/
		bf3703_power_off(s);
		return err;
	}

	sensor->ver = err;
	DPRINTK_BYD(" chip version 0x%02x detected\n",
								sensor->ver);
	err = bf3703_power_off(s);
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

	DPRINTK_BYD("index=%d\n",frms->index);
	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (frms->pixel_format == bf3703_formats[ifmt].pixelformat)
			break;
	}
	/* Is requested pixelformat not found on sensor? */
	if (ifmt == NUM_CAPTURE_FORMATS)
		return -EINVAL;

	/* Do we already reached all discrete framesizes? */
	if (frms->index >= ARRAY_SIZE(bf3703_sizes))
		return -EINVAL;

	frms->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	frms->discrete.width = bf3703_sizes[frms->index].width;
	frms->discrete.height = bf3703_sizes[frms->index].height;

	DPRINTK_BYD("discrete(%d,%d)\n",frms->discrete.width,frms->discrete.height);
	return 0;
}

/*QCIF,QVGA,CIF,VGA,SVGA can support up to 30fps*/
const struct v4l2_fract bf3703_frameintervals[] = {
  { .numerator = 1, .denominator = 15 },
  { .numerator = 1, .denominator = 15 },
};

static int ioctl_enum_frameintervals(struct v4l2_int_device *s,
					struct v4l2_frmivalenum *frmi)
{
	int ifmt;
	int isize = ARRAY_SIZE(bf3703_frameintervals) - 1;

	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (frmi->pixel_format == bf3703_formats[ifmt].pixelformat)
			break;
	}

	DPRINTK_BYD("frmi->width = %d,frmi->height = %d,frmi->index = %d,ifmt = %d\n",
		frmi->width,frmi->height,frmi->index,ifmt);

	/* Is requested pixelformat not found on sensor? */
	if (ifmt == NUM_CAPTURE_FORMATS)
		return -EINVAL;

	/* Do we already reached all discrete framesizes? */
	if (frmi->index > isize)
		return -EINVAL;


	frmi->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	frmi->discrete.numerator =
				bf3703_frameintervals[frmi->index].numerator;
	frmi->discrete.denominator =
				bf3703_frameintervals[frmi->index].denominator;

	return 0;

}

static struct v4l2_int_ioctl_desc bf3703_ioctl_desc[] = {
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

static struct v4l2_int_slave bf3703_slave = {
	.ioctls		= bf3703_ioctl_desc,
	.num_ioctls	= ARRAY_SIZE(bf3703_ioctl_desc),
};

static struct v4l2_int_device bf3703_int_device = {
	.module	= THIS_MODULE,
	.name	= BF3703_DRIVER_NAME,
	.priv	= &bf3703,
	.type	= v4l2_int_type_slave,
	.u	= {
		.slave = &bf3703_slave,
	},
};

/*
 * bf3703_probe - sensor driver i2c probe handler
 * @client: i2c driver client device structure
 *
 * Register sensor as an i2c client device and V4L2
 * device.
 */
static int __init
bf3703_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct bf3703_sensor *sensor = &bf3703;
	int err;

	DPRINTK_BYD("entering \n");

	if (i2c_get_clientdata(client))
		return -EBUSY;

	sensor->pdata = client->dev.platform_data;

	if (!sensor->pdata) {
		dev_err(&client->dev, "No platform data?\n");
		return -ENODEV;
	}

	sensor->v4l2_int_device = &bf3703_int_device;
	sensor->i2c_client = client;

	i2c_set_clientdata(client, sensor);

	/* Make the default preview format 320*240 YUV */
	sensor->pix.width = bf3703_sizes[BF3703_VGA].width;
	sensor->pix.height = bf3703_sizes[BF3703_VGA].height;


	sensor->pix.pixelformat = V4L2_PIX_FMT_YUYV;

	err = v4l2_int_device_register(sensor->v4l2_int_device);
	if (err)
		i2c_set_clientdata(client, NULL);

	DPRINTK_BYD("exit \n");
	return 0;
}

/*
 * bf3703_remove - sensor driver i2c remove handler
 * @client: i2c driver client device structure
 *
 * Unregister sensor as an i2c client device and V4L2
 * device. Complement of bf3703_probe().
 */
static int __exit
bf3703_remove(struct i2c_client *client)
{
	struct bf3703_sensor *sensor = i2c_get_clientdata(client);

	if (!client->adapter)
		return -ENODEV;	/* our client isn't attached */

	v4l2_int_device_unregister(sensor->v4l2_int_device);
	i2c_set_clientdata(client, NULL);

	return 0;
}

static const struct i2c_device_id bf3703_id[] = {
	{ BF3703_DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, bf3703_id);

static struct i2c_driver bf3703sensor_i2c_driver = {
	.driver = {
		.name	= BF3703_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
	.probe	= bf3703_probe,
	.remove	= __exit_p(bf3703_remove),
	.id_table = bf3703_id,
};

static struct bf3703_sensor bf3703 = {
	.timeperframe = {
		.numerator = 1,
		.denominator = 15,
	},
	.state = SENSOR_NOT_DETECTED,
};

/*
 * bf3703sensor_init - sensor driver module_init handler
 *
 * Registers driver as an i2c client driver.  Returns 0 on success,
 * error code otherwise.
 */
static int __init bf3703sensor_init(void)
{
	int err;

	DPRINTK_BYD("\n");
	err = i2c_add_driver(&bf3703sensor_i2c_driver);
	if (err) {
		printk(KERN_ERR "Failed to register" BF3703_DRIVER_NAME ".\n");
		return  err;
	}
	return 0;
}
late_initcall(bf3703sensor_init);

/*
 * bf3703sensor_cleanup - sensor driver module_exit handler
 *
 * Unregisters/deletes driver as an i2c client driver.
 * Complement of bf3703sensor_init.
 */
static void __exit bf3703sensor_cleanup(void)
{
	i2c_del_driver(&bf3703sensor_i2c_driver);
}
module_exit(bf3703sensor_cleanup);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("BF3703 camera sensor driver");
