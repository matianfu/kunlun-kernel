/*
 * twl4030-pwm.c
 * Copyright (C) 2012 Actnova Technologies
 * Author: UGlee Ma <matianfu@actnova.com>
 *
 * twl6030-pwm.c
 * Driver for PHOENIX (TWL6030) Pulse Width Modulator
 *
 * Copyright (C) 2010 Texas Instruments
 * Author: Hemanth V <hemanthv@ti.com>
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
#include <linux/platform_device.h>
#include <linux/i2c/twl.h>
#include <linux/slab.h>

#define TWL4030_PWM_DEBUG 	

/** all following regs are offset **/
#define REG_PMBR1		(0x0D)
#define REG_GPBR1		(0x0C)
#define REG_PWM0ON		(0x00)
#define REG_PWM0OFF		(0x01)
#define REG_PWM1ON		(0x00)
#define REG_PWM1OFF		(0x01)

/** opaque struct defined in pwm.h **/
struct pwm_device {
	const char *label;
	unsigned int pwm_id;
};

/** pwm state **/
typedef enum {
	PWM_FREE,
	PWM_REQUESTED
} pwm_state_t;

static pwm_state_t pwm_state[2] = {PWM_FREE, PWM_FREE};


int pwm_config(struct pwm_device *pwm, int duty_ns, int period_ns)
{
	int id, ontime;
	int ret;
	
#if defined (TWL4030_PWM_DEBUG)

	printk("pwm_config...........................................................................\n");

#endif /** debug **/

	if (pwm == NULL || period_ns == 0 || period_ns > 127 || duty_ns > period_ns)
		return -EINVAL;

	id = pwm -> pwm_id;
	if (id < 0 || id > 1)
		return -EINVAL;

	if (pwm_state[id] == PWM_FREE)
		return -EPERM;	/** TODO is this proper ??? **/

	/** TODO This seems not safe if only one value written successfully **/
	/** write period_ns **/
	if (id == 0) {
		ret = twl_i2c_write_u8(TWL4030_MODULE_PWM0, period_ns, REG_PWM0OFF);
	}
	else if (id == 1) {
		ret = twl_i2c_write_u8(TWL4030_MODULE_PWM1, period_ns, REG_PWM1OFF);
	}
	if (ret < 0) {
		pr_err("%s: Failed to Configure PWM, Error %d\n",
			pwm->label, ret);
		return ret;
	}
	
	/** write ontime **/
	ontime = period_ns - duty_ns; 
	if (id == 0) {
		ret = twl_i2c_write_u8(TWL4030_MODULE_PWM0, ontime, REG_PWM0ON);
	}
	else if (id == 1) {
		ret = twl_i2c_write_u8(TWL4030_MODULE_PWM1, ontime, REG_PWM1ON);
	}

	if (ret < 0) {
		pr_err("%s: Failed to configure PWM, Error %d\n",
			pwm->label, ret);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(pwm_config);


int pwm_enable(struct pwm_device *pwm)
{
	u8 val = 0;
	int id, ret = 0;

#if defined (TWL4030_PWM_DEBUG)

	printk("pwm_enable...........................................................................\n");

#endif /** debug **/


	
	if (pwm == NULL) return -EINVAL;
	
	id = pwm -> pwm_id;
	if (id < 0 || id > 1) return -EINVAL;

	if (pwm_state[id] == PWM_FREE) return -EPERM;
	

	ret = twl_i2c_read_u8(TWL4030_MODULE_INTBR, &val, REG_GPBR1);
	if (ret < 0) {
		pr_err("%s: Failed to enable PWM, Error %d\n", pwm->label, ret);
		return ret;
	}

	if (id == 0) {
		val |= (1 << 2);	/* pwm0 enabled */
		val |= (1 << 0);	/* pwm0 clock enabled */
	}
	else if (id == 1) {
		val |= (1 << 3);	/* pwm1 enabled */
		val |= (1 << 1);	/* pwm1 clock enabled */
	}

	ret = twl_i2c_write_u8(TWL4030_MODULE_INTBR, val, REG_GPBR1);
	if (ret < 0) {
		pr_err("%s: Failed to enable PWM, Error %d\n", pwm->label, ret);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(pwm_enable);

void pwm_disable(struct pwm_device *pwm)
{
	u8 val = 0;
	int id, ret = 0;

#if defined (TWL4030_PWM_DEBUG)

	printk("pwm_disable...........................................................................\n");

#endif /** debug **/


	if (pwm == NULL)
		return;

	id = pwm -> pwm_id;
	if (id < 0 || id > 1) 
		return;

	ret = twl_i2c_read_u8(TWL4030_MODULE_INTBR, &val, REG_GPBR1);
	if (ret < 0) {
		pr_err("%s: Failed to disable PWM, Error %d\n",
			pwm->label, ret);
		return;
	}

	if (id == 0) {
		val &= ~(1 << 2);	/* pwm0 disabled */
		val &= ~(1 << 0);	/* pwm0 clock disabled */
	}
	else if (id ==  1) {
		val &= ~(1 << 3);	/* pwm1 disabled */
		val &= ~(1 << 1);	/* pwm1 clock disabled */
	}

	ret = twl_i2c_write_u8(TWL4030_MODULE_INTBR, val, REG_GPBR1);
	if (ret < 0) {
		pr_err("%s: Failed to disable PWM, Error %d\n",
			pwm->label, ret);
		return;
	}

	return;
}
EXPORT_SYMBOL(pwm_disable);

struct pwm_device *pwm_request(int pwm_id, const char *label)
{
	u8 val;
	int ret;
	struct pwm_device *pwm;

#if defined (TWL4030_PWM_DEBUG)

	printk("pwm_request...........................................................................\n");

#endif /** debug **/



	/** only pwm0 and pwm1 supported **/
	if (pwm_id < 0 || pwm_id > 1)
		return NULL;

	/** already requested **/
	if (pwm_state[pwm_id] != PWM_FREE)
		return NULL;

	/** alloc **/
	pwm = kzalloc(sizeof(struct pwm_device), GFP_KERNEL);
	if (pwm == NULL) {
		pr_err("%s: failed to allocate memory\n", label);
		return NULL;
	}

	pwm->label = label;
	pwm->pwm_id = pwm_id;

	/* Configure PWM */
	ret = twl_i2c_read_u8(TWL4030_MODULE_INTBR, &val, REG_PMBR1);
	if (ret < 0) {
		pr_err("%s: Failed to configure PWM, Error %d\n",
			pwm->label, ret);
		kfree(pwm);
		return NULL;
	}
	
	if (pwm_id == 0) {
		val &= ~(1 << 3);	/** enable pwm0 **/
		val |= (1 << 2);
	}
	else if (pwm_id == 1) {
		val |= ((0x03) << 4);	/** enable pwm1 **/
	}

	ret = twl_i2c_write_u8(TWL4030_MODULE_INTBR, val, REG_PMBR1);
	if (ret < 0) {
		pr_err("%s: Failed to configure PWM, Error %d\n",
			 pwm->label, ret);
		kfree(pwm);
		return NULL;
	}

	pwm_state[pwm_id] = PWM_REQUESTED;	

	return pwm;
}
EXPORT_SYMBOL(pwm_request);


void pwm_free(struct pwm_device *pwm) {

	u8 val;
	int id, ret;
	const char* label;

#if defined (TWL4030_PWM_DEBUG)

	printk("pwm_free...........................................................................\n");

#endif /** debug **/


	if (pwm == NULL) return;
	
	id = pwm -> pwm_id;
	label = pwm -> label;
	if (id < 0 || id > 1) return;
	
	pwm_disable(pwm);
	kfree(pwm);

	pwm_state[id] = PWM_FREE;

	ret = twl_i2c_read_u8(TWL4030_MODULE_INTBR, &val, REG_PMBR1);
        if (ret < 0) {
                pr_err("%s: Failed to unconfig PWM, Error %d\n", label, ret);
                return;
        }

	if (id == 0) {
		val &= ~((0x03) << 2);	/** disable pwm0, enable gpio 6 **/
	}
	else if (id == 1) {
		val &= ~((0x03) << 4);  /** disable pwm1, enable gpio 7 **/
	}

        ret = twl_i2c_write_u8(TWL4030_MODULE_INTBR, val, REG_PMBR1);
        if (ret < 0) {
                pr_err("%s: Failed to configure PWM, Error %d\n", label, ret);
                return;
        }
}
EXPORT_SYMBOL(pwm_free);
