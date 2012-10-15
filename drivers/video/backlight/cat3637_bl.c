/*
 * =====================================================================================
 *
 *       Filename:  cat3637_bl.c
 *
 *    Description:  CAT3637 LED drvier on Board Kunlun
 *
 *        Version:  1.0
 *        Created:  05/05/2010 04:46:43 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Daqing Li (daqli), daqli@via-telecom.com
 *        Company:  VIA TELECOM
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
 * =====================================================================================
 */

#include <linux/module.h>

#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/if_ether.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/ethtool.h>
#include <linux/completion.h>
#include <linux/bitops.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/fb.h>
#include <linux/backlight.h>


//#define CONFIG_BACKLIGHT_CAT3637_DEBUG 1

#if CONFIG_BACKLIGHT_CAT3637_DEBUG
#define DPRINTK_BL(fmt,args...)  printk(KERN_INFO "CAT3637: " fmt,##args)
#else
#define DPRINTK_BL(fmt,args...)
#endif

#define CAT3637
#define LCD_BACKLIGHT_GPIO 155

/* Flag to signal when the battery is low */
#define CAT3637BL_BATTLOW       BL_CORE_DRIVER1

#define CAT3637_NUM_LEVELS  16
static int backlight_cur_level = CAT3637_NUM_LEVELS;


static int cat3637_initialized = 0;


/*the sequence of driver loaded make sure lcd driver load first */
static int lcd_enabled = 1;
static struct backlight_device *cat3637_backlight_device;
static struct generic_bl_info *bl_machinfo;


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  cat3637_init
 *  Description:  initialize GPIO pin which cat3637 use as input
 * =====================================================================================
 */
