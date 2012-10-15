/*
 * OHCI HCD (Host Controller Driver) for USB.
 *
 * (C) Copyright 1999 Roman Weissgaerber <weissg@vienna.at>
 * (C) Copyright 2000-2005 David Brownell
 * (C) Copyright 2002 Hewlett-Packard Company
 * (C) Copyright 2007-2009 Texas Instruments
 * (C) Copyright 2007 Vikram Pandita <vikram.pandita@ti.com>
 * (C) Copyright 2008 Romit Dasgupta <romit@ti.com>
 * (C) Copyright 2008-2009 Anand Gadiyar <gadiyar@ti.com>
 * (C) Copyright 2011 Karfield Chen <kfchen@via-telecom.com>
 *
 * OMAP Bus Glue
 *
 * Modified for OMAP by Tony Lindgren <tony@atomide.com>
 * Based on the 2.4 OMAP OHCI driver originally done by MontaVista Software Inc.
 * and on ohci-sa1111.c by Christopher Hoover <ch@hpl.hp.com>
 *
 * This file is licenced under the GPL.
 */

#include <linux/signal.h>	/* IRQF_DISABLED */
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/spinlock.h>

#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/mach-types.h>

#include <plat/irqs.h>
#include <plat/usb.h>

#if defined(CONFIG_MACH_OMAP_KUNLUN) || \
    defined(CONFIG_MACH_OMAP_KL9C) || \
    defined(CONFIG_MACH_OMAP_WUDANG) || \
    defined(CONFIG_MACH_OMAP_JT) || \
    defined(CONFIG_MACH_OMAP_KUNLUN_P2) || \
    defined(CONFIG_MACH_OMAP_KUNLUN_N710E)
#include <plat/mux.h>
#include <mach/board-kunlun.h>
#endif

/* configurations used in legacy code, we defines those here */
#define CONFIG_OMAP_OHCI_PHY_MODE
#define CONFIG_OMAP_OHCI_TLL_MODE_2PIN_DAT_SE0_PORT2

/* Merge the code from former 'ehci-omap.h' */
#include <mach/hardware.h>
#include "../../../arch/arm/mach-omap2/cm-regbits-34xx.h"

/*
 * OMAP USBHOST Register addresses: PHYSICAL ADDRESSES
 * 	Use omap_readl()/omap_writel() functions
 */

/* USBHOST: TLL, UUH, OHCI, EHCI */
#define	OMAP_USBHOST_BASE	(L4_34XX_BASE + 0x60000) //? sure?

/* TLL Register Set */
#define	OMAP_USBHOST_TLL_BASE	(OMAP_USBHOST_BASE + 0x2000)
#define	OMAP_USBTLL_REVISION	(OMAP_USBHOST_TLL_BASE + 0x00)
#define	OMAP_USBTLL_SYSCONFIG	(OMAP_USBHOST_TLL_BASE + 0x10)
	#define	OMAP_USBTLL_SYSCONFIG_CACTIVITY_SHIFT	8
	#define	OMAP_USBTLL_SYSCONFIG_SIDLEMODE_SHIFT	3
	#define	OMAP_USBTLL_SYSCONFIG_ENAWAKEUP_SHIFT	2
	#define	OMAP_USBTLL_SYSCONFIG_SOFTRESET_SHIFT	1
	#define	OMAP_USBTLL_SYSCONFIG_AUTOIDLE_SHIFT	0
#define	OMAP_USBTLL_SYSSTATUS	(OMAP_USBHOST_TLL_BASE + 0x14)
	#define	OMAP_USBTLL_SYSSTATUS_RESETDONE_SHIFT	0
#define	OMAP_USBTLL_IRQSTATUS	(OMAP_USBHOST_TLL_BASE + 0x18)
#define	OMAP_USBTLL_IRQENABLE	(OMAP_USBHOST_TLL_BASE + 0x1C)

#define	OMAP_TLL_SHARED_CONF	(OMAP_USBHOST_TLL_BASE + 0x30)
	#define	OMAP_TLL_SHARED_CONF_USB_90D_DDR_EN_SHFT	6
	#define	OMAP_TLL_SHARED_CONF_USB_180D_SDR_EN_SHIFT	5
	#define	OMAP_TLL_SHARED_CONF_USB_DIVRATION_SHIFT	2
	#define	OMAP_TLL_SHARED_CONF_FCLK_REQ_SHIFT		1
	#define	OMAP_TLL_SHARED_CONF_FCLK_IS_ON_SHIFT		0

#define	OMAP_TLL_CHANNEL_CONF(num)\
			(OMAP_USBHOST_TLL_BASE + (0x040 + 0x004 * num))
	#define	OMAP_TLL_CHANNEL_CONF_FSLSMODE(x)		(((x)&0xf)<<24)
	#define	OMAP_TLL_CHANNEL_CONF_FSLSMODE_SHIFT		24
	#define	OMAP_TLL_CHANNEL_CONF_ULPINOBITSTUFF_SHIFT	11
	#define	OMAP_TLL_CHANNEL_CONF_ULPI_ULPIAUTOIDLE_SHIFT	10
	#define	OMAP_TLL_CHANNEL_CONF_UTMIAUTOIDLE_SHIFT	9
	#define	OMAP_TLL_CHANNEL_CONF_ULPIDDRMODE_SHIFT		8
	#define	OMAP_TLL_CHANNEL_CONF_CHANMODE_SHIFT		1
	#define	OMAP_TLL_CHANNEL_CONF_CHANEN_SHIFT		0

#define	OMAP_TLL_ULPI_VENDOR_ID_LO(num)\
			(OMAP_USBHOST_TLL_BASE + (0x800 + 0x100 * num))

#define	OMAP_TLL_ULPI_VENDOR_ID_HI(num)\
			(OMAP_USBHOST_TLL_BASE + (0x801 + 0x100 * num))

#define	OMAP_TLL_ULPI_PRODUCT_ID_LO(num)\
			(OMAP_USBHOST_TLL_BASE + (0x802 + 0x100 * num))

#define	OMAP_TLL_ULPI_PRODUCT_ID_hi(num)\
			(OMAP_USBHOST_TLL_BASE + (0x803 + 0x100 * num))

#define	OMAP_TLL_ULPI_FUNCTION_CTRL(num)\
			(OMAP_USBHOST_TLL_BASE + (0x804 + 0x100 * num))
#define	OMAP_TLL_ULPI_INTERFACE_CTRL(num)\
			(OMAP_USBHOST_TLL_BASE + (0x807 + 0x100 * num))
#define	OMAP_TLL_ULPI_OTG_CTRL(num)\
			(OMAP_USBHOST_TLL_BASE + (0x80A + 0x100 * num))
#define	OMAP_TLL_ULPI_INT_EN_RISE(num)\
			(OMAP_USBHOST_TLL_BASE + (0x80D + 0x100 * num))
#define	OMAP_TLL_ULPI_INT_EN_FALL(num)\
			(OMAP_USBHOST_TLL_BASE + (0x810 + 0x100 * num))
#define	OMAP_TLL_ULPI_INT_STATUS(num)\
			(OMAP_USBHOST_TLL_BASE + (0x813 + 0x100 * num))
