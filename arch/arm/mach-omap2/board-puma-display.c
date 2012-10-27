
#include <plat/display.h>

/** UGlee: this header contains the declaration non-static functions **/
#include <mach/board-kunlun.h>


/** For details, check omap_dss_device struct definition in 
	arch/arm/plat-omap/include/plat/display.h	**/

/** In kunlun, these functions are used to enable/disable LCD panel
	power supply	**/

/** It seems that set_backligh and get_backlight may be provided here,
	is it standard way? or android use different scheme?
	to be clarified further.  **/

/** here we do NOTHING. now **/
static int puma_lcd_panel_enable(struct omap_dss_device *dssdev) {
	return 0;
}

static void puma_lcd_panel_disable(struct omap_dss_device *dssdev) {

}


/** omap dss device **/
static struct omap_dss_device puma_lcd_panel_device = {
        .name = "lcd",
        .driver_name = "lg_panel",
        .type = OMAP_DISPLAY_TYPE_DPI,
        .phy.dpi.data_lines = 24,
        .ctrl.pixel_size = 24,
        .platform_enable = puma_lcd_panel_enable,
        .platform_disable = puma_lcd_panel_disable,
};


/** omap dss device pointer array **/
static struct omap_dss_device* puma_dss_devices[] = {
	&puma_lcd_panel_device,
}; 

/** omap dss board info **/
static struct omap_dss_board_info puma_dss_data = {

	.num_devices = ARRAY_SIZE(puma_dss_devices),
	.devices = puma_dss_devices,
	.default_device = &puma_lcd_panel_device,
};

/** call this function in machine init **/
void __init puma_display_init(void) {

	omap_display_init(&puma_dss_data);
}


