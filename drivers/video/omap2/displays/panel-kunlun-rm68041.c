/*
 * kunlun panel support(driver ic is ILI9327)
 *
 * Copyright (C) 2010 VIA TELECOM
 * Author: daqli <daqli@via-telecom.com>
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
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/ili9327.h>

#include <mach/gpio.h>
#include <plat/mux.h>
#include <asm/mach-types.h>
#include <plat/control.h>
#include <plat/display.h>

static int kunlun_spi_suspend(struct spi_device *spi, pm_message_t mesg);
static int kunlun_spi_resume(struct spi_device *spi);
static int init_lcd_panel(struct spi_device *spi);

#ifdef CONFIG_BACKLIGHT_CAT3637
extern void backlight_enable(void);
extern void backlight_disable(int has_loaded);
#endif

#define PANEL_DBG(format, ...) \
		printk(KERN_ERR "kunlun panel: " format, \
		## __VA_ARGS__)

#define LCD_PANEL_RESET_GPIO		93

/*
* According to the datasheet of RM68041.
* 320x480@60Hz.
* - <zhi.wang@windriver.com>
*/

#define LCD_XRES 320
#define LCD_YRES 480
#define LCD_FRAMERATE 60

#define HSW 10
#define HFP 40
#define HBP 20
#define VFP 4       //8
#define VBP 2       //6
#define VSW 2

#define LCD_LINE_CLOCK (VSW + VFP + LCD_XRES + VBP)
#define LCD_FRAME_CLOCK (HSW + HFP + (LCD_LINE_CLOCK * LCD_YRES) + HBP)
#define LCD_PIXEL_CLOCK (LCD_FRAME_CLOCK * LCD_FRAMERATE / 1000) /* in KHz */

/*according DPI interface description in rm68041 manual*/


/*LCD Panel Manual
 * defines HFB, HSW, HBP, VFP, VSW, VBP as shown below
 */
static struct omap_video_timings kunlun_panel_timings = {
	/* 320 x 480 */
	.x_res          = LCD_XRES,
	.y_res          = LCD_YRES,
	.pixel_clock    = LCD_PIXEL_CLOCK,
	.hfp            = HFP,
	.hsw            = HSW,
	.hbp            = HBP,
	.vfp            = VFP,
	.vsw            = VSW,
	.vbp            = VBP,
};

static int kunlun_panel_probe(struct omap_dss_device *dssdev)
{

	dssdev->panel.config = OMAP_DSS_LCD_TFT |
                           OMAP_DSS_LCD_IVS |
                           OMAP_DSS_LCD_IHS;

	dssdev->panel.timings = kunlun_panel_timings;
	dssdev->ctrl.pixel_size = 16;

	return 0;
}


static void  kunlun_panel_remove(struct omap_dss_device *dssdev)
{
}

static int kunlun_panel_enable(struct omap_dss_device *dssdev)
{
	int r = 0;
	r = omapdss_dpi_display_enable(dssdev);
	if (r)
		goto err0;

	if (dssdev->platform_enable) {
		r = dssdev->platform_enable(dssdev);
		if (r)
			goto err1;
	}
	return r;
err1:
	omapdss_dpi_display_disable(dssdev);
err0:
	return r;
}

static void kunlun_panel_disable(struct omap_dss_device *dssdev)
{
	if (dssdev->platform_disable)
		dssdev->platform_disable(dssdev);

	omapdss_dpi_display_disable(dssdev);
}

static int kunlun_panel_suspend(struct omap_dss_device *dssdev)
{
	kunlun_panel_disable(dssdev);
	return 0;
}

static int kunlun_panel_resume(struct omap_dss_device *dssdev)
{
    kunlun_panel_enable(dssdev);
    return 0;
}

static void kunlun_panel_get_timings(struct omap_dss_device *dssdev,
				struct omap_video_timings *timings)
{
	*timings = dssdev->panel.timings;
}

static struct omap_dss_driver kunlun_driver = {
	.probe		= kunlun_panel_probe,
	.remove		 = kunlun_panel_remove,

