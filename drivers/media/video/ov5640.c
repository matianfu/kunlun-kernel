/*
 * drivers/media/video/ov5640.c
 *
 * Sony ov5640 sensor driver
 *
 * Copyright (C) 2008 Hewlett Packard
 *
 * Leverage mt9p012.c
 *
 * Contacts:
 *   Atanas Filipov <afilipov@mm-sol.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/i2c.h>
#include <linux/delay.h>

#include <media/v4l2-int-device.h>
#include <media/ov5640.h>

#include "ov5640_regs.h"
#include "omap34xxcam.h"
#include "isp/isp.h"
#include "isp/ispcsi2.h"

#define OV5640_DRIVER_NAME  "ov5640"
#define OV5640_MOD_NAME "OV5640: "
#define ov5640_I2C_ADDR_W        0x3c//0x78
#define ov5640_I2C_ADDR_R         0x3c//0x79

#define I2C_M_WR 0

//#define OV5640_XCLK_NOM_1	24000000

const static struct ov5640_reg initial_list[] = {
{0x3103, 0x11, I2C_8BIT}, 
{0x3008, 0x82, I2C_8BIT},
{0x3008, 0x42, I2C_8BIT},
{0x3103, 0x03, I2C_8BIT},
//{0x3017, 0xec, I2C_8BIT},
{0x3017, 0xef, I2C_8BIT},
//{0x3018, 0xc0, I2C_8BIT},
{0x3018, 0xc3, I2C_8BIT},
{0x3034, 0x1a, I2C_8BIT},
{0x3035, 0x11, I2C_8BIT},
//{0x3036, 0x54, I2C_8BIT},
//vga milt=18
{0x3036, 0x12, I2C_8BIT},
{0x3037, 0x13, I2C_8BIT},
{0x3108, 0x01, I2C_8BIT},
{0x3630, 0x36, I2C_8BIT},
{0x3631, 0x0e, I2C_8BIT},
{0x3632, 0xe2, I2C_8BIT},
{0x3633, 0x12, I2C_8BIT},
{0x3621, 0xe0, I2C_8BIT},
{0x3704, 0xa0, I2C_8BIT},
{0x3703, 0x5a, I2C_8BIT},
{0x3715, 0x78, I2C_8BIT},
{0x3717, 0x01, I2C_8BIT},
{0x370b, 0x60, I2C_8BIT},
{0x3705, 0x1a, I2C_8BIT},
{0x3905, 0x02, I2C_8BIT},
{0x3906, 0x10, I2C_8BIT},
{0x3901, 0x0a, I2C_8BIT},
{0x3731, 0x12, I2C_8BIT},
{0x3600, 0x08, I2C_8BIT},
{0x3601, 0x33, I2C_8BIT},
{0x302d, 0x60, I2C_8BIT},
{0x3620, 0x52, I2C_8BIT},
{0x371b, 0x20, I2C_8BIT},
{0x471c, 0x50, I2C_8BIT},
{0x3a13, 0x43, I2C_8BIT},
{0x3a18, 0x00, I2C_8BIT},
{0x3a19, 0xf8, I2C_8BIT},
{0x3635, 0x13, I2C_8BIT},
{0x3636, 0x03, I2C_8BIT},
{0x3634, 0x40, I2C_8BIT},
{0x3622, 0x01, I2C_8BIT},
{0x3c01, 0x34, I2C_8BIT},
{0x3c04, 0x28, I2C_8BIT},
{0x3c05, 0x98, I2C_8BIT},
{0x3c06, 0x00, I2C_8BIT},
{0x3c07, 0x07, I2C_8BIT},
{0x3c08, 0x00, I2C_8BIT},
{0x3c09, 0x1c, I2C_8BIT},
{0x3c0a, 0x9c, I2C_8BIT},
{0x3c0b, 0x40, I2C_8BIT},
{0x3820, 0x40, I2C_8BIT},
{0x3821, 0x06, I2C_8BIT},
/*{0x3814, 0x11, I2C_8BIT}, */
{0x3814, 0x31, I2C_8BIT},
/*{0x3815, 0x11, I2C_8BIT}, */
{0x3815, 0x31, I2C_8BIT},
{0x3800, 0x00, I2C_8BIT},
{0x3801, 0x00, I2C_8BIT},
{0x3802, 0x00, I2C_8BIT},
{0x3803, 0x00, I2C_8BIT},
{0x3804, 0x0a, I2C_8BIT},
{0x3805, 0x3f, I2C_8BIT},
{0x3806, 0x07, I2C_8BIT},
{0x3807, 0x9f, I2C_8BIT},
{0x3808, 0x0a, I2C_8BIT},
{0x3809, 0x20, I2C_8BIT},
{0x380a, 0x07, I2C_8BIT},
{0x380b, 0x98, I2C_8BIT},
{0x380c, 0x0b, I2C_8BIT},
{0x380d, 0x1c, I2C_8BIT},
{0x380e, 0x07, I2C_8BIT},
{0x380f, 0xb0, I2C_8BIT},
{0x3810, 0x00, I2C_8BIT},
{0x3811, 0x10, I2C_8BIT},
{0x3812, 0x00, I2C_8BIT},
{0x3813, 0x04, I2C_8BIT},
//{0x3618, 0x04, I2C_8BIT},
{0x3618, 0x00, I2C_8BIT},
//{0x3612, 0x2b, I2C_8BIT},
{0x3612, 0x29, I2C_8BIT},
{0x3708, 0x64, I2C_8BIT},
//{0x3709, 0x12, I2C_8BIT},
{0x3709, 0x52, I2C_8BIT},
//{0x370c, 0x00, I2C_8BIT},
{0x370c, 0x03, I2C_8BIT},
{0x3a02, 0x07, I2C_8BIT},
{0x3a03, 0xb0, I2C_8BIT},
{0x3a08, 0x01, I2C_8BIT},
{0x3a09, 0x27, I2C_8BIT},
{0x3a0a, 0x00, I2C_8BIT},
{0x3a0b, 0xf6, I2C_8BIT},
{0x3a0e, 0x06, I2C_8BIT},
{0x3a0d, 0x08, I2C_8BIT},
{0x3a14, 0x07, I2C_8BIT},
{0x3a15, 0xb0, I2C_8BIT},
{0x4001, 0x02, I2C_8BIT},
{0x4004, 0x06, I2C_8BIT},
{0x4005, 0x1a, I2C_8BIT},
{0x3000, 0x00, I2C_8BIT},
{0x3002, 0x1c, I2C_8BIT},
{0x3004, 0xff, I2C_8BIT},
{0x3006, 0xc3, I2C_8BIT},
{0x3007, 0xff, I2C_8BIT},
{0x300e, 0x45, I2C_8BIT},
{0x302e, 0x08, I2C_8BIT},
{0x4300, 0x30, I2C_8BIT},
//{0x4837, 0x0a, I2C_8BIT},
{0x4837, 0x24, I2C_8BIT},
{0x501f, 0x00, I2C_8BIT},
{0x440e, 0x00, I2C_8BIT},
{0x5000, 0xa7, I2C_8BIT},
/**/
{0x3824, 0x02, I2C_8BIT},
//{0x5001, 0x83, I2C_8BIT},
{0x5001, 0xff, I2C_8BIT},
/******/
{0x3503, 0x00, I2C_8BIT},
{0x5180, 0xff, I2C_8BIT},
{0x5181, 0xf2, I2C_8BIT},
{0x5182, 0x00, I2C_8BIT},
{0x5183, 0x14, I2C_8BIT},
{0x5184, 0x25, I2C_8BIT},
{0x5185, 0x24, I2C_8BIT},
{0x5186, 0x09, I2C_8BIT},
{0x5187, 0x09, I2C_8BIT},
{0x5188, 0x09, I2C_8BIT},
{0x5189, 0x75, I2C_8BIT},
{0x518a, 0x54, I2C_8BIT},
{0x518b, 0xe0, I2C_8BIT},
{0x518c, 0xb2, I2C_8BIT},
{0x518d, 0x42, I2C_8BIT},
{0x518e, 0x3d, I2C_8BIT},
{0x518f, 0x56, I2C_8BIT},
{0x5190, 0x46, I2C_8BIT},
{0x5191, 0xf8, I2C_8BIT},
{0x5192, 0x04, I2C_8BIT},
{0x5193, 0x70, I2C_8BIT},
{0x5194, 0xf0, I2C_8BIT},
{0x5195, 0xf0, I2C_8BIT},
{0x5196, 0x03, I2C_8BIT},
{0x5197, 0x01, I2C_8BIT},
{0x5198, 0x04, I2C_8BIT},
{0x5199, 0x12, I2C_8BIT},
{0x519a, 0x04, I2C_8BIT},
{0x519b, 0x00, I2C_8BIT},
{0x519c, 0x06, I2C_8BIT},
{0x519d, 0x82, I2C_8BIT},
{0x519e, 0x38, I2C_8BIT},
{0x5381, 0x1e, I2C_8BIT},
{0x5382, 0x5b, I2C_8BIT},
{0x5383, 0x08, I2C_8BIT},
{0x5384, 0x0a, I2C_8BIT},
{0x5385, 0x7e, I2C_8BIT},
{0x5386, 0x88, I2C_8BIT},
{0x5387, 0x7c, I2C_8BIT},
{0x5388, 0x6c, I2C_8BIT},
{0x5389, 0x10, I2C_8BIT},
{0x538a, 0x01, I2C_8BIT},
{0x538b, 0x98, I2C_8BIT},
{0x5300, 0x08, I2C_8BIT},
{0x5301, 0x30, I2C_8BIT},
{0x5302, 0x10, I2C_8BIT},
{0x5303, 0x00, I2C_8BIT},
{0x5304, 0x08, I2C_8BIT},
{0x5305, 0x30, I2C_8BIT},
{0x5306, 0x08, I2C_8BIT},
{0x5307, 0x16, I2C_8BIT},
{0x5309, 0x08, I2C_8BIT},
{0x530a, 0x30, I2C_8BIT},
{0x530b, 0x04, I2C_8BIT},
{0x530c, 0x06, I2C_8BIT},
{0x5480, 0x01, I2C_8BIT},
{0x5481, 0x08, I2C_8BIT},
{0x5482, 0x14, I2C_8BIT},
{0x5483, 0x28, I2C_8BIT},
{0x5484, 0x51, I2C_8BIT},
{0x5485, 0x65, I2C_8BIT},
{0x5486, 0x71, I2C_8BIT},
{0x5487, 0x7d, I2C_8BIT},
{0x5488, 0x87, I2C_8BIT},
{0x5489, 0x91, I2C_8BIT},
{0x548a, 0x9a, I2C_8BIT},
{0x548b, 0xaa, I2C_8BIT},
{0x548c, 0xb8, I2C_8BIT},
{0x548d, 0xcd, I2C_8BIT},
{0x548e, 0xdd, I2C_8BIT},
{0x548f, 0xea, I2C_8BIT},
{0x5490, 0x1d, I2C_8BIT},
{0x5580, 0x02, I2C_8BIT},
{0x5583, 0x40, I2C_8BIT},
{0x5584, 0x10, I2C_8BIT},
{0x5589, 0x10, I2C_8BIT},
{0x558a, 0x00, I2C_8BIT},
{0x558b, 0xf8, I2C_8BIT},
{0x5800, 0x23, I2C_8BIT},
{0x5801, 0x14, I2C_8BIT},
{0x5802, 0x0f, I2C_8BIT},
{0x5803, 0x0f, I2C_8BIT},
{0x5804, 0x12, I2C_8BIT},
{0x5805, 0x26, I2C_8BIT},
{0x5806, 0x0c, I2C_8BIT},
{0x5807, 0x08, I2C_8BIT},
{0x5808, 0x05, I2C_8BIT},
{0x5809, 0x05, I2C_8BIT},
{0x580a, 0x08, I2C_8BIT},
{0x580b, 0x0d, I2C_8BIT},
{0x580c, 0x08, I2C_8BIT},
{0x580d, 0x03, I2C_8BIT},
{0x580e, 0x00, I2C_8BIT},
{0x580f, 0x00, I2C_8BIT},
{0x5810, 0x03, I2C_8BIT},
{0x5811, 0x09, I2C_8BIT},
{0x5812, 0x07, I2C_8BIT},
{0x5813, 0x03, I2C_8BIT},
{0x5814, 0x00, I2C_8BIT},
{0x5815, 0x01, I2C_8BIT},
{0x5816, 0x03, I2C_8BIT},
{0x5817, 0x08, I2C_8BIT},
{0x5818, 0x0d, I2C_8BIT},
{0x5819, 0x08, I2C_8BIT},
{0x581a, 0x05, I2C_8BIT},
{0x581b, 0x06, I2C_8BIT},
{0x581c, 0x08, I2C_8BIT},
{0x581d, 0x0e, I2C_8BIT},
{0x581e, 0x29, I2C_8BIT},
{0x581f, 0x17, I2C_8BIT},
{0x5820, 0x11, I2C_8BIT},
{0x5821, 0x11, I2C_8BIT},
{0x5822, 0x15, I2C_8BIT},
{0x5823, 0x28, I2C_8BIT},
{0x5824, 0x46, I2C_8BIT},
{0x5825, 0x26, I2C_8BIT},
{0x5826, 0x08, I2C_8BIT},
{0x5827, 0x26, I2C_8BIT},
{0x5828, 0x64, I2C_8BIT},
{0x5829, 0x26, I2C_8BIT},
{0x582a, 0x24, I2C_8BIT},
{0x582b, 0x22, I2C_8BIT},
{0x582c, 0x24, I2C_8BIT},
{0x582d, 0x24, I2C_8BIT},
{0x582e, 0x06, I2C_8BIT},
{0x582f, 0x22, I2C_8BIT},
{0x5830, 0x40, I2C_8BIT},
{0x5831, 0x42, I2C_8BIT},
{0x5832, 0x24, I2C_8BIT},
{0x5833, 0x26, I2C_8BIT},
{0x5834, 0x24, I2C_8BIT},
{0x5835, 0x22, I2C_8BIT},
{0x5836, 0x22, I2C_8BIT},
{0x5837, 0x26, I2C_8BIT},
{0x5838, 0x44, I2C_8BIT},
{0x5839, 0x24, I2C_8BIT},
{0x583a, 0x26, I2C_8BIT},
{0x583b, 0x28, I2C_8BIT},
{0x583c, 0x42, I2C_8BIT},
{0x5025, 0x00, I2C_8BIT},
{0x3a0f, 0x30, I2C_8BIT},
{0x3a10, 0x28, I2C_8BIT},
{0x3a1b, 0x30, I2C_8BIT},
{0x3a1e, 0x26, I2C_8BIT},
{0x3a11, 0x60, I2C_8BIT},
{0x3a1f, 0x14, I2C_8BIT},

