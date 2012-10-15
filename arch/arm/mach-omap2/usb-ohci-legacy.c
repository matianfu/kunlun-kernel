/*
 * linux/arch/arm/mach-omap2/usb-ohci.c
 *
 * This file will contain the board specific details for the
 * Synopsys OHCI host controller on OMAP3430
 *
 * Copyright (C) 2009 Texas Instruments
 * Author: Vikram Pandita <vikram.pandita@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <asm/io.h>
#include "mux.h"

#include <plat/hardware.h>
#include <plat/irqs.h>
#include <plat/usb.h>

/* Figure out the correct base address for ohci controller */
#define OMAP34XX_HSUSB_HOST_BASE (L4_34XX_BASE + 0x64000)

/* OHCI platform specific data */
#if defined(CONFIG_USB_OHCI_HCD) || defined(CONFIG_USB_OHCI_HCD_MODULE)
static struct resource ohci_resources[] = {
	[0] = {
		.start   = OMAP34XX_HSUSB_HOST_BASE + 0x400,
		.end     = OMAP34XX_HSUSB_HOST_BASE + 0x400 + SZ_1K - 1,
		.flags   = IORESOURCE_MEM,
	},
	[1] = {         /* general IRQ */
		.start   = 76,
		.flags   = IORESOURCE_IRQ,
	}
};

/* The dmamask must be set for OHCI to work */
static u64 ohci_dmamask = ~(u32)0;

static void usb_release(struct device *dev)
{
	/* normally not freed */
}

static struct platform_device ohci_device = {
	.name           = "ohci-omap",
	.id             = 0,
	.dev = {
		.release		= usb_release,
		.dma_mask               = &ohci_dmamask,
		.coherent_dma_mask      = 0xffffffff,
		.platform_data          = NULL,
	},
	.num_resources  = ARRAY_SIZE(ohci_resources),
	.resource       = ohci_resources,
};
#endif /* OHCI specific data */

/* MUX settings for OHCI pins */
/*
 * setup_ehci_io_mux - initialize IO pad mux for USBHOST
 */
#if defined(CONFIG_USB_OHCI_HCD) || defined(CONFIG_USB_OHCI_HCD_MODULE)
static void setup_ohci_io_mux(void)
{

/* Configure pin-muxing on a per-port basis
 * We can configure a port for either ULPI PHY mode (EHCI), or for
 * ULPI TLL mode (EHCI), or for serial PHY mode (OHCI)
 * TODO: Add pin-mux settings for PORT 2 serial PHY mode
 * 	 and for OHCI TLL mode
 */

/* PHY mode for board: 750-2083-001
 * On this board, ISP1301 is connected to Port 1
 * Another ISP1301 transceiver is connected to Port 3
 */

#if defined(CONFIG_OMAP_OHCI_PHY_MODE_3PIN_PORT1) || \
    defined(CONFIG_OMAP_OHCI_PHY_MODE_4PIN_PORT1)

	omap_cfg_reg(AF10_3430_USB1FS_PHY_MM1_RXDP);
	omap_cfg_reg(AG9_3430_USB1FS_PHY_MM1_RXDM);
	omap_cfg_reg(W13_3430_USB1FS_PHY_MM1_RXRCV);
	omap_cfg_reg(W12_3430_USB1FS_PHY_MM1_TXSE0);
	omap_cfg_reg(W11_3430_USB1FS_PHY_MM1_TXDAT);
	omap_cfg_reg(Y11_3430_USB1FS_PHY_MM1_TXEN_N);

#endif /* Port 1 */

/* PHY mode for board: 750-2099-001(C)
 * On this board, there is only one ISP1301 connected to Port3
 */

#if defined(CONFIG_OMAP_OHCI_PHY_MODE_3PIN_PORT3) || \
    defined(CONFIG_OMAP_OHCI_PHY_MODE_4PIN_PORT3)

	omap_cfg_reg(AH3_3430_USB3FS_PHY_MM3_RXDP);
	omap_cfg_reg(AE3_3430_USB3FS_PHY_MM3_RXDM);
	omap_cfg_reg(AD1_3430_USB3FS_PHY_MM3_RXRCV);
	omap_cfg_reg(AE1_3430_USB3FS_PHY_MM3_TXSE0);
	omap_cfg_reg(AD2_3430_USB3FS_PHY_MM3_TXDAT);
	omap_cfg_reg(AC1_3430_USB3FS_PHY_MM3_TXEN_N);

#endif	/* Port3: */

	return;
}
#endif /* CONFIG_USB_OHCI_HCD */


void __init usb_ohci_init(void)
{
#if  defined(CONFIG_BOOTCASE_VIATELECOM) && defined(CONFIG_TWL4030_BCI_CHARGE_TYPE)
       extern const char * get_bootcase(void);
       /*disable USB when boot by charger*/
       if(!strncmp(get_bootcase(), "charger", strlen("charger"))){
              return ;
       }
#endif
#if defined(CONFIG_USB_OHCI_HCD) || defined(CONFIG_USB_OHCI_HCD_MODULE)

	/* Setup Pin IO MUX for OHCI */
	if (cpu_is_omap34xx())
		setup_ohci_io_mux();

	if (platform_device_register(&ohci_device) < 0) {
		printk(KERN_ERR "Unable to register FS-USB (OHCI) device\n");
		return;
	}
#endif
}
