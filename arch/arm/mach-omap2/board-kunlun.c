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

/** UGlee: This file contains declarations **/
#include <mach/board-kunlun.h>

#include <plat/common.h>
#include <plat/control.h>
#include <plat/board.h>
#include <plat/usb.h>
#include <plat/opp_twl_tps.h>
#include <plat/timer-gp.h>

#include "mux.h"
#include "sdram-hynix-h8mbx00u0mer-0em.h"
#include "smartreflex-class1p5.h"
#include "board-zoom2-wifi.h"
#include <linux/skbuff.h>
#include <linux/ti_wilink_st.h>
#if defined(CONFIG_SENSORS_ADBS_A320)
#include <linux/i2c/adbs-a320.h>
#endif

#ifdef CONFIG_MACH_OMAP3_VIBRAOTR
void __init kunlun_init_vibrator(void);
#endif

#define McBSP3_BT_GPIO 22

struct regulator *vaux1_regulator = NULL;

/******* UGlee test begin ******************************/

#include <linux/i2c/twl.h>

/** TWL5030 PWM1 **/
#define REG_INTBR_GPBR1				0xc
#define REG_INTBR_GPBR1_PWM1_OUT_EN		(0x1 << 3)
#define REG_INTBR_GPBR1_PWM1_OUT_EN_MASK	(0x1 << 3)
#define REG_INTBR_GPBR1_PWM1_CLK_EN		(0x1 << 1)
#define REG_INTBR_GPBR1_PWM1_CLK_EN_MASK	(0x1 << 1)

/* pin mux for LCD backlight*/
#define REG_INTBR_PMBR1				0xd
#define REG_INTBR_PMBR1_PWM1_PIN_EN		(0x3 << 4)
#define REG_INTBR_PMBR1_PWM1_PIN_MASK		(0x3 << 4)



static void twl5030_pwm1_test(void) {
	
	u8 pmbr1, gpbr1;
	int enable = 1;

	/** config **/	
	/** read interface bit register INTBR, 
    	PMBR1 Pad-Multiplexing Register Bank, P636, Table 12-46 **/
	twl_i2c_read_u8(TWL4030_MODULE_INTBR, &pmbr1, REG_INTBR_PMBR1);
	pmbr1 &= ~REG_INTBR_PMBR1_PWM1_PIN_MASK;	/** enable GPIO_7 function **/
	pmbr1 |=  REG_INTBR_PMBR1_PWM1_PIN_EN;		/** enable PWM1 function **/
	twl_i2c_write_u8(TWL4030_MODULE_INTBR, pmbr1, REG_INTBR_PMBR1);

	/** set brightness **/
	twl_i2c_write_u8(TWL4030_MODULE_PWM1, 0x0C, 0);		/* PWM1ON */
	twl_i2c_write_u8(TWL4030_MODULE_PWM1, 0x7F, 1);		/* PWM1OFF */


	/** enable output **/
	/** read gpbr1 **/
	twl_i2c_read_u8(TWL4030_MODULE_INTBR, &gpbr1, REG_INTBR_GPBR1);
	gpbr1 &= ~REG_INTBR_GPBR1_PWM1_OUT_EN_MASK;
	gpbr1 |= (enable ? REG_INTBR_GPBR1_PWM1_OUT_EN : 0);
	twl_i2c_write_u8(TWL4030_MODULE_INTBR, gpbr1, REG_INTBR_GPBR1);

	twl_i2c_read_u8(TWL4030_MODULE_INTBR, &gpbr1, REG_INTBR_GPBR1);
	gpbr1 &= ~REG_INTBR_GPBR1_PWM1_CLK_EN_MASK;
	gpbr1 |= (enable ? REG_INTBR_GPBR1_PWM1_CLK_EN : 0);
	twl_i2c_write_u8(TWL4030_MODULE_INTBR, gpbr1, REG_INTBR_GPBR1);
}