//{0x4202, 0x00, I2C_8BIT},

{0x3008, 0x02, I2C_8BIT},
{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}
};

/**
 * struct ov5640_sensor - main structure for storage of sensor information
 * @pdata: access functions and data for platform level information
 * @v4l2_int_device: V4L2 device structure structure
 * @i2c_client: iic client device structure
 * @pix: V4L2 pixel format information structure
 * @timeperframe: time per frame expressed as V4L fraction
 * @scaler:
 * @ver: ov5640 chip version
 * @fps: frames per second value
 */
struct ov5640_sensor {
	const struct ov5640_platform_data *pdata;
	struct v4l2_int_device *v4l2_int_device;
	struct i2c_client *i2c_client;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	int scaler;
	int ver;
	int fps;
	bool resuming;
};

static struct ov5640_sensor ov5640;
static struct i2c_driver ov5640sensor_i2c_driver;
static unsigned long xclk_current = OV5640_XCLK_NOM_1;
static unsigned long cntclk = 2000000;

/* list of image formats supported by ov5640 sensor */
const static struct v4l2_fmtdesc ov5640_formats[] = {
	{
		.description	= "YUYV",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
	}
};

#define NUM_CAPTURE_FORMATS ARRAY_SIZE(ov5640_formats)

static enum v4l2_power current_power_state;

/* Structure of Sensor settings that change with image size */
static struct ov5640_sensor_settings ov5640_settings[] = {
	 /* NOTE: must be in same order as image_size array */

	/* QUART_MP */
	{
		.clk = {
			.pre_pll_div = 1,
			.pll_mult = 18,
//			.post_pll_div = 1,
			.vt_pix_clk_div = 10,
			.vt_sys_clk_div = 1,
		},
		.mipi = {
			.data_lanes = 2,
			.ths_prepare = 2,
			//.ths_zero = 5,
			.ths_zero = 1,
			.ths_settle_lower = 8,
			.ths_settle_upper = 28,
		},
	    .frame = {
	        .frame_len_lines_min = 629,
	        .line_len_pck = 3440,
	        .x_addr_start = 813,
	        .x_addr_end = 2466,
	        .y_addr_start = 0,
	        .y_addr_end = 2463,
	        .x_output_size = 320,
	        .y_output_size = 240,
	        .x_total_size = 2844,
	        .y_total_size = 1968,
	        .isp_x_offset = 16,
	        .isp_y_offset = 6,
	        .x_odd_inc = 1,
	        .x_even_inc = 1,
	        .y_odd_inc = 1,
	        .y_even_inc = 1,
	    },

	},

	/* VGA */
	{
		.clk = {
//			.pre_pll_div = 1,
			.pre_pll_div = 3,
			.pll_mult = 18,
//			.post_pll_div = 1,
			.vt_pix_clk_div = 10,
			.vt_sys_clk_div = 1,
		},
		.mipi = {
			.data_lanes = 2,
			.ths_prepare = 2,
			.ths_zero = 5,
			.ths_settle_lower = 8,
			.ths_settle_upper = 28,
		},
        .frame = {
            .frame_len_lines_min = 629,
            .line_len_pck = 3440,
            .x_addr_start = 0,
            .x_addr_end = 3279,
            .y_addr_start = 0,
            .y_addr_end = 2463,
            .x_output_size = 640,
            .y_output_size = 480,
            .x_total_size = 2844,
            .y_total_size = 1968,
            .isp_x_offset = 16,
            .isp_y_offset = 6,
            .x_odd_inc = 1,
            .x_even_inc = 1,
            .y_odd_inc = 1,
            .y_even_inc = 1,
        },
	},

	/* 729p_MP */
	{
		.clk = {
			.pre_pll_div = 1,
			.pll_mult = 36,
			//.post_pll_div = 1,
			.vt_pix_clk_div = 10,
			.vt_sys_clk_div = 1,
		},
		.mipi = {
			.data_lanes = 2,
			.ths_prepare = 6,
			.ths_zero = 9,
			.ths_settle_lower = 23,
			.ths_settle_upper = 59,
		},
        .frame = {
           .frame_len_lines_min = 1250,
           .line_len_pck = 3456,
           .x_addr_start = 344,
           .x_addr_end = 2936,
           .y_addr_start = 503,
           .y_addr_end = 1960,
           .x_output_size = 1280,
           .y_output_size = 720,
           .x_total_size = 2500,
           .y_total_size = 1120,
           .isp_x_offset = 16,
           .isp_y_offset = 4,
           .x_odd_inc = 1,
           .x_even_inc = 1,
           .y_odd_inc = 1,
           .y_even_inc = 1,
       },

	},

    /* 1080P_MP */
    {
        .clk = {
            .pre_pll_div = 1,
            .pll_mult = 18,
            //.post_pll_div = 1,
            .vt_pix_clk_div = 10,
            .vt_sys_clk_div = 1,
        },
        .mipi = {
            .data_lanes = 2,
            .ths_prepare = 2,
            .ths_zero = 4,
            .ths_settle_lower = 8,
            .ths_settle_upper = 24,
        },
        .frame = {
            .frame_len_lines_min = 629,
            .line_len_pck = 3440,
            .x_addr_start = 0,
            .x_addr_end = 2591,
            .y_addr_start = 0,
            .y_addr_end = 1943,
            .x_output_size = 1920,
            .y_output_size = 1080,
            .x_total_size = 2500,
            .y_total_size = 1120,
            .isp_x_offset = 16,
            .isp_y_offset = 4,
            .x_odd_inc = 1,
            .x_even_inc = 1,
            .y_odd_inc = 1,
            .y_even_inc = 1,
        },
	},

	 /* FIVE_MP */
	{
	    .clk = {
	        .pre_pll_div = 1,
	        .pll_mult = 24,
	        //.post_pll_div = 1,
	        .vt_pix_clk_div = 10,
	        .vt_sys_clk_div = 1,
	    },
	    .mipi = {
	        .data_lanes = 2,
	        .ths_prepare = 2,
	        //.ths_zero = 4,
	        .ths_zero = 6,
	        .ths_settle_lower = 8,
	        .ths_settle_upper = 24,
	    },
	    .frame = {
	        .frame_len_lines_min = 2510,
	        .line_len_pck = 3440,
	        .x_addr_start = 0,
	        .x_addr_end = 2591,
	        .y_addr_start = 0,
	        .y_addr_end = 1943,
	        .x_output_size = 2592,
	        .y_output_size = 1944,
	        .x_total_size = 2844,
	        .y_total_size = 1968,
	        .isp_x_offset = 16,
	        .isp_y_offset = 6,
	        .x_odd_inc = 1,
	        .x_even_inc = 1,
	        .y_odd_inc = 1,
	        .y_even_inc = 1,
	    },
	},
};

#define OV5640_MODES_COUNT ARRAY_SIZE(ov5640_settings)

static unsigned isize_current = OV5640_MODES_COUNT - 1;
static struct ov5640_clock_freq current_clk;

struct i2c_list {
	struct i2c_msg *reg_list;
	unsigned int list_size;
};

/**
 * struct vcontrol - Video controls
 * @v4l2_queryctrl: V4L2 VIDIOC_QUERYCTRL ioctl structure
 * @current_value: current value of this control
 */
