/*
 *
 * Copyright (c) 2011 Wind River System Inc
 *
 * Modified from mach-omap2/board-zoom2.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Author     : Xiaochuan.Wu <xiaochuan.wu@windriver.com>
 * Date       : 2011-08-20
 * Version    : 1.0.0
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/input/matrix_keypad.h>
#include <linux/gpio.h>
#include <linux/i2c/twl.h>
#include <linux/regulator/machine.h>
#include <linux/synaptics_i2c_rmi.h>
#include <linux/mmc/host.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <linux/pwm_backlight.h>

#include <plat/common.h>
#include <plat/usb.h>
#include <plat/control.h>
#ifdef CONFIG_SERIAL_OMAP
#include <plat/omap-serial.h>
#include <plat/serial.h>
#endif
#include <linux/switch.h>


#include <linux/leds.h>
#include <plat/led.h>
#include <linux/leds-omap-display.h>

#ifdef CONFIG_PANEL_SIL9022
#include <mach/sil9022.h>
#endif

#if defined(CONFIG_SENSORS_MMA7455)
#include <linux/mma7455.h>
#endif

#if defined(CONFIG_SENSORS_ADBS_A320)
#include <linux/i2c/adbs-a320.h>
#endif

#if defined(CONFIG_SENSORS_AKM8975)
#include <linux/akm8975.h>
#endif

#include <mach/board-kunlun.h>

#include "mux.h"
#include "hsmmc.h"
#include "twl4030.h"
#include <linux/wakelock.h>


#include <media/v4l2-int-device.h>

#define BLUETOOTH_UART	UART2

#define WL127X_BTEN_GPIO	109
#define KPD_LED_EN_GPIO		154

#if defined(CONFIG_VIDEO_HI253) && defined(CONFIG_VIDEO_OMAP3)
#include <media/hi253.h>
extern struct platform_data kunlun_hi253_platform_data;
#endif

#if defined(CONFIG_VIDEO_BF3703) && defined(CONFIG_VIDEO_OMAP3)
#include <media/bf3703.h>
extern struct bf3703_platform_data kunlun_bf3703_platform_data;
#endif

#ifdef CONFIG_WL127X_RFKILL
#include <linux/wl127x-rfkill.h>
#endif

static struct wake_lock uart_lock;

#ifdef CONFIG_BACKLIGHT_CAT3637

#include <linux/backlight.h>
static struct generic_bl_info kunlun_backlight_data = {
	.name = BL_MACHINE_NAME,
	.max_intensity = 100,
	.default_intensity = 50,
};

struct platform_device kunlun_backlight_device = {
	.name          = "bl_cat3637",
	.id            = -1,
	.dev            = {
		.platform_data = &kunlun_backlight_data,
	},
};
#endif

/** UGlee begin **/
#ifdef CONFIG_BACKLIGHT_OMAP3PWM

static struct generic_bl_info puma_backlight_data = {
	.name = "lcd",
	.max_intensity = 100;
	.default_intensity = 30,
};

struct platform_device puma_backlight_device = {
	.name	= "bl_omap3pwm",
	.id	= -1,
	.dev	= {
		.platform_data = &puma_backlight_data,
	},
};

#endif
/** UGlee end **/


#ifdef CONFIG_WL127X_RFKILL
static struct wl127x_rfkill_platform_data wl127x_plat_data = {
	.bt_nshutdown_gpio = 109, 	/* Bluetooth Enable GPIO */
	.fm_enable_gpio = 161,		/* FM Enable GPIO */
};

static struct platform_device kunlun_wl127x_device = {
	.name           = "wl127x-rfkill",
	.id             = -1,
	.dev.platform_data = &wl127x_plat_data,
};
#endif

#ifdef CONFIG_SENSORS_MMA7455
static struct mma7455_platform_data kunlun_mma7455_platform_data = {
	.power_control = vaux1_control,
};
#endif

#ifdef CONFIG_SENSORS_AKM8975
#define AKM8975_DRDY_GPIO 178
static struct akm8975_platform_data kunlun_akm8975_platform_data = {
	.power_control = vaux1_control,
};

static void akm8975_dev_init(void) {
	gpio_direction_input(AKM8975_DRDY_GPIO);
}
#endif

#if defined(CONFIG_SENSORS_ADBS_A320)