#define	OMAP_TLL_ULPI_INT_LATCH(num)\
			(OMAP_USBHOST_TLL_BASE + (0x814 + 0x100 * num))
#define	OMAP_TLL_ULPI_DEBUG(num)\
			(OMAP_USBHOST_TLL_BASE + (0x815 + 0x100 * num))
#define	OMAP_TLL_ULPI_SCRATCH_REGISTER(num)\
			(OMAP_USBHOST_TLL_BASE + (0x816 + 0x100 * num))

#define OMAP_TLL_CHANNEL_COUNT		3
	#define OMAP_TLL_CHANNEL_1_EN_MASK	1
	#define OMAP_TLL_CHANNEL_2_EN_MASK	2
	#define OMAP_TLL_CHANNEL_3_EN_MASK	4

/* UHH Register Set */
#define	OMAP_USBHOST_UHH_BASE	(OMAP_USBHOST_BASE + 0x4000)
#define	OMAP_UHH_REVISION	(OMAP_USBHOST_UHH_BASE + 0x00)
#define	OMAP_UHH_SYSCONFIG	(OMAP_USBHOST_UHH_BASE + 0x10)
	#define	OMAP_UHH_SYSCONFIG_MIDLEMODE_SHIFT	12
	#define	OMAP_UHH_SYSCONFIG_CACTIVITY_SHIFT	8
	#define	OMAP_UHH_SYSCONFIG_SIDLEMODE_SHIFT	3
	#define	OMAP_UHH_SYSCONFIG_ENAWAKEUP_SHIFT	2
	#define	OMAP_UHH_SYSCONFIG_SOFTRESET_SHIFT	1
	#define	OMAP_UHH_SYSCONFIG_AUTOIDLE_SHIFT	0

#define	OMAP_UHH_SYSSTATUS	(OMAP_USBHOST_UHH_BASE + 0x14)
#define	OMAP_UHH_HOSTCONFIG	(OMAP_USBHOST_UHH_BASE + 0x40)
	#define	OMAP_UHH_HOSTCONFIG_P1_ULPI_BYPASS_SHIFT	0
	#define	OMAP_UHH_HOSTCONFIG_P2_ULPI_BYPASS_SHIFT	11
	#define	OMAP_UHH_HOSTCONFIG_P3_ULPI_BYPASS_SHIFT	12
	#define OMAP_UHH_HOSTCONFIG_INCR4_BURST_EN_SHIFT	2
	#define OMAP_UHH_HOSTCONFIG_INCR8_BURST_EN_SHIFT	3
	#define OMAP_UHH_HOSTCONFIG_INCR16_BURST_EN_SHIFT	4
	#define OMAP_UHH_HOSTCONFIG_INCRX_ALIGN_EN_SHIFT	5
	#define	OMAP_UHH_HOSTCONFIG_ULPI_BYPASS_MASK		\
	((1<<OMAP_UHH_HOSTCONFIG_P1_ULPI_BYPASS_SHIFT) | \
	 (1<<OMAP_UHH_HOSTCONFIG_P2_ULPI_BYPASS_SHIFT) | \
	 (1<<OMAP_UHH_HOSTCONFIG_P3_ULPI_BYPASS_SHIFT))

#define	OMAP_UHH_DEBUG_CSR	(OMAP_USBHOST_UHH_BASE + 0x44)

/* EHCI Register Set */
#define	OMAP_USBHOST_EHCI_BASE	(OMAP_USBHOST_BASE + 0x4800)
#define	EHCI_INSNREG04_ULPI		(OMAP_USBHOST_EHCI_BASE + 0xA0)
#define	EHCI_INSNREG05_ULPI		(OMAP_USBHOST_EHCI_BASE + 0xA4)
	#define	EHCI_INSNREG05_ULPI_CONTROL_SHIFT	31
	#define	EHCI_INSNREG05_ULPI_PORTSEL_SHIFT	24
	#define	EHCI_INSNREG05_ULPI_OPSEL_SHIFT		22
	#define	EHCI_INSNREG05_ULPI_REGADD_SHIFT	16
	#define	EHCI_INSNREG05_ULPI_EXTREGADD_SHIFT	8
	#define	EHCI_INSNREG05_ULPI_WRDATA_SHIFT	0

/* OHCI Register Set */
#define	OMAP_USBHOST_OHCI_BASE	(OMAP_USBHOST_BASE + 0x4400)

#ifndef CONFIG_ARCH_OMAP
#error "This file is OMAP bus glue.  CONFIG_OMAP must be defined."
#endif

#define OMAP_TLL_SYSCONFIG_SIDLEMODE_SHIFT	3

/* FSLSMODE defination, added by sguan, 2009-8-14 */
#define UNI_6PIN_PHY_MODE_DAT_SE0   (0x00)
#define UNI_6PIN_PHY_MODE_DP_DM     (0x01)
#define BI_3PIN_PHY_MODE            (0x02)
#define BI_4PIN_PHY_MODE            (0x03)
#define UNI_6PIN_TLL_MODE_DAT_SE0   (0x04)
#define UNI_6PIN_TLL_MODE_DP_DM     (0x05)
#define BI_3PIN_TLL_MODE            (0x06)
#define BI_4PIN_TLL_MODE            (0x07)
#define BI_2PIN_TLL_MODE_DAT_SE0    (0x0A)
#define BI_2PIN_TLL_MODE_DP_DM      (0x0B)

#define PORT_SETTING_DISABLE        (0xFF)

/* End of added*/
/* End of merging header file 'ehci-omap.h' */

spinlock_t     lock;

/* Define USBHOST clocks for clock management */
struct ohci_omap_clock_defs {
    struct clk	*usbhost_ick_clk;
    struct clk	*usbhost2_120m_fck_clk;
    struct clk	*usbhost1_48m_fck_clk;
    struct clk	*usbtll_fck_clk;
    struct clk	*usbtll_ick_clk;
    unsigned	suspended:1;
    /*
     * TODO:
     * host_enabled should be put in separate place.
     */
    unsigned	host_enabled:1;
};

#ifdef CONFIG_OMAP34XX_OFFMODE
static struct ohci_context_registers {
    u32	usbtll_sysconfig;
    u32	usbtll_irqenable;
    u32	tll_shared_conf;
    u32	tll_channel_conf[3];
    u8	ulpi_function_ctrl[3];
    u8	ulpi_interface_ctrl[3];
    u8	ulpi_otg_ctrl[3];
    u8	ulpi_usb_int_en_rise[3];
    u8	ulpi_usb_int_en_fall[3];
    u8	ulpi_usb_int_status[3];
    u8	ulpi_vendor_int_en[3];
    u8	ulpi_vendor_int_status[3];
} ohci_context;
#endif

/* Clock names as per clock framework: May change so keep as #defs */
#define USBHOST_ICLK            "usbhost_ick"
#define USBHOST_120M_FCLK       "usbhost_120m_fck"
#define USBHOST_48M_FCLK        "usbhost_48m_fck"
#define USBHOST_TLL_ICLK        "usbtll_ick"
#define USBHOST_TLL_FCLK        "usbtll_fck"