static struct vcontrol {
	struct v4l2_queryctrl qc;
	int current_value;
} ov5640sensor_video_control[] = {
	{
		{
			.id = V4L2_CID_EXPOSURE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Exposure",
			.minimum = OV5640_MIN_EXPOSURE,
			.maximum = OV5640_MAX_EXPOSURE,
			.step = OV5640_EXPOSURE_STEP,
			.default_value = OV5640_DEF_EXPOSURE,
		},
		.current_value = OV5640_DEF_EXPOSURE,
	},
	{
		{
			.id = V4L2_CID_GAIN,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Analog Gain",
			.minimum = OV5640_EV_MIN_GAIN,
			.maximum = OV5640_EV_MAX_GAIN,
			.step = OV5640_EV_GAIN_STEP,
			.default_value = OV5640_EV_DEF_GAIN,
		},
		.current_value = OV5640_EV_DEF_GAIN,
	},
	{
		{
			.id = V4L2_CID_TEST_PATTERN,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Test Pattern",
			.minimum = OV5640_MIN_TEST_PATT_MODE,
			.maximum = OV5640_MAX_TEST_PATT_MODE,
			.step = OV5640_MODE_TEST_PATT_STEP,
			.default_value = OV5640_MIN_TEST_PATT_MODE,
		},
		.current_value = OV5640_MIN_TEST_PATT_MODE,
	}
};

/**
 * find_vctrl - Finds the requested ID in the video control structure array
 * @id: ID of control to search the video control array for
 *
 * Returns the index of the requested ID from the control structure array
 */
static int find_vctrl(int id)
{
	int i;

	if (id < V4L2_CID_BASE)
		return -EDOM;

	for (i = (ARRAY_SIZE(ov5640sensor_video_control) - 1); i >= 0; i--)
		if (ov5640sensor_video_control[i].qc.id == id)
			break;
	if (i < 0)
		i = -EINVAL;
	return i;
}

/**
 * ov5640_read_reg - Read a value from a register in an ov5640 sensor device
 * @client: i2c driver client structure
 * @data_length: length of data to be read
 * @reg: register address / offset
 * @val: stores the value that gets read
 *
 * Read a value from a register in an ov5640 sensor device.
 * The value is returned in 'val'.
 * Returns zero if successful, or non-zero otherwise.
 */
static int ov5640_read_reg(struct i2c_client *client, u16 data_length, u16 reg,
			   u32 *val)
{
	int err;
	struct i2c_msg msg[1];
	unsigned char data[4] = {0};

	if (!client->adapter)
		return -ENODEV;
	if (data_length != I2C_8BIT && data_length != I2C_16BIT
			&& data_length != I2C_32BIT)
		return -EINVAL;

	msg->addr = ov5640_I2C_ADDR_R; // client->addr;
	msg->flags = 0;
	msg->len = 2;
	msg->buf = data;

	/* Write addr - high byte goes out first */
	data[0] = (u8) (reg >> 8);;
	data[1] = (u8) (reg & 0xff);
	err = i2c_transfer(client->adapter, msg, 1);

	/* Read back data */
	if (err >= 0) {
		msg->len = data_length;
		msg->flags = I2C_M_RD;
		err = i2c_transfer(client->adapter, msg, 1);
	}
	if (err >= 0) {
		*val = 0;
		/* high byte comes first */
		if (data_length == I2C_8BIT)
			*val = data[0];
		else if (data_length == I2C_16BIT)
			*val = data[1] + (data[0] << 8);
		else
			*val = data[3] + (data[2] << 8) +
				(data[1] << 16) + (data[0] << 24);
		return 0;
	}
	v4l_err(client, "read from offset 0x%x error %d", reg, err);

	return err;
}

/**
 * Write a value to a register in ov5640 sensor device.
 * @client: i2c driver client structure.
 * @reg: Address of the register to read value from.
 * @val: Value to be written to a specific register.
 * Returns zero if successful, or non-zero otherwise.
 */
static int ov5640_write_reg(struct i2c_client *client, u16 reg, u32 val,
			    u16 data_length)
{
	int err = 0;
	struct i2c_msg msg[1];
	unsigned char data[6];
	int retries = 0;

	if (!client->adapter)
		return -ENODEV;

	if (data_length != I2C_8BIT && data_length != I2C_16BIT
			&& data_length != I2C_32BIT)
		return -EINVAL;

retry:
	msg->addr = ov5640_I2C_ADDR_W; //client->addr;
	msg->flags = I2C_M_WR;
	msg->len = data_length+2;  /* add address bytes */
	msg->buf = data;

	/* high byte goes out first */
	data[0] = (u8) (reg >> 8);
	data[1] = (u8) (reg & 0xff);
	if (data_length == I2C_8BIT) {
		data[2] = val & 0xff;
	} else if (data_length == I2C_16BIT) {
		data[2] = (val >> 8) & 0xff;
		data[3] = val & 0xff;
	} else {
		data[2] = (val >> 24) & 0xff;
		data[3] = (val >> 16) & 0xff;
		data[4] = (val >> 8) & 0xff;
		data[5] = val & 0xff;
	}

	if (data_length == 1)
		dev_dbg(&client->dev, "OV5640 Wrt:[0x%04X]=0x%02X\n",
				reg, val);
	else if (data_length == 2)
		dev_dbg(&client->dev, "OV5640 Wrt:[0x%04X]=0x%04X\n",
				reg, val);

	err = i2c_transfer(client->adapter, msg, 1);

	if (err >= 0)
		return 0;

	if (retries <= 5) {
		v4l_info(client, "Retrying I2C... %d\n", retries);
		retries++;
		mdelay(20);
		goto retry;
	}

	return err;
}

/**
 * Initialize a list of ov5640 registers.
 * The list of registers is terminated by the pair of values
 * {OV3640_REG_TERM, OV3640_VAL_TERM}.
 * @client: i2c driver client structure.
 * @reglist[]: List of address of the registers to write data.
 * Returns zero if successful, or non-zero otherwise.
 */
static int ov5640_write_regs(struct i2c_client *client,
			     const struct ov5640_reg reglist[])
{
	int err = 0;
	const struct ov5640_reg *list = reglist;
	int i = 0;

printk("-----------------------------------------begin writting regs\n");

	while (!((list->reg == I2C_REG_TERM)
		&& (list->val == I2C_VAL_TERM))) {
		err = ov5640_write_reg(client, list->reg,
				list->val, list->length);

		printk("-----------------------------reg number:%d reg:0x%x\n", ++i, list->reg);
		if (err)
			return err;

		list++;
	}

printk("---------------------------------------------after written regs\n");
	return 0;
}

/**
 * ov5640_find_size - Find the best match for a requested image capture size
 * @width: requested image width in pixels
 * @height: requested image height in pixels
 *
 * Find the best match for a requested image capture size.  The best match
 * is chosen as the nearest match that has the same number or fewer pixels
 * as the requested size, or the smallest image size if the requested size
 * has fewer pixels than the smallest image.
 * Since the available sizes are subsampled in the vertical direction only,
 * the routine will find the size with a height that is equal to or less
 * than the requested height.
 */
static unsigned ov5640_find_size(unsigned int width, unsigned int height)
{
	unsigned isize;

	for (isize = 0; isize < OV5640_MODES_COUNT; isize++) {
		if ((ov5640_settings[isize].frame.y_output_size >= height) &&
		    (ov5640_settings[isize].frame.x_output_size >= width))
			break;
	}
	printk("%s...........%d\n",__func__,__LINE__);

	printk(KERN_DEBUG "ov5640_find_size: Req Size=%dx%d, "
			"Calc Size=%dx%d\n", width, height,
			ov5640_settings[isize].frame.x_output_size,
			ov5640_settings[isize].frame.y_output_size);

	return isize;
}

/**
 * Set CSI2 Virtual ID.
 * @client: i2c client driver structure
 * @id: Virtual channel ID.
 *
 * Sets the channel ID which identifies data packets sent by this device
 * on the CSI2 bus.
 **/

static int ov5640_set_virtual_id(struct i2c_client *client, u32 id)
{
	printk("%s...........%d\n",__func__,__LINE__);
//	return ov5640_write_reg(client, OV5640_REG_CCP2_CHANNEL_ID, (0x3 & id), I2C_8BIT);
	return 0;
}

/**
 * ov5640_set_framerate - Sets framerate by adjusting frame_length_lines reg.
 * @s: pointer to standard V4L2 device structure
 * @fper: frame period numerator and denominator in seconds
 *
 * The maximum exposure time is also updated since it is affected by the
 * frame rate.
 **/
static int ov5640_set_framerate(struct v4l2_int_device *s,
				struct v4l2_fract *fper)
{
#if 1
	int err = 0;
	u16 isize = isize_current;
	u32 frame_length_lines, line_time_q8;
	struct ov5640_sensor *sensor = s->priv;
	struct ov5640_sensor_settings *ss;

	if ((fper->numerator == 0) || (fper->denominator == 0)) {
		/* supply a default nominal_timeperframe */
		fper->numerator = 1;
		fper->denominator = OV5640_DEF_FPS;
	}
	printk("%s...........%d\n",__func__,__LINE__);

	sensor->fps = fper->denominator / fper->numerator;
	if (sensor->fps < OV5640_MIN_FPS) {
		sensor->fps = OV5640_MIN_FPS;
		fper->numerator = 1;
		fper->denominator = sensor->fps;
	} else if (sensor->fps > OV5640_MAX_FPS) {
		sensor->fps = OV5640_MAX_FPS;
		fper->numerator = 1;
		fper->denominator = sensor->fps;
	}

	ss = &ov5640_settings[isize_current];

	line_time_q8 = ((u32)ss->frame.line_len_pck * 1000000) /
			(current_clk.vt_pix_clk >> 8); /* usec's */

	frame_length_lines = (((u32)fper->numerator * 1000000 * 256 /
			       fper->denominator)) / line_time_q8;

	/* Range check frame_length_lines */
	if (frame_length_lines > OV5640_MAX_FRAME_LENGTH_LINES)
		frame_length_lines = OV5640_MAX_FRAME_LENGTH_LINES;
	else if (frame_length_lines < ss->frame.frame_len_lines_min)
		frame_length_lines = ss->frame.frame_len_lines_min;

	ov5640_settings[isize].frame.frame_len_lines = frame_length_lines;

	printk(KERN_DEBUG "OV5640 Set Framerate: fper=%d/%d, "
	       "frame_len_lines=%d, max_expT=%dus\n", fper->numerator,
	       fper->denominator, frame_length_lines, OV5640_MAX_EXPOSURE);

	return err;
#endif

	return 0;
}


/**
 * ov5640sensor_calc_xclk - Calculate the required xclk frequency
 *
 * Xclk is not determined from framerate for the OV5640
 */
static unsigned long ov5640sensor_calc_xclk(void)
{
	printk("%s...........%d\n",__func__,__LINE__);
	return OV5640_XCLK_NOM_1;
}

/**
 * Sets the correct orientation based on the sensor version.
 *   IU046F2-Z   version=2  orientation=3
 *   IU046F4-2D  version>2  orientation=0
 */
static int ov5640_set_orientation(struct i2c_client *client, u32 ver)
{
#if 0
	int err;
	u8 orient;

	orient = (ver <= 0x2) ? 0x3 : 0x0;
	printk("%s...........%d\n",__func__,__LINE__);
	err = ov5640_write_reg(client, OV5640_REG_IMAGE_ORIENTATION,
			       orient, I2C_8BIT);
	return err;
#endif
	return true;
}

/**
 * ov5640sensor_set_exposure_time - sets exposure time per input value
 * @exp_time: exposure time to be set on device (in usec)
 * @s: pointer to standard V4L2 device structure
 * @lvc: pointer to V4L2 exposure entry in ov5640sensor_video_controls array
 *
 * If the requested exposure time is within the allowed limits, the HW
 * is configured to use the new exposure time, and the
 * ov5640sensor_video_control[] array is updated with the new current value.
 * The function returns 0 upon success.  Otherwise an error code is
 * returned.
 */
