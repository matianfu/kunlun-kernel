/*
 * linux/drivers/i2c/chips/twl4030-poweroff.c
 *
 * Power off device
 *
 * Copyright (C) 2008 Nokia Corporation
 *
 * Written by Peter De Schrijver <peter.de-schrijver@nokia.com>
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License. See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/pm.h>
#include <linux/i2c/twl.h>

#if  defined(CONFIG_TWL4030_POWEROFF_VIATELECOM)
#define PWR_P1_SW_EVENTS	0x10
#define PWR_P2_SW_EVENTS	0x11
#define PWR_P3_SW_EVENTS	0x12
#define PWR_DEVOFF	(1<<0)
#define PWR_STOPON_POWERON (1<<6)

#define CFG_P123_TRANSITION	0x03
#define SEQ_OFFSYNC	(1<<0)

#define PWR_CFG_P1_TRANSITION	(0x00)
#define PWR_CFG_P2_TRANSITION	(0x01)
#define PWR_CFG_P3_TRANSITION	(0x02)
#define STARTON_SWBUG	(1<<7)
#define STARTON_VBAT	(1<<4)
#define STARTON_RTC	(1<<3)
#define STARTON_CHG	(1<<1)
#define STARTON_PWON	(1<<0)

#define PWR_STS_BOOT	(0x04)
#define SETUP_DONE_BCK	(1<<2)

#define PWR_PROTECT_KEY (0x0E)

#define WATCHDOG_CFG	(0x03)
#define WATCHDOG_FIRE	(0x01)

static inline void twl4030_enable_write(void)
{
    int err;

    err = twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, 0xC0,
				   PWR_PROTECT_KEY);
    if (err) {
        printk(KERN_WARNING "I2C error %d while writing TWL4030 PM_MASTER PWR_PROTECT_KEY\n", err);
        return ;
    }

    err = twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, 0x0C,
				   PWR_PROTECT_KEY);
    if (err) {
        printk(KERN_WARNING "I2C error %d while writing TWL4030 PM_MASTER PWR_PROTECT_KEY\n", err);
        return ;
    }

}

static inline void twl4030_disable_write(void)
{
    int err;
    
    err = twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, 0x00,
				   PWR_PROTECT_KEY);
    if (err) {
        printk(KERN_WARNING "I2C error %d while writing TWL4030 PM_MASTER PWR_PROTECT_KEY\n", err);
        return ;
    }

}

static void twl4030_poweroff(void)
{
    u8 uninitialized_var(val);
    int err;
    int i = 0;

#if defined(CONFIG_AP2MODEM_VIATELECOM)
    extern void ap_poweroff_modem(void);
    ap_poweroff_modem( );
#endif

    /* Clear the STARTON_VBAT and STARTON_SWBUG
      * STARTON_VBAT will cause the auto reboot if battery insert;
      * STARTON_SWBUG will cause the auto reboot if watchdog has been expired
      * Mark the STARTON_PWON, which enable switch on transition if power on has been pressed
      */
    twl4030_enable_write();
    for(i=0; i<3; i++){
        err = twl_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &val,
        			  PWR_CFG_P1_TRANSITION + i);
        if (err) {
            printk(KERN_WARNING "I2C error %d while writing TWL4030 PM_MASTER PWR_CFG_P%d_TRANSITION\n", err, i+1);
            goto fail;
        }

        val &= (~(STARTON_SWBUG | STARTON_VBAT));
        val |= STARTON_PWON | STARTON_RTC | STARTON_CHG;

        err = twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, val,
        			   PWR_CFG_P1_TRANSITION + i);
        if (err) {
            printk(KERN_WARNING "I2C error %d while writing TWL4030 PM_MASTER PWR_CFG_P%d_TRANSITION\n", err, i+1);
            goto fail;
        }
    }
    twl4030_disable_write();

#if 0
    /*fire the watchdog*/
    val = WATCHDOG_FIRE;
    err = twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER, val,
        			   WATCHDOG_CFG);
    if (err) {
        printk(KERN_WARNING "I2C error %d while writing TWL4030 PM_MASTER PWR_CFG_P%d_TRANSITION\n", err, i+1);
        goto fail ;
    }  
#endif

    /* Make sure SEQ_OFFSYNC is set so that all the res goes to wait-on */
    err = twl_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &val,
    			   CFG_P123_TRANSITION);
    if (err) {
    	pr_warning("I2C error %d while reading TWL4030 PM_MASTER CFG_P123_TRANSITION\n",
    		err);
    	return;
    }

    val |= SEQ_OFFSYNC;
    err = twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, val,
    			    CFG_P123_TRANSITION);
    if (err) {
    	pr_warning("I2C error %d while writing TWL4030 PM_MASTER CFG_P123_TRANSITION\n",
    		err);
    	return;
    }

    err = twl_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &val,
    			  PWR_P1_SW_EVENTS);
    if (err) {
    	pr_warning("I2C error %d while reading TWL4030 PM_MASTER P1_SW_EVENTS\n",
    		err);
    	return;
    }

    val |= PWR_STOPON_POWERON | PWR_DEVOFF;

    err = twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, val,
    			   PWR_P1_SW_EVENTS);

    if (err) {
    	pr_warning("I2C error %d while writing TWL4030 PM_MASTER P1_SW_EVENTS\n",
    		err);
    	return;
    }

    return;