struct usb_hcd *g_usb_hcd = 0;
void ohci_resume_rh_autostop(void)
{
    struct usb_hcd *hcd = g_usb_hcd;
	struct ohci_hcd	*ohci = hcd_to_ohci (hcd);
    set_bit(HCD_FLAG_POLL_RH, &hcd->flags);
    if (ohci->autostop) {
        spin_lock (&ohci->lock);
        ohci_rh_resume (ohci);
        spin_unlock (&ohci->lock);
    } else
        usb_hcd_resume_root_hub(hcd);
}
EXPORT_SYMBOL(ohci_resume_rh_autostop);


/*-------------------------------------------------------------------------*/

static int ohci_omap_init(struct usb_hcd *hcd)
{
    struct ohci_hcd		*ohci = hcd_to_ohci(hcd);
    int			ret;

    dev_dbg(hcd->self.controller, "starting USB Controller\n");

    if ((ret = ohci_init(ohci)) < 0)
        return ret;

    /* board init will have already handled HMC and mux setup.
     * any external transceiver should already be initialized
     * too, so all configured ports use the right signaling now.
     */

    return 0;
}

static void ohci_omap_stop(struct usb_hcd *hcd)
{
    dev_dbg(hcd->self.controller, "stopping USB Controller\n");
}


/*-------------------------------------------------------------------------*/
/**
 * usb_hcd_omap_probe - initialize OMAP-based HCDs
 * Context: !in_interrupt()
 *
 * Allocates basic resources for this USB host controller, and
 * then invokes the start() method for the HCD associated with it
 * through the hotplug entry's driver_data.
 */
static int usb_hcd_omap_probe(const struct hc_driver *driver,
        struct platform_device *pdev)
{
    int retval;
    int i;
    unsigned long flags;
    u32 uhh_hostconfig_value;
    unsigned char ohci_port_mode_setting[3] =
    {
        PORT_SETTING_DISABLE,
        PORT_SETTING_DISABLE,
        PORT_SETTING_DISABLE
    };

    struct usb_hcd *hcd = 0;
    struct ohci_hcd *ohci;
    struct ohci_omap_clock_defs *ohci_clocks;

    if (pdev->num_resources != 2) {
        printk(KERN_ERR "hcd probe: invalid num_resources: %i\n",
                pdev->num_resources);
        return -ENODEV;
    }

    if (pdev->resource[0].flags != IORESOURCE_MEM
            || pdev->resource[1].flags != IORESOURCE_IRQ) {
        printk(KERN_ERR "hcd probe: invalid resource type\n");
        return -ENODEV;
    }
    spin_lock_init(&lock);
    hcd = usb_create_hcd(driver, &pdev->dev, dev_name(&pdev->dev));
    if (!hcd) {
        retval = -ENOMEM;
        goto err0;
    }

    g_usb_hcd = hcd;

    /* Phy mode setting */
#if defined(CONFIG_OMAP_OHCI_PHY_MODE_3PIN_PORT1)
    ohci_port_mode_setting[0] = BI_3PIN_PHY_MODE;
#elif defined(CONFIG_OMAP_OHCI_TLL_MODE_2PIN_DP_DM_PORT1)
    ohci_port_mode_setting[0] = BI_2PIN_TLL_MODE_DP_DM;
#elif defined(CONFIG_OMAP_OHCI_PHY_MODE_6PIN_DP_DM_PORT1)
    ohci_port_mode_setting[0] = UNI_6PIN_PHY_MODE_DP_DM;
#elif defined(CONFIG_OMAP_OHCI_PHY_MODE_6PIN_DAT_SE0_PORT1)
    ohci_port_mode_setting[0] = UNI_6PIN_PHY_MODE_DAT_SE0;
#elif defined(CONFIG_OMAP_OHCI_PHY_MODE_4PIN_PORT1)
    ohci_port_mode_setting[0] = BI_4PIN_PHY_MODE;
#else
    ohci_port_mode_setting[0] = PORT_SETTING_DISABLE;
#endif

#if defined(CONFIG_OMAP_OHCI_PHY_MODE_3PIN_PORT2)
    ohci_port_mode_setting[1] = BI_3PIN_PHY_MODE;
#elif defined(CONFIG_OMAP_OHCI_TLL_MODE_2PIN_DP_DM_PORT2)
    ohci_port_mode_setting[1] = BI_2PIN_TLL_MODE_DP_DM;
#elif defined(CONFIG_OMAP_OHCI_TLL_MODE_2PIN_DAT_SE0_PORT2)
    ohci_port_mode_setting[1] = BI_2PIN_TLL_MODE_DAT_SE0;
#elif defined(CONFIG_OMAP_OHCI_PHY_MODE_6PIN_DP_DM_PORT2)
    ohci_port_mode_setting[1] = UNI_6PIN_PHY_MODE_DP_DM;
#elif defined(CONFIG_OMAP_OHCI_PHY_MODE_6PIN_DAT_SE0_PORT2)
    ohci_port_mode_setting[1] = UNI_6PIN_PHY_MODE_DAT_SE0;
#elif defined(CONFIG_OMAP_OHCI_PHY_MODE_4PIN_PORT2)
    ohci_port_mode_setting[1] = BI_4PIN_PHY_MODE;
#else
    ohci_port_mode_setting[1] = PORT_SETTING_DISABLE;
#endif

#if defined(CONFIG_OMAP_OHCI_PHY_MODE_3PIN_PORT3)
    ohci_port_mode_setting[2] = BI_3PIN_PHY_MODE;
#elif defined(CONFIG_OMAP_OHCI_TLL_MODE_2PIN_DP_DM_PORT3)
    ohci_port_mode_setting[2] = BI_2PIN_TLL_MODE_DP_DM;
#elif defined(CONFIG_OMAP_OHCI_PHY_MODE_6PIN_DP_DM_PORT3)
    ohci_port_mode_setting[2] = UNI_6PIN_PHY_MODE_DP_DM;
#elif defined(CONFIG_OMAP_OHCI_PHY_MODE_6PIN_DAT_SE0_PORT3)
    ohci_port_mode_setting[2] = UNI_6PIN_PHY_MODE_DAT_SE0;
#elif defined(CONFIG_OMAP_OHCI_PHY_MODE_4PIN_PORT3)
    ohci_port_mode_setting[2] = BI_4PIN_PHY_MODE;
#else
    ohci_port_mode_setting[2] = PORT_SETTING_DISABLE;
#endif


    ohci_clocks = (struct ohci_omap_clock_defs *)
        (((char *)hcd_to_ohci(hcd)) + sizeof(struct ohci_hcd));

    /* Enable Clocks for USBHOST */
    ohci_clocks->usbhost_ick_clk = clk_get(&pdev->dev,
            USBHOST_ICLK);
    if (IS_ERR(ohci_clocks->usbhost_ick_clk))
        return PTR_ERR(ohci_clocks->usbhost_ick_clk);
    clk_enable(ohci_clocks->usbhost_ick_clk);

    ohci_clocks->usbhost2_120m_fck_clk = clk_get(&pdev->dev,
            USBHOST_120M_FCLK);
    if (IS_ERR(ohci_clocks->usbhost2_120m_fck_clk)) {
        clk_disable(ohci_clocks->usbhost_ick_clk);
        clk_put(ohci_clocks->usbhost_ick_clk);
        return PTR_ERR(ohci_clocks->usbhost2_120m_fck_clk);
    }
    clk_enable(ohci_clocks->usbhost2_120m_fck_clk);

