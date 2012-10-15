/*
 *
 * Copyright (c) 2011 Wind River System Inc
 *
 * Modified from mach-omap2/board-zoom2-dispaly.c
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
#include <linux/gpio.h>
#include <linux/i2c/twl.h>
#include <linux/spi/spi.h>
#include <linux/spi/ili9327.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/consumer.h>
#include <plat/common.h>
#include <plat/control.h>
#include <plat/mcspi.h>
#include <plat/display.h>
#include <plat/omap-pm.h>
#include "mux.h"

#ifdef CONFIG_TOUCHSCREEN_ADS7846
#include <linux/spi/ads7846.h>
#endif

#if defined(CONFIG_SENSORS_ADBS_A320)
#include <linux/i2c/adbs-a320.h>
#endif

#include <mach/board-kunlun.h>

#define ENABLE_VPLL2_DEDICATED          0x05
#define ENABLE_VPLL2_DEV_GRP            0xE0
#define TWL4030_VPLL2_DEV_GRP       0x33
#define TWL4030_VPLL2_DEDICATED     0x36
#define TS_GPIO                         153


static int kunlun_panel_power_enable(int enable)
{
	int ret;
	struct regulator *vdds_dsi_reg;

	/*fix bug(display yellow when suspend)*/
	enable = 1;
	twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
				(enable) ? ENABLE_VPLL2_DEDICATED : 0,
				TWL4030_VPLL2_DEDICATED);
	twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
				(enable) ? ENABLE_VPLL2_DEV_GRP : 0,
				TWL4030_VPLL2_DEV_GRP);

	vdds_dsi_reg = regulator_get(NULL, "vdds_dsi");
	if (IS_ERR(vdds_dsi_reg)) {
		pr_err("Unable to get vdds_dsi regulator\n");
		return PTR_ERR(vdds_dsi_reg);
	}

	if (enable)
		ret = regulator_enable(vdds_dsi_reg);
	else
		ret = regulator_disable(vdds_dsi_reg);

	return ret;
}

static int kunlun_lcd_panel_enable(struct omap_dss_device *dssdev)
{
	int ret;

	ret = kunlun_panel_power_enable(1);
	return ret;
}

static void kunlun_lcd_panel_disable(struct omap_dss_device *dssdev)
{
	kunlun_panel_power_enable(0);
}

static struct omap_dss_device kunlun_lcd_panel_device = {
	.name = "lcd",
	.driver_name = "rm68041_panel",
	.type = OMAP_DISPLAY_TYPE_DPI,
	.phy.dpi.data_lines = 16,
	.ctrl.pixel_size = 16,
	.platform_enable = kunlun_lcd_panel_enable,
	.platform_disable = kunlun_lcd_panel_disable,
};

static struct omap_dss_device *kunlun_dss_devices[] = {
	&kunlun_lcd_panel_device,
};

static struct omap_dss_board_info kunlun_dss_data = {
	.num_devices = ARRAY_SIZE(kunlun_dss_devices),
	.devices = kunlun_dss_devices,
	.default_device = &kunlun_lcd_panel_device,
};

#ifdef CONFIG_TOUCHSCREEN_ADS7846
static void ads7846_dev_init(void)
{
	if (gpio_request(TS_GPIO, "touch") < 0)
		printk(KERN_ERR "can't get ads746 pen down GPIO\n");

	gpio_direction_input(TS_GPIO);
}

static int ads7846_get_pendown_state(void)
{
	return !gpio_get_value(TS_GPIO);
}

#endif

#ifdef CONFIG_TOUCHSCREEN_ADS7846
static struct ads7846_platform_data ads7846_config = {
   /*Liqi: measure the value of x and y to change axis */
    .x_min                  = 250,
    .y_min                  = 230,
    .x_max                  = 3740,
    .y_max                  = 3800,
    .x_plate_ohms           = 80,
    .pressure_max           = 255,
    .debounce_max           = 10,
    .debounce_tol           = 10,
    .debounce_rep           = 1,
    .get_pendown_state      = ads7846_get_pendown_state,
    .keep_vref_on           = 1,
    .vaux_control           = vaux1_control,
    .settle_delay_usecs     = 30,
};

static struct omap2_mcspi_device_config ads7846_mcspi_config = {
	.turbo_mode	= 0,
	.single_channel	= 1,  /* 0: slave, 1: master */
};
#endif

static struct omap2_mcspi_device_config kunlun_lcd_mcspi_config = {
	.turbo_mode             = 0,
	.single_channel         = 1,  /* 0: slave, 1: master */
};

static struct ili9327_platform_data kunlun_ili9327_platform_data = {
	.power_control = vaux1_control,
};

static struct spi_board_info kunlun_spi_board_info[] __initdata = {
#ifdef CONFIG_TOUCHSCREEN_ADS7846
	{
        	.modalias		= "ads7846",
        	.bus_num		= 3,	//spi3 cs0
        	.chip_select		= 0,
        	.max_speed_hz		= 1500000,
        	.controller_data	= &ads7846_mcspi_config,
        	.irq			= OMAP_GPIO_IRQ(TS_GPIO),
        	.platform_data		= &ads7846_config,
	},
#endif
	{
		.modalias		= "kunlun_disp_spi",
		.bus_num		= 1,
		.chip_select		= 0,
		.max_speed_hz		= 375000,
		.controller_data	= &kunlun_lcd_mcspi_config,
		.platform_data		= &kunlun_ili9327_platform_data,
	},
};

void __init kunlun_display_init(void)
{
	omap_display_init(&kunlun_dss_data);
	spi_register_board_info(kunlun_spi_board_info,
				ARRAY_SIZE(kunlun_spi_board_info));
#ifdef CONFIG_TOUCHSCREEN_ADS7846
	ads7846_dev_init();
#endif
}