static int ov5640sensor_set_exposure_time(u32 exp_time,
					  struct v4l2_int_device *s,
					  struct vcontrol *lvc)
{
#if 0
	int err = 0, i;
	struct ov5640_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	u16 coarse_int_time = 0;
	u32 line_time_q8 = 0;
	struct ov5640_sensor_settings *ss;
	printk("%s...........%d\n",__func__,__LINE__);

	if ((current_power_state == V4L2_POWER_ON) || sensor->resuming) {
		if (exp_time < OV5640_MIN_EXPOSURE) {
			v4l_err(client, "Exposure time %d us not within"
					" the legal range.\n", exp_time);
			v4l_err(client, "Exposure time must be between"
					" %d us and %d us\n",
					OV5640_MIN_EXPOSURE,
					OV5640_MAX_EXPOSURE);
			exp_time = OV5640_MIN_EXPOSURE;
		}

		if (exp_time > OV5640_MAX_EXPOSURE) {
			v4l_err(client, "Exposure time %d us not within"
					" the legal range.\n", exp_time);
			v4l_err(client, "Exposure time must be between"
					" %d us and %d us\n",
					OV5640_MIN_EXPOSURE,
					OV5640_MAX_EXPOSURE);
			exp_time = OV5640_MAX_EXPOSURE;
		}

		ss = &ov5640_settings[isize_current];

		line_time_q8 = ((u32)ss->frame.line_len_pck * 1000000) /
				(current_clk.vt_pix_clk >> 8); /* usec's */

		coarse_int_time = ((exp_time * 256) + (line_time_q8 >> 1)) /
				  line_time_q8;

		if (coarse_int_time > ss->frame.frame_len_lines - 2)
			err = ov5640_write_reg(client,
					       OV5640_REG_FRAME_LEN_LINES,
					       coarse_int_time + 2,
					       I2C_16BIT);
		else
			err = ov5640_write_reg(client,
					       OV5640_REG_FRAME_LEN_LINES,
					       ss->frame.frame_len_lines,
					       I2C_16BIT);

		err = ov5640_write_reg(client, OV5640_REG_COARSE_INT_TIME,
				       coarse_int_time, I2C_16BIT);
	}

	if (err) {
		v4l_err(client, "Error setting exposure time: %d", err);
	} else {
		i = find_vctrl(V4L2_CID_EXPOSURE);
		if (i >= 0) {
			lvc = &ov5640sensor_video_control[i];
			lvc->current_value = exp_time;
		}
	}

	return err;
#endif 
 	return true;
}

/**
 * This table describes what should be written to the sensor register for each
 * gain value. The gain(index in the table) is in terms of 0.1EV, i.e. 10
 * indexes in the table give 2 time more gain
 *
 * Elements in TS2_8_GAIN_TBL doesn't comply linearity. This is because
 * there is nonlinear dependecy between analogue_gain_code_global and real gain
 * value: Gain_analog = 256 / (256 - analogue_gain_code_global)
 */

static const u16 OV5640_EV_GAIN_TBL[OV5640_EV_TABLE_GAIN_MAX + 1] = {
	/* Gain x1 */
	0,  16, 33, 48, 62, 74, 88, 98, 109, 119,

	/* Gain x2 */
	128, 136, 144, 152, 159, 165, 171, 177, 182, 187,

	/* Gain x4 */
	192, 196, 200, 204, 208, 211, 214, 216, 219, 222,

	/* Gain x8 */
	224
};

/**
 * ov5640sensor_set_gain - sets sensor analog gain per input value
 * @gain: analog gain value to be set on device
 * @s: pointer to standard V4L2 device structure
 * @lvc: pointer to V4L2 analog gain entry in ov5640sensor_video_control array
 *
 * If the requested analog gain is within the allowed limits, the HW
 * is configured to use the new gain value, and the ov5640sensor_video_control
 * array is updated with the new current value.
 * The function returns 0 upon success.  Otherwise an error code is
 * returned.
 */
static int ov5640sensor_set_gain(u16 lineargain, struct v4l2_int_device *s,
				 struct vcontrol *lvc)
{
#if 0
	int err = 0, i;
	u16 reg_gain = 0;
	struct ov5640_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	printk("%s...........%d\n",__func__,__LINE__);

	if (current_power_state == V4L2_POWER_ON || sensor->resuming) {

		if (lineargain < OV5640_EV_MIN_GAIN) {
			lineargain = OV5640_EV_MIN_GAIN;
			v4l_err(client, "Gain out of legal range.");
		}
		if (lineargain > OV5640_EV_MAX_GAIN) {
			lineargain = OV5640_EV_MAX_GAIN;
			v4l_err(client, "Gain out of legal range.");
		}

		reg_gain = OV5640_EV_GAIN_TBL[lineargain];

		err = ov5640_write_reg(client, OV5640_REG_ANALOG_GAIN_GLOBAL,
					reg_gain, I2C_16BIT);
	}

	if (err) {
		v4l_err(client, "Error setting analog gain: %d", err);
	} else {
		i = find_vctrl(V4L2_CID_GAIN);
		if (i >= 0) {
			lvc = &ov5640sensor_video_control[i];
			lvc->current_value = lineargain;
		}
	}

	return err;
#endif 
	return true;
}

/**
 * ov5640_update_clocks - calcs sensor clocks based on sensor settings.
 * @isize: image size enum
 */
static int ov5640_update_clocks(u32 xclk, unsigned isize)
{
	current_clk.vco_clk =
			xclk * ov5640_settings[isize].clk.pll_mult /
			ov5640_settings[isize].clk.pre_pll_div;// / /
//			ov5640_settings[isize].clk.post_pll_div;

	current_clk.vt_pix_clk = current_clk.vco_clk * 2 /
			(ov5640_settings[isize].clk.vt_pix_clk_div *
			ov5640_settings[isize].clk.vt_sys_clk_div);

	if (ov5640_settings[isize].mipi.data_lanes == 2)
		current_clk.mipi_clk = current_clk.vco_clk;
	else
		current_clk.mipi_clk = current_clk.vco_clk / 2;

	current_clk.ddr_clk = current_clk.mipi_clk / 2;

	printk(KERN_DEBUG "OV5640: xclk=%u, vco_clk=%u, "
		"vt_pix_clk=%u,  mipi_clk=%u,  ddr_clk=%u\n",
		xclk, current_clk.vco_clk, current_clk.vt_pix_clk,
		current_clk.mipi_clk, current_clk.ddr_clk);

/******************************************************/
   printk("%s...........%d\n",__func__,__LINE__);
   printk("OV5640: xclk=%u, vco_clk=%u, "
       "vt_pix_clk=%u,  mipi_clk=%u,  ddr_clk=%u\n",
       xclk, current_clk.vco_clk, current_clk.vt_pix_clk,
       current_clk.mipi_clk, current_clk.ddr_clk);
/******************************************************/

	return 0;
}

/**
 * ov5640_setup_pll - initializes sensor PLL registers.
 * @c: i2c client driver structure
 * @isize: image size enum
 */
static int ov5640_setup_pll(struct i2c_client *client, unsigned isize)
{
#if 1
	u32 rgpltd_reg;
	u32 rgpltd[3] = {2, 0, 1};
	printk("%s...........%d\n",__func__,__LINE__);

	ov5640_write_reg(client, OV5640_REG_PRE_PLL_CLK_DIV,
		ov5640_settings[isize].clk.pre_pll_div, I2C_16BIT);

	ov5640_write_reg(client, OV5640_REG_PLL_MULTIPLIER,
		ov5640_settings[isize].clk.pll_mult, I2C_16BIT);

	ov5640_read_reg(client, I2C_8BIT, OV5640_REG_RGPLTD_RGCLKEN,
		&rgpltd_reg);
	rgpltd_reg &= ~RGPLTD_MASK;
	rgpltd_reg |= rgpltd[ov5640_settings[isize].clk.post_pll_div >> 1];
	ov5640_write_reg(client, OV5640_REG_RGPLTD_RGCLKEN,
		rgpltd_reg, I2C_8BIT);

	ov5640_write_reg(client, OV5640_REG_VT_PIX_CLK_DIV,
		ov5640_settings[isize].clk.vt_pix_clk_div, I2C_16BIT);

	ov5640_write_reg(client, OV5640_REG_VT_SYS_CLK_DIV,
		ov5640_settings[isize].clk.vt_sys_clk_div, I2C_16BIT);

	printk(KERN_DEBUG "OV5640: pre_pll_clk_div=%u, pll_mult=%u, "
		"rgpltd=0x%x, vt_pix_clk_div=%u, vt_sys_clk_div=%u\n",
		ov5640_settings[isize].clk.pre_pll_div,
		ov5640_settings[isize].clk.pll_mult, rgpltd_reg,
		ov5640_settings[isize].clk.vt_pix_clk_div,
		ov5640_settings[isize].clk.vt_sys_clk_div);
/*******************************************/
	printk("OV5640: pre_pll_clk_div=%u, pll_mult=%u, "
		"rgpltd=0x%x, vt_pix_clk_div=%u, vt_sys_clk_div=%u\n",
		ov5640_settings[isize].clk.pre_pll_div,
		ov5640_settings[isize].clk.pll_mult, rgpltd_reg,
		ov5640_settings[isize].clk.vt_pix_clk_div,
		ov5640_settings[isize].clk.vt_sys_clk_div);
/*******************************************/

#endif
	return 0;
}

/**
 * ov5640_setup_mipi - initializes sensor & isp MIPI registers.
 * @c: i2c client driver structure
 * @isize: image size enum
 */
static int ov5640_setup_mipi(struct v4l2_int_device *s, unsigned isize)
{
	struct ov5640_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;

	/* NOTE: Make sure ov5640_update_clocks is called 1st */
	printk("%s...............................%d\n",__func__,__LINE__);

//	ov5640_write_reg(client, OV5640_REG_TESTDI, 0x04, I2C_8BIT);

	/* Set sensor Mipi timing params */
//	ov5640_write_reg(client, 0x481a, 0x06, I2C_8BIT);

//	ov5640_write_reg(client, OV5640_REG_RGTHSPREPARE, \
		ov5640_settings[isize].mipi.ths_prepare, I2C_8BIT);

//	ov5640_write_reg(client, OV5640_REG_RGTHSZERO, \
		ov5640_settings[isize].mipi.ths_zero, I2C_8BIT);
#if 0
ov5640_write_reg(client,0x4800, 0x04, I2C_8BIT);
ov5640_write_reg(client,0x4801, 0x04, I2C_8BIT);
ov5640_write_reg(client,0x4805, 0x10, I2C_8BIT);
ov5640_write_reg(client,0x4805, 0x10, I2C_8BIT);
#endif
	/* Set number of lanes in sensor */
	if (ov5640_settings[isize].mipi.data_lanes == 2)
		ov5640_write_reg(client, 0x300e, 0x45, I2C_8BIT);
	else
		ov5640_write_reg(client, 0x300e, 0x05, I2C_8BIT);

	/* Enable MIPI */
/*	ov5640_write_reg(client, 0x4202, 0x00, I2C_8BIT); */

/*******************************************************************/

	/* Set number of lanes in isp */
	sensor->pdata->csi2_lane_count(s,
				       ov5640_settings[isize].mipi.data_lanes);