    ohci_clocks->usbhost1_48m_fck_clk = clk_get(&pdev->dev,
            USBHOST_48M_FCLK);
    if (IS_ERR(ohci_clocks->usbhost1_48m_fck_clk)) {
        clk_disable(ohci_clocks->usbhost_ick_clk);
        clk_put(ohci_clocks->usbhost_ick_clk);
        clk_disable(ohci_clocks->usbhost2_120m_fck_clk);
        clk_put(ohci_clocks->usbhost2_120m_fck_clk);
        return PTR_ERR(ohci_clocks->usbhost1_48m_fck_clk);
    }
    clk_enable(ohci_clocks->usbhost1_48m_fck_clk);

    /* Configure TLL for 60Mhz clk for ULPI */
    ohci_clocks->usbtll_fck_clk = clk_get(&pdev->dev,
            USBHOST_TLL_FCLK);
    if (IS_ERR(ohci_clocks->usbtll_fck_clk)) {
        clk_disable(ohci_clocks->usbhost_ick_clk);
        clk_put(ohci_clocks->usbhost_ick_clk);
        clk_disable(ohci_clocks->usbhost2_120m_fck_clk);
        clk_put(ohci_clocks->usbhost2_120m_fck_clk);
        clk_disable(ohci_clocks->usbhost1_48m_fck_clk);
        clk_put(ohci_clocks->usbhost1_48m_fck_clk);
        return PTR_ERR(ohci_clocks->usbtll_fck_clk);
    }
    clk_enable(ohci_clocks->usbtll_fck_clk);

    ohci_clocks->usbtll_ick_clk = clk_get(&pdev->dev,
            USBHOST_TLL_ICLK);
    if (IS_ERR(ohci_clocks->usbtll_ick_clk)) {
        clk_disable(ohci_clocks->usbhost_ick_clk);
        clk_put(ohci_clocks->usbhost_ick_clk);
        clk_disable(ohci_clocks->usbhost2_120m_fck_clk);
        clk_put(ohci_clocks->usbhost2_120m_fck_clk);
        clk_disable(ohci_clocks->usbhost1_48m_fck_clk);
        clk_put(ohci_clocks->usbhost1_48m_fck_clk);
        clk_disable(ohci_clocks->usbtll_fck_clk);
        clk_put(ohci_clocks->usbtll_fck_clk);
        return PTR_ERR(ohci_clocks->usbtll_ick_clk);
    }
    spin_lock_irqsave(&lock, flags);
    clk_enable(ohci_clocks->usbtll_ick_clk);

    ohci_clocks->suspended = 0;

    /* Disable Auto Idle of USBTLL */
    cm_write_mod_reg((0 << OMAP3430ES2_AUTO_USBTLL_SHIFT),
            CORE_MOD, CM_AUTOIDLE3);

    /* Wait for TLL to be Active */
    while ((cm_read_mod_reg(CORE_MOD, OMAP2430_CM_IDLEST3) &
                (1 << OMAP3430ES2_ST_USBTLL_SHIFT)));

    /* perform TLL soft reset, and wait until reset is complete */
    omap_writel(1 << OMAP_USBTLL_SYSCONFIG_SOFTRESET_SHIFT,
            OMAP_USBTLL_SYSCONFIG);

    omap_readl(OMAP_USBTLL_SYSCONFIG);
    /* Wait for TLL reset to complete */
    while (!(omap_readl(OMAP_USBTLL_SYSSTATUS) &
                (1 << OMAP_USBTLL_SYSSTATUS_RESETDONE_SHIFT)));

    spin_unlock_irqrestore(&lock, flags);

    /* smart idle mode */
    omap_writel((1 << OMAP_USBTLL_SYSCONFIG_ENAWAKEUP_SHIFT) |
            (2 << OMAP_USBTLL_SYSCONFIG_SIDLEMODE_SHIFT) |
            (0 << OMAP_USBTLL_SYSCONFIG_CACTIVITY_SHIFT) |
            (0 << OMAP_USBTLL_SYSCONFIG_AUTOIDLE_SHIFT),
            OMAP_USBTLL_SYSCONFIG);

    /* Put UHH in SmartIdle/SmartStandby mode */
    omap_writel((1 << OMAP_UHH_SYSCONFIG_AUTOIDLE_SHIFT) |
            (1 << OMAP_UHH_SYSCONFIG_ENAWAKEUP_SHIFT) |
            (2 << OMAP_UHH_SYSCONFIG_SIDLEMODE_SHIFT) |
            (0 << OMAP_UHH_SYSCONFIG_CACTIVITY_SHIFT) |
            (2 << OMAP_UHH_SYSCONFIG_MIDLEMODE_SHIFT),
            OMAP_UHH_SYSCONFIG);

#ifdef CONFIG_OMAP_OHCI_PHY_MODE
    /* TLL in FS-PHY mode operation */
    uhh_hostconfig_value = (1 << OMAP_UHH_HOSTCONFIG_INCR4_BURST_EN_SHIFT) |
        (1 << OMAP_UHH_HOSTCONFIG_INCR8_BURST_EN_SHIFT) |
        (1 << OMAP_UHH_HOSTCONFIG_INCR16_BURST_EN_SHIFT) |
        (0 << OMAP_UHH_HOSTCONFIG_INCRX_ALIGN_EN_SHIFT);

    if (omap_rev() >= OMAP3430_REV_ES3_0){

        /* For ES 3, we have per-port control for the ULPI Bypass
         * The ULPI Bypass needs to be set to 0 only if the EHCI PHY Mode
         * is selected for that port.
         * Hence it is easier to make it conditional on EHCI_PHY_MODE
         *
         * ES 2 does not have per-port control. Hence it is not possible to have
         * EHCI in PHY Mode and OHCI both working at the same time
         *
         * FIXME: This common code should be moved elsewhere
         *
         */

#ifndef CONFIG_OMAP_EHCI_PHY_MODE_PORT1
        uhh_hostconfig_value |=
            (1 << OMAP_UHH_HOSTCONFIG_P1_ULPI_BYPASS_SHIFT);
#endif

#ifndef CONFIG_OMAP_EHCI_PHY_MODE_PORT2
        uhh_hostconfig_value |=
            (1 << OMAP_UHH_HOSTCONFIG_P2_ULPI_BYPASS_SHIFT);
#endif

#ifndef CONFIG_OMAP_EHCI_PHY_MODE_PORT3
        uhh_hostconfig_value |=
            (1 << OMAP_UHH_HOSTCONFIG_P3_ULPI_BYPASS_SHIFT);
#endif
    } else {
        uhh_hostconfig_value |=
            (1 << OMAP_UHH_HOSTCONFIG_P1_ULPI_BYPASS_SHIFT);
    }

    omap_writel(uhh_hostconfig_value, OMAP_UHH_HOSTCONFIG);

#if 0
    /* Ensure BYPASS bit is not set */
    while (!(omap_readl(OMAP_UHH_HOSTCONFIG) &
                (1 << OMAP_UHH_HOSTCONFIG_P3_ULPI_BYPASS_SHIFT)));
#endif

    pr_debug("Entered UTMI PHY MODE: success");

