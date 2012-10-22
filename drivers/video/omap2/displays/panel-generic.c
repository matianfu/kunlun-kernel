/*
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

#if 0	/** original backup **/
static struct omap_video_timings generic_panel_timings = {
	/* 640 x 480 @ 60 Hz  Reduced blanking VESA CVT 0.31M3-R */
	.x_res		= 640,
	.y_res		= 480,
	.pixel_clock	= 23500,
	.hfp		= 48,
	.hsw		= 32,
	.hbp		= 80,
	.vfp		= 3,
	.vsw		= 4,
	.vbp		= 7,
};
#endif

static struct omap_video_timings generic_panel_timings = {

	.x_res		= 1024,
	.y_res		= 600,
	.pixel_clock	= 51200,
	.hfp		= 160,
	.hsw		= 80,
	.hbp		= 160,
	.vfp		= 12,
	.vsw		= 6,
	.vbp		= 23,

};

static int generic_panel_power_on(struct omap_dss_device *dssdev)
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

static void generic_panel_power_off(struct omap_dss_device *dssdev)
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
static int generic_panel_probe(struct omap_dss_device *dssdev)
{
	dssdev->panel.config = OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS | OMAP_DSS_LCD_IHS;
	dssdev->panel.timings = generic_panel_timings;

	return 0;
}

static void generic_panel_remove(struct omap_dss_device *dssdev)
{
}

static int generic_panel_enable(struct omap_dss_device *dssdev)
{
	int r = 0;

	r = generic_panel_power_on(dssdev);
	if (r)
		return r;

	dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

	return 0;
}

static void generic_panel_disable(struct omap_dss_device *dssdev)
{
	generic_panel_power_off(dssdev);

	dssdev->state = OMAP_DSS_DISPLAY_DISABLED;
}

static int generic_panel_suspend(struct omap_dss_device *dssdev)
{
	generic_panel_power_off(dssdev);
	dssdev->state = OMAP_DSS_DISPLAY_SUSPENDED;
	return 0;
}

static int generic_panel_resume(struct omap_dss_device *dssdev)
{
	int r = 0;

	r = generic_panel_power_on(dssdev);
	if (r)
		return r;

	dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

	return 0;
}

static void generic_panel_set_timings(struct omap_dss_device *dssdev,
		struct omap_video_timings *timings)
{
	dpi_set_timings(dssdev, timings);
}

static void generic_panel_get_timings(struct omap_dss_device *dssdev,
		struct omap_video_timings *timings)
{
	*timings = dssdev->panel.timings;
}

static int generic_panel_check_timings(struct omap_dss_device *dssdev,
		struct omap_video_timings *timings)
{
	return dpi_check_timings(dssdev, timings);
}

static struct omap_dss_driver generic_driver = {
	.probe		= generic_panel_probe,
	.remove		= generic_panel_remove,

	.enable		= generic_panel_enable,
	.disable	= generic_panel_disable,
	.suspend	= generic_panel_suspend,
	.resume		= generic_panel_resume,

	.set_timings	= generic_panel_set_timings,
	.get_timings	= generic_panel_get_timings,
	.check_timings	= generic_panel_check_timings,

	.driver         = {
		.name   = "generic_panel",
		.owner  = THIS_MODULE,
	},
};

static int __init generic_panel_drv_init(void)
{
	return omap_dss_register_driver(&generic_driver);
}

static void __exit generic_panel_drv_exit(void)
{
	omap_dss_unregister_driver(&generic_driver);
}

module_init(generic_panel_drv_init);
module_exit(generic_panel_drv_exit);
MODULE_LICENSE("GPL");