	.enable		= kunlun_panel_enable,
	.disable	= kunlun_panel_disable,
	.suspend	= kunlun_panel_suspend,
	.resume		= kunlun_panel_resume,

	.get_recommended_bpp = omapdss_default_get_recommended_bpp,
	.get_timings	= kunlun_panel_get_timings,

	.driver		= {
		.name	= "rm68041_panel",
		.owner 	= THIS_MODULE,
	},
};

static int spi_send_cmd(struct spi_device *spi, unsigned char reg_addr)
{
	int ret = 0;
	unsigned int cmd = 0;
	cmd = 0x0000 | reg_addr; /* register address write */
	if (spi_write(spi, (unsigned char *)&cmd, 2))
		printk(KERN_WARNING "error in spi_write %x\n", cmd);

	udelay(10);
	return ret;
}

static int spi_send_data(struct spi_device *spi,unsigned char reg_data)
{
	int ret = 0;
	unsigned int data = 0;
	data = 0x0100 | reg_data ; /* register data write */
	if (spi_write(spi, (unsigned char *)&data, 2))
		printk(KERN_WARNING "error in spi_write %x\n", data);

	udelay(10);
	return ret;
}

static int init_lcd_panel(struct spi_device *spi)
{
	//rm68040
	//IOVCC=VCC=2.8V
	spi_send_cmd(spi, 0x11);
	mdelay(20);
	spi_send_cmd(spi, 0xB4);
	spi_send_data(spi, 0x10);

	spi_send_cmd(spi, 0xD0);
	spi_send_data(spi, 0x07);//02
	spi_send_data(spi, 0x41);
	spi_send_data(spi, 0x1D);//13

	spi_send_cmd(spi, 0xD1);
	spi_send_data(spi, 0x00);//00
	spi_send_data(spi, 0x0e);//0X28
	spi_send_data(spi, 0x0e);//19

	spi_send_cmd(spi, 0xD2);
	spi_send_data(spi, 0x01);
	spi_send_data(spi, 0x11);

	spi_send_cmd(spi, 0xC0);
	spi_send_data(spi, 0x00);
	spi_send_data(spi, 0x3B);
	spi_send_data(spi, 0x00);
	spi_send_data(spi, 0x02);//12
	spi_send_data(spi, 0x11);//01

	spi_send_cmd(spi, 0xC1);
	spi_send_data(spi, 0x10);
	//spi_send_data(spi, 0x11);
	spi_send_data(spi, 0x13);
	spi_send_data(spi, 0x88);

	spi_send_cmd(spi, 0xC5);
	spi_send_data(spi, 0x02);
	//spi_send_data(spi, 0x07);

	spi_send_cmd(spi, 0xC6);
	spi_send_data(spi, 0x03);

	spi_send_cmd(spi, 0xC8);
	spi_send_data(spi, 0x02);
	spi_send_data(spi, 0x46);
	spi_send_data(spi, 0x14);
	spi_send_data(spi, 0x31);
	spi_send_data(spi, 0x0A);
	spi_send_data(spi, 0x04);
	spi_send_data(spi, 0x37);
	spi_send_data(spi, 0x24);
	spi_send_data(spi, 0x57);
	spi_send_data(spi, 0x13);
	spi_send_data(spi, 0x06);
	spi_send_data(spi, 0x0C);

	spi_send_cmd(spi, 0xF3);
	spi_send_data(spi, 0x24);
	spi_send_data(spi, 0x1A);

	spi_send_cmd(spi, 0xF6);
	spi_send_data(spi, 0x80);

	spi_send_cmd(spi, 0xF7);
	spi_send_data(spi, 0x80);

	spi_send_cmd(spi, 0x36);
	spi_send_data(spi, 0x0A);
	//spi_send_data(spi, 0x08);

	spi_send_cmd(spi, 0x3A);
	spi_send_data(spi, 0x66);

	spi_send_cmd(spi, 0x2A);
	spi_send_data(spi, 0x00);
	spi_send_data(spi, 0x00);
	spi_send_data(spi, 0x01);
	spi_send_data(spi, 0x3F);

	spi_send_cmd(spi, 0x2B);
	spi_send_data(spi, 0x00);
	spi_send_data(spi, 0x00);
	spi_send_data(spi, 0x01);
	spi_send_data(spi, 0xDF);

	mdelay(120);
	spi_send_cmd(spi, 0x29);

	//udelay(120000);
	//spi_send_cmd(spi, 0x2C);
	return 0;
}