    /* Program Common TLL register */
    omap_writel((1 << OMAP_TLL_SHARED_CONF_FCLK_IS_ON_SHIFT) |
            (1 << OMAP_TLL_SHARED_CONF_USB_DIVRATION_SHIFT) |
            (0 << OMAP_TLL_SHARED_CONF_USB_180D_SDR_EN_SHIFT) |
            (0 << OMAP_TLL_SHARED_CONF_USB_90D_DDR_EN_SHFT),
            OMAP_TLL_SHARED_CONF);

    /* Program the 3 TLL channels upfront */
    for (i = 0; i < OMAP_TLL_CHANNEL_COUNT; i++) {

        /* Enable only required ports */
        if(PORT_SETTING_DISABLE == ohci_port_mode_setting[i])
        {
            printk("ohci port %d not set: continue\n", i);
            continue;
        }

        /* Disable AutoIdle */
        omap_writel(omap_readl(OMAP_TLL_CHANNEL_CONF(i)) &
                ~(1 << OMAP_TLL_CHANNEL_CONF_UTMIAUTOIDLE_SHIFT),
                OMAP_TLL_CHANNEL_CONF(i));

        /* Disable BitStuffing */
        omap_writel(omap_readl(OMAP_TLL_CHANNEL_CONF(i)) |
                (1 << OMAP_TLL_CHANNEL_CONF_ULPINOBITSTUFF_SHIFT),
                OMAP_TLL_CHANNEL_CONF(i));

        /* SDR Mode */
        omap_writel(omap_readl(OMAP_TLL_CHANNEL_CONF(i)) &
                ~(1 << OMAP_TLL_CHANNEL_CONF_ULPIDDRMODE_SHIFT),
                OMAP_TLL_CHANNEL_CONF(i));

        /* CHANMODE: UTMI-to-serial FS/LS mode */
        omap_writel(omap_readl(OMAP_TLL_CHANNEL_CONF(i)) |
                (1 << OMAP_TLL_CHANNEL_CONF_CHANMODE_SHIFT),
                OMAP_TLL_CHANNEL_CONF(i));

        /* FSLSMODE: According to the ohci_port_mode_setting[] table */
        omap_writel(omap_readl(OMAP_TLL_CHANNEL_CONF(i)) |
                ohci_port_mode_setting[i] << OMAP_TLL_CHANNEL_CONF_FSLSMODE_SHIFT
                , OMAP_TLL_CHANNEL_CONF(i));

        /* Enable the channel */
        omap_writel(omap_readl(OMAP_TLL_CHANNEL_CONF(i)) |
                (1<<OMAP_TLL_CHANNEL_CONF_CHANEN_SHIFT),
                OMAP_TLL_CHANNEL_CONF(i));
    }
#else
    pr_debug("\nOHCI PHY Mode not  3-pin PHY mode is implemented");
#endif /* CONFIG_OMAP_OHCI_PHY_MODE */


    hcd->rsrc_start = pdev->resource[0].start;
    hcd->rsrc_len = pdev->resource[0].end - pdev->resource[0].start + 1;

    /*
       if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
       dev_dbg(&pdev->dev, "request_mem_region failed\n");
       retval = -EBUSY;
       goto err1;
       }
       */

    hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
    if (!hcd->regs) {
        dev_err(&pdev->dev, "can't ioremap OHCI HCD\n");
        retval = -ENOMEM;
        goto err2;
    }

    /*
       pr_debug("\n\n-->VIRT-OHCI-BASE [0x%x], [0x%x] irq[%d]\n\n",
       hcd->regs, (unsigned int)io_p2v( 0x48064400 ),
       pdev->resource[1].start);
       */

    ohci = hcd_to_ohci(hcd);
    ohci_hcd_init(ohci);

    ohci_clocks->host_enabled = 1;

    //irq = platform_get_irq(pdev, 0);
    //if (irq < 0) {
    //	retval = -ENXIO;
    //	goto err3;
    //}
    retval = usb_add_hcd(hcd, pdev->resource[1].start, IRQF_DISABLED);
    if (retval)
        goto err3;

    return 0;
err3:
    iounmap(hcd->regs);
err2:
    //	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
    //err1:
    usb_put_hcd(hcd);
    clk_disable(ohci_clocks->usbhost_ick_clk);
    clk_put(ohci_clocks->usbhost_ick_clk);
    clk_disable(ohci_clocks->usbhost2_120m_fck_clk);
    clk_put(ohci_clocks->usbhost2_120m_fck_clk);
    clk_disable(ohci_clocks->usbhost1_48m_fck_clk);
    clk_put(ohci_clocks->usbhost1_48m_fck_clk);
    clk_disable(ohci_clocks->usbtll_fck_clk);
    clk_put(ohci_clocks->usbtll_fck_clk);
    clk_disable(ohci_clocks->usbtll_ick_clk);
    clk_put(ohci_clocks->usbtll_ick_clk);
err0:
    //	clk_put(usb_dc_ck);
    //	clk_put(usb_host_ck);
    return retval;
}

/* may be called with controller, bus, and devices active */

/**
 * usb_hcd_omap_remove - shutdown processing for OMAP-based HCDs
 * @dev: USB Host Controller being removed
 * Context: !in_interrupt()
 *
 * Reverses the effect of usb_hcd_omap_probe(), first invoking
 * the HCD's stop() method.  It is always called from a thread
 * context, normally "rmmod", "apmd", or something similar.
 */
    static inline void
usb_hcd_omap_remove(struct usb_hcd *hcd, struct platform_device *pdev)
{
    struct ohci_omap_clock_defs *ohci_clocks;
    struct ohci_hcd		*ohci = hcd_to_ohci (hcd);
    unsigned long flags;

    ohci_clocks = (struct ohci_omap_clock_defs *)
        (((char *)hcd_to_ohci(hcd)) + sizeof(struct ohci_hcd));

    usb_remove_hcd(hcd);
    if (ohci->transceiver) {
        (void) otg_set_host(ohci->transceiver, 0);
        put_device(ohci->transceiver->dev);
    }
    iounmap(hcd->regs);
    //release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
    spin_lock_irqsave(&lock, flags);

    /* Reset OMAP modules for insmod/rmmod to work */
    omap_writel((1 << 1), OMAP_UHH_SYSCONFIG);
    while (!(omap_readl(OMAP_UHH_SYSSTATUS) & (1 << 0)));
    while (!(omap_readl(OMAP_UHH_SYSSTATUS) & (1 << 1)));
    while (!(omap_readl(OMAP_UHH_SYSSTATUS) & (1 << 2)));
    pr_debug("UHH RESET DONE OMAP_UHH_SYSSTATUS %x !!\n",
            omap_readl(OMAP_UHH_SYSSTATUS));

    omap_writel((1<<1), OMAP_USBTLL_SYSCONFIG);
    while (!(omap_readl(OMAP_USBTLL_SYSSTATUS) & (1<<0)));
    pr_debug("TLL RESET DONE");

    spin_unlock_irqrestore(&lock, flags);

    if (ohci_clocks->usbtll_fck_clk != NULL) {
        clk_disable(ohci_clocks->usbtll_fck_clk);
        clk_put(ohci_clocks->usbtll_fck_clk);
        ohci_clocks->usbtll_fck_clk = NULL;
    }

    if (ohci_clocks->usbhost_ick_clk != NULL) {
        clk_disable(ohci_clocks->usbhost_ick_clk);
        clk_put(ohci_clocks->usbhost_ick_clk);
        ohci_clocks->usbhost_ick_clk = NULL;
    }

    if (ohci_clocks->usbhost1_48m_fck_clk != NULL) {
        clk_disable(ohci_clocks->usbhost1_48m_fck_clk);
        clk_put(ohci_clocks->usbhost1_48m_fck_clk);
        ohci_clocks->usbhost1_48m_fck_clk = NULL;
    }

    if (ohci_clocks->usbhost2_120m_fck_clk != NULL) {
        clk_disable(ohci_clocks->usbhost2_120m_fck_clk);
        clk_put(ohci_clocks->usbhost2_120m_fck_clk);
        ohci_clocks->usbhost2_120m_fck_clk = NULL;
    }

    if (ohci_clocks->usbtll_ick_clk != NULL) {
        clk_disable(ohci_clocks->usbtll_ick_clk);
        clk_put(ohci_clocks->usbtll_ick_clk);
        ohci_clocks->usbtll_ick_clk = NULL;
    }
    usb_put_hcd(hcd);
}
/*-------------------------------------------------------------------------*/
static void ohci_context_restore(void);

