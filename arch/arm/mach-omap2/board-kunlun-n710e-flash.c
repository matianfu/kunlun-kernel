/*
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
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/types.h>
#include <linux/io.h>

#include <asm/mach/flash.h>
#include <plat/board.h>
#include <plat/gpmc.h>
#include <plat/nand.h>

#include <mach/board-kunlun.h>

#if defined(CONFIG_MTD_NAND_OMAP2) || \
		defined(CONFIG_MTD_NAND_OMAP2_MODULE)
/* NAND chip access: 16 bit */
static struct omap_nand_platform_data kunlun_nand_data = {
	.nand_setup	= NULL,
	.dma_channel	= -1,	/* disable DMA in OMAP NAND driver */
	.dev_ready	= NULL,
	.devsize	= 1,	/* '0' for 8-bit, '1' for 16-bit device */
};

static struct mtd_partition kunlun_nand_512M_partitions[] = {
	/* All the partition sizes are listed in terms of NAND block size */
	{
		.name		= "X-Loader-NAND",
		.offset		= 0,
		.size		= 0x0080000,
		.mask_flags	= MTD_WRITEABLE,	/* force read-only */
	},
	{
		.name		= "U-Boot-NAND",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x80000 */
		.size		= 0x00140000,           /* 1.3M */
		.mask_flags	= MTD_WRITEABLE,	/* force read-only */
	},
	{
		.name		= "Boot Env-NAND",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x001C0000 */
		.size		= 0x0040000,
	},
	{
		.name		= "Kernel-NAND",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x00200000 */
		.size		= 0x1E00000,            /* 30M */
	},
	{
		.name		= "system",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x02000000 */
		.size		= 0x7500000,            /* 117M */
	},
	{
		.name		= "userdata",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x09500000 */
		.size		= 0xEA00000,		/* 226M */
	},
	{
		.name		= "cache",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x17F00000 */
		.size		= 0x02000000,           /* 32M */
	},
	{
		.name		= "recovery",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x19F00000 */
		.size		= 0x02000000,           /* 32M */
	},
	{
		.name		= "misc",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x1BF00000 */
		.size		= 0x00020000,           /* 128K */
	},
	{
		.name		= "auto_install",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x1BF20000 */
		.size		= 0x04000000,           /* 64M */
	},
};

/**
 * kunlun_flash_init - Identify devices connected to GPMC and register.
 *
 * @return - void.
 */
void __init kunlun_flash_init(void)
{
	int nandcs;
	u32 gpmc_base_add = OMAP34XX_GPMC_VIRT;

	nandcs = OMAP3430_NAND_CS;

	kunlun_nand_data.cs		= nandcs;
	kunlun_nand_data.parts		= kunlun_nand_512M_partitions;
	kunlun_nand_data.nr_parts	= ARRAY_SIZE(kunlun_nand_512M_partitions);
	kunlun_nand_data.ecc_opt	= 0x1;
	kunlun_nand_data.gpmc_baseaddr	= (void *)(gpmc_base_add);
	kunlun_nand_data.gpmc_cs_baseaddr	= (void *)(gpmc_base_add +
						GPMC_CS0_BASE +
						nandcs * GPMC_CS_SIZE);
	gpmc_nand_init(&kunlun_nand_data);
}
#else
void __init kunlun_flash_init(void)
{
}
#endif /* CONFIG_MTD_NAND_OMAP2 || CONFIG_MTD_NAND_OMAP2_MODULE */