static struct adbs_a320_platform_data kunlun_adbs_a320_platform_data = {
	.power_control = vaux2_control,
	.init_device_hw = init_ofn_hw,
	.gpio_motion = ADBS_A320_IRQ_GPIO,
	.gpio_shtdwn = ADBS_A320_SHDN_GPIO,
	.gpio_nrst = ADBS_A320_NRST_GPIO,
    };

#endif

static struct omap_led_config kunlun_button_led_config[] = {
	{
		.cdev	= {
			.name	= "button-backlight",
		},
		.gpio	= KPD_LED_EN_GPIO,
	},
};

static struct omap_led_platform_data kunlun_button_led_data = {
	.nr_leds	= ARRAY_SIZE(kunlun_button_led_config),
	.leds		= kunlun_button_led_config,
};

static struct platform_device kunlun_button_led_device = {
	.name	= "button-backlight",
	.id	= -1,
	.dev	= {
		.platform_data	= &kunlun_button_led_data,
	},
};

static int kunlun_board_keymap[] = {

/**
	KEY(3, 2, KEY_BACK),
	KEY(3, 1, KEY_HOME),
	KEY(3, 3, KEY_ENTER),
	KEY(4, 1, KEY_END),
	KEY(4, 3, KEY_MENU),
	KEY(2, 0, KEY_VOLUMEUP),
	KEY(2, 1, KEY_VOLUMEDOWN)
**/

// defined in matrix_keypad.h
// #define KEY(row, col, val)
	KEY(1, 0, KEY_HOME),	// S1001
	KEY(1, 1, KEY_BACK),	// S1002
	KEY(1, 2, KEY_MENU),	// S1003
};

static struct matrix_keymap_data kunlun_board_map_data = {
	.keymap		= kunlun_board_keymap,
	.keymap_size	= ARRAY_SIZE(kunlun_board_keymap),
};

static struct twl4030_keypad_data kunlun_kp_twl4030_data = {
	.keymap_data	= &kunlun_board_map_data,
	.rows		= 8,
	.cols		= 8,
	.rep		= 0,
};

static struct __initdata twl4030_power_data kunlun_t2scripts_data;

static struct regulator_consumer_supply kunlun_vmmc1_supply = {
	.supply		= "vmmc",
};

static struct regulator_consumer_supply kunlun_vmmc2_supply = {
	.supply		= "vmmc",
};