static irqreturn_t ohci_omap_irq(struct usb_hcd *hcd)
{
	u32 usbtll_sysconfig, uhh_sysconfig;
	struct ohci_omap_clock_defs *ohci_clocks;

	ohci_clocks = (struct ohci_omap_clock_defs *)
			(((char *)hcd_to_ohci(hcd)) + sizeof(struct ohci_hcd));

	if (unlikely(ohci_clocks->suspended)) {
		printk(KERN_DEBUG "%s: OHCI IRQ triggered with clocks disabled\n", __func__);
		clk_enable(ohci_clocks->usbtll_fck_clk);
		ohci_context_restore();

		clk_enable(ohci_clocks->usbhost_ick_clk);
		clk_enable(ohci_clocks->usbhost2_120m_fck_clk);
		clk_enable(ohci_clocks->usbhost1_48m_fck_clk);
		ohci_clocks->suspended = 0;

		/* Need to set back to NoStandby,Noidle
		 * FIXME: Maybe SmartIdle, SmartStandby will also work
		 */

		usbtll_sysconfig = omap_readl(OMAP_USBTLL_SYSCONFIG);
		usbtll_sysconfig &= ~(3 << OMAP_TLL_SYSCONFIG_SIDLEMODE_SHIFT);
		usbtll_sysconfig |= (1 << OMAP_TLL_SYSCONFIG_SIDLEMODE_SHIFT);
		omap_writel(usbtll_sysconfig, OMAP_USBTLL_SYSCONFIG);

		uhh_sysconfig = omap_readl(OMAP_UHH_SYSCONFIG);
		uhh_sysconfig &= ~(3 << OMAP_UHH_SYSCONFIG_MIDLEMODE_SHIFT);
		uhh_sysconfig &= ~(3 << OMAP_UHH_SYSCONFIG_SIDLEMODE_SHIFT);
		uhh_sysconfig |= (1 << OMAP_UHH_SYSCONFIG_MIDLEMODE_SHIFT);
		uhh_sysconfig |= (1 << OMAP_UHH_SYSCONFIG_SIDLEMODE_SHIFT);
		omap_writel(uhh_sysconfig, OMAP_UHH_SYSCONFIG);
	}
	return ohci_irq(hcd);
}

    static int
ohci_omap_start(struct usb_hcd *hcd)
{
    struct omap_usb_config *config;
    struct ohci_omap_clock_defs *ohci_clocks;
    int		ret;
    struct ohci_hcd	*ohci = hcd_to_ohci(hcd);

    ohci_clocks = (struct ohci_omap_clock_defs *)
        (((char *)hcd_to_ohci(hcd)) + sizeof(struct ohci_hcd));

    if (!ohci_clocks->host_enabled)
        return 0;
    config = hcd->self.controller->platform_data;
#ifdef HACK_OMAP_OHCI /* todo */
    if (config->otg || config->rwc) {
        ohci->hc_control = OHCI_CTRL_RWC;
        writel(OHCI_CTRL_RWC, &ohci->regs->control);
    }
#endif

    ohci->hc_control = OHCI_CTRL_RWC;
    writel(OHCI_CTRL_RWC, &ohci->regs->control);
    if ((ret = ohci_run(ohci)) < 0) {
        dev_err(hcd->self.controller, "can't start\n");
        ohci_stop(hcd);
        return ret;
    }
    return 0;
}

#ifdef CONFIG_PM

#ifdef CONFIG_OMAP34XX_OFFMODE
static void ohci_context_save(void)
{
    int i;

    if (is_sil_rev_less_than(OMAP3430_REV_ES3_1)) {
        ohci_context.usbtll_sysconfig =
            omap_readl(OMAP_USBTLL_SYSCONFIG);
        ohci_context.usbtll_irqenable =
            omap_readl(OMAP_USBTLL_IRQENABLE);
        ohci_context.tll_shared_conf =
            omap_readl(OMAP_TLL_SHARED_CONF);

        for (i = 0; i < 3; i++) {
            ohci_context.tll_channel_conf[i] =
                omap_readl(OMAP_TLL_CHANNEL_CONF(i));
            ohci_context.ulpi_function_ctrl[i] =
                omap_readb(OMAP_TLL_ULPI_FUNCTION_CTRL(i));
            ohci_context.ulpi_interface_ctrl[i] =
                omap_readb(OMAP_TLL_ULPI_INTERFACE_CTRL(i));
            ohci_context.ulpi_otg_ctrl[i] =
                omap_readb(OMAP_TLL_ULPI_OTG_CTRL(i));
            ohci_context.ulpi_usb_int_en_rise[i] =
                omap_readb(OMAP_TLL_ULPI_INT_EN_RISE(i));
            ohci_context.ulpi_usb_int_en_fall[i] =
                omap_readb(OMAP_TLL_ULPI_INT_EN_FALL(i));
            ohci_context.ulpi_usb_int_status[i] =
                omap_readb(OMAP_TLL_ULPI_INT_STATUS(i));
        }
    }
}