fail:
    printk(KERN_ERR "Fail to power off the system.\n");
    while(1) ;
}

static void *f_restart;
void twl4030_pm_restart(char mode, const char *cmd)
{
    u8 val;
    int err;
    int i = 0;

#if defined(CONFIG_AP2MODEM_VIATELECOM)
    extern void ap_poweroff_modem(void);
    ap_poweroff_modem( );
#endif

    /* Clear the STARTON_VBAT and STARTON_SWBUG
      * STARTON_VBAT will cause the auto reboot if battery insert;
      * STARTON_SWBUG will cause the auto reboot if watchdog has been expired
      * Mark the STARTON_PWON, which enable switch on transition if power on has been pressed
      */
    twl4030_enable_write();
    for(i=0; i<3; i++){
        err = twl_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &val,
        			  PWR_CFG_P1_TRANSITION + i);
        if (err) {
            printk(KERN_WARNING "I2C error %d while writing TWL4030 PM_MASTER PWR_CFG_P%d_TRANSITION\n", err, i+1);
            goto fail;
        }

        val &= (~(STARTON_VBAT));
        val |= STARTON_PWON | STARTON_SWBUG |STARTON_RTC | STARTON_CHG;

        err = twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, val,
        			   PWR_CFG_P1_TRANSITION + i);
        if (err) {
            printk(KERN_WARNING "I2C error %d while writing TWL4030 PM_MASTER PWR_CFG_P%d_TRANSITION\n", err, i+1);
            goto fail;
        }
    }
    twl4030_disable_write();

    err = twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, SETUP_DONE_BCK, PWR_STS_BOOT);
    if(err){
        printk("I2C error %d while writing TWL4030 PM_MASTER PWR_STS_BOOT\n", err);
        goto fail;
    }

    printk(KERN_WARNING "Restart the system...\n");

    /*fire the watchdog*/
    val = WATCHDOG_FIRE;
    err = twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER, val,
        			   WATCHDOG_CFG);
    if (err) {
        printk(KERN_WARNING "I2C error %d while writing TWL4030 PM_MASTER PWR_CFG_P%d_TRANSITION\n", err, i+1);
        goto fail ;
    }    
    
    return;
    
fail:
    printk(KERN_ERR "Fail to restart the system.\n");
    while(1) ;
    
}
#else
#define PWR_P1_SW_EVENTS	0x10
#define PWR_DEVOFF	(1<<0)
#define PWR_STOPON_POWERON (1<<6)

#define CFG_P123_TRANSITION	0x03
#define SEQ_OFFSYNC	(1<<0)

void twl4030_poweroff(void)
{
	u8 uninitialized_var(val);
	int err;

	/* Make sure SEQ_OFFSYNC is set so that all the res goes to wait-on */
	err = twl_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &val,
				   CFG_P123_TRANSITION);
	if (err) {
		pr_warning("I2C error %d while reading TWL4030 PM_MASTER CFG_P123_TRANSITION\n",
			err);
		return;
	}

	val |= SEQ_OFFSYNC;
	err = twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, val,
				    CFG_P123_TRANSITION);
	if (err) {
		pr_warning("I2C error %d while writing TWL4030 PM_MASTER CFG_P123_TRANSITION\n",
			err);
		return;
	}

	err = twl_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &val,
				  PWR_P1_SW_EVENTS);
	if (err) {
		pr_warning("I2C error %d while reading TWL4030 PM_MASTER P1_SW_EVENTS\n",
			err);
		return;
	}

	val |= PWR_STOPON_POWERON | PWR_DEVOFF;

	err = twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, val,
				   PWR_P1_SW_EVENTS);

	if (err) {
		pr_warning("I2C error %d while writing TWL4030 PM_MASTER P1_SW_EVENTS\n",
			err);
		return;
	}

	return;
}
#endif
static int __init twl4030_poweroff_init(void)
{
       u8 uninitialized_var(val);

	pm_power_off = twl4030_poweroff;
#if  defined(CONFIG_TWL4030_POWEROFF_VIATELECOM)
       twl_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &val,
				  PWR_P1_SW_EVENTS);
       val |= PWR_STOPON_POWERON;
       twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, val,
				   PWR_P1_SW_EVENTS);

       twl_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &val,
				  PWR_P2_SW_EVENTS);
       val |= PWR_STOPON_POWERON;
       twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, val,
				   PWR_P2_SW_EVENTS);

       twl_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &val,
				  PWR_P3_SW_EVENTS);
       val |= PWR_STOPON_POWERON;
       twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, val,
				   PWR_P3_SW_EVENTS);
       f_restart = arm_pm_restart;
       arm_pm_restart = twl4030_pm_restart;
#endif
       
       return 0;
}

static void __exit twl4030_poweroff_exit(void)
{
	pm_power_off = NULL;
#if  defined(CONFIG_TWL4030_POWEROFF_VIATELECOM)
       arm_pm_restart = f_restart;
#endif
}

module_init(twl4030_poweroff_init);
module_exit(twl4030_poweroff_exit);

MODULE_DESCRIPTION("Triton2 device power off");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peter De Schrijver");