/* VMMC1 for OMAP VDD_MMC1 (i/o) and MMC1 card */
static struct regulator_init_data kunlun_vmmc1 = {
	.constraints = {
		.min_uV			= 1850000,
		.max_uV			= 3150000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &kunlun_vmmc1_supply,
};

/* VMMC2 for MMC2 card */
static struct regulator_init_data kunlun_vmmc2 = {
	.constraints = {
		.min_uV			= 1850000,
		.max_uV			= 1850000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &kunlun_vmmc2_supply,
};

/* VSIM for OMAP VDD_MMC1A (i/o for DAT4..DAT7) */
static struct regulator_init_data kunlun_vsim = {
	.constraints = {
		.min_uV			= 1800000,	/** changed from 3000000 **/
		.max_uV			= 1800000,	/** changed from 3000000 **/
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
};

static struct gpio_switch_platform_data kunlun_headset_switch_data = {
	.name		= "h2w",
	.mute		=  OMAP_MAX_GPIO_LINES + 6, /* TWL4030 GPIO_6 */
	.cable_in1	=  OMAP_MAX_GPIO_LINES + 2, /* TWL4030 GPIO_2 */
	.cable_in2	=  OMAP_MAX_GPIO_LINES + 1, /* TWL4030 GPIO_1 */
};

static struct platform_device kunlun_headset_switch_device = {
	.name		= "switch-gpio",
	.id		= -1,
	.dev		= {
		.platform_data = &kunlun_headset_switch_data,
	}
};

/**
struct platform_pwm_backlight_data {
        int pwm_id;
        unsigned int max_brightness;
        unsigned int dft_brightness;
        unsigned int pwm_period_ns;
        int (*init)(struct device *dev);
        int (*notify)(struct device *dev, int brightness);
        void (*exit)(struct device *dev);
}; **/

static struct platform_pwm_backlight_data puma_pwm_bl_platform_data = {
	.pwm_id 	= 1,
	.max_brightness = 100,
	.dft_brightness = 20,
	.pwm_period_ns	= 100,
};

static struct platform_device puma_pwm_bl_device = {
	.name		= "pwm-backlight",
	.id		= -1,
	.dev		= {
		.platform_data = &puma_pwm_bl_platform_data,
	},
};

static struct platform_device *kunlun_board_devices[] __initdata = {

//	&puma_pwm_bl_device,

#ifdef CONFIG_BACKLIGHT_CAT3637
//	&kunlun_backlight_device,
#endif
#ifdef CONFIG_WL127X_RFKILL
	&kunlun_wl127x_device,
#endif
//	&kunlun_headset_switch_device,
//	&kunlun_button_led_device,
	
};

static struct omap2_hsmmc_info mmc[] __initdata = {
	{
		.name		= "external",
		.mmc		= 1,
		.caps		= MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA,
		.gpio_wp	= -EINVAL,
		.power_saving	= true,
	},
	{
		.name		= "internal",
		.mmc		= 2,
		.caps		= MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
		.nonremovable	= true,
		.power_saving	= true,
	},
	{
		.mmc		= 3,
		.caps		= MMC_CAP_4_BIT_DATA,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
	},
	{}      /* Terminator */
};

static struct regulator_consumer_supply kunlun_vpll2_supply = {
	.supply         = "vdds_dsi",
};

static struct regulator_consumer_supply kunlun_vdda_dac_supply = {
	.supply         = "vdda_dac",
};

static struct regulator_init_data kunlun_vpll2 = {
	.constraints = {
		.min_uV                 = 1800000,
		.max_uV                 = 1800000,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &kunlun_vpll2_supply,
};

static struct regulator_init_data kunlun_vdac = {
	.constraints = {
		.min_uV                 = 1800000,
		.max_uV                 = 1800000,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &kunlun_vdda_dac_supply,
};

static int kunlun_twl_gpio_setup(struct device *dev,
		unsigned gpio, unsigned ngpio)
{
	/* gpio + 0 is "mmc0_cd" (input/IRQ) */
	mmc[0].gpio_cd = -EINVAL;

#ifdef CONFIG_MMC_EMBEDDED_SDIO
	/* The controller that is connected to the 128x device
	 * should have the card detect gpio disabled. This is
	 * achieved by initializing it with a negative value
	 */
	mmc[CONFIG_TIWLAN_MMC_CONTROLLER - 1].gpio_cd = -EINVAL;
#endif

	omap2_hsmmc_init(mmc);

	/* link regulators to MMC adapters ... we "know" the
	 * regulators will be set up only *after* we return.
	*/
	kunlun_vmmc1_supply.dev = mmc[0].dev;
	kunlun_vmmc2_supply.dev = mmc[1].dev;

	return 0;
}


static int kunlun_batt_table[] = {
/* 0 C*/
30800, 29500, 28300, 27100,
26000, 24900, 23900, 22900, 22000, 21100, 20300, 19400, 18700, 17900,
17200, 16500, 15900, 15300, 14700, 14100, 13600, 13100, 12600, 12100,
11600, 11200, 10800, 10400, 10000, 9630,  9280,  8950,  8620,  8310,
8020,  7730,  7460,  7200,  6950,  6710,  6470,  6250,  6040,  5830,
5640,  5450,  5260,  5090,  4920,  4760,  4600,  4450,  4310,  4170,
4040,  3910,  3790,  3670,  3550
};

static struct twl4030_bci_platform_data kunlun_bci_data = {
	.battery_tmp_tbl	= kunlun_batt_table,
	.tblsize		= ARRAY_SIZE(kunlun_batt_table),
};

static struct twl4030_usb_data kunlun_usb_data = {
	.usb_mode	= T2_USB_MODE_ULPI,
};

static struct twl4030_gpio_platform_data kunlun_gpio_data = {
	.gpio_base	= OMAP_MAX_GPIO_LINES,
	.irq_base	= TWL4030_GPIO_IRQ_BASE,
	.irq_end	= TWL4030_GPIO_IRQ_END,
	.setup		= kunlun_twl_gpio_setup,
	.debounce	= 0x06, //Headset GPIO for detection and key
};

static struct twl4030_madc_platform_data kunlun_madc_data = {
	.irq_line	= 1,
};

static struct twl4030_codec_audio_data kunlun_audio_data = {
	.audio_mclk	= 26000000,
	.ramp_delay_value = 2, /* 81 ms */
	.hs_extmute = 1,
};

static struct twl4030_codec_data kunlun_codec_data = {
	.audio_mclk = 26000000,
	.audio = &kunlun_audio_data,
};


/*used by many devices, fixed 3.0V*/
static struct regulator_consumer_supply kunlun_vaux1_supply = {
	.supply	= "vcc_vaux1",
};

static struct regulator_init_data kunlun_vaux1 = {
	.constraints = {
		.min_uV			= 3000000,
		.max_uV			= 3000000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &kunlun_vaux1_supply,
};

static struct regulator_consumer_supply kunlun_vaux2_supply = {
	.supply         = "vcc_vaux2",
};

/*used by OFN mouse VDD_OFN_1V8*/
/** vaux2 is used by tp, seems good in voltage setting **/
static struct regulator_init_data kunlun_vaux2 = {
	.constraints = {
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &kunlun_vaux2_supply,
};

static struct regulator_init_data kunlun_vpll1 = {
	.constraints = {
		.min_uV			= 1000000,
		.max_uV			= 1800000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 0,
	.consumer_supplies	= NULL,
};

static struct regulator_init_data kunlun_vaux3 = {
	.constraints = {
		.min_uV			= 1500000,
		.max_uV			= 2800000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
};

/*used by camera AVDD_CAM_2V8*/
static struct regulator_init_data kunlun_vaux4 = {
	.constraints = {
		.min_uV			= 2800000,
		.max_uV			= 2800000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
};

#ifdef CONFIG_REGULATOR_KUNLUN
/* VMMC2 for V_KBD_BL */
static struct regulator_init_data kunlun_kbl = {
	.constraints = {
		.min_uV			= 1850000,
		.max_uV			= 3150000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
};

/*used by GPS_PWR_EN, GPIO_16*/
static struct regulator_init_data kunlun_gps = {
	.constraints = {
		.valid_modes_mask	= 0,
		.valid_ops_mask		= 0,
	},
};

/*used by V_CAM_EN GPIO_113 for VDD_CAM_VCM_2V8, which parent is vaux2 DVDD_CAM_1V5*/
static struct regulator_init_data kunlun_cam = {
	.constraints = {
		.valid_modes_mask	= 0,
		.valid_ops_mask		= 0,
	},
};

/*used by WL_SHDN_N, GPIO_158*/
static struct regulator_init_data kunlun_wlan = {
	.constraints = {
		.valid_modes_mask	= 0,
		.valid_ops_mask		= 0,
	},
};

/*used by BT_RST_N, GPIO_109*/
static struct regulator_init_data kunlun_bt = {
	.constraints = {
		.valid_modes_mask	= 0,
		.valid_ops_mask		= 0,
	},
};

/*used by FM_EN, GPIO_161*/
static struct regulator_init_data kunlun_fm = {
	.constraints = {
		.valid_modes_mask	= 0,
		.valid_ops_mask		= 0,
	},
};

#endif

static struct twl4030_platform_data kunlun_twldata = {
	.irq_base	= TWL4030_IRQ_BASE,
	.irq_end	= TWL4030_IRQ_END,