static void ohci_context_restore(void)
{
    int i;

    if (is_sil_rev_less_than(OMAP3430_REV_ES3_1)) {

#if 0
        /* FIXME: This was reported as not working. Temporarily
         * disable this infinite loop pending investigation
         */

        /* perform TLL soft reset, and wait until reset is complete */
        omap_writel(1 << OMAP_USBTLL_SYSCONFIG_SOFTRESET_SHIFT,
                OMAP_USBTLL_SYSCONFIG);

        /* Wait for TLL reset to complete */
        while (!(omap_readl(OMAP_USBTLL_SYSSTATUS) &
                    (1 << OMAP_USBTLL_SYSSTATUS_RESETDONE_SHIFT)));
#endif

        omap_writel(ohci_context.usbtll_sysconfig,
                OMAP_USBTLL_SYSCONFIG);
        omap_writel(ohci_context.tll_shared_conf,
                OMAP_TLL_SHARED_CONF);

        for (i = 0; i < 3; i++) {

            omap_writel(ohci_context.tll_channel_conf[i],
                    OMAP_TLL_CHANNEL_CONF(i));
            omap_writeb(ohci_context.ulpi_interface_ctrl[i],
                    OMAP_TLL_ULPI_INTERFACE_CTRL(i));
            omap_writeb(ohci_context.ulpi_function_ctrl[i],
                    OMAP_TLL_ULPI_FUNCTION_CTRL(i));
            omap_writeb(ohci_context.ulpi_otg_ctrl[i],
                    OMAP_TLL_ULPI_OTG_CTRL(i));
            omap_writeb(ohci_context.ulpi_usb_int_en_rise[i],
                    OMAP_TLL_ULPI_INT_EN_RISE(i));
            omap_writeb(ohci_context.ulpi_usb_int_en_fall[i],
                    OMAP_TLL_ULPI_INT_EN_FALL(i));
            omap_writeb(ohci_context.ulpi_usb_int_status[i],
                    OMAP_TLL_ULPI_INT_STATUS(i));
        }
        omap_writel(ohci_context.usbtll_irqenable,
                OMAP_USBTLL_IRQENABLE);
    }
}
#else

static void ohci_context_save(void)
{
}

static void ohci_context_restore(void)
{
}

#endif /* CONFIG_OMAP34XX_OFFMODE */

static int omap_ohci_bus_suspend(struct usb_hcd *hcd)
{
    struct ohci_omap_clock_defs *ohci_clocks;
    struct ohci_hcd *ohci = hcd_to_ohci(hcd);
    int ret = 0;
	int ints;
    u32 uhh_sysconfig, usbtll_sysconfig;

    ohci_clocks = (struct ohci_omap_clock_defs *)
        (((char *)hcd_to_ohci(hcd)) + sizeof(struct ohci_hcd));

    printk(KERN_DEBUG "%s: states=%s\n", __func__, ohci_clocks->suspended ? "SLEEP" : "WAKE");

    if (!ohci_clocks->suspended) {
        ret = ohci_bus_suspend(hcd);
        if (ret)
            return ret;

		/* Disable interrupts */
		ohci_writel(ohci, OHCI_INTR_MIE, &ohci->regs->intrdisable);
		(void)ohci_readl(ohci, &ohci->regs->intrdisable);

		/* Make sure any pending interrupts are cleared */
		ints = ohci_readl(ohci, &ohci->regs->intrstatus);
		ints &=  ohci_readl(ohci, &ohci->regs->intrenable);

		if (ints) {
			/*
			 * Warn if there were any pending interrupts
			 * and clear them
			 */
			ohci_writel(ohci, ints, &ohci->regs->intrstatus);
			/* Read back to flush posted writes */
			(void) ohci_readl(ohci, &ohci->regs->intrstatus);

			printk(KERN_DEBUG "%s: cleared pending IRQs\n", __func__);
		}


        mdelay(8); /* MSTANDBY assertion delayed by ~8ms */

        /* Need to set ForceStandby,ForceIdle here
         * else the domain may not be able to transition
         * back during clk_enable if there was a pending event.
         */

        uhh_sysconfig = omap_readl(OMAP_UHH_SYSCONFIG);
        uhh_sysconfig &= ~(3 << OMAP_UHH_SYSCONFIG_MIDLEMODE_SHIFT);
        uhh_sysconfig &= ~(3 << OMAP_UHH_SYSCONFIG_SIDLEMODE_SHIFT);
        omap_writel(uhh_sysconfig, OMAP_UHH_SYSCONFIG);

		usbtll_sysconfig = omap_readl(OMAP_USBTLL_SYSCONFIG);
		usbtll_sysconfig &= ~(3 << OMAP_TLL_SYSCONFIG_SIDLEMODE_SHIFT);
		omap_writel(usbtll_sysconfig, OMAP_USBTLL_SYSCONFIG);

        ohci_context_save();
        clk_disable(ohci_clocks->usbhost_ick_clk);
        clk_disable(ohci_clocks->usbhost2_120m_fck_clk);
        clk_disable(ohci_clocks->usbhost1_48m_fck_clk);
        clk_disable(ohci_clocks->usbtll_ick_clk);
        clk_disable(ohci_clocks->usbtll_fck_clk);
        ohci_clocks->suspended = 1;
        clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
    }

    return ret;
}

static int omap_ohci_bus_resume(struct usb_hcd *hcd)
{
    struct ohci_omap_clock_defs *ohci_clocks;
    struct ohci_hcd *ohci = hcd_to_ohci(hcd);
    int ret = 0;
    u32 uhh_sysconfig, usbtll_sysconfig;

    ohci_clocks = (struct ohci_omap_clock_defs *)
        (((char *)hcd_to_ohci(hcd)) + sizeof(struct ohci_hcd));

    printk(KERN_DEBUG "%s: states=%s\n", __func__, ohci_clocks->suspended ? "SLEEP" : "WAKE");

    if (ohci_clocks->suspended) {
        clk_enable(ohci_clocks->usbtll_ick_clk);
        clk_enable(ohci_clocks->usbtll_fck_clk);
        ohci_context_restore();

        clk_enable(ohci_clocks->usbhost_ick_clk);
        clk_enable(ohci_clocks->usbhost2_120m_fck_clk);
        clk_enable(ohci_clocks->usbhost1_48m_fck_clk);
        ohci_clocks->suspended = 0;

        /* Need to set back to NoStandby,Noidle
         * FIXME: Maybe SmartIdle, SmartStandby will also work
         */
		usbtll_sysconfig = omap_readl(OMAP_USBTLL_SYSCONFIG);
		usbtll_sysconfig &= ~(3 << OMAP_TLL_SYSCONFIG_SIDLEMODE_SHIFT);
		usbtll_sysconfig |= (1 << OMAP_TLL_SYSCONFIG_SIDLEMODE_SHIFT);
		omap_writel(usbtll_sysconfig, OMAP_USBTLL_SYSCONFIG);

        uhh_sysconfig = omap_readl(OMAP_UHH_SYSCONFIG);
        uhh_sysconfig &= ~(3 << OMAP_UHH_SYSCONFIG_MIDLEMODE_SHIFT);
        uhh_sysconfig &= ~(3 << OMAP_UHH_SYSCONFIG_SIDLEMODE_SHIFT);
        uhh_sysconfig |= (1 << OMAP_UHH_SYSCONFIG_MIDLEMODE_SHIFT);
        uhh_sysconfig |= (1 << OMAP_UHH_SYSCONFIG_SIDLEMODE_SHIFT);
        omap_writel(uhh_sysconfig, OMAP_UHH_SYSCONFIG);
		set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

		/* Re-enable interrupts generation */
		ohci_writel(ohci, OHCI_INTR_MIE, &ohci->regs->intrenable);
		(void)ohci_readl(ohci, &ohci->regs->intrdisable);

        ret = ohci_bus_resume(hcd);
    }

    return ret;
}
#endif

