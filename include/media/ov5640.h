#define OV5640_PIDH 0x300a
#define OV5640_PIDL 0x300b

#define OV5640_PIDH_MAGIC 0x56
#define OV5640_PIDL_MAGIC 0x40

#define I2C_8BIT	1
#define I2C_16BIT	2
#define I2C_32BIT	4

#define OV5640_REG_SW_RESET 0x3008

/* CSI2 Virtual ID */
#define OV5640_CSI2_VIRTUAL_ID  0x0

/* Still capture 5 MP */
#define OV5640_IMAGE_WIDTH_MAX  2068
#define OV5640_IMAGE_HEIGHT_MAX 1052

/* Analog gain values */
#define OV5640_EV_MIN_GAIN      0
#define OV5640_EV_MAX_GAIN      30
#define OV5640_EV_DEF_GAIN      21
#define OV5640_EV_GAIN_STEP     1
/* maximum index in the gain EVT */
#define OV5640_EV_TABLE_GAIN_MAX    30

/* Exposure time values */
#define OV5640_MIN_EXPOSURE     250
#define OV5640_MAX_EXPOSURE     128000
#define OV5640_DEF_EXPOSURE     33000
#define OV5640_EXPOSURE_STEP    50

/* Test Pattern Values */
#define OV5640_MIN_TEST_PATT_MODE   0
#define OV5640_MAX_TEST_PATT_MODE   4
#define OV5640_MODE_TEST_PATT_STEP  1

#define OV5640_TEST_PATT_SOLID_COLOR    1
#define OV5640_TEST_PATT_COLOR_BAR  2
#define OV5640_TEST_PATT_PN9        4

#define OV5640_MAX_FRAME_LENGTH_LINES   0xFFFF

#define SENSOR_DETECTED     1
#define SENSOR_NOT_DETECTED 0

struct ov5640_platform_data {
    int (*power_set)(struct v4l2_int_device *s, enum v4l2_power power);
    int (*ifparm)(struct v4l2_ifparm *p);
    int (*priv_data_set)(struct v4l2_int_device *s, void *);
    u32 (*set_xclk)(struct v4l2_int_device *s, u32 xclkfreq);
    int (*cfg_interface_bridge)(u32);
    int (*csi2_lane_count)(struct v4l2_int_device *s, int count);
    int (*csi2_cfg_vp_out_ctrl)(struct v4l2_int_device *s, u8 vp_out_ctrl);
    int (*csi2_ctrl_update)(struct v4l2_int_device *s, bool);
    int (*csi2_cfg_virtual_id)(struct v4l2_int_device *s, u8 ctx, u8 id);
    int (*csi2_ctx_update)(struct v4l2_int_device *s, u8 ctx, bool);
    int (*csi2_calc_phy_cfg0)(struct v4l2_int_device *s, u32, u32, u32);
};

#if 0
struct ov5640_sensor
{
    const struct ov5640_platform_data   *pdata;
    struct v4l2_int_device              *v4l2_int_device;
    struct i2c_client                   *i2c_client;
    struct v4l2_pix_format              pix;
    struct v4l2_fract                   timeperframe;
    int                                 scaler;
    int                                 ver;
    int                                 fps;
    unsigned long                       width;
    unsigned long                       height;
    int                                 state;
};


struct ov5640_reg {
	u16	reg;
	u8	val;
	u16	length;
};

struct ov5640_clk_settings {
    u16 pre_pll_div;
    u16 pll_mult;
    u16 post_pll_div;
    u16 vt_pix_clk_div;
    u16 vt_sys_clk_div;
};

/**
 * struct struct mipi_settings - struct for storage of sensor
 * mipi settings
 */
struct ov5640_mipi_settings {
    u16 data_lanes;
    u16 ths_prepare;
    u16 ths_zero;
    u16 ths_settle_lower;
    u16 ths_settle_upper;
};

/**
 * struct struct frame_settings - struct for storage of sensor
 * frame settings
 */
struct ov5640_frame_settings {
    u16 frame_len_lines_min;
    u16 frame_len_lines;
    u16 line_len_pck;
    u16 x_addr_start;
    u16 x_addr_end;
    u16 y_addr_start;
    u16 y_addr_end;
    u16 x_output_size;
    u16 y_output_size;
    u16 x_even_inc;
    u16 x_odd_inc;
    u16 y_even_inc;
    u16 y_odd_inc;
    u16 v_mode_add;
    u16 h_mode_add;
    u16 h_add_ave;
};

/**
 * struct struct ov5640_sensor_settings - struct for storage of
 * sensor settings.
 */
struct ov5640_sensor_settings {
    struct ov5640_clk_settings clk;
    struct ov5640_mipi_settings mipi;
    struct ov5640_frame_settings frame;
};
#endif