	/* platform_data for children goes here */
	.bci		= &kunlun_bci_data,
	.madc		= &kunlun_madc_data,
	.usb		= &kunlun_usb_data,
	.gpio		= &kunlun_gpio_data,
	.keypad		= &kunlun_kp_twl4030_data,
	.power		= &kunlun_t2scripts_data,
	.codec		= &kunlun_codec_data,
	.vmmc1		= &kunlun_vmmc1,
	.vmmc2		= &kunlun_vmmc2,
	.vsim		= &kunlun_vsim,
	.vpll2		= &kunlun_vpll2,
	.vdac		= &kunlun_vdac,
	.vpll1		= &kunlun_vpll1,
	.vaux1		= &kunlun_vaux1,
	.vaux2		= &kunlun_vaux2,
	.vaux3		= &kunlun_vaux3,
	.vaux4		= &kunlun_vaux4,
#ifdef CONFIG_REGULATOR_KUNLUN
	/*GPIO switch*/
	.kbl		= &kunlun_kbl,
	.gps		= &kunlun_gps,
	.cam		= &kunlun_cam,
	.wlan		= &kunlun_wlan,
	.bt		= &kunlun_bt,
	.fm		= &kunlun_fm,
#endif
};

static struct i2c_board_info __initdata kunlun_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO("twl4030", 0x48),
		.flags		= I2C_CLIENT_WAKE,
		.irq		= INT_34XX_SYS_NIRQ,
		.platform_data	= &kunlun_twldata,
	},
};

static struct i2c_board_info __initdata kunlun_i2c_bus2_info[] = {
#if defined(CONFIG_VIDEO_HI253) && defined(CONFIG_VIDEO_OMAP3)
	{
		I2C_BOARD_INFO("Hi253", Hi253_I2C_ADDR),
		.platform_data = &kunlun_hi253_platform_data,
	},
#endif

#if defined(CONFIG_VIDEO_BF3703) && defined(CONFIG_VIDEO_OMAP3)
	{
		I2C_BOARD_INFO("bf3703", BF3703_I2C_ADDR),
		.platform_data = &kunlun_bf3703_platform_data,
	},
#endif
	/** UGlee added, for touchscreen, no idea which address is OK  **/	

