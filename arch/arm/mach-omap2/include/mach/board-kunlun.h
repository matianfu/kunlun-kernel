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

/*
 * Defines for kunlun boards
 */

#ifndef __OMAP3_BOARD_KUNLUN_H
#define __OMAP3_BOARD_KUNLUN_H

#include <plat/display.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/consumer.h>

struct flash_partitions {
	struct mtd_partition *parts;
	int nr_parts;
};

#define BL_MACHINE_NAME "lcd"
extern void __init kunlun_peripherals_init(void);
extern void __init kunlun_display_init(void);
extern void __init kunlun_flash_init(void);

#define ZOOM2_HEADSET_EXTMUTE_GPIO	153
#define OMAP3430_NAND_CS    0

extern int vaux1_control(int enable);

#if defined(CONFIG_SENSORS_ADBS_A320)
extern int vaux2_control(int enable);
extern int init_ofn_hw(void);
#endif

#ifdef CONFIG_BOOTCASE_VIATELECOM
extern char * get_bootcase(void);
#endif

extern int get_wifi_mac(unsigned char *addr, int len);

extern void haier_n710e_wlan_irq_defconfig(void);
extern void haier_n710e_wlan_irq_config(void);

#endif