printk("---------test phy----%s line:%d current_clk.mipi_clk:%ld ov5640_settings[isize].mipi.ths_settle_lower:%d\nov5640_settings[isize].mipi.ths_settle_upper:%d ov5640_settings[isize].mipi.ths_zero:%d\n", __func__, __LINE__, current_clk.mipi_clk, ov5640_settings[isize].mipi.ths_settle_lower, ov5640_settings[isize].mipi.ths_settle_upper, ov5640_settings[isize].mipi.ths_zero);
	/* Send settings to ISP-CSI2 Receiver PHY */
	sensor->pdata->csi2_calc_phy_cfg0(s, current_clk.mipi_clk,
		ov5640_settings[isize].mipi.ths_settle_lower,
		ov5640_settings[isize].mipi.ths_settle_upper);


	/* Dump some registers for debug purposes */
	printk(KERN_DEBUG "ov5640:THSPREPARE=0x%02X\n",
		ov5640_settings[isize].mipi.ths_prepare);
	printk(KERN_DEBUG "ov5640:THSZERO=0x%02X\n",
		ov5640_settings[isize].mipi.ths_zero);
	printk(KERN_DEBUG "ov5640:LANESEL=0x%02X\n",
		(ov5640_settings[isize].mipi.data_lanes == 2) ? 0 : 1);

	return 0;
}

/**
 * ov5640_configure_frame - initializes image frame registers
 * @c: i2c client driver structure
 * @isize: image size enum
 */
static int ov5640_configure_frame(struct i2c_client *client, unsigned isize)
{
	u32 val;
	printk("%s...........%d\n",__func__,__LINE__);
	
#if 0
	ov5640_write_reg(client, 0x3800,
	    (ov5640_settings[isize].frame.x_addr_start&0xFF00)>>8, I2C_8BIT);
	ov5640_write_reg(client, 0x3801,
	    (ov5640_settings[isize].frame.x_addr_start&0xFF), I2C_8BIT);
	ov5640_write_reg(client, 0x3802,
	    (ov5640_settings[isize].frame.y_addr_start&0xFF00)>>8, I2C_8BIT);
	ov5640_write_reg(client, 0x3803,
	    (ov5640_settings[isize].frame.y_addr_start&0xFF), I2C_8BIT);
	ov5640_write_reg(client, 0x3804,
	    (ov5640_settings[isize].frame.x_addr_end&0xFF00)>>8, I2C_8BIT);
	ov5640_write_reg(client, 0x3805,
	    (ov5640_settings[isize].frame.x_addr_end&0xFF), I2C_8BIT);
	ov5640_write_reg(client, 0x3806,
	    (ov5640_settings[isize].frame.y_addr_end&0xFF00)>>8, I2C_8BIT);
	ov5640_write_reg(client, 0x3807,
	    (ov5640_settings[isize].frame.y_addr_end&0xFF), I2C_8BIT);
	ov5640_write_reg(client, 0x3808,
	    (ov5640_settings[isize].frame.x_output_size&0xFF00)>>8, I2C_8BIT);
	ov5640_write_reg(client, 0x3809,
	    (ov5640_settings[isize].frame.x_output_size&0xFF), I2C_8BIT);
	ov5640_write_reg(client, 0x380a,
	    (ov5640_settings[isize].frame.y_output_size&0xFF00)>>8, I2C_8BIT);
	ov5640_write_reg(client, 0x380b,
	    (ov5640_settings[isize].frame.y_output_size&0xFF), I2C_8BIT);
	ov5640_write_reg(client, 0x380c,
	    (ov5640_settings[isize].frame.x_total_size&0xFF00)>>8, I2C_8BIT);
	ov5640_write_reg(client, 0x380d,
	    (ov5640_settings[isize].frame.x_total_size&0xFF), I2C_8BIT);
	ov5640_write_reg(client, 0x380e,
	    (ov5640_settings[isize].frame.y_total_size&0xFF00)>>8, I2C_8BIT);
	ov5640_write_reg(client, 0x380f,
	    (ov5640_settings[isize].frame.y_total_size&0xFF), I2C_8BIT);
	ov5640_write_reg(client, 0x3810,
	    (ov5640_settings[isize].frame.isp_x_offset&0xFF00)>>8, I2C_8BIT);
	ov5640_write_reg(client, 0x3811,
	    (ov5640_settings[isize].frame.isp_x_offset&0xFF), I2C_8BIT);
	ov5640_write_reg(client, 0x3812,
	    (ov5640_settings[isize].frame.isp_y_offset&0xFF00)>>8, I2C_8BIT);
	ov5640_write_reg(client, 0x3813,
	    (ov5640_settings[isize].frame.isp_y_offset&0xFF), I2C_8BIT);
	ov5640_write_reg(client, 0x3814,
	    ((ov5640_settings[isize].frame.x_odd_inc&0xF)<<4 |(ov5640_settings[isize].frame.x_even_inc&0xF)),I2C_8BIT);
	ov5640_write_reg(client, 0x3815,
	    ((ov5640_settings[isize].frame.y_odd_inc&0xF)<<4 |(ov5640_settings[isize].frame.y_even_inc&0xF)),I2C_8BIT);
#endif 

	ov5640_write_reg(client, 0x3820, 0x41, I2C_8BIT);
	ov5640_write_reg(client, 0x3821, 0x07, I2C_8BIT);

	ov5640_write_reg(client, 0x3803, 0x04, I2C_8BIT);
	ov5640_write_reg(client, 0x3806, 0x07, I2C_8BIT);
	ov5640_write_reg(client, 0x3807, 0x9b, I2C_8BIT);
	ov5640_write_reg(client, 0x3808, 0x02, I2C_8BIT);
	ov5640_write_reg(client, 0x3809, 0x80, I2C_8BIT);
	ov5640_write_reg(client, 0x380a, 0x01, I2C_8BIT);
	ov5640_write_reg(client, 0x380b, 0xe0, I2C_8BIT);
	ov5640_write_reg(client, 0x380c, 0x07, I2C_8BIT);
	ov5640_write_reg(client, 0x380d, 0x68, I2C_8BIT);
	ov5640_write_reg(client, 0x380e, 0x03, I2C_8BIT);
	ov5640_write_reg(client, 0x380f, 0xd8, I2C_8BIT);
	ov5640_write_reg(client, 0x3813, 0x06, I2C_8BIT);
	
	ov5640_write_reg(client, 0x3618, 0x00, I2C_8BIT);
	ov5640_write_reg(client, 0x3612, 0x29, I2C_8BIT);
	ov5640_write_reg(client, 0x3709, 0x52, I2C_8BIT);
	ov5640_write_reg(client, 0x370c, 0x03, I2C_8BIT);

	return 0;
}

 /**
 * ov5640_configure_test_pattern - Configure 3 possible test pattern modes
 * @ mode: Test pattern mode. Possible modes : 1 , 2 and 4.
 * @s: pointer to standard V4L2 device structure
 * @lvc: pointer to V4L2 exposure entry in ov5640sensor_video_controls array
 *
 * If the requested test pattern mode is within the allowed limits, the HW
 * is configured for that particular test pattern, and the
 * ov5640sensor_video_control[] array is updated with the new current value.
 * The function returns 0 upon success.  Otherwise an error code is
 * returned.
 */
static int ov5640_configure_test_pattern(int mode, struct v4l2_int_device *s,
					 struct vcontrol *lvc)
{
#if 0
	struct ov5640_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	printk("%s...........%d\n",__func__,__LINE__);

	if ((current_power_state == V4L2_POWER_ON) || sensor->resuming) {

		switch (mode) {
		case OV5640_TEST_PATT_COLOR_BAR:
		case OV5640_TEST_PATT_PN9:
			/* red */
			ov5640_write_reg(client, OV5640_REG_TEST_PATT_RED,
							0x07ff, I2C_16BIT);
			/* green-red */
			ov5640_write_reg(client, OV5640_REG_TEST_PATT_GREENR,
							0x00ff,	I2C_16BIT);
			/* blue */
			ov5640_write_reg(client, OV5640_REG_TEST_PATT_BLUE,
							0x0000, I2C_16BIT);
			/* green-blue */
			ov5640_write_reg(client, OV5640_REG_TEST_PATT_GREENB,
							0x0000,	I2C_16BIT);
			break;
		case OV5640_TEST_PATT_SOLID_COLOR:
			/* red */
			ov5640_write_reg(client, OV5640_REG_TEST_PATT_RED,
				(OV5640_BLACK_LEVEL_AVG & 0x00ff), I2C_16BIT);
			/* green-red */
			ov5640_write_reg(client, OV5640_REG_TEST_PATT_GREENR,
				(OV5640_BLACK_LEVEL_AVG & 0x00ff), I2C_16BIT);
			/* blue */
			ov5640_write_reg(client, OV5640_REG_TEST_PATT_BLUE,
				(OV5640_BLACK_LEVEL_AVG & 0x00ff), I2C_16BIT);
			/* green-blue */
			ov5640_write_reg(client, OV5640_REG_TEST_PATT_GREENB,
				(OV5640_BLACK_LEVEL_AVG & 0x00ff), I2C_16BIT);
			break;
		}
		/* test-pattern mode */
		ov5640_write_reg(client, OV5640_REG_TEST_PATT_MODE,
						(mode & 0x7), I2C_16BIT);
		/* Disable sensor ISP processing */
		ov5640_write_reg(client, OV5640_REG_TESBYPEN,
					(mode == 0) ? 0x0 : 0x10, I2C_8BIT);
	}
	lvc->current_value = mode;
	return 0;
#endif

	return true;
}
/**
 * ov5640_configure - Configure the ov5640 for the specified image mode
 * @s: pointer to standard V4L2 device structure
 *
 * Configure the ov5640 for a specified image size, pixel format, and frame
 * period.  xclk is the frequency (in Hz) of the xclk input to the ov5640.
 * fper is the frame period (in seconds) expressed as a fraction.
 * Returns zero if successful, or non-zero otherwise.
 * The actual frame period is returned in fper.
 */
/*******************************************************/
struct ov5640_reg ov5640_preview_tbl[] = {
/* @@ MIPI_2lane_5M to vga(YUV) 30fps 99 1280 960 98 0 0 */
    {0x3503, 0x00, I2C_8BIT}, /* enable AE back from capture to preview */
    {0x3035, 0x11, I2C_8BIT},
    {0x3036, 0x38, I2C_8BIT},
    {0x3820, 0x41, I2C_8BIT},
    {0x3821, 0x07, I2C_8BIT},
    {0x3814, 0x31, I2C_8BIT},
    {0x3815, 0x31, I2C_8BIT},
    {0x3803, 0x04, I2C_8BIT},
    {0x3807, 0x9b, I2C_8BIT},
    {0x3808, 0x05, I2C_8BIT},
    {0x3809, 0x00, I2C_8BIT},
    {0x380a, 0x03, I2C_8BIT},
    {0x380b, 0xc0, I2C_8BIT},
    {0x380c, 0x07, I2C_8BIT},
    {0x380d, 0x68, I2C_8BIT},
    {0x380e, 0x04, I2C_8BIT},
    {0x380f, 0x9c, I2C_8BIT},
    {0x3813, 0x06, I2C_8BIT},
    {0x3618, 0x00, I2C_8BIT},
    {0x3612, 0x29, I2C_8BIT},
    {0x3708, 0x64, I2C_8BIT},
    {0x3709, 0x52, I2C_8BIT},
    {0x370c, 0x03, I2C_8BIT},
    {0x5001, 0xa3, I2C_8BIT},
    {0x4004, 0x02, I2C_8BIT},
    {0x4005, 0x1a, I2C_8BIT},//0x18
    {0x4837, 0x15, I2C_8BIT},//0x44
    {0x4713, 0x03, I2C_8BIT},
    {0x4407, 0x04, I2C_8BIT},
    {0x460b, 0x35, I2C_8BIT},
    {0x460c, 0x22, I2C_8BIT},
    {0x3824, 0x02, I2C_8BIT},
    {0x3a02, 0x0b, I2C_8BIT},
    {0x3a03, 0x88, I2C_8BIT},
    {0x3a14, 0x0b, I2C_8BIT},
    {0x3a15, 0x86, I2C_8BIT},
};