static void cat3637_init(void)
{
    /*set GPIO155 low level,LCD BL shutdown */
    gpio_direction_output(LCD_BACKLIGHT_GPIO,1);
    gpio_set_value(LCD_BACKLIGHT_GPIO,1);
    //udelay(40);

    backlight_cur_level = 0;    
    DPRINTK_BL("cat3637 initialized\n");
 
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  cat3637_init
 *  Description: shutdown
 * =====================================================================================
 */
static void cat3637_off(void)
{
    /*set GPIO155 low level,LCD BL shutdown */
    gpio_direction_output(LCD_BACKLIGHT_GPIO,0);
    mdelay(3);
    backlight_cur_level = 0;
    DPRINTK_BL("cat3637 shutdown\n");
   
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  cat3637_set_led_current
 *  Description:  set led current
 *  Parameters :  @level: range of 0 - 31,0 is full scale and 31 is zero current 
 * =====================================================================================
 */

static DEFINE_SPINLOCK(cat3637_lock);
 
static void cat3637_set_led_current (int level )
{    
    int num_pulses = level;

    unsigned long flags;

    if(level < 0 || level >= CAT3637_NUM_LEVELS){
        printk(KERN_ERR "invalid parameter:range of level:0-15\n");
        return ;
    }  

    if(!cat3637_initialized){
    	if (gpio_request(LCD_BACKLIGHT_GPIO, "led-bl-155") < 0) {
			printk(KERN_ERR "can't get led-bl-155 GPIO\n");
	}
    	
        cat3637_init();
        cat3637_initialized = 1;
    } 

    if(level == backlight_cur_level){
        /*need not to be adjusted*/
        return;
    }else if(level == 0){
        /*turn off backlight*/
        cat3637_off();
        return;
    }else{
        if(num_pulses < backlight_cur_level){
            num_pulses = (num_pulses + CAT3637_NUM_LEVELS) - backlight_cur_level;
        }else if(num_pulses > backlight_cur_level){
            num_pulses -= backlight_cur_level;
        }
    }

    DPRINTK_BL("level = %d,num_pulses = %d,backlight_cur_level = %d\n",level,num_pulses,backlight_cur_level);

    spin_lock_irqsave(&cat3637_lock,flags);

    if(backlight_cur_level == 0){
        gpio_set_value(LCD_BACKLIGHT_GPIO,1);
        udelay(40);
    }    
    
    if(num_pulses > 0){
        gpio_set_value(LCD_BACKLIGHT_GPIO,0);
        udelay(10);
    }
    /*produce level times pulse*/
    while(num_pulses--){
        gpio_set_value(LCD_BACKLIGHT_GPIO,0);
        udelay(1);
        gpio_set_value(LCD_BACKLIGHT_GPIO,1);
        udelay(1);
    }
    gpio_set_value(LCD_BACKLIGHT_GPIO,1);
    spin_unlock_irqrestore(&cat3637_lock,flags);
    backlight_cur_level = level;
}		/* -----  end of function cat3637_set_led_current  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  backlight_set_level
 *  Description:  set backlight level,exported for external call
 *  Parameters : @percent: range of 0 - 100,where 0 is the darkesst,100 is the brightest 
 * =====================================================================================
 */
static void backlight_set_level(int percent )
{
    int level = 0;
    if(percent < 0 || percent > 100){
        printk(KERN_ERR "invalid parameter: percent should be 0 - 100\n");
        return;
    }  
    //100 / (CAT3637_NUM_LEVELS - 1) = 6.67
    level = (CAT3637_NUM_LEVELS - 1) * percent / 100;
    
 
    cat3637_set_led_current(level);
}		/* -----  end of function backlight_set_level  ----- */


static int cat3637bl_send_intensity(struct backlight_device *bd)
{
	int intensity = bd->props.brightness;

	if (bd->props.power != FB_BLANK_UNBLANK)
		intensity = 0;
	if (bd->props.state & BL_CORE_FBBLANK)
		intensity = 0;
	if (bd->props.state & BL_CORE_SUSPENDED)
		intensity = 0;
    /*solve white screen problem when resume*/
	if(bd->props.state & BL_CORE_SUSPENDRESUME)
	    intensity = bd->props.brightness;
	if (bd->props.state & CAT3637BL_BATTLOW)
		intensity &= bl_machinfo->limit_mask;

      DPRINTK_BL("intensity = %d\n",intensity);
      DPRINTK_BL("props.power = %u\n",bd->props.state);
      if(lcd_enabled){
	    bl_machinfo->set_bl_intensity(intensity);
	    bd->props.brightness = intensity;
	}else{
	    bl_machinfo->set_bl_intensity(0);
	}

	if (bl_machinfo->kick_battery)
		bl_machinfo->kick_battery();

	return 0;
}

static int cat3637bl_get_intensity(struct backlight_device *bd)
{
	return bd->props.brightness;
}

/*
 * Called when the battery is low to limit the backlight intensity.
 * If limit==0 clear any limit, otherwise limit the intensity
 */
void corgibl_limit_intensity_kunlun(int limit)
{
	struct backlight_device *bd = cat3637_backlight_device;

	mutex_lock(&bd->ops_lock);
	if (limit)
		bd->props.state |= CAT3637BL_BATTLOW;
	else
		bd->props.state &= ~CAT3637BL_BATTLOW;

    DPRINTK_BL("%s(%d) entering\n",__func__,limit);
    
	backlight_update_status(cat3637_backlight_device);
	mutex_unlock(&bd->ops_lock);
}
EXPORT_SYMBOL(corgibl_limit_intensity_kunlun);


/*
 * Called when lcd driver resume
 */
void backlight_enable(void)
{
    DPRINTK_BL("%s entering\n",__func__);
    lcd_enabled = 1;   
}

EXPORT_SYMBOL(backlight_enable);

/*
 * Called when lcd driver suspend
 */
void backlight_disable(int backlight_loaded)
{
    DPRINTK_BL("%s entering\n",__func__);
    if(!backlight_loaded){ 
        DPRINTK_BL("backlight driver has not been loaded\n");
        backlight_set_level(0);
        return;
    }else{
        /*lcd driver is about to suspend when android is dimming backlight*/
         lcd_enabled = 0;
    }
}

EXPORT_SYMBOL(backlight_disable);


static struct backlight_ops cat3637bl_ops = {
	.options = BL_CORE_SUSPENDRESUME,
	.get_brightness = cat3637bl_get_intensity,
	.update_status  = cat3637bl_send_intensity,
};

static int cat3637bl_probe(struct platform_device *pdev)
{
	struct backlight_properties props;
	struct generic_bl_info *machinfo = pdev->dev.platform_data;
	const char *name = "bl_cat3637";
	struct backlight_device *bd = NULL;

	bl_machinfo = machinfo;
	if (!machinfo->limit_mask)
		machinfo->limit_mask = -1;

	if (machinfo->name)
		name = machinfo->name;
	
	bl_machinfo->set_bl_intensity = backlight_set_level;
	memset(&props, 0, sizeof(struct backlight_properties));
	props.max_brightness = machinfo->max_intensity;

	if(!cat3637_initialized){
		if (gpio_request(LCD_BACKLIGHT_GPIO, "led-bl-155") < 0) {
			printk(KERN_ERR "can't get led-bl-155 GPIO\n");
		}
    	cat3637_init();
		cat3637_initialized = 1;
	}
	
	bd = backlight_device_register (name,
		&pdev->dev, NULL, &cat3637bl_ops, &props);

	if (IS_ERR (bd))
		return PTR_ERR (bd);

	platform_set_drvdata(pdev, bd);

	bd->props.max_brightness = machinfo->max_intensity;
	bd->props.power = FB_BLANK_UNBLANK;
	bd->props.brightness = machinfo->default_intensity;
	backlight_update_status(bd);
	cat3637_backlight_device = bd;

	DPRINTK_BL("kunlun Backlight Driver Initialized.\n");
	return 0;
}

static int cat3637bl_remove(struct platform_device *pdev)
{
	struct backlight_device *bd = cat3637_backlight_device;

	bd->props.power = 0;
	bd->props.brightness = 0;

	backlight_update_status(bd);

	backlight_device_unregister(bd);

	DPRINTK_BL("kunlun Backlight Driver Unloaded\n");
	return 0;
}

static struct platform_driver cat3637bl_driver = {
	.probe		= cat3637bl_probe,
	.remove		= cat3637bl_remove,
	.driver		= {
		.name	= "bl_cat3637",
		.owner  = THIS_MODULE,
     },
};

static int __init cat3637bl_init(void)
{
	return platform_driver_register(&cat3637bl_driver);
}

static void __exit cat3637bl_exit(void)
{
	platform_driver_unregister(&cat3637bl_driver);
}

pure_initcall(cat3637bl_init);
module_exit(cat3637bl_exit);

MODULE_AUTHOR("daqing <daqli@via-telecom.com>");
MODULE_DESCRIPTION("CAT3637 Backlight Driver");
MODULE_LICENSE("GPL");


