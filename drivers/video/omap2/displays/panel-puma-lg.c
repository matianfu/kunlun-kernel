/*
 * 

This file is copied from generic panel support, UGlee
Sat Oct 27 14:11:36 CST 2012

 *
 * Generic panel support
 *
 * Copyright (C) 2008 Nokia Corporation
 * Author: Tomi Valkeinen <tomi.valkeinen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/delay.h>

#include <plat/display.h>


static struct omap_video_timings lg_panel_timings = {

	.x_res		= 1024,
	.y_res		= 600,
	.pixel_clock	= 24000,
	.hfp		= 160,
	.hsw		= 80,
	.hbp		= 160,
	.vfp		= 12,
	.vsw		= 6,
	.vbp		= 23,

};

static int lg_panel_power_on(struct omap_dss_device *dssdev)
{
	int r;

	r = omapdss_dpi_display_enable(dssdev);
	if (r)
		goto err0;

	if (dssdev->platform_enable) {
		r = dssdev->platform_enable(dssdev);
		if (r)
			goto err1;
	}

	return 0;
err1:
	omapdss_dpi_display_disable(dssdev);
err0:
	return r;
}

static void lg_panel_power_off(struct omap_dss_device *dssdev)
{
	if (dssdev->platform_disable)
		dssdev->platform_disable(dssdev);

	omapdss_dpi_display_disable(dssdev);
}
/**
static int otter1_panel_probe(struct omap_dss_device *dssdev)
{
	printk(KERN_INFO "Otter1 LCD probe called\n");
	dssdev->panel.config	= OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |
				  OMAP_DSS_LCD_IHS;
	dssdev->panel.timings	= otter1_panel_timings;

	omap_writel(0x00020000,0x4a1005cc); //PCLK impedance
	gpio_request(175, "LCD_VENDOR0");
	gpio_request(176, "LCD_VENDOR1");
	gpio_direction_input(175);
	gpio_direction_input(176);
    gpio_request(47, "3V_ENABLE");
    gpio_request(37, "OMAP_RGB_SHTDN");

    //Already done in uboot
    //otter1_panel_power_on();
	mutex_init(&lock_power_on);
	mutex_init(&lock_panel);
	return 0;
}
*/
static int lg_panel_probe(struct omap_dss_device *dssdev)
{
	dssdev->panel.config = OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS | OMAP_DSS_LCD_IHS;
	dssdev->panel.timings = lg_panel_timings;

	return 0;
}

static void lg_panel_remove(struct omap_dss_device *dssdev)
{
}

static int lg_panel_enable(struct omap_dss_device *dssdev)
{
	int r = 0;

	r = lg_panel_power_on(dssdev);
	if (r)
		return r;

	dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

	return 0;
}

static void lg_panel_disable(struct omap_dss_device *dssdev)
{
	lg_panel_power_off(dssdev);

	dssdev->state = OMAP_DSS_DISPLAY_DISABLED;
}

static int lg_panel_suspend(struct omap_dss_device *dssdev)
{
	lg_panel_power_off(dssdev);
	dssdev->state = OMAP_DSS_DISPLAY_SUSPENDED;
	return 0;
}

static int lg_panel_resume(struct omap_dss_device *dssdev)
{
	int r = 0;

	r = lg_panel_power_on(dssdev);
	if (r)
		return r;

	dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

	return 0;
}

static void lg_panel_set_timings(struct omap_dss_device *dssdev,
		struct omap_video_timings *timings)
{
	dpi_set_timings(dssdev, timings);
}

static void lg_panel_get_timings(struct omap_dss_device *dssdev,
		struct omap_video_timings *timings)
{
	*timings = dssdev->panel.timings;
}

static int lg_panel_check_timings(struct omap_dss_device *dssdev,
		struct omap_video_timings *timings)
{
	return dpi_check_timings(dssdev, timings);
}

static struct omap_dss_driver generic_driver = {
	.probe		= lg_panel_probe,
	.remove		= lg_panel_remove,

	.enable		= lg_panel_enable,
	.disable	= lg_panel_disable,
	.suspend	= lg_panel_suspend,
	.resume		= lg_panel_resume,

	.set_timings	= lg_panel_set_timings,
	.get_timings	= lg_panel_get_timings,
	.check_timings	= lg_panel_check_timings,

	.driver         = {
		.name   = "lg_panel",
		.owner  = THIS_MODULE,
	},
};

static int __init lg_panel_drv_init(void)
{
	return omap_dss_register_driver(&generic_driver);
}

static void __exit lg_panel_drv_exit(void)
{
	omap_dss_unregister_driver(&generic_driver);
}

module_init(lg_panel_drv_init);
module_exit(lg_panel_drv_exit);
MODULE_LICENSE("GPL");