/*******************************************************/

static int ov5640_configure(struct v4l2_int_device *s)
{
	struct ov5640_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	unsigned isize = isize_current;
	int err, i;
	struct vcontrol *lvc = NULL;
	printk("%s............................................%d\n",__func__,__LINE__);

	err = ov5640_write_reg(client, OV5640_REG_SW_RESET, 0x01, I2C_8BIT);
	mdelay(5);

	ov5640_write_regs(client, initial_list);

	//ov5640_setup_pll(client, isize);

	ov5640_setup_mipi(s, isize);

	/* configure image size and pixel format */
	ov5640_configure_frame(client, isize);

//	ov5640_set_orientation(client, sensor->ver);

	sensor->pdata->csi2_cfg_vp_out_ctrl(s, 2);
//	sensor->pdata->csi2_ctrl_update(s, false);
	sensor->pdata->csi2_ctrl_update(s, true);

	sensor->pdata->csi2_cfg_virtual_id(s, 0, 0);
//	sensor->pdata->csi2_ctx_update(s, 0, false);
	sensor->pdata->csi2_ctx_update(s, 0, true);
//	ov5640_set_virtual_id(client, OV5640_CSI2_VIRTUAL_ID);
	/* Set initial exposure and gain */
	i = find_vctrl(V4L2_CID_EXPOSURE);
	if (i >= 0) {
		lvc = &ov5640sensor_video_control[i];
		ov5640sensor_set_exposure_time(lvc->current_value,
					sensor->v4l2_int_device, lvc);
	}

	i = find_vctrl(V4L2_CID_GAIN);
	if (i >= 0) {
		lvc = &ov5640sensor_video_control[i];
		ov5640sensor_set_gain(lvc->current_value,
				sensor->v4l2_int_device, lvc);
	}

#if 0
	i = find_vctrl(V4L2_CID_TEST_PATTERN);
	if (i >= 0) {
		lvc = &ov5640sensor_video_control[i];
		ov5640_configure_test_pattern(lvc->current_value,
				sensor->v4l2_int_device, lvc);
	}
#endif

/**********************************************************/
ov5640_write_reg(client, 0x4800, 0x24, I2C_8BIT);
msleep(10);
ov5640_write_reg(client, 0x4202, 0x0f, I2C_8BIT);

msleep(66);
ov5640_write_regs(client, ov5640_preview_tbl);

ov5640_write_reg(client, 0x350b, (0x7f & 0x3ff) & 0xff, I2C_8BIT);
ov5640_write_reg(client, 0x350a, ((0x7f & 0x3ff) & 0xff) >> 8, I2C_8BIT);

ov5640_write_reg(client, 0x3502, (0x1207 & 0x0f) << 4, I2C_8BIT);
ov5640_write_reg(client, 0x3501, (0x1207 & 0xfff) >> 4, I2C_8BIT);
ov5640_write_reg(client, 0x3501, 0x1207 >> 12, I2C_8BIT);

/*************/
//for exposure conversion
//...
//set ae_target
//...
/*************/

ov5640_write_reg(client, 0x3503, 0x0, I2C_8BIT);
/**********************************************************/

	/* configure streaming ON */
	err = ov5640_write_reg(client, 0x4202, 0x00, I2C_8BIT);
	mdelay(1);

/*********************************/
msleep(10);
ov5640_write_reg(client, 0x4800, 0x04, I2C_8BIT);
/*********************************/
unsigned int val = 0;
int x = 0;
void * v_addr;

v_addr = ioremap(0x48002116, 2);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_dx2:0x%x\n", val);

v_addr = ioremap(0x48002118, 2);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_dy2:0x%x\n", val);

v_addr = ioremap(0x48002134, 2);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_dx0:0x%x\n", val);

v_addr = ioremap(0x48002136, 2);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_dy0:0x%x\n", val);

v_addr = ioremap(0x48002138, 2);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_dx1:0x%x\n", val);

v_addr = ioremap(0x4800213a, 2);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_dy1:0x%x\n", val);

/********************************************/

v_addr = ioremap(0x480BD810, 4);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_sysconfig:0x%x\n", val);


v_addr = ioremap(0x480BD970, 4);
val = *((volatile unsigned int *)v_addr);
printk("----------------csiphy_reg0:0x%x\n", val);

v_addr = ioremap(0x480BD974, 4);
val = *((volatile unsigned int *)v_addr);
printk("----------------csiphy_reg1:0x%x\n", val);

v_addr = ioremap(0x480BD978, 4);
val = *((volatile unsigned int *)v_addr);
printk("----------------csiphy_reg2:0x%x\n", val);

/********************************************/

v_addr = ioremap(0x480BD850, 4);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_complexio_cfg1:0x%x\n", val);

v_addr = ioremap(0x480BD86C, 4);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_timing:0x%x\n", val);


v_addr = ioremap(0x480BD840, 4);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_ctrl:0x%x\n", val);

v_addr = ioremap(0x480BD81C, 4);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_irqenable:0x%x\n", val);

v_addr = ioremap(0x480BD818, 4);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_irqstatus:0x%x\n", val);

v_addr = ioremap((0x480BD860 + (x * 0x20)), 4);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_complexio1_irqenable:0x%x\n", val);

v_addr = ioremap((0x480BD854 + (x * 0x20)), 4);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_complexio1_irqstat:0x%x\n", val);
//-------------------------------------------------------------
v_addr = ioremap((0x480bd870 + (x * 0x20)), 4);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_ctx_ctrl1:0x%x\n", val);

v_addr = ioremap((0x480bd874 + (x * 0x20)), 4);
val = *((volatile unsigned int __force *)v_addr);
printk("----------------csi2_ctx_ctrl2:0x%x\n", val);

v_addr = ioremap((0x480bd9c0 + (x * 0x8)), 4);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_ctx_transcodeh:0x%x\n", val);

v_addr = ioremap((0x480bd9c4 + (x * 0x8)), 4);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_ctx_transcodev:0x%x\n", val);

v_addr = ioremap((0x480bd878 + (x * 0x20)), 4);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_ctx_dat_ping_addr:0x%x\n", val);

v_addr = ioremap((0x480bd880 + (x * 0x20)), 4);
val = *((volatile unsigned int *)v_addr);
printk("----------------csi2_ctx_dat_pong_addr:0x%x\n", val);
/*********************************/

	return err;
}

/**
 * ov5640_detect - Detect if an ov5640 is present, and if so which revision
 * @client: pointer to the i2c client driver structure
 *
 * Detect if an ov5640 is present, and if so which revision.
 * A device is considered to be detected if the manufacturer ID (MIDH and MIDL)
 * and the product ID (PID) registers match the expected values.
 * Any value of the version ID (VER) register is accepted.
 * Returns a negative error number if no device is detected, or the
 * non-negative value of the version ID register if a device is detected.
 */

#define OV5640_REG_PIDH_ADDR 0x300a
#define OV5640_REG_PIDL_ADDR 0x300b

static int ov5640_detect(struct i2c_client *client)
{
	u32 pid_value,ver_value;
	struct ov5640_sensor *sensor;

	if (!client)
		return -ENODEV;

	printk("%s...........%d\n",__func__,__LINE__);
	
	sensor = i2c_get_clientdata(client);

	if (ov5640_read_reg(client, I2C_8BIT, OV5640_REG_PIDH_ADDR, &pid_value))
		return -ENODEV;
	if (ov5640_read_reg(client, I2C_8BIT, OV5640_REG_PIDL_ADDR, &ver_value))
		return -ENODEV;

	printk("%s..............pid_value =%x,ver_value=%x.......\n ",__func__,pid_value,ver_value);
	printk("%s..............pid_value =%x,ver_value=%x........\n ",__func__,pid_value,ver_value);
	printk("%s..............pid_value =%x,ver_value=%x.......\n ",__func__,pid_value,ver_value);
	printk("%s..............pid_value =%x,ver_value=%x........\n ",__func__,pid_value,ver_value);
	mdelay(100);

	return (pid_value<<8 | ver_value);
}

/**
 * ioctl_queryctrl - V4L2 sensor interface handler for VIDIOC_QUERYCTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @qc: standard V4L2 VIDIOC_QUERYCTRL ioctl structure
 *
 * If the requested control is supported, returns the control information
 * from the ov5640sensor_video_control[] array.
 * Otherwise, returns -EINVAL if the control is not supported.
 */
static int ioctl_queryctrl(struct v4l2_int_device *s, struct v4l2_queryctrl *qc)
{
	int i;

	printk("%s...........%d\n",__func__,__LINE__);
	i = find_vctrl(qc->id);
	if (i == -EINVAL)
		qc->flags = V4L2_CTRL_FLAG_DISABLED;

	if (i < 0)
		return -EINVAL;

	*qc = ov5640sensor_video_control[i].qc;
	return 0;
}

/**
 * ioctl_g_ctrl - V4L2 sensor interface handler for VIDIOC_G_CTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @vc: standard V4L2 VIDIOC_G_CTRL ioctl structure
 *
 * If the requested control is supported, returns the control's current
 * value from the ov5640sensor_video_control[] array.
 * Otherwise, returns -EINVAL if the control is not supported.
 */
static int ioctl_g_ctrl(struct v4l2_int_device *s, struct v4l2_control *vc)
{
	struct vcontrol *lvc;
	int i;
	printk("%s...........%d\n",__func__,__LINE__);

	i = find_vctrl(vc->id);
	if (i < 0)
		return -EINVAL;
	lvc = &ov5640sensor_video_control[i];

	switch (vc->id) {
	case  V4L2_CID_EXPOSURE:
		vc->value = lvc->current_value;
		break;
	case V4L2_CID_GAIN:
		vc->value = lvc->current_value;
		break;
	case V4L2_CID_TEST_PATTERN:
		vc->value = lvc->current_value;
		break;
	}

	return 0;
}

/**
 * ioctl_s_ctrl - V4L2 sensor interface handler for VIDIOC_S_CTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @vc: standard V4L2 VIDIOC_S_CTRL ioctl structure
 *
 * If the requested control is supported, sets the control's current
 * value in HW (and updates the ov5640sensor_video_control[] array).
 * Otherwise, * returns -EINVAL if the control is not supported.
 */
