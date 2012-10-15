#ifndef __LINUX_I2C_ADBS_A320_H
#define __LINUX_I2C_ADBS_A320_H

/* platform data for the ADBS-A320 sensor */
/*******************************************************************************
  ADBS_A320 Register Addresses
*******************************************************************************/
#define A320_PRODUCTID               0x00    // 0x83
#define A320_REVISIONID              0x01    // 0x01
#define A320_MOTION                  0x02
#define A320_DELTAX                  0x03
#define A320_DELTAY                  0x04
#define A320_SQUAL                   0x05
#define A320_SHUTTERUPPER            0x06
#define A320_SHUTTERLOWER            0x07
#define A320_MAXIMUMPIXEL            0x08
#define A320_PIXELSUM                0x09
#define A320_MINIMUMPIXEL            0x0A
#define A320_PIXELGRAB               0x0B
#define A320_CRC0                    0x0C
#define A320_CRC1                    0x0D
#define A320_CRC2                    0x0E
#define A320_CRC3                    0x0F
#define A320_SELFTEST                0x10
#define A320_CONFIGURATIONBITS       0x11
#define A320_LED_CONTROL             0x1A
#define A320_IO_MODE                 0x1C
#define A320_MOTION_CONTROL          0x1D
#define A320_OBSERVATION             0x2E
#define A320_SOFTRESET               0x3A    // 0x5A
#define A320_SHUTTER_MAX_HI          0x3B
#define A320_SHUTTER_MAX_LO          0x3C
#define A320_INVERSEREVISIONID       0x3E    // 0xFE
#define A320_INVERSEPRODUCTID        0x3F    // 0x7C
#define A320_OFN_ENGINE              0x60
#define A320_OFN_RESOLUTION          0x62
#define A320_OFN_SPEED_CONTROL       0x63
#define A320_OFN_SPEED_ST12          0x64
#define A320_OFN_SPEED_ST21          0x65
#define A320_OFN_SPEED_ST23          0x66
#define A320_OFN_SPEED_ST32          0x67
#define A320_OFN_SPEED_ST34          0x68
#define A320_OFN_SPEED_ST43          0x69
#define A320_OFN_SPEED_ST45          0x6A
#define A320_OFN_SPEED_ST54          0x6B
#define A320_OFN_AD_CTRL             0x6D
#define A320_OFN_AD_ATH_HIGH         0x6E
#define A320_OFN_AD_DTH_HIGH         0x6F
#define A320_OFN_AD_ATH_LOW          0x70
#define A320_OFN_AD_DTH_LOW          0x71
#define A320_OFN_QUANTIZE_CTRL       0x73
#define A320_OFN_XYQ_THRESH          0x74
#define A320_OFN_FPD_CTRL            0x75
#define A320_OFN_ORIENTATION_CTRL    0x77

/* Configuration Register Individual Bit Field Settings */
#define A320_MOTION_MOT       		 0x80
#define A320_MOTION_OVF       		 0x10
#define A320_MOTION_GPIO      		 0x01

/* IO define */
#define ADBS_A320_IRQ_GPIO            (25)
#define ADBS_A320_SHDN_GPIO           (12)        //GPIO_12
#define ADBS_A320_NRST_GPIO           (13)        //GPIO_13

#define LEVEL_AUTO                  0
#define LEVEL_LOWER                 1
#define LEVEL_LOW                   2
#define LEVEL_MIDDLE                3
#define LEVEL_HIGH                  4
#define LEVEL_HIGHER                5

#define OFN_KEY_NUM     (4)

struct adbs_a320_platform_data   {
    int (*power_control)(int enable);
    int (*init_device_hw)(void);
    int gpio_motion;
    int gpio_shtdwn;
    int gpio_nrst;
};

typedef struct __ROCKER_THRESHOLD_STRU
{
    int x_set;
    int y_set;
} ROCKER_THRESHOLD;

enum DEVICE_KEY_CODE
{
    DEVICE_KEY_UP = 1,
    DEVICE_KEY_DOWN = 0,
    DEVICE_KEY_LEFT = 3,
    DEVICE_KEY_RIGHT = 2
};

enum OFN_KEY_CODE
{
	OFN_KEY_NULL = 0,
	OFN_KEY_UP,
	OFN_KEY_DOWN,
	OFN_KEY_LEFT,
	OFN_KEY_RIGHT,
	OFN_KEY_ALL
};

#endif
