/*
 *
 * Copyright (c) 2011 Wind River System Inc
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
#include <linux/gpio.h>
#include <linux/delay.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/delay.h>

#include <mach/board-kunlun.h>

#include <plat/common.h>
#include <plat/control.h>
#include <plat/board.h>
#include <plat/usb.h>
#include <plat/opp_twl_tps.h>
#include <plat/timer-gp.h>

#include "pm.h"
#include "mux.h"
#include "smartreflex-class1p5.h"
#include "board-zoom2-wifi.h"
#include <linux/skbuff.h>
#include <linux/ti_wilink_st.h>

#define McBSP3_BT_GPIO 164

#ifdef CONFIG_MACH_OMAP3_VIBRAOTR
void __init kunlun_init_vibrator(void);
#endif

struct regulator *vaux1_regulator = NULL;

int vaux1_control(int enable)
{
	if (!vaux1_regulator)
		vaux1_regulator = regulator_get(NULL, "vcc_vaux1");

	if (IS_ERR(vaux1_regulator)) {
		printk("can't get VAUX1 regulator\n");
		vaux1_regulator = NULL;
		return PTR_ERR(vaux1_regulator);
	}

	return enable ? regulator_enable(vaux1_regulator) :
				regulator_disable(vaux1_regulator);
}

#ifdef CONFIG_PM
static struct omap_volt_vc_data vc_config = {
	/* MPU */
	.vdd0_on	= 1200000, /* 1.2v */
	.vdd0_onlp	= 1000000, /* 1.0v */
	.vdd0_ret	=  975000, /* 0.975v */
	.vdd0_off	=  600000, /* 0.6v */
	/* CORE */
	.vdd1_on	= 1150000, /* 1.15v */
	.vdd1_onlp	= 1000000, /* 1.0v */
	.vdd1_ret	=  975000, /* 0.975v */
	.vdd1_off	=  600000, /* 0.6v */

	.clksetup	= 0xff,
	.voltoffset	= 0xff,
	.voltsetup2	= 0xff,
	.voltsetup_time1 = 0xfff,
	.voltsetup_time2 = 0xfff,
};

#ifdef CONFIG_TWL4030_CORE
static struct omap_volt_pmic_info omap_pmic_mpu = { /* and iva */
	.name = "twl",
	.slew_rate = 4000,
	.step_size = 12500,
	.i2c_addr = 0x12,
	.i2c_vreg = 0x0, /* (vdd0) VDD1 -> VDD1_CORE -> VDD_MPU */
	.vsel_to_uv = omap_twl_vsel_to_uv,
	.uv_to_vsel = omap_twl_uv_to_vsel,
	.onforce_cmd = omap_twl_onforce_cmd,
	.on_cmd = omap_twl_on_cmd,
	.sleepforce_cmd = omap_twl_sleepforce_cmd,
	.sleep_cmd = omap_twl_sleep_cmd,
	.vp_config_erroroffset = 0,
	.vp_vstepmin_vstepmin = 0x01,
	.vp_vstepmax_vstepmax = 0x04,
	.vp_vlimitto_timeout_us = 0x200,
	.vp_vlimitto_vddmin = 0x14,
	.vp_vlimitto_vddmax = 0x44,
};

static struct omap_volt_pmic_info omap_pmic_core = {
	.name = "twl",
	.slew_rate = 4000,
	.step_size = 12500,
	.i2c_addr = 0x12,
	.i2c_vreg = 0x1, /* (vdd1) VDD2 -> VDD2_CORE -> VDD_CORE */
	.vsel_to_uv = omap_twl_vsel_to_uv,
	.uv_to_vsel = omap_twl_uv_to_vsel,
	.onforce_cmd = omap_twl_onforce_cmd,
	.on_cmd = omap_twl_on_cmd,
	.sleepforce_cmd = omap_twl_sleepforce_cmd,
	.sleep_cmd = omap_twl_sleep_cmd,
	.vp_config_erroroffset = 0,
	.vp_vstepmin_vstepmin = 0x01,
	.vp_vstepmax_vstepmax = 0x04,
	.vp_vlimitto_timeout_us = 0x200,
	.vp_vlimitto_vddmin = 0x18,
	.vp_vlimitto_vddmax = 0x42,
};
#endif /* CONFIG_TWL4030_CORE */
#endif /* CONFIG_PM */