static int ioctl_s_ctrl(struct v4l2_int_device *s, struct v4l2_control *vc)
{
	int retval = -EINVAL;
	int i;
	struct vcontrol *lvc;

	printk("%s...........%d\n",__func__,__LINE__);
	i = find_vctrl(vc->id);
	if (i < 0)
		return -EINVAL;
	lvc = &ov5640sensor_video_control[i];

	switch (vc->id) {
	case V4L2_CID_EXPOSURE:
		retval = ov5640sensor_set_exposure_time(vc->value, s, lvc);
		break;
	case V4L2_CID_GAIN:
		retval = ov5640sensor_set_gain(vc->value, s, lvc);
		break;
	case V4L2_CID_TEST_PATTERN:
		retval = ov5640_configure_test_pattern(vc->value, s, lvc);
		break;
	}

	return retval;
}


/**
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

	printk("%s...........%d\n",__func__,__LINE__);
	switch (fmt->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		if (index >= NUM_CAPTURE_FORMATS)
			return -EINVAL;
	break;
	default:
		return -EINVAL;
	}

	fmt->flags = ov5640_formats[index].flags;
	strlcpy(fmt->description, ov5640_formats[index].description,
					sizeof(fmt->description));
	fmt->pixelformat = ov5640_formats[index].pixelformat;

	return 0;
}

/**
 * ioctl_try_fmt_cap - Implement the CAPTURE buffer VIDIOC_TRY_FMT ioctl
 * @s: pointer to standard V4L2 device structure
 * @f: pointer to standard V4L2 VIDIOC_TRY_FMT ioctl structure
 *
 * Implement the VIDIOC_TRY_FMT ioctl for the CAPTURE buffer type.  This
 * ioctl is used to negotiate the image capture size and pixel format
 * without actually making it take effect.
 */
static int ioctl_try_fmt_cap(struct v4l2_int_device *s, struct v4l2_format *f)
{
	unsigned isize;
	int ifmt;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct ov5640_sensor *sensor = s->priv;
	struct v4l2_pix_format *pix2 = &sensor->pix;
	printk("%s...........%d\n",__func__,__LINE__);

	isize = ov5640_find_size(pix->width, pix->height);
	isize_current = isize;

	pix->width = ov5640_settings[isize].frame.x_output_size;
	pix->height = ov5640_settings[isize].frame.y_output_size;
	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (pix->pixelformat == ov5640_formats[ifmt].pixelformat)
			break;
	}
	if (ifmt == NUM_CAPTURE_FORMATS)
		ifmt = 0;
	pix->pixelformat = ov5640_formats[ifmt].pixelformat;
	pix->field = V4L2_FIELD_NONE;
	pix->bytesperline = pix->width * 2;
	pix->sizeimage = pix->bytesperline * pix->height;
	pix->priv = 0;
	pix->colorspace = V4L2_COLORSPACE_SRGB;
	*pix2 = *pix;

	return 0;
}

/**
 * ioctl_s_fmt_cap - V4L2 sensor interface handler for VIDIOC_S_FMT ioctl
 * @s: pointer to standard V4L2 device structure
 * @f: pointer to standard V4L2 VIDIOC_S_FMT ioctl structure
 *
 * If the requested format is supported, configures the HW to use that
 * format, returns error code if format not supported or HW can't be
 * correctly configured.
 */
static int ioctl_s_fmt_cap(struct v4l2_int_device *s, struct v4l2_format *f)
{
	struct ov5640_sensor *sensor = s->priv;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	int rval;

	printk("%s...........%d\n",__func__,__LINE__);
	rval = ioctl_try_fmt_cap(s, f);
	if (rval)
		return rval;
	else
		sensor->pix = *pix;

	return rval;
}

/**
 * ioctl_g_fmt_cap - V4L2 sensor interface handler for ioctl_g_fmt_cap
 * @s: pointer to standard V4L2 device structure
 * @f: pointer to standard V4L2 v4l2_format structure
 *
 * Returns the sensor's current pixel format in the v4l2_format
 * parameter.
 */
static int ioctl_g_fmt_cap(struct v4l2_int_device *s, struct v4l2_format *f)
{
	struct ov5640_sensor *sensor = s->priv;
	f->fmt.pix = sensor->pix;
	printk("%s...........%d\n",__func__,__LINE__);

	return 0;
}

/**
 * ioctl_g_pixclk - V4L2 sensor interface handler for ioctl_g_pixclk
 * @s: pointer to standard V4L2 device structure
 * @pixclk: pointer to unsigned 32 var to store pixelclk in HZ
 *
 * Returns the sensor's current pixel clock in HZ
 */
static int ioctl_priv_g_pixclk(struct v4l2_int_device *s, u32 *pixclk)
{
	*pixclk = current_clk.vt_pix_clk;

	printk("%s...........%d\n",__func__,__LINE__);

	return 0;
}

/**
 * ioctl_g_activesize - V4L2 sensor interface handler for ioctl_g_activesize
 * @s: pointer to standard V4L2 device structure
 * @pix: pointer to standard V4L2 v4l2_pix_format structure
 *
 * Returns the sensor's current active image basesize.
 */
static int ioctl_priv_g_activesize(struct v4l2_int_device *s,
				   struct v4l2_rect *pix)
{
	struct ov5640_frame_settings *frm;

	frm = &ov5640_settings[isize_current].frame;
	pix->left = frm->x_addr_start /
		((frm->x_even_inc + frm->x_odd_inc) / 2);
	pix->top = frm->y_addr_start /
		((frm->y_even_inc + frm->y_odd_inc) / 2);
	pix->width = ((frm->x_addr_end + 1) - frm->x_addr_start) /
		((frm->x_even_inc + frm->x_odd_inc) / 2);
	pix->height = ((frm->y_addr_end + 1) - frm->y_addr_start) /
		((frm->y_even_inc + frm->y_odd_inc) / 2);

	printk("%s...........%d\n",__func__,__LINE__);
	return 0;
}

/**
 * ioctl_g_fullsize - V4L2 sensor interface handler for ioctl_g_fullsize
 * @s: pointer to standard V4L2 device structure
 * @pix: pointer to standard V4L2 v4l2_pix_format structure
 *
 * Returns the sensor's biggest image basesize.
 */
static int ioctl_priv_g_fullsize(struct v4l2_int_device *s,
				 struct v4l2_rect *pix)
{
	struct ov5640_frame_settings *frm;

	frm = &ov5640_settings[isize_current].frame;
	pix->left = 0;
	pix->top = 0;
	pix->width = frm->line_len_pck;
	pix->height = frm->frame_len_lines;
	printk("%s...........%d\n",__func__,__LINE__);

	return 0;
}

/**
 * ioctl_g_pixelsize - V4L2 sensor interface handler for ioctl_g_pixelsize
 * @s: pointer to standard V4L2 device structure
 * @pix: pointer to standard V4L2 v4l2_pix_format structure
 *
 * Returns the sensor's configure pixel size.
 */
static int ioctl_priv_g_pixelsize(struct v4l2_int_device *s,
				  struct v4l2_rect *pix)
{
	struct ov5640_frame_settings *frm;

	printk("%s...........%d\n",__func__,__LINE__);
	frm = &ov5640_settings[isize_current].frame;
	pix->left = 0;
	pix->top = 0;
	pix->width = (frm->x_even_inc + frm->x_odd_inc) / 2;
	pix->height = (frm->y_even_inc + frm->y_odd_inc) / 2;

	return 0;
}

/**
 * ioctl_priv_g_pixclk_active - V4L2 sensor interface handler
 *                              for ioctl_priv_g_pixclk_active
 * @s: pointer to standard V4L2 device structure
 * @pixclk: pointer to unsigned 32 var to store pixelclk in HZ
 *
 * The function calculates optimal pixel clock which can use
 * the data received from sensor complying with all the
 * peculiarities of the sensors and the currently selected mode.
 */
static int ioctl_priv_g_pixclk_active(struct v4l2_int_device *s, u32 *pixclk)
{
	struct v4l2_rect full, active;
	u32 sens_pixclk;
	printk("%s...........%d\n",__func__,__LINE__);

	ioctl_priv_g_activesize(s, &active);
	ioctl_priv_g_fullsize(s, &full);
	ioctl_priv_g_pixclk(s, &sens_pixclk);

	*pixclk = (sens_pixclk / full.width) * active.width;
	printk("%s...........%d\n",__func__,__LINE__);

	return 0;
}

/**
 * ioctl_g_parm - V4L2 sensor interface handler for VIDIOC_G_PARM ioctl
 * @s: pointer to standard V4L2 device structure
 * @a: pointer to standard V4L2 VIDIOC_G_PARM ioctl structure
 *
 * Returns the sensor's video CAPTURE parameters.
 */
static int ioctl_g_parm(struct v4l2_int_device *s, struct v4l2_streamparm *a)
{
	struct ov5640_sensor *sensor = s->priv;
	struct v4l2_captureparm *cparm = &a->parm.capture;
	printk("%s...........%d\n",__func__,__LINE__);

	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(a, 0, sizeof(*a));
	a->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	cparm->capability = V4L2_CAP_TIMEPERFRAME;
	cparm->timeperframe = sensor->timeperframe;

	return 0;
}

/**
 * ioctl_s_parm - V4L2 sensor interface handler for VIDIOC_S_PARM ioctl
 * @s: pointer to standard V4L2 device structure
 * @a: pointer to standard V4L2 VIDIOC_S_PARM ioctl structure
 *
 * Configures the sensor to use the input parameters, if possible.  If
 * not possible, reverts to the old parameters and returns the
 * appropriate error code.
 */
static int ioctl_s_parm(struct v4l2_int_device *s, struct v4l2_streamparm *a)
{
	struct ov5640_sensor *sensor = s->priv;
	struct v4l2_fract *timeperframe = &a->parm.capture.timeperframe;
	int err = 0;

	sensor->timeperframe = *timeperframe;
	printk("%s...........%d\n",__func__,__LINE__);
	ov5640sensor_calc_xclk();
	ov5640_update_clocks(xclk_current, isize_current);
	err = ov5640_set_framerate(s, &sensor->timeperframe);
	*timeperframe = sensor->timeperframe;

	return err;
}

/**
 * ioctl_g_priv - V4L2 sensor interface handler for vidioc_int_g_priv_num
 * @s: pointer to standard V4L2 device structure
 * @p: void pointer to hold sensor's private data address
 *
 * Returns device's (sensor's) private data area address in p parameter
 */
static int ioctl_g_priv(struct v4l2_int_device *s, void *p)
{
	struct ov5640_sensor *sensor = s->priv;
	printk("%s...........%d\n",__func__,__LINE__);

	return sensor->pdata->priv_data_set(s, p);
}

static int __ov5640_power_off_standby(struct v4l2_int_device *s,
				      enum v4l2_power on)
{
	struct ov5640_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int rval;
	printk("%s...........%d\n",__func__,__LINE__);

	rval = sensor->pdata->power_set(s, on);
	if (rval < 0) {
		v4l_err(client, "Unable to set the power state: "
			OV5640_DRIVER_NAME " sensor\n");
		return rval;
	}

	sensor->pdata->set_xclk(s, 0);
	return 0;
}

static int ov5640_power_off(struct v4l2_int_device *s)
{
	printk("%s...........%d\n",__func__,__LINE__);
	return __ov5640_power_off_standby(s, V4L2_POWER_OFF);
}

static int ov5640_power_standby(struct v4l2_int_device *s)
{
	printk("%s...........%d\n",__func__,__LINE__);
	return __ov5640_power_off_standby(s, V4L2_POWER_STANDBY);
}