	{
		I2C_BOARD_INFO("ssd253x-ts", 0x48),
		.platform_data = NULL,
	},

};


static struct i2c_board_info __initdata kunlun_i2c_bus3_info[] = {
#ifdef CONFIG_TOUCHSCREEN_ZT2092
	{
		I2C_BOARD_INFO(ZT2092_I2C_NAME,  ZT2092_I2C_ADDR),
		.platform_data = &kunlun_zt2092_platform_data,
		.irq = OMAP_GPIO_IRQ(ZT2092_PENDOWN_GPIO),
	},
#endif
#ifdef CONFIG_SENSORS_MMA7455
        {
		I2C_BOARD_INFO("mma7455", 0x1D),
		.platform_data = &kunlun_mma7455_platform_data,
        },
#endif
#if defined(CONFIG_SENSORS_ADBS_A320)
    {
		I2C_BOARD_INFO("adbs_a320", 0x33),
		.platform_data = &kunlun_adbs_a320_platform_data,
		.irq = OMAP_GPIO_IRQ(ADBS_A320_IRQ_GPIO),
    },
#endif
#ifdef CONFIG_SENSORS_AKM8975	/* This macro is set by default, commented out, UGlee */
    	{
		I2C_BOARD_INFO("akm8975",0x0C),
		.platform_data = &kunlun_akm8975_platform_data,
		.irq = NULL,	// the akm8975 use polling ,not irq
	},
#endif
};

static int __init omap_i2c_init(void)
{
	/* Disable OMAP 3630 internal pull-ups for I2Ci */
	if (cpu_is_omap3630()) {
		u32 prog_io;
		
		/** CONTROL_PROG_IO1 is a special register, see TRM **/
		prog_io = omap_ctrl_readl(OMAP343X_CONTROL_PROG_IO1);
		/* Program (bit 19)=1 to disable internal pull-up on I2C1 */
		prog_io |= OMAP3630_PRG_I2C1_PULLUPRESX;
		/* Program (bit 0)=1 to disable internal pull-up on I2C2 */

		/** 	UGlee, clear bit to enable I2C2 pull-up 	**/
		////
		prog_io &= ~(OMAP3630_PRG_I2C2_PULLUPRESX);
		////
		omap_ctrl_writel(prog_io, OMAP343X_CONTROL_PROG_IO1);

		prog_io = omap_ctrl_readl(OMAP36XX_CONTROL_PROG_IO2);
		/* Program (bit 7)=1 to disable internal pull-up on I2C3 */
		prog_io |= OMAP3630_PRG_I2C3_PULLUPRESX;
		omap_ctrl_writel(prog_io, OMAP36XX_CONTROL_PROG_IO2);

		prog_io = omap_ctrl_readl(OMAP36XX_CONTROL_PROG_IO_WKUP1);
		/* Program (bit 5)=1 to disable internal pull-up on I2C4(SR) */
		prog_io |= OMAP3630_PRG_SR_PULLUPRESX;
		omap_ctrl_writel(prog_io, OMAP36XX_CONTROL_PROG_IO_WKUP1);
	}

	omap_register_i2c_bus(1, 100, NULL, kunlun_i2c_boardinfo,
			ARRAY_SIZE(kunlun_i2c_boardinfo));
	omap_register_i2c_bus(2, 100, NULL, kunlun_i2c_bus2_info,
			ARRAY_SIZE(kunlun_i2c_bus2_info));
	omap_register_i2c_bus(3, 100, NULL, kunlun_i2c_bus3_info,
			ARRAY_SIZE(kunlun_i2c_bus3_info));
	return 0;
}

static struct omap_musb_board_data musb_board_data = {
	.interface_type		= MUSB_INTERFACE_ULPI,
#ifdef CONFIG_USB_MUSB_OTG
	.mode = MUSB_OTG,
#elif defined(CONFIG_USB_MUSB_HDRC_HCD)
	.mode = MUSB_HOST,
#elif defined(CONFIG_USB_GADGET_MUSB_HDRC)
	.mode = MUSB_PERIPHERAL,
#endif
	.power			= 100,
};

static void plat_hold_wakelock(void *up, int flag)
{
	struct uart_omap_port *up2 = (struct uart_omap_port *)up;

	/* Specific wakelock for bluetooth usecases */
	if ((up2->pdev->id == BLUETOOTH_UART)
			&& ((flag == WAKELK_TX) || (flag == WAKELK_RX)))
		wake_lock_timeout(&uart_lock, 2*HZ);
}

static struct omap_uart_port_info omap_serial_platform_data[] = {
	{
		.use_dma	= 0,
		.dma_rx_buf_size = DEFAULT_RXDMA_BUFSIZE,
		.dma_rx_poll_rate = DEFAULT_RXDMA_POLLRATE,
		.dma_rx_timeout = DEFAULT_RXDMA_TIMEOUT,
		.idle_timeout	= DEFAULT_IDLE_TIMEOUT,
		.flags		= 1,
		.plat_hold_wakelock = NULL,
	},
	{
		.use_dma	= 0,
		.dma_rx_buf_size = DEFAULT_RXDMA_BUFSIZE,
		.dma_rx_poll_rate = DEFAULT_RXDMA_POLLRATE,
		.dma_rx_timeout = DEFAULT_RXDMA_TIMEOUT,
		.idle_timeout	= DEFAULT_IDLE_TIMEOUT,
		.flags		= 1,
		.plat_hold_wakelock = plat_hold_wakelock,
	},
	{
		.use_dma	= 0,
		.dma_rx_buf_size = DEFAULT_RXDMA_BUFSIZE,
		.dma_rx_poll_rate = DEFAULT_RXDMA_POLLRATE,
		.dma_rx_timeout = DEFAULT_RXDMA_TIMEOUT,
		.idle_timeout	= DEFAULT_IDLE_TIMEOUT,
		.flags		= 1,
		.plat_hold_wakelock = NULL,
	},
	{
		.use_dma	= 0,
		.dma_rx_buf_size = DEFAULT_RXDMA_BUFSIZE,
		.dma_rx_poll_rate = DEFAULT_RXDMA_POLLRATE,
		.dma_rx_timeout = DEFAULT_RXDMA_TIMEOUT,
		.idle_timeout	= DEFAULT_IDLE_TIMEOUT,
		.flags		= 1,
		.plat_hold_wakelock = NULL,
	},
	{
		.flags		= 0
	}
};

static void enable_board_wakeup_source(void)
{
	/* T2 interrupt line (keypad) */
	omap_mux_init_signal("sys_nirq",
		OMAP_WAKEUP_EN | OMAP_PIN_INPUT_PULLUP);
}

static void config_wlan_gpio(void)
{
	omap_mux_init_signal("gpio_162", OMAP_PIN_INPUT);
}

static void config_bt_gpio(void)
{
	/* configure BT_EN gpio */
	omap_mux_init_signal("gpio_109", OMAP_PIN_INPUT);
}

static void config_mux_mcbsp3(void)
{
	/*Mux setting for GPIO164 McBSP3*/
	omap_mux_init_signal("gpio_164", OMAP_PIN_OUTPUT);
}

void __init kunlun_peripherals_init(void)
{
	/** UGlee **/
	wake_lock_init(&uart_lock, WAKE_LOCK_SUSPEND, "uart_wake_lock");

	twl4030_get_scripts(&kunlun_t2scripts_data);
	omap_i2c_init();

	config_wlan_gpio();
	config_bt_gpio();
	platform_add_devices(kunlun_board_devices,
		ARRAY_SIZE(kunlun_board_devices));
	omap_serial_init(omap_serial_platform_data);
	usb_musb_init(&musb_board_data);

#ifdef CONFIG_SENSORS_AKM8975	/* This macro is set by default, commented out, UGlee */
//	akm8975_dev_init();
#endif
	config_mux_mcbsp3();
	enable_board_wakeup_source();

	/**
	if (gpio_request(156, "io156") < 0) {
		printk(KERN_ERR "can't get 156 pen down GPIO\n");
	}
	gpio_direction_output(156, 1);
	if (gpio_request(157, "io157") < 0) {
		printk(KERN_ERR "can't get 156 pen down GPIO\n");
	}
	gpio_direction_output(157, 0);
	**/
}