static void __init omap_kunlun_map_io(void)
{
	omap2_set_globals_36xx();
	omap34xx_map_common_io();
}

static struct omap_board_config_kernel kunlun_config[] __initdata = {
};

static struct cpuidle_params kunlun_cpuidle_params_table[] = {
	/* C1 */
	{1, 2, 2, 5},
	/* C2 */
	{1, 10, 10, 30},
	/* C3 */
	{1, 50, 50, 300},
	/* C4 */
	{1, 1500, 1800, 4000},
	/* C5 */
	{1, 2500, 7500, 12000},
	/* C6 */
	{1, 3000, 8500, 15000},
	/* C7 */
	{1, 10000, 30000, 300000},
};

struct omap_sdrc_params *kunlun_get_sdram_timings(void);

static void __init omap_kunlun_init_irq(void)
{
	struct omap_sdrc_params *sdrc_params;

	omap_board_config = kunlun_config;
	omap_board_config_size = ARRAY_SIZE(kunlun_config);
	omap3_pm_init_cpuidle(kunlun_cpuidle_params_table);
	sdrc_params = kunlun_get_sdram_timings();
	omap2_init_common_hw(sdrc_params, sdrc_params);
	omap2_gp_clockevent_set_gptimer(1);
	omap_init_irq();
}

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux board_mux[] __initdata = {
	OMAP3_MUX(DSS_DATA18, OMAP_MUX_MODE2 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA19, OMAP_MUX_MODE2 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA20, OMAP_MUX_MODE2 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA21, OMAP_MUX_MODE2 | OMAP_PIN_INPUT_PULLDOWN),

	/* TOUCH_IRQ_N GPIO 153 */
	OMAP3_MUX(MCBSP4_DR, OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLUP),

	/* TOUCH_RST GPIO 179 */
	OMAP3_MUX(MCSPI2_SIMO, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),

#ifdef CONFIG_SENSORS_TMD27713T
	/*CSI2_DY0 GOIO 113 */
	OMAP3_MUX(CSI2_DY0, OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLUP | OMAP_WAKEUP_EN),
#endif

#ifdef CONFIG_REGULATOR_KUNLUN
	OMAP3_MUX(ETK_D3, OMAP_MUX_MODE2 | OMAP_PIN_INPUT_PULLUP),
	OMAP3_MUX(ETK_D4, OMAP_MUX_MODE2 | OMAP_PIN_INPUT_PULLUP),
	OMAP3_MUX(ETK_D5, OMAP_MUX_MODE2 | OMAP_PIN_INPUT_PULLUP),
	OMAP3_MUX(ETK_D6, OMAP_MUX_MODE2 | OMAP_PIN_INPUT_PULLUP),
	OMAP3_MUX(MCBSP1_CLKX, OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLDOWN),
	OMAP3_MUX(MCSPI1_CS1, OMAP_MUX_MODE3 | OMAP_PIN_INPUT_PULLUP),
	OMAP3_MUX(MCSPI1_CS2, OMAP_MUX_MODE3 | OMAP_PIN_INPUT_PULLUP),
	OMAP3_MUX(MCBSP1_DX, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(MCBSP1_DR, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
#endif
/*Camera*/
	OMAP3_MUX(CAM_XCLKA,OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), /*CAM_MCLK*/
	OMAP3_MUX(CAM_HS,OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_VS,OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_PCLK,OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_FLD,OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(CAM_D0,OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D1,OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D2,OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D3,OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D4,OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D5,OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D6,OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D7,OMAP_MUX_MODE0 | OMAP_PIN_INPUT),

	OMAP3_MUX(CAM_D11,OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(CAM_XCLKB,OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), /*V_CAM_EN*/
	OMAP3_MUX(CAM_STROBE,OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),

	OMAP3_MUX(CAM_D11,OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),

	/*  add by jhy for Cam use GPIO_IIC  2011.6.3 Beg*/
#if 1 /*defined(CONFIG_CAM_GPIO_I2C)*/
	OMAP3_MUX(ETK_D9,OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLUP),
	OMAP3_MUX(ETK_D10,OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLUP),
#endif
       /*Control between AP and CBP*/
       OMAP3_MUX(ETK_D2, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*GPS_PWR_EN GPIO_16*/
       OMAP3_MUX(ETK_D7, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*C_USB_EN GPIO_21*/
       OMAP3_MUX(ETK_D8, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*C_PCM_EN GPIO_22*/
       OMAP3_MUX(HDQ_SIO, OMAP_MUX_MODE4 | OMAP_PIN_INPUT), /*MANU_MODE GPIO_170*/ 
       OMAP3_MUX(UART1_RTS, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*MODEM_RESET, GPIO_149*/
       OMAP3_MUX(MCSPI1_SOMI, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*MDM_RSV2, MDM_STATUS GPIO_173*/
	OMAP3_MUX(ABR_OUT,OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};
#else
#define board_mux	NULL
#endif

/*config the pins between AP and CP*/
static void __init cbp_pin_init(void)
{
#ifdef CONFIG_BOOTCASE_VIATELECOM
    /*boot by charger, not need to power on CP, set all the pin to be input gpio to avoid power consume*/
    if(!strncmp(get_bootcase(), "charger", strlen("charger"))){
        omap_mux_init_gpio(16, OMAP_PIN_INPUT_PULLDOWN);/*GPS_PWR_EN GPIO_16*/
        gpio_direction_input(16);
        omap_mux_init_gpio(21, OMAP_PIN_INPUT_PULLDOWN);/*C_USB_EN GPIO_21*/
        gpio_direction_input(21);
        omap_mux_init_gpio(22, OMAP_PIN_INPUT_PULLDOWN);/*C_PCM_EN GPIO_22*/
        gpio_direction_input(22);
        omap_mux_init_gpio(170, OMAP_PIN_INPUT_PULLDOWN);/*MANU_MODE GPIO_170*/
        gpio_direction_input(170);
        omap_mux_init_gpio(149, OMAP_PIN_INPUT_PULLDOWN); /*MODEM_RESET, GPIO_149*/
        gpio_direction_input(149);
        omap_mux_init_gpio(173, OMAP_PIN_INPUT_PULLDOWN);/*MDM_RSV2, MDM_STATUS GPIO_173*/
        gpio_direction_input(173);
        omap_ctrl_writew(OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLDOWN, 0xa56);/*MODEM_PWR_EN, GPIO_126*/
        gpio_direction_input(126);
        omap_ctrl_writew(OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLDOWN, 0xa54); /*AP_RDY GPIO_127*/
        gpio_direction_input(127);
        omap_ctrl_writew(OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLDOWN, 0xa58); /*GPIO_128, No used*/
        gpio_direction_input(128);
        omap_ctrl_writew(OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLDOWN, 0xa5a);/*MDM_READY, GPIO_129*/
        gpio_direction_input(129);
        omap_mux_init_signal("mcspi1_cs3.gpio_177", OMAP_PIN_INPUT);
        gpio_direction_input(177);
        omap_mux_init_signal("etk_d15.gpio_29", OMAP_PIN_INPUT);
        gpio_direction_input(29);
    }
    else
#endif
    {
        gpio_direction_output(173, 0);/*MDM_SRV2 to low*/
        /*Note: gpio_126 ~ gpio_129 is powered by VSIM 3.0V, which is extern gpio in OMPA3430 and can only configged by regiser write*/
        omap_ctrl_writew(OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT, 0xa56);/*MODEM_PWR_EN, GPIO_126*/
        omap_ctrl_writew(OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT, 0xa54); /*AP_RDY GPIO_127*/
        omap_ctrl_writew(OMAP_MUX_MODE4 | OMAP_PIN_INPUT, 0xa58); /*GPIO_128, No used*/
        omap_ctrl_writew(OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLDOWN | OMAP_PIN_OFF_WAKEUPENABLE | OMAP_PIN_OFF_INPUT_PULLDOWN, 0xa5a);/*MDM_READY, GPIO_129*/
        /*config the ohci usb pin*/
        omap_mux_init_signal("mcspi1_cs3.mm2_txdat", OMAP_PIN_INPUT); /*C_USB_TXDAT, config to OHCI*/
        omap_mux_init_signal("etk_d15.mm2_txse0", OMAP_PIN_INPUT);/*C_USB_TXSE0, config to OHCI*/
    }
}

static const struct usbhs_omap_platform_data usbhs_pdata __initconst = {
	.port_mode[0]		= OMAP_USBHS_PORT_MODE_UNUSED,
	.port_mode[1]		= OMAP_OHCI_PORT_MODE_TLL_2PIN_DATSE0,
	.port_mode[2]		= OMAP_USBHS_PORT_MODE_UNUSED,
	.phy_reset		= false,
	.reset_gpio_port[0]	= -EINVAL,
	.reset_gpio_port[1]	= -EINVAL,
	.reset_gpio_port[2]	= -EINVAL,
};

int plat_kim_suspend(struct platform_device *pdev, pm_message_t state)
{
	/* TODO: wait for HCI-LL sleep */
	return 0;
}

int plat_kim_resume(struct platform_device *pdev)
{
	return 0;
}

/* wl128x BT, FM, GPS connectivity chip */
static int gpios[] = {109, 161, -1};

struct ti_st_plat_data wilink_pdata = {
	.nshutdown_gpio = 109,
	.dev_name = "/dev/ttyS1",
	.flow_cntrl = 1,
	.baud_rate = 3000000,
	.suspend = plat_kim_suspend,
	.resume = plat_kim_resume,
};

static struct platform_device wl127x_device = {
	.name           = "kim",
	.id             = -1,
	.dev.platform_data = &wilink_pdata,
};

static struct platform_device btwilink_device = {
	.name = "btwilink",
	.id = -1,
};

static struct platform_device *kunlun_devices[] __initdata = {
	&wl127x_device,
	&btwilink_device,
};

/* OPP MPU/IVA Clock Frequency */
struct opp_frequencies {
	unsigned long mpu;
	unsigned long iva;
	unsigned long ena;
};

static struct opp_frequencies opp_freq_add_table[] __initdata = {
	{
		.mpu = 800000000,
		.iva = 660000000,
		.ena = OMAP3630_CONTROL_FUSE_OPP120_VDD1,
	},
	{
		.mpu = 1000000000,
		.iva =  800000000,
		.ena = OMAP3630_CONTROL_FUSE_OPP1G_VDD1,
	},
	{
		.mpu = 1200000000,
		.iva =   65000000,
		.ena = OMAP3630_CONTROL_FUSE_OPP1_2G_VDD1,
	},
	{ 0, 0, 0 },
};

/* Fix to prevent VIO leakage on wl127x */
static int wl127x_vio_leakage_fix(void)
{
	int ret = 0;

	pr_info(" wl127x_vio_leakage_fix\n");

	ret = gpio_request(gpios[0], "wl127x_bten");
	if (ret < 0) {
		pr_err("wl127x_bten gpio_%d request fail",
			gpios[0]);
		goto fail;
	}

	gpio_direction_output(gpios[0], 1);
	mdelay(10);
	gpio_direction_output(gpios[0], 0);
	udelay(64);

	gpio_free(gpios[0]);
fail:
	return ret;
}

#ifdef CONFIG_USB_OHCI_HCD_OMAP3_LEGACY
extern int usb_ohci_init(void);
#endif
static void __init omap_kunlun_init(void)
{
	omap3_mux_init(board_mux, OMAP_PACKAGE_CBP);
	config_wlan_mux();
	kunlun_peripherals_init();
	kunlun_flash_init();
	kunlun_display_init();
	omap_mux_init_gpio(64, OMAP_PIN_OUTPUT);
	omap_mux_init_gpio(109, OMAP_PIN_OUTPUT);
	omap_mux_init_gpio(161, OMAP_PIN_OUTPUT);
	omap_mux_init_gpio(McBSP3_BT_GPIO, OMAP_PIN_OUTPUT);
       cbp_pin_init();
#ifdef CONFIG_USB_OHCI_HCD_OMAP3_LEGACY
    usb_ohci_init();
#else
	usb_uhhtll_init(&usbhs_pdata);
#endif
	sr_class1p5_init();

#ifdef CONFIG_PM
#ifdef CONFIG_TWL4030_CORE
	omap_voltage_register_pmic(&omap_pmic_core, "core");
	omap_voltage_register_pmic(&omap_pmic_mpu, "mpu");
#endif
	omap_voltage_init_vc(&vc_config);
#endif
	platform_add_devices(kunlun_devices, ARRAY_SIZE(kunlun_devices));
	wl127x_vio_leakage_fix();
#ifdef CONFIG_MACH_OMAP3_VIBRAOTR
        kunlun_init_vibrator();
#endif
}

/* must be called after omap2_common_pm_init() */
static int __init kunlun_opp_init(void)
{
	struct omap_hwmod *mh, *dh;
	struct omap_opp *mopp, *dopp;
	struct device *mdev, *ddev;
	struct opp_frequencies *opp_freq;
	unsigned long hw_support;


	if (!cpu_is_omap3630())
		return 0;

	mh = omap_hwmod_lookup("mpu");
	if (!mh || !mh->od) {
		pr_err("%s: no MPU hwmod device.\n", __func__);
		return 0;
	}

	dh = omap_hwmod_lookup("iva");
	if (!dh || !dh->od) {
		pr_err("%s: no DSP hwmod device.\n", __func__);
		return 0;
	}

	mdev = &mh->od->pdev.dev;
	ddev = &dh->od->pdev.dev;

	/* add MPU and IVA clock frequencies */
	for (opp_freq = opp_freq_add_table; opp_freq->mpu; opp_freq++) {
		/* check enable/disable status of MPU frequecy setting */
		mopp = opp_find_freq_exact(mdev, opp_freq->mpu, false);
		hw_support = omap_ctrl_readl(opp_freq->ena);

		if (IS_ERR(mopp))
			mopp = opp_find_freq_exact(mdev, opp_freq->mpu, true);
		if (IS_ERR(mopp) || !hw_support) {
			pr_err("%s: MPU does not support %lu MHz\n", __func__, opp_freq->mpu / 1000000);
			continue;
		}

		/* check enable/disable status of IVA frequency setting */
		dopp = opp_find_freq_exact(ddev, opp_freq->iva, false);
		if (IS_ERR(dopp))
			dopp = opp_find_freq_exact(ddev, opp_freq->iva, true);
		if (IS_ERR(dopp)) {
			pr_err("%s: DSP does not support %lu MHz\n", __func__, opp_freq->iva / 1000000);
			continue;
		}

		/* try to enable MPU frequency setting */
		if (opp_enable(mopp)) {
			pr_err("%s: OPP cannot enable MPU:%lu MHz\n", __func__, opp_freq->mpu / 1000000);
			continue;
		}

		/* try to enable IVA frequency setting */
		if (opp_enable(dopp)) {
			pr_err("%s: OPP cannot enable DSP:%lu MHz\n", __func__, opp_freq->iva / 1000000);
			opp_disable(mopp);
			continue;
		}

		/* verify that MPU and IVA frequency settings are available */
		mopp = opp_find_freq_exact(mdev, opp_freq->mpu, true);
		dopp = opp_find_freq_exact(ddev, opp_freq->iva, true);
		if (!mopp || !dopp) {
			pr_err("%s: OPP requested MPU: %lu MHz and DSP: %lu MHz not found\n",
				__func__, opp_freq->mpu / 1000000, opp_freq->iva / 1000000);
			continue;
		}

		dev_info(mdev, "OPP enabled %lu MHz\n", opp_freq->mpu / 1000000);
		dev_info(ddev, "OPP enabled %lu MHz\n", opp_freq->iva / 1000000);
	}

	return 0;
}
device_initcall(kunlun_opp_init);

MACHINE_START(OMAP_KUNLUN, "OMAP3630 kunlun board - Haier N710E")
	.phys_io	= 0x48000000,
	.io_pg_offst	= (0xFA000000 >> 18) & 0xfffc,
	.boot_params	= 0x80000100,
	.map_io		= omap_kunlun_map_io,
	.init_irq	= omap_kunlun_init_irq,
	.init_machine	= omap_kunlun_init,
	.timer		= &omap_timer,
MACHINE_END