static int ov5640_power_on(struct v4l2_int_device *s)
{
	struct ov5640_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int rval;

	printk("%s...........%d\n",__func__,__LINE__);

	rval = sensor->pdata->power_set(s, V4L2_POWER_ON);
	if (rval < 0) {
		v4l_err(client, "Unable to set the power state: "
			OV5640_DRIVER_NAME " sensor\n");
		sensor->pdata->set_xclk(s, 0);
		return rval;
	}
	sensor->pdata->set_xclk(s, xclk_current);

	return 0;
}

/**
 * ioctl_s_power - V4L2 sensor interface handler for vidioc_int_s_power_num
 * @s: pointer to standard V4L2 device structure
 * @on: power state to which device is to be set
 *
 * Sets devices power state to requrested state, if possible.
 */
static int ioctl_s_power(struct v4l2_int_device *s, enum v4l2_power on)
{
	struct ov5640_sensor *sensor = s->priv;
	struct vcontrol *lvc;
	int i;
	printk("%s...........%d\n",__func__,__LINE__);

	switch (on) {
	case V4L2_POWER_ON:
		ov5640_power_on(s);
		if (current_power_state == V4L2_POWER_STANDBY) {
printk("-----------------------------ioctl_s_power--------------xxxx here\n");
			sensor->resuming = true;
			ov5640_configure(s);
		}
		break;
	case V4L2_POWER_OFF:
		ov5640_power_off(s);
#if 0
		/* Reset defaults for controls */
		i = find_vctrl(V4L2_CID_GAIN);
		if (i >= 0) {
			lvc = &ov5640sensor_video_control[i];
			lvc->current_value = OV5640_EV_DEF_GAIN;
		}
		i = find_vctrl(V4L2_CID_EXPOSURE);
		if (i >= 0) {
			lvc = &ov5640sensor_video_control[i];
			lvc->current_value = OV5640_DEF_EXPOSURE;
		}
		i = find_vctrl(V4L2_CID_TEST_PATTERN);
		if (i >= 0) {
			lvc = &ov5640sensor_video_control[i];
			lvc->current_value = OV5640_MIN_TEST_PATT_MODE;
		}
#endif
		break;
	case V4L2_POWER_STANDBY:
		ov5640_power_standby(s);
		break;
	}

	sensor->resuming = false;
	current_power_state = on;
	return 0;
}

/**
 * ioctl_init - V4L2 sensor interface handler for VIDIOC_INT_INIT
 * @s: pointer to standard V4L2 device structure
 *
 * Initialize the sensor device (call ov5640_configure())
 */
static int ioctl_init(struct v4l2_int_device *s)
{
	printk("%s...........%d\n",__func__,__LINE__);
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
	printk("%s...........%d\n",__func__,__LINE__);
	return 0;
}

/**
 * ioctl_dev_init - V4L2 sensor interface handler for vidioc_int_dev_init_num
 * @s: pointer to standard V4L2 device structure
 *
 * Initialise the device when slave attaches to the master.  Returns 0 if
 * ov5640 device could be found, otherwise returns appropriate error.
 */
static int ioctl_dev_init(struct v4l2_int_device *s)
{
	struct ov5640_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int err;

	printk("%s...........%d\n",__func__,__LINE__);
	err = ov5640_power_on(s);
	if (err)
		return -ENODEV;

	err = ov5640_detect(client);
	if (err < 0) {
		v4l_err(client, "Unable to detect "
				OV5640_DRIVER_NAME " sensor\n");

		/*
		 * Turn power off before leaving the function.
		 * If not, CAM Pwrdm will be ON which is not needed
		 * as there is no sensor detected.
		 */
		ov5640_power_off(s);

		return err;
	}
	sensor->ver = err;
	v4l_info(client, OV5640_DRIVER_NAME
		" chip version 0x%02x detected\n", sensor->ver);

#if 1
	err = ov5640_power_off(s);
	if (err) {
		printk("-------power off err in func:%s line:%d\n", __func__, __LINE__);
		return -ENODEV;
	}
#endif

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
	printk("%s...........%d\n",__func__,__LINE__);

	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (frms->pixel_format == ov5640_formats[ifmt].pixelformat)
			break;
	}
	/* Is requested pixelformat not found on sensor? */
	if (ifmt == NUM_CAPTURE_FORMATS)
		return -EINVAL;

	/* Check that the index we are being asked for is not
	   out of bounds. */
	if (frms->index >= OV5640_MODES_COUNT)
		return -EINVAL;

	frms->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	frms->discrete.width =
		ov5640_settings[frms->index].frame.x_output_size;
	frms->discrete.height =
		ov5640_settings[frms->index].frame.y_output_size;

	return 0;
}

static const struct v4l2_fract ov5640_frameintervals[] = {
	{ .numerator = 3, .denominator = 30 },
	{ .numerator = 2, .denominator = 25 },
	{ .numerator = 1, .denominator = 15 },
	{ .numerator = 1, .denominator = 20 },
	{ .numerator = 1, .denominator = 25 },
	{ .numerator = 1, .denominator = 30 },
};

static int ioctl_enum_frameintervals(struct v4l2_int_device *s,
				     struct v4l2_frmivalenum *frmi)
{
	int ifmt;
	printk("%s...........%d\n",__func__,__LINE__);

	/* Check that the requested format is one we support */
	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (frmi->pixel_format == ov5640_formats[ifmt].pixelformat)
			break;
	}

	if (ifmt == NUM_CAPTURE_FORMATS)
		return -EINVAL;

	/* Check that the index we are being asked for is not
	   out of bounds. */
	if (frmi->index >= ARRAY_SIZE(ov5640_frameintervals))
		return -EINVAL;

	/* Make sure that the 8MP size reports a max of 10fps */
	if (frmi->width == OV5640_IMAGE_WIDTH_MAX &&
	    frmi->height == OV5640_IMAGE_HEIGHT_MAX) {
		if (frmi->index != 0)
			return -EINVAL;
	}

	frmi->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	frmi->discrete.numerator =
				ov5640_frameintervals[frmi->index].numerator;
	frmi->discrete.denominator =
				ov5640_frameintervals[frmi->index].denominator;

	return 0;
}

static struct v4l2_int_ioctl_desc ov5640_ioctl_desc[] = {
	{ .num = vidioc_int_enum_framesizes_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_enum_framesizes},
	{ .num = vidioc_int_enum_frameintervals_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_enum_frameintervals},
	{ .num = vidioc_int_dev_init_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_dev_init},
	{ .num = vidioc_int_dev_exit_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_dev_exit},
	{ .num = vidioc_int_s_power_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_s_power },
	{ .num = vidioc_int_g_priv_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_priv },
	{ .num = vidioc_int_init_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_init },
	{ .num = vidioc_int_enum_fmt_cap_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_enum_fmt_cap },
	{ .num = vidioc_int_try_fmt_cap_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_try_fmt_cap },
	{ .num = vidioc_int_g_fmt_cap_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_fmt_cap },
	{ .num = vidioc_int_s_fmt_cap_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_s_fmt_cap },
	{ .num = vidioc_int_g_parm_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_parm },
	{ .num = vidioc_int_s_parm_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_s_parm },
	{ .num = vidioc_int_queryctrl_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_queryctrl },
	{ .num = vidioc_int_g_ctrl_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_ctrl },
	{ .num = vidioc_int_s_ctrl_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_s_ctrl },
	{ .num = vidioc_int_priv_g_pixclk_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_priv_g_pixclk },
	{ .num = vidioc_int_priv_g_activesize_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_priv_g_activesize },
	{ .num = vidioc_int_priv_g_fullsize_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_priv_g_fullsize },
	{ .num = vidioc_int_priv_g_pixelsize_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_priv_g_pixelsize },
	{ .num = vidioc_int_priv_g_pixclk_active_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_priv_g_pixclk_active },
};

static struct v4l2_int_slave ov5640_slave = {
	.ioctls = ov5640_ioctl_desc,
	.num_ioctls = ARRAY_SIZE(ov5640_ioctl_desc),
};

static struct v4l2_int_device ov5640_int_device = {
	.module = THIS_MODULE,
	.name = OV5640_DRIVER_NAME,
	.priv = &ov5640,
	.type = v4l2_int_type_slave,
	.u = {
		.slave = &ov5640_slave,
	},
};

/**
 * ov5640_probe - sensor driver i2c probe handler
 * @client: i2c driver client device structure
 *
 * Register sensor as an i2c client device and V4L2
 * device.
 */
static int __devinit ov5640_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct ov5640_sensor *sensor = &ov5640;
	int err;

	printk("%s...................probe.................%d\n",__func__,__LINE__);
	if (i2c_get_clientdata(client))
		return -EBUSY;

	sensor->pdata = client->dev.platform_data;

	if (!sensor->pdata) {
		v4l_err(client, "no platform data?\n");
		return -ENODEV;
	}

	sensor->v4l2_int_device = &ov5640_int_device;
	sensor->i2c_client = client;

	i2c_set_clientdata(client, sensor);

	/* Make the default capture format QCIF V4L2_PIX_FMT_SRGGB10 */
	sensor->pix.width = OV5640_IMAGE_WIDTH_MAX;
	sensor->pix.height = OV5640_IMAGE_HEIGHT_MAX;
	sensor->pix.pixelformat = V4L2_PIX_FMT_SRGGB10;

	printk("%s...........%d\n",__func__,__LINE__);
	err = v4l2_int_device_register(sensor->v4l2_int_device);
	printk("%s...........%d\n",__func__,__LINE__);
	if (err)
		i2c_set_clientdata(client, NULL);

	return 0;
}

/**
 * ov5640_remove - sensor driver i2c remove handler
 * @client: i2c driver client device structure
 *
 * Unregister sensor as an i2c client device and V4L2
 * device.  Complement of ov5640_probe().
 */
static int __exit ov5640_remove(struct i2c_client *client)
{
	struct ov5640_sensor *sensor = i2c_get_clientdata(client);

	if (!client->adapter)
		return -ENODEV;	/* our client isn't attached */

	v4l2_int_device_unregister(sensor->v4l2_int_device);
	i2c_set_clientdata(client, NULL);

	return 0;
}

static const struct i2c_device_id ov5640_id[] = {
	{ OV5640_DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, ov5640_id);

static struct i2c_driver ov5640sensor_i2c_driver = {
	.driver = {
		.name = OV5640_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
	.probe = ov5640_probe,
	.remove = __exit_p(ov5640_remove),
	.id_table = ov5640_id,
};

static struct ov5640_sensor ov5640 = {
	.timeperframe = {
		.numerator = 1,
		.denominator = 30,
	},
};

/**
 * ov5640sensor_init - sensor driver module_init handler
 *
 * Registers driver as an i2c client driver.  Returns 0 on success,
 * error code otherwise.
 */
static int __init ov5640sensor_init(void)
{
	int err;

	err = i2c_add_driver(&ov5640sensor_i2c_driver);
	if (err) {
		printk(KERN_ERR "Failed to register" OV5640_DRIVER_NAME ".\n");
		return err;
	}
	return 0;
}
late_initcall(ov5640sensor_init);

/**
 * ov5640sensor_cleanup - sensor driver module_exit handler
 *
 * Unregisters/deletes driver as an i2c client driver.
 * Complement of ov5640sensor_init.
 */
static void __exit ov5640sensor_cleanup(void)
{
	i2c_del_driver(&ov5640sensor_i2c_driver);
}
module_exit(ov5640sensor_cleanup);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ov5640 camera sensor driver");