static void omap_ohci_shutdown(struct usb_hcd *hcd)
{
    struct ohci_omap_clock_defs *ohci_clocks;
    u32 uhh_sysconfig, usbtll_sysconfig;
    ohci_clocks = (struct ohci_omap_clock_defs *)
        (((char *)hcd_to_ohci(hcd)) + sizeof(struct ohci_hcd));

    return; //tmp workaround, fix crash bug, already enable clk, but access register will cause crash. ????
    
    if (ohci_clocks->suspended) {
        clk_enable(ohci_clocks->usbhost_ick_clk);
        clk_enable(ohci_clocks->usbtll_ick_clk);
        clk_enable(ohci_clocks->usbtll_fck_clk);
        clk_enable(ohci_clocks->usbhost1_48m_fck_clk);
        clk_enable(ohci_clocks->usbhost2_120m_fck_clk);

		/* Need to set back to NoStandby,Noidle
		 * FIXME: Maybe SmartIdle, SmartStandby will also work
		 */
		usbtll_sysconfig = omap_readl(OMAP_USBTLL_SYSCONFIG);
		usbtll_sysconfig &= ~(3 << OMAP_TLL_SYSCONFIG_SIDLEMODE_SHIFT);
		omap_writel(usbtll_sysconfig, OMAP_USBTLL_SYSCONFIG);

		uhh_sysconfig = omap_readl(OMAP_UHH_SYSCONFIG);
		uhh_sysconfig &= ~(3 << OMAP_UHH_SYSCONFIG_MIDLEMODE_SHIFT);
		uhh_sysconfig &= ~(3 << OMAP_UHH_SYSCONFIG_SIDLEMODE_SHIFT);
		uhh_sysconfig |= (1 << OMAP_UHH_SYSCONFIG_MIDLEMODE_SHIFT);
		uhh_sysconfig |= (1 << OMAP_UHH_SYSCONFIG_SIDLEMODE_SHIFT);
		omap_writel(uhh_sysconfig, OMAP_UHH_SYSCONFIG);

        ohci_clocks->suspended = 0;
    }
    ohci_shutdown(hcd);
}


/*-------------------------------------------------------------------------*/

static const struct hc_driver ohci_omap_hc_driver = {
    .description =		hcd_name,
    .product_desc =		"OMAP OHCI",
    .hcd_priv_size =	sizeof(struct ohci_hcd)
        + sizeof(struct ohci_omap_clock_defs),

    /*
     * generic hardware linkage
     */
    .irq =			ohci_omap_irq,
    .flags =		HCD_USB11 | HCD_MEMORY,

    /*
     * basic lifecycle operations
     */
    .reset =		ohci_omap_init,
    .start =		ohci_omap_start,
    .stop =			ohci_omap_stop,
    .shutdown =		omap_ohci_shutdown,

    /*
     * managing i/o requests and associated device resources
     */
    .urb_enqueue =		ohci_urb_enqueue,
    .urb_dequeue =		ohci_urb_dequeue,
    .endpoint_disable =	ohci_endpoint_disable,

    /*
     * scheduling support
     */
    .get_frame_number =	ohci_get_frame,

    /*
     * root hub support
     */
    .hub_status_data =	ohci_hub_status_data,
    .hub_control =		ohci_hub_control,
#ifdef	CONFIG_PM
    .bus_suspend =		omap_ohci_bus_suspend,
    .bus_resume =		omap_ohci_bus_resume,
#endif
    .start_port_reset =	ohci_start_port_reset,
};

/*-------------------------------------------------------------------------*/

static int ohci_hcd_omap_drv_probe(struct platform_device *dev)
{
    return usb_hcd_omap_probe(&ohci_omap_hc_driver, dev);
}

static int ohci_hcd_omap_drv_remove(struct platform_device *dev)
{
    struct usb_hcd		*hcd = platform_get_drvdata(dev);
    struct ohci_omap_clock_defs *ohci_clocks;
    u32 uhh_sysconfig, usbtll_sysconfig;

    ohci_clocks = (struct ohci_omap_clock_defs *)
        (((char *)hcd_to_ohci(hcd)) + sizeof(struct ohci_hcd));

    if (ohci_clocks->suspended) {
        clk_enable(ohci_clocks->usbhost_ick_clk);
        clk_enable(ohci_clocks->usbtll_ick_clk);
        clk_enable(ohci_clocks->usbtll_fck_clk);
        clk_enable(ohci_clocks->usbhost1_48m_fck_clk);
        clk_enable(ohci_clocks->usbhost2_120m_fck_clk);

		/* Need to set back to NoStandby,Noidle
		 * FIXME: Maybe SmartIdle, SmartStandby will also work
		 */
		usbtll_sysconfig = omap_readl(OMAP_USBTLL_SYSCONFIG);
		usbtll_sysconfig &= ~(3 << OMAP_TLL_SYSCONFIG_SIDLEMODE_SHIFT);
		usbtll_sysconfig |= (1 << OMAP_TLL_SYSCONFIG_SIDLEMODE_SHIFT);
		omap_writel(usbtll_sysconfig, OMAP_USBTLL_SYSCONFIG);

		uhh_sysconfig = omap_readl(OMAP_UHH_SYSCONFIG);
		uhh_sysconfig &= ~(3 << OMAP_UHH_SYSCONFIG_MIDLEMODE_SHIFT);
		uhh_sysconfig &= ~(3 << OMAP_UHH_SYSCONFIG_SIDLEMODE_SHIFT);
		uhh_sysconfig |= (1 << OMAP_UHH_SYSCONFIG_MIDLEMODE_SHIFT);
		uhh_sysconfig |= (1 << OMAP_UHH_SYSCONFIG_SIDLEMODE_SHIFT);
		omap_writel(uhh_sysconfig, OMAP_UHH_SYSCONFIG);
        ohci_clocks->suspended = 0;
    }
    usb_hcd_omap_remove(hcd, dev);
    //	platform_set_drvdata(dev, NULL);

    return 0;
}

/*-------------------------------------------------------------------------*/

#if 0

static int ohci_omap_suspend(struct platform_device *dev, pm_message_t message)
{
    struct ohci_hcd	*ohci = hcd_to_ohci(platform_get_drvdata(dev));

    if (time_before(jiffies, ohci->next_statechange))
        msleep(5);
    ohci->next_statechange = jiffies;
    omap_ohci_bus_suspend(ohci_to_hcd(ohci));
    ohci_to_hcd(ohci)->state = HC_STATE_SUSPENDED;
    dev->dev.power.power_state = PMSG_SUSPEND;
    return 0;
}

static int ohci_omap_resume(struct platform_device *dev)
{
    struct usb_hcd *hcd = platform_get_drvdata(dev);
    struct ohci_hcd	*ohci = hcd_to_ohci(hcd);

    if (time_before(jiffies, ohci->next_statechange))
        msleep(5);
    ohci->next_statechange = jiffies;

    dev->dev.power.power_state = PMSG_ON;
    ohci_finish_controller_resume(hcd);
    return 0;
}

#endif

/*-------------------------------------------------------------------------*/

/*
 * Driver definition to register with the OMAP bus
 */
static struct platform_driver ohci_hcd_omap3_driver = {
    .probe		= ohci_hcd_omap_drv_probe,
    .remove		= ohci_hcd_omap_drv_remove,
    .shutdown	= usb_hcd_platform_shutdown,
#if 0
    .suspend	= ohci_omap_suspend,
    .resume		= ohci_omap_resume,
#endif
    .driver		= {
        .owner	= THIS_MODULE,
        .name	= "ohci-omap",
    },
};

MODULE_ALIAS("platform:ohci");