int vaux1_control(int enable)
{
	
	printk(KERN_ERR "vaux1_control, enabled = %d -------------------------------------------------------------------------------------------------------------------\n", enable);

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

// #if defined(CONFIG_SENSORS_ADBS_A320)
struct regulator *vaux2_regulator = NULL;

int vaux2_control(int enable)
{
	int ret;
	if (!vaux2_regulator)
		vaux2_regulator = regulator_get(NULL, "vcc_vaux2");

	if (IS_ERR(vaux2_regulator)) {
		printk("can't get vcc_vaux2 regulator\n");
		vaux2_regulator = NULL;
		return PTR_ERR(vaux2_regulator);
	}

	ret = enable ? regulator_enable(vaux2_regulator) :
					regulator_disable(vaux2_regulator);

	return ret;
}

// #endif

#if  defined(CONFIG_SENSORS_ADBS_A320)

int init_ofn_hw(void)
{
	int ret = 0;

	ret = gpio_request(ADBS_A320_SHDN_GPIO, "a320_shdn");

	if (ret < 0) {
		printk(KERN_ERR "can't get ofn mouse shdn GPIO\n");
		return ret;
	}

	gpio_direction_output(ADBS_A320_SHDN_GPIO, 0);

	ret = gpio_request(ADBS_A320_NRST_GPIO, "a320_nrst");

	if (ret < 0) {
		printk(KERN_ERR "can't get ofn mouse nrst GPIO\n");
		return ret;
	}

	gpio_direction_output(ADBS_A320_NRST_GPIO, 1);

	ret = gpio_request(ADBS_A320_IRQ_GPIO, "a320_irq");

	if (ret < 0) {
		printk(KERN_ERR "can't get ofn mouse irq GPIO\n");
		return ret;
	}

	gpio_direction_input(ADBS_A320_NRST_GPIO);

	// power on timin
	// set shutdown low
	gpio_set_value(ADBS_A320_SHDN_GPIO, 0);

	// set NRESET high
	gpio_set_value(ADBS_A320_NRST_GPIO, 1);

	// hardware reset device
	gpio_set_value(ADBS_A320_NRST_GPIO, 0);

	mdelay(2);

	// clear reset
	gpio_set_value(ADBS_A320_NRST_GPIO, 1);

	return ret;
}

#endif

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

static void __init omap_kunlun_init_irq(void)
{
	omap_board_config = kunlun_config;
	omap_board_config_size = ARRAY_SIZE(kunlun_config);
	omap2_init_common_hw(h8mbx00u0mer0em_sdrc_params,
			h8mbx00u0mer0em_sdrc_params);
	omap2_gp_clockevent_set_gptimer(1);
	omap_init_irq();
}

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux board_mux[] __initdata = {

	// GPMC
	// SDRC

	/* DSS for LCD */
	OMAP3_MUX(DSS_PCLK, 	OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), 	/*PCLK*/
	OMAP3_MUX(DSS_HSYNC, 	OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),	/*HSYNC*/
	OMAP3_MUX(DSS_VSYNC, 	OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), 	/*VSYNC*/
    	OMAP3_MUX(DSS_ACBIAS, 	OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), 	/*ACBIAS*/
    	OMAP3_MUX(DSS_DATA0, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT), 	/*DATA0*/
    	OMAP3_MUX(DSS_DATA1, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT), 	/*DATA1*/
    	OMAP3_MUX(DSS_DATA2, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT), 	/*DATA2*/
    	OMAP3_MUX(DSS_DATA3, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT), 	/*DATA3*/
    	OMAP3_MUX(DSS_DATA4, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT), 	/*DATA4*/
    	OMAP3_MUX(DSS_DATA5, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT), 	/*DATA5*/
    	OMAP3_MUX(DSS_DATA6, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT), 	/*DATA6*/
    	OMAP3_MUX(DSS_DATA7, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT), 	/*DATA7*/
    	OMAP3_MUX(DSS_DATA8, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT), 	/*DATA8*/
    	OMAP3_MUX(DSS_DATA9, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT), 	/*DATA9*/
    	OMAP3_MUX(DSS_DATA10, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT), 	/*DATA10*/
    	OMAP3_MUX(DSS_DATA11, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT), 	/*DATA11*/
    	OMAP3_MUX(DSS_DATA12, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT), 	/*DATA12*/
    	OMAP3_MUX(DSS_DATA13, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT), 	/*DATA13*/
    	OMAP3_MUX(DSS_DATA14, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT), 	/*DATA14*/
    	OMAP3_MUX(DSS_DATA15, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT), 	/*DATA15*/
	OMAP3_MUX(DSS_DATA16,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/*DATA16*/
	OMAP3_MUX(DSS_DATA17,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/*DATA17*/
	OMAP3_MUX(DSS_DATA18,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/*DATA18*/
	OMAP3_MUX(DSS_DATA19,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/*DATA19*/
	OMAP3_MUX(DSS_DATA20,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/*DATA20*/
	OMAP3_MUX(DSS_DATA21,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/*DATA21*/
	OMAP3_MUX(DSS_DATA22,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/*DATA22*/
	OMAP3_MUX(DSS_DATA23,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/*DATA23*/
	
	// CVIDEO1_OUT
	// CVIDEO1_VFB
	// CVIDEO2_OUT
	// CVIDEO2_VFB
	// CVIDEO1_RSET

	OMAP3_MUX(CAM_HS,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */
	OMAP3_MUX(CAM_VS,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */
	OMAP3_MUX(CAM_XCLKA,	OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),	/* CAM_MCLK */
	OMAP3_MUX(CAM_PCLK,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* not used */
	OMAP3_MUX(CAM_FLD,	OMAP_MUX_MODE4 | OMAP_PIN_INPUT),	/* CAM_RST_B, active low, float or pullup (the cam module pullup internally, GPIO_98 */
	OMAP3_MUX(CAM_D0,	OMAP_MUX_MODE2 | OMAP_PIN_INPUT),	/* CSI2_CLK_P */
	OMAP3_MUX(CAM_D1, 	OMAP_MUX_MODE2 | OMAP_PIN_INPUT),	/* CSI2_CLK_N */
	OMAP3_MUX(CAM_D2,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */
	OMAP3_MUX(CAM_D3,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */
	OMAP3_MUX(CAM_D4,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */
	OMAP3_MUX(CAM_D5,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */
	OMAP3_MUX(CAM_D6,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */
	OMAP3_MUX(CAM_D7,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */
	OMAP3_MUX(CAM_D8,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */
	OMAP3_MUX(CAM_D9,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */
	OMAP3_MUX(CAM_D10,	OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLUP),/* BT_RST_N, active low?, GPIO_109 */
	OMAP3_MUX(CAM_D11,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */
	OMAP3_MUX(CAM_XCLKB,	OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),	/* ULPI_VUSB_EN, active high?, GPIO_111 */
	OMAP3_MUX(CAM_WEN,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */
	OMAP3_MUX(CAM_STROBE,	OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),	/* CAM_AF_PWDN, active high? low? GPIO_126 */
	OMAP3_MUX(CSI2_DX0,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* CSI2_DATA_LANE0_P */
	OMAP3_MUX(CSI2_DY0,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* CSI2_DATA_LANE0_N */
	OMAP3_MUX(CSI2_DX1,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* CSI2_DATA_LANE1_P */
	OMAP3_MUX(CSI2_DY1,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* CSI2_DATA_LANE1_N */

	// SDMMC1
	OMAP3_MUX(SDMMC1_CLK, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT), 		/* MMC1_CLK, SD CARD */
	OMAP3_MUX(SDMMC1_CMD, 	OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), 		/* MMC1_CMD, SD CARD */
	OMAP3_MUX(SDMMC1_DAT0, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP), 	/* MMC1_DAT0, SD CARD */
	OMAP3_MUX(SDMMC1_DAT1, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP), 	/* MMC1_DAT1, SD CARD */
	OMAP3_MUX(SDMMC1_DAT2, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP), 	/* MMC1_DAT2, SD CARD */
	OMAP3_MUX(SDMMC1_DAT3, 	OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP), 	/* MMC1_DAT3, SD CARD */
 

	// SIM I/O
	OMAP3_MUX(SIM_IO,	OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),	/* MDM_KEYON */
	OMAP3_MUX(SIM_CLK,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */
	OMAP3_MUX(SIM_PWRCTRL,	OMAP_MUX_MODE4 | OMAP_PIN_INPUT),	/* WCDMA_RI, don't know high or low, refer to manual */
	OMAP3_MUX(SIM_RST,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */

	// HSUSB0, aka OTG
	OMAP3_MUX(HSUSB0_CLK,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* HSUSB0_CLK */
	OMAP3_MUX(HSUSB0_STP,	OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),	/* HSUSB0_STP */
	OMAP3_MUX(HSUSB0_DIR,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* HSUSB0_DIR */
	OMAP3_MUX(HSUSB0_NXT,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* HSUSB0_NXT */
	OMAP3_MUX(HSUSB0_DATA0, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(HSUSB0_DATA1, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(HSUSB0_DATA2, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(HSUSB0_DATA3, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(HSUSB0_DATA4, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(HSUSB0_DATA5, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(HSUSB0_DATA6, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(HSUSB0_DATA7, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),

	// SDMMC2
	OMAP3_MUX(SDMMC2_CLK,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* SD2_CLK */
	OMAP3_MUX(SDMMC2_CMD,	OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),	/* SD2_CMD */
	OMAP3_MUX(SDMMC2_DAT0,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* SD2_DATA0 */
	OMAP3_MUX(SDMMC2_DAT1,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* SD2_DATA1 */
	OMAP3_MUX(SDMMC2_DAT2,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* SD2_DATA2 */
	OMAP3_MUX(SDMMC2_DAT3,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* SD2_DATA3 */
	OMAP3_MUX(SDMMC2_DAT7,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */
	OMAP3_MUX(SDMMC2_DAT6,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */
	OMAP3_MUX(SDMMC2_DAT5,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */
	OMAP3_MUX(SDMMC2_DAT4,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */

	// UART1, connect to CID Card Scanner, with level shifter and gpio
	OMAP3_MUX(UART1_TX,	OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),	/* UART1_TX */
	OMAP3_MUX(UART1_RX,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* UART1_RX */
	OMAP3_MUX(UART1_CTS,	OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),	/* UART1_EN, GPIO_150 */
	OMAP3_MUX(UART1_RTS,	OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),	/* MDM_RST, GPIO_149, active high? low? */
	
	// UART3, connect to GPS, and testpoint
	OMAP3_MUX(UART3_CTS_RCTX,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */
	OMAP3_MUX(UART3_RTS_SD,		OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */
	OMAP3_MUX(UART3_RX_IRRX,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* UART3_RX */
	OMAP3_MUX(UART3_TX_IRTX,	OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),	/* UART3_TX */

	// MCBSP1, used as gpio, mostly
	OMAP3_MUX(MCBSP1_CLKR,	OMAP_MUX_MODE4 | OMAP_PIN_INPUT),	/* M_SEN_RDY, GPIO_156, active high? low? */
	OMAP3_MUX(MCBSP1_FSR,	OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),	/* WL_SHDN_N, GPIO_157, active high? low? */
	OMAP3_MUX(MCBSP_CLKS,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* clock TWL_CLK256FS */
	OMAP3_MUX(MCBSP1_DX,	OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLUP),/* TOUCH_RST, GPIO_158 */
	OMAP3_MUX(MCBSP1_DR,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCBSP1_FSX,	OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),	/* USB_SW_SEL, gpio_161 */
	OMAP3_MUX(MCBSP1_CLKX,	OMAP_MUX_MODE4 | OMAP_PIN_INPUT),	/* WLAN_IRQ, GPIO_162 */
	
	// MCBSP3, as MCBSP
	OMAP3_MUX(MCBSP3_DX,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCBSP3_DR,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCBSP3_CLKX,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCBSP3_FSX,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),

	// MCBSP4, as gpio mostly
	OMAP3_MUX(MCBSP4_CLKX,	OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),	/* OVP_REV_N, GPIO_152, TODO active high? low? */
	OMAP3_MUX(MCBSP4_DR,	OMAP_MUX_MODE4 | OMAP_PIN_INPUT),	/* TOUCH_IRQ, GPIO_153, TODO pull? */
	OMAP3_MUX(MCBSP4_DX,	OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),	/* KPD_LED_EN, GPIO_154, TODO active high? low? */
	OMAP3_MUX(MCBSP4_FSX,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused */

	
	// I2C1, dedicated for twl5030
	OMAP3_MUX(I2C1_SCL,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP),
	OMAP3_MUX(I2C1_SDA,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP),
	
	// I2C2, ???
	OMAP3_MUX(I2C2_SCL,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP),
	OMAP3_MUX(I2C2_SDA,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP),

	// I2C3, ???
	OMAP3_MUX(I2C3_SCL,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP),
	OMAP3_MUX(I2C3_SDA,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP),

	// I2C4, smart-reflex
	OMAP3_MUX(I2C4_SCL,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP),
	OMAP3_MUX(I2C4_SDA,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP),
	
	//HDQ_SIO
	OMAP3_MUX(HDQ_SIO,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* unused in kernel */

	// MCSPI1
	OMAP3_MUX(MCSPI1_CLK,	OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLUP),/* LCD_SCL, GPIO_171 */
	OMAP3_MUX(MCSPI1_SIMO,	OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLUP),/* LCD_SDA, GPIO_172 */
	OMAP3_MUX(MCSPI1_SOMI,	OMAP_MUX_MODE4 | OMAP_PIN_INPUT),	/* WL_CLK_REQ, GPIO_173, TODO active high? low? */
	OMAP3_MUX(MCSPI1_CS0,	OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLUP),/* LCD_CS, GPIO_174, active low */
	OMAP3_MUX(MCSPI1_CS1,	OMAP_MUX_MODE3 | OMAP_PIN_OUTPUT),	/* WLAN_MMC3_CMD */
	OMAP3_MUX(MCSPI1_CS2,	OMAP_MUX_MODE3 | OMAP_PIN_INPUT),	/* WLAN_MMC3_CLK */
	OMAP3_MUX(MCSPI1_CS3,	OMAP_MUX_MODE3 | OMAP_PIN_INPUT),	/* HSUSB2_DATA2 */

	// MCSPI2
	OMAP3_MUX(MCSPI2_CLK,	OMAP_MUX_MODE3 | OMAP_PIN_INPUT),	/* HSUSB2_DATA7 */
	OMAP3_MUX(MCSPI2_SIMO,	OMAP_MUX_MODE3 | OMAP_PIN_INPUT),	/* HSUSB2_DATA4 */
	OMAP3_MUX(MCSPI2_SOMI,	OMAP_MUX_MODE3 | OMAP_PIN_INPUT),	/* HSUSB2_DATA5 */
	OMAP3_MUX(MCSPI2_CS0,	OMAP_MUX_MODE3 | OMAP_PIN_INPUT),	/* HSUSB2_DATA6 */
	OMAP3_MUX(MCSPI2_CS1,	OMAP_MUX_MODE3 | OMAP_PIN_INPUT),	/* HSUSB2_DATA3 */
	
	// UART2, connect to bluetooth
	OMAP3_MUX(UART2_CTS,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* BT_UART_CTS_N */
	OMAP3_MUX(UART2_RTS,	OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),	/* BT_UART_RTS_N */
	OMAP3_MUX(UART2_TX,	OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),	/* BT_UART_TXD */
	OMAP3_MUX(UART2_RX,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* BT_UART_RXD */

	// MCBSP2
	OMAP3_MUX(MCBSP2_FSX,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* MCBSP2_FSX */
	OMAP3_MUX(MCBSP2_CLKX,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* MCBSP2_CLKX */
	OMAP3_MUX(MCBSP2_DR,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* MCBSP2_DR */
	OMAP3_MUX(MCBSP2_DX,	OMAP_MUX_MODE0 | OMAP_PIN_INPUT),	/* MCBSP2_DX */

	// SYS_32K
	// SYS_XTALIN
	// SYS_XTALOUT
	// SYS_XTALGND
	// SYS_CLKREQ
	// SYS_NIRQ
	// SYS_NRESPWERON
	// SYS_NRESWARM
	// SYS_BOOT0~6
	// SYS_OFF_MODE
	// SYS_CLKOUT1
	// SYS_CLKOUT2
	// JTAG, JTAG EMU0, JTAG EMU1

	// ETK
	OMAP3_MUX(ETK_CLK,	OMAP_MUX_MODE4 | OMAP_PIN_INPUT),	/* W_PCM_CLK, GPIO_12, listen to Modem in off mode */
	OMAP3_MUX(ETK_CTL,	OMAP_MUX_MODE4 | OMAP_PIN_INPUT),	/* VFAULT_N, GPIO_13, TODO pull? */
	OMAP3_MUX(ETK_D0,	OMAP_MUX_MODE4 | OMAP_PIN_INPUT),	/* OVP_DIR_N, GPIO_14, TODO pull? or drive? */
	OMAP3_MUX(ETK_D1,	OMAP_MUX_MODE4 | OMAP_PIN_INPUT),	/* ULPI_CS, GPIO_15, TODO pull? */
	OMAP3_MUX(ETK_D2,	OMAP_MUX_MODE4 | OMAP_PIN_INPUT),	/* LCD_BL_EN2, GPIO_16, TODO used? */
	OMAP3_MUX(ETK_D3,	OMAP_MUX_MODE2 | OMAP_PIN_INPUT),	/* WLAN_MMC3_DATA3, TODO pull? */
	OMAP3_MUX(ETK_D4,	OMAP_MUX_MODE2 | OMAP_PIN_INPUT),	/* WLAN_MMC3_DATA0 */
	OMAP3_MUX(ETK_D5,	OMAP_MUX_MODE2 | OMAP_PIN_INPUT),	/* WLAN_MMC3_DATA1 */
	OMAP3_MUX(ETK_D6,	OMAP_MUX_MODE2 | OMAP_PIN_INPUT),	/* WLAN_MMC3_DATA2 */
	OMAP3_MUX(ETK_D7,	OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),	/* USB_SW_EN, GPIO_21 */
	OMAP3_MUX(ETK_D8,	OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),	/* W_PCM_EN, GPIO_22 */
	OMAP3_MUX(ETK_D9,	OMAP_MUX_MODE4 | OMAP_PIN_INPUT),	/* VREG_MSME, GPIO_23 */
	OMAP3_MUX(ETK_D10,	OMAP_MUX_MODE3 | OMAP_PIN_OUTPUT),	/* HSUSB2_CLK */
	OMAP3_MUX(ETK_D11,	OMAP_MUX_MODE3 | OMAP_PIN_OUTPUT),	/* HSUSB2_STP */
	OMAP3_MUX(ETK_D12,	OMAP_MUX_MODE3 | OMAP_PIN_INPUT),	/* HSUSB2_DIR */
	OMAP3_MUX(ETK_D13,	OMAP_MUX_MODE3 | OMAP_PIN_INPUT),	/* HSUSB2_NXT */
	OMAP3_MUX(ETK_D14,	OMAP_MUX_MODE3 | OMAP_PIN_INPUT),	/* HSUSB2_DATA0 */
	OMAP3_MUX(ETK_D15,	OMAP_MUX_MODE3 | OMAP_PIN_INPUT),	/* HSUSB2_DATA1 */
	
#if 0



	OMAP3_MUX(MCBSP4_DX,	OMAP_MUX_MODE7 | OMAP_PIN_INPUT),	/* LVDS_EN, DNP default, so put in safe mode */



	
//    OMAP3_MUX(DSS_DATA22, 	OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*V_CAM_EN GPIO_92*/
//    OMAP3_MUX(DSS_DATA23, 	OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*LCD_RST GPIO_93*/
    OMAP3_MUX(MCBSP4_FSX, 	OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*LCD_BL_EN GPIO_155*/
    OMAP3_MUX(MCSPI1_CLK, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), /*LCD LCD_SPI_CLK*/
    OMAP3_MUX(MCSPI1_SIMO, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), /*LCD LCD_SPI_SIMO*/
    OMAP3_MUX(MCSPI1_CS0, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), /*LCD LCD_SPI_CS*/

/*Touch Screen*/
//    	OMAP3_MUX(DSS_DATA18, OMAP_MUX_MODE2 | OMAP_PIN_INPUT | OMAP_PIN_OFF_INPUT_PULLUP), /*TS_SPI3_CLK*/
//    	OMAP3_MUX(DSS_DATA19, OMAP_MUX_MODE2 | OMAP_PIN_INPUT | OMAP_PIN_OFF_INPUT_PULLUP), /*TS_SPI3_SIMO*/
//    	OMAP3_MUX(DSS_DATA20, OMAP_MUX_MODE2 | OMAP_PIN_INPUT), /*TS_SPI3_SOMI*/
//    	OMAP3_MUX(DSS_DATA21, OMAP_MUX_MODE2 | OMAP_PIN_INPUT_PULLDOWN | OMAP_PIN_OFF_INPUT_PULLUP), /*TS_SPI3_CS0*/
    	
	/** gpio_153 **/
	OMAP3_MUX(MCBSP4_DR, OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLUP), /*TOUCH_IRQ_N, GPIO_153*/
//    	OMAP3_MUX(MCBSP4_DX, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),/*TOUCH_I2C_EN GPIO_154*/

/*Camera*/
    OMAP3_MUX(CAM_HS, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*CAM_HS*/
    OMAP3_MUX(CAM_VS, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*CAM_VS*/
    OMAP3_MUX(CAM_XCLKA, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), /*CAM_XCLKA*/
    OMAP3_MUX(CAM_PCLK, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*CAM_PCLK*/
    OMAP3_MUX(CAM_FLD, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*CAMERA_RST, GPIO_98*/
    OMAP3_MUX(CAM_D0, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*CAM_D0*/
    OMAP3_MUX(CAM_D1, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*CAM_D1*/
    OMAP3_MUX(CAM_D2, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*CAM_D2*/
    OMAP3_MUX(CAM_D3, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*CAM_D3*/
    OMAP3_MUX(CAM_D4, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*CAM_D4*/
    OMAP3_MUX(CAM_D5, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*CAM_D5*/
    OMAP3_MUX(CAM_D6, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*CAM_D6*/
    OMAP3_MUX(CAM_D7, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*CAM_D7*/
    OMAP3_MUX(CAM_D11, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*GPIO_110,CAM_PWDN*/
    OMAP3_MUX(CAM_XCLKB, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), /*CAM_XCLKA->sub CAM_MCLK_S*/
    OMAP3_MUX(CAM_STROBE, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), /*CAM_STROBE*/
    OMAP3_MUX(MCSPI2_SIMO, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*GPIO_179,SUB_CAM_RESET*/
    OMAP3_MUX(MCSPI2_SOMI, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),/* GPIO_180,SUBCAM_PWDN*/
/*Audio Interface */
/*I2S between TWL5030 and AP*/
    OMAP3_MUX(MCBSP2_FSX, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*McBSP2_FSX, TWL5030 I2S*/
    OMAP3_MUX(MCBSP2_CLKX, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*McBSP2_CLKX, TWL5030 I2S*/
    OMAP3_MUX(MCBSP2_DR, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*McBSP2_DR, TWL5030 I2S*/
    OMAP3_MUX(MCBSP2_DX, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), /*McBSP2_DX, TWL5030 I2S*/

/*PCM among AP, CBP and TWL5030, reconfig while audio driver probe*/
    OMAP3_MUX(MCBSP3_DX, OMAP_MUX_MODE7 | OMAP_PIN_INPUT), /*McBSP3_DX, BT_TWL5030_MODEM_PCM*/
    OMAP3_MUX(MCBSP3_DR, OMAP_MUX_MODE7 | OMAP_PIN_OUTPUT), /*McBSP3_DR, BT_TWL5030_MODEM_PCM*/
    OMAP3_MUX(MCBSP3_CLKX, OMAP_MUX_MODE7 | OMAP_PIN_OUTPUT), /*McBSP3_CLKX, BT_TWL5030_MODEM_PCM*/
    OMAP3_MUX(MCBSP3_FSX, OMAP_MUX_MODE7 | OMAP_PIN_INPUT), /*McBSP3_FSX, BT_TWL5030_MODEM_PCM*/
    OMAP3_MUX(MCBSP_CLKS, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*TWL5030_CLK256FS*/
/*FM*/
    OMAP3_MUX(GPMC_NCS4, OMAP_MUX_MODE2 | OMAP_PIN_OUTPUT),/*FM I2S_CLK*/ 
    OMAP3_MUX(GPMC_NCS5, OMAP_MUX_MODE2 | OMAP_PIN_INPUT), /*FM I2S_DR*/
    OMAP3_MUX(GPMC_NCS6, OMAP_MUX_MODE2 | OMAP_PIN_INPUT), /*FM I2S_DX*/

	/** UGlee, this is BL_PWM_AP **/
    	// OMAP3_MUX(GPMC_NCS7, OMAP_MUX_MODE2 | OMAP_PIN_INPUT_PULLUP), /*FM I2S_FS*/
	OMAP3_MUX(GPMC_NCS7, 	OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLDOWN),


    OMAP3_MUX(MCBSP1_CLKR, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*FM FM_RX_EN GPIO_156*/

	/** UGlee, this is WLAN_SHDN_N **/
	// OMAP3_MUX(MCBSP1_FSR, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*FM FM_TX_EN GPIO_157*/
	OMAP3_MUX(MCBSP1_FSR, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),

	
    OMAP3_MUX(MCBSP1_FSX, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*FM_EN, GPIO_161*/

/*HSUSB0*/
    	OMAP3_MUX(HSUSB0_CLK, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*HSUSB0_CLK*/
    	OMAP3_MUX(HSUSB0_STP, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), /*HSUSB0_STP*/
    	OMAP3_MUX(HSUSB0_DIR, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*HSUSB0_DIR*/
    	OMAP3_MUX(HSUSB0_NXT, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*HSUSB0_NXT*/
    	OMAP3_MUX(HSUSB0_DATA0, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*HSUSB0_DATA0*/
    	OMAP3_MUX(HSUSB0_DATA1, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*HSUSB0_DATA1*/
    	OMAP3_MUX(HSUSB0_DATA2, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*HSUSB0_DATA2*/
    	OMAP3_MUX(HSUSB0_DATA3, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*HSUSB0_DATA3*/
    	OMAP3_MUX(HSUSB0_DATA4, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*HSUSB0_DATA4*/
    	OMAP3_MUX(HSUSB0_DATA5, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*HSUSB0_DATA5*/
    	OMAP3_MUX(HSUSB0_DATA6, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*HSUSB0_DATA6*/
    	OMAP3_MUX(HSUSB0_DATA7, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*HSUSB0_DATA7*/

/*WLAN*/
    OMAP3_MUX(ETK_D3, OMAP_MUX_MODE2 | OMAP_PIN_INPUT_PULLUP), /* WLAN MMC3_DATA3 */
    OMAP3_MUX(ETK_D4, OMAP_MUX_MODE2 | OMAP_PIN_INPUT_PULLUP), /* WLAN MMC3_DATA0 */
    OMAP3_MUX(ETK_D5, OMAP_MUX_MODE2 | OMAP_PIN_INPUT_PULLUP), /* WLAN MMC3_DATA1 */
    OMAP3_MUX(ETK_D6, OMAP_MUX_MODE2 | OMAP_PIN_INPUT_PULLUP), /* WLAN MMC3_DATA2 */
    OMAP3_MUX(MCBSP1_CLKX, OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLDOWN), /* WLAN IRQ , GPIO_162*/
    OMAP3_MUX(MCSPI1_CS1, OMAP_MUX_MODE3 | OMAP_PIN_INPUT_PULLUP), /* WLAN MMC3_CMD */
    OMAP3_MUX(MCSPI1_CS2, OMAP_MUX_MODE3 | OMAP_PIN_INPUT_PULLUP), /* WLAN MMC3_CLK */

    	// OMAP3_MUX(MCBSP1_DX, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /* WL_SHDN_N, GPIO_158*/
	/** GPIO 158 touch reset **/
	OMAP3_MUX(MCBSP1_DX, OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLUP),

    OMAP3_MUX(MCBSP1_DR, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /* WL_TCXO_VDD_EN, GPIO_159 */

/*CBP DIGITAL USB*/
    OMAP3_MUX(MCSPI1_CS3, OMAP_MUX_MODE4 | OMAP_PIN_INPUT), /*C_USB_TXDAT, config to input gpio before OHCI init*/
    OMAP3_MUX(ETK_D15, OMAP_MUX_MODE4 | OMAP_PIN_INPUT), /*C_USB_TXSE0, config to input gpio before OHCI init*/

/*Control between AP and CBP*/
    OMAP3_MUX(ETK_D2, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*GPS_PWR_EN GPIO_16*/
    OMAP3_MUX(ETK_D7, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*C_USB_EN GPIO_21*/
    OMAP3_MUX(ETK_D8, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*C_PCM_EN GPIO_22*/
    OMAP3_MUX(HDQ_SIO, OMAP_MUX_MODE4 | OMAP_PIN_INPUT), /*MANU_MODE GPIO_170*/ 
    OMAP3_MUX(UART1_RTS, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*MODEM_RESET, GPIO_149*/
    OMAP3_MUX(MCSPI1_SOMI, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*MDM_RSV2, MDM_STATUS GPIO_173*/

/*I2C1 for TWL5030*/
    OMAP3_MUX(I2C1_SCL, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP), /*I2C1_SCL, TWL5030*/
    OMAP3_MUX(I2C1_SDA, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP), /*I2C1_SDA, TWL5030*/

/*I2C2 for Camera*/
    OMAP3_MUX(I2C2_SCL, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP), /*I2C2_SCL,Camera*/
    OMAP3_MUX(I2C2_SDA, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP), /*I2C2_SDA,Camera*/

/*I2C3 for Accelerator*/
    OMAP3_MUX(I2C3_SCL, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP), /*I2C3_SCL,accelerator*/
    OMAP3_MUX(I2C3_SDA, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP), /*I2C3_SDA,accelerator*/

/*I2C4 for WTL5030*/
    OMAP3_MUX(I2C4_SCL, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP), /*I2C4_SCL, TWL5030*/
    OMAP3_MUX(I2C4_SDA, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP), /*I2C4_SDA, TWL5030*/

/* SD card  */
    OMAP3_MUX(SDMMC1_CLK, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), /*MMC1_CLK, SD CARD*/
    OMAP3_MUX(SDMMC1_CMD, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP), /*MMC1_CMD, SD CARD*/
    OMAP3_MUX(SDMMC1_DAT0, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP), /*MMC1_DAT0, SD CARD*/
    OMAP3_MUX(SDMMC1_DAT1, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP), /*MMC1_DAT1, SD CARD*/
    OMAP3_MUX(SDMMC1_DAT2, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP), /*MMC1_DAT2, SD CARD*/
    OMAP3_MUX(SDMMC1_DAT3, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP), /*MMC1_DAT3, SD CARD*/
    OMAP3_MUX(SDMMC2_DAT7, OMAP_MUX_MODE4 | OMAP_PIN_INPUT), /*SD_CD, GPIO_139*/

/*UART1 for CBP*/
    OMAP3_MUX(UART1_TX, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*UART1_TX*/
    OMAP3_MUX(UART1_RX, OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLUP), /*UART1_RX*/
    OMAP3_MUX(UART1_CTS, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),  /*UART1_EN GPIO_150*/
              
/*UART2 for BT*/
    OMAP3_MUX(UART2_CTS, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP), /*UART2_CTS, BT*/
    OMAP3_MUX(UART2_RTS, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), /*UART2_RTS, BT*/
    OMAP3_MUX(UART2_TX, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), /*UART2_TX, BT*/
    OMAP3_MUX(UART2_RX, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*UART2_RX, BT*/
    OMAP3_MUX(CAM_D10, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /*BT_RST_N, GPIO_109*/

/*UART3 for console*/
    OMAP3_MUX(UART3_TX_IRTX, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), /*UART3_TX*/
    OMAP3_MUX(UART3_RX_IRRX, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP),  /*UART3_RX*/

/*Over Volatge Protect, No used now*/
    OMAP3_MUX(CSI2_DX0, OMAP_MUX_MODE7 | OMAP_PIN_OUTPUT), /* OVP_VOL_FAULT, GPIO_112*/
    OMAP3_MUX(ETK_D0, OMAP_MUX_MODE7 | OMAP_PIN_OUTPUT), /* OVP_DIR_N, GPIO_14*/
    OMAP3_MUX(MCBSP4_CLKX, OMAP_MUX_MODE7 | OMAP_PIN_OUTPUT), /* OVP_REV_N, GPIO_152*/

/*Accelerator*/
    OMAP3_MUX(CSI2_DX1, OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLDOWN), /* ACC_IRQ1, GPIO_114*/
    OMAP3_MUX(CSI2_DY1, OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLDOWN), /* ACC_IRQ2, GPIO_115*/

#ifdef CONFIG_SENSORS_AKM8975
/*Sensor Akm8975*/
    //OMAP3_MUX(MCSPI2_CLK, OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLDOWN), /* AKM8975_RDY, GPIO_178*/
#endif

/*System Control*/
    OMAP3_MUX(CSI2_DX1, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*SYS_32K*/
    OMAP3_MUX(SYS_CLKREQ, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*SYS_CLKREQ*/
    OMAP3_MUX(SYS_NIRQ, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP | OMAP_WAKEUP_EN), /*SYS_nIRQ*/
    OMAP3_MUX(SYS_BOOT0, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*SYSBOOT0*/
    OMAP3_MUX(SYS_BOOT1, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*SYSBOOT1*/
    OMAP3_MUX(SYS_BOOT2, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*SYSBOOT2*/
    OMAP3_MUX(SYS_BOOT3, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*SYSBOOT3*/
    OMAP3_MUX(SYS_BOOT4, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*SYSBOOT4*/
    OMAP3_MUX(SYS_BOOT5, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*SYSBOOT5*/
    OMAP3_MUX(SYS_BOOT6, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*SYSBOOT6*/
    OMAP3_MUX(SYS_OFF_MODE, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /*SYS_OFF_MODE*/

/*JTAG*/
    OMAP3_MUX(JTAG_NTRST, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /* JTAG_nTRST*/
    OMAP3_MUX(JTAG_TCK, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /* JTAG_TCK*/
    OMAP3_MUX(JTAG_TMS_TMSC, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /* JTAG_TMS*/
    OMAP3_MUX(JTAG_TDI, OMAP_MUX_MODE0 | OMAP_PIN_INPUT), /* JTAG_TDI*/
    OMAP3_MUX(JTAG_EMU0, OMAP_MUX_MODE7 | OMAP_PIN_OUTPUT), /* JTAG_EMU0*/
    OMAP3_MUX(JTAG_EMU1, OMAP_MUX_MODE7 | OMAP_PIN_OUTPUT),  /* JTAG_EMU1*/

#ifdef CONFIG_NFC_PN544
/*NFC*/
    OMAP3_MUX(CSI2_DY0, OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLDOWN), /* NFC_IRQ, GPIO_113*/
    OMAP3_MUX(SYS_CLKOUT2, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT), /* SYS_CLOCK2*/
    OMAP3_MUX(ETK_D1, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),  /* NFC_CLK_ACK, GPIO_15*/
    OMAP3_MUX(ETK_D9, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT), /* NFC_VEN, GPIO_23*/
    OMAP3_MUX(CAM_D8, OMAP_MUX_MODE4 | OMAP_PIN_INPUT), /* UICC_PWR_REQ, GPIO_107*/
    OMAP3_MUX(CAM_D9, OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLDOWN), /* NFC_CLK_REQ, GPIO_108*/
#endif

#endif
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
        omap_ctrl_writew(OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT , 0xa56);/*MODEM_PWR_EN, GPIO_126*/
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
	// .port_mode[1]		= OMAP_OHCI_PORT_MODE_TLL_2PIN_DATSE0,
	.port_mode[1]		= OMAP_EHCI_PORT_MODE_PHY,
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
	// mux table
	omap3_mux_init(board_mux, OMAP_PACKAGE_CBP);

	// simple not right
	// config_wlan_mux();
	kunlun_peripherals_init();
	kunlun_flash_init();

	// UGlee
	// kunlun_display_init();
	puma_display_init();

	// UGlee, for test twl 5030 pwm1 output, and failed, crashed the kernel, Sun Oct  7 12:23:39 CST 2012
	// twl5030_pwm1_test();

	// UGlee, dirty job
	// vaux1_control(1);
	// omap_mux_init_gpio(64, OMAP_PIN_OUTPUT);
	// omap_mux_init_gpio(109, OMAP_PIN_OUTPUT);
	// omap_mux_init_gpio(161, OMAP_PIN_OUTPUT);
	// omap_mux_init_gpio(McBSP3_BT_GPIO, OMAP_PIN_OUTPUT);

	/** UGlee, quick fix for gpio request fail */
        // cbp_pin_init();

#ifdef CONFIG_USB_OHCI_HCD_OMAP3_LEGACY
        usb_ohci_init();
	printk("omap_kunlun_init, usb_ohci_init() called.");
#else
	usb_uhhtll_init(&usbhs_pdata);
	printk("omap_kunlun_init, usb_uhhtll_init() called.");
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

	// printk("---- omap_kunlun_init ---- end --------------------------------------------------------------------------------\n");
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

MACHINE_START(OMAP_KUNLUN, "OMAP3630 kunlun board")
	.phys_io	= 0x48000000,
	.io_pg_offst	= (0xFA000000 >> 18) & 0xfffc,
	.boot_params	= 0x80000100,
	.map_io		= omap_kunlun_map_io,
	.init_irq	= omap_kunlun_init_irq,
	.init_machine	= omap_kunlun_init,
	.timer		= &omap_timer,
MACHINE_END
