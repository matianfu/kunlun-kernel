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

#include <mach/board-kunlun.h>

/*
 * Disabling VPLL2 will cause yellow screen problem
 * when suspend & shutdown phone.
 * Original routines will cause incorrect usecount on VPLL2.
 *
 * - <zhi.wang@windriver.com>
 * */
#define WORKAROUND_YELLOW_SCREEN 1

/* Touchscreen. */
#if defined(CONFIG_TOUCHSCREEN_FT5X0X)
#include <linux/ft5x0x_ts.h>
#endif

#define ENABLE_VPLL2_DEDICATED          0x05
#define ENABLE_VPLL2_DEV_GRP            0xE0
#define TWL4030_VPLL2_DEV_GRP       0x33
#define TWL4030_VPLL2_DEDICATED     0x36

#ifdef WORKAROUND_YELLOW_SCREEN
static int vdds_dsi_reg_enabled = 0;
#endif

static int kunlun_panel_power_enable(int enable)
{
	int ret;
	static struct regulator *vdds_dsi_reg = NULL;

       if(!vdds_dsi_reg){
	    vdds_dsi_reg = regulator_get(NULL, "vdds_dsi");
	    if (IS_ERR(vdds_dsi_reg)) {
		pr_err("Unable to get vdds_dsi regulator\n");
		return PTR_ERR(vdds_dsi_reg);
	    }
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

#ifdef WORKAROUND_YELLOW_SCREEN
	if (!vdds_dsi_reg_enabled) {
		ret = kunlun_panel_power_enable(1);
		vdds_dsi_reg_enabled = 1;
	}
#endif

	ret = kunlun_panel_power_enable(1);

	return ret;
}

static void kunlun_lcd_panel_disable(struct omap_dss_device *dssdev)
{
#ifndef WORKAROUND_YELLOW_SCREEN
	kunlun_panel_power_enable(0);
#endif
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

static struct omap2_mcspi_device_config kunlun_lcd_mcspi_config = {
	.turbo_mode             = 0,
	.single_channel         = 1,  /* 0: slave, 1: master */
};

static struct ili9327_platform_data kunlun_ili9327_platform_data = {
	.power_control = vaux1_control,
};

/* Touchscreen. */
#if defined(CONFIG_TOUCHSCREEN_FT5X0X)
static void ft5x0x_dev_init(void)
{
	printk("_____ft5x0x_dev_init\n");

	if (gpio_request(FT5X0X_PENDOWN_GPIO, "ft5x0x_pendown") < 0)
		printk(KERN_ERR "can't get ft5x0x pen down GPIO\n");

	gpio_direction_input(FT5X0X_PENDOWN_GPIO);

	if (gpio_request(FT5X0X_RST_GPIO, "ft5x0x_rst") < 0)
		printk(KERN_ERR "can't get ft5x0x reset GPIO\n");

	gpio_direction_output(FT5X0X_RST_GPIO, 1);
	gpio_set_value(FT5X0X_RST_GPIO,1);

#ifdef CONFIG_FT5X0X_GPIO_I2C
	if (gpio_request(FT5X0X_SCL_GPIO, "ft5x0x_scl") < 0)
		printk(KERN_ERR "can't get ft5x0x SCL GPIO\n");

	if (gpio_request(FT5X0X_SDA_GPIO, "ft5x0x_sda") < 0)
		printk(KERN_ERR "can't get ft5x0x SDA GPIO\n");

	gpio_direction_output(FT5X0X_SCL_GPIO, 1);
	gpio_direction_output(FT5X0X_SDA_GPIO, 1);
#endif
}

#endif /* CONFIG_TOUCHSCREEN_FT5X0X */

static struct spi_board_info kunlun_spi_board_info[] __initdata = {
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

	/* Touchscreen. */
#ifdef CONFIG_TOUCHSCREEN_FT5X0X
	ft5x0x_dev_init();
#endif

	spi_register_board_info(kunlun_spi_board_info,
				ARRAY_SIZE(kunlun_spi_board_info));
}