static void kunlun_lcd_reset(void)
{
	gpio_request(LCD_PANEL_RESET_GPIO,"LCD_RESET");

	/*LCD_RST is output pin*/
 	gpio_direction_output(LCD_PANEL_RESET_GPIO, 1);
	/*LCD_RST pin is expected to hold low for 5ms at least*/
	mdelay(5);
	gpio_set_value(LCD_PANEL_RESET_GPIO,0);
	mdelay(20);
	gpio_set_value(LCD_PANEL_RESET_GPIO,1);
	mdelay(50);
}


static int kunlun_spi_probe(struct spi_device *spi)
{
	struct ili9327_platform_data *pdata = spi->dev.platform_data;
	int err;

	/* enable voltage */
	if (pdata != NULL && pdata->power_control != NULL) {
		err = pdata->power_control(POWER_ENABLE);
		if (err != 0) {
			dev_dbg(&spi->dev, "ili9327 power on failed\n");
			return err;
		}
	}

	/* config spi interface */
	spi->mode = SPI_MODE_3;
	spi->bits_per_word = 9;
	spi_setup(spi);

#ifdef CONFIG_BACKLIGHT_CAT3637
	/*turn off backlight when backlight driver not be loaded */
	//backlight_disable(0);
#endif

	gpio_request(LCD_PANEL_RESET_GPIO,"LCD_RESET");

	/*LCD_RST is output pin*/
 	gpio_direction_output(LCD_PANEL_RESET_GPIO, 1);
	
	//kunlun_lcd_reset();

	/* init lcd panel controller */
	init_lcd_panel(spi);

	PANEL_DBG("%s:ili9327 initialized\n",__FUNCTION__);
	mdelay(1);

	/* register display driver */
	omap_dss_register_driver(&kunlun_driver);

	return 0;
}

static int kunlun_spi_remove(struct spi_device *spi)
{
	struct ili9327_platform_data *pdata = spi->dev.platform_data;
	omap_dss_unregister_driver(&kunlun_driver);

	/* disable voltage */
	if (pdata != NULL && pdata->power_control != NULL) {
		if (pdata->power_control(POWER_DISABLE) != 0) {
			dev_dbg(&spi->dev, "ili9327 power off failed\n");
		}
	}

	return 0;
}

static int kunlun_spi_suspend(struct spi_device *spi, pm_message_t mesg)
{
	spi_send_cmd(spi,0x10);
	mdelay(10);

	return 0;
}

static int kunlun_spi_resume(struct spi_device *spi)
{
	/* reinitialize the panel */
	spi_setup(spi);
	spi_send_cmd(spi,0x11);
	mdelay(120);

	return 0;
}

static struct spi_driver kunlun_spi_driver = {
	.probe           = kunlun_spi_probe,
	.remove	= __devexit_p(kunlun_spi_remove),
	.suspend         = kunlun_spi_suspend,
	.resume          = kunlun_spi_resume,
	.driver         = {
		.name   = "kunlun_disp_spi",
		.bus    = &spi_bus_type,
		.owner  = THIS_MODULE,
	},
};

static int __init kunlun_lcd_init(void)
{
	return spi_register_driver(&kunlun_spi_driver);
}

static void __exit kunlun_lcd_exit(void)
{
	return spi_unregister_driver(&kunlun_spi_driver);
}

module_init(kunlun_lcd_init);
module_exit(kunlun_lcd_exit);
MODULE_LICENSE("GPL");
