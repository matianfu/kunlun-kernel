/*
 * linux/drivers/power/twl4030_bci_battery.c
 *
 * OMAP2430/3430 BCI battery driver for Linux
 *
 * Copyright (C) 2008 Texas Instruments, Inc.
 * Author: Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/wakelock.h>
#include <linux/platform_device.h>
#include <linux/i2c/twl.h>
#include <linux/power_supply.h>
#include <asm/mach-types.h>
#include <linux/i2c/twl4030-madc.h>
#include <mach/board-kunlun.h>
#include "twl4030_bci_charge.h"

//#define BCI_BAT_DEBUG
#ifdef BCI_BAT_DEBUG
#define BATDPRT(x...)  printk(KERN_INFO "[Battery] " x)
#else
#define BATDPRT(x...)  do{} while(0)
#endif
#define BATPRT(x...)  printk(KERN_INFO "[Battery] " x)

//#define BCI_LEVEL_WAKEUP_DEBUG
#ifdef BCI_LEVEL_WAKEUP_DEBUG
static int ldbg_full;
static int ldbg_volt;
static int ldbg_ichg;
#endif

#define BCI_VBAT_LEVEL_WAKEUP

/*Whether active the function*/
#define BCI_BE_ACTIVE_FUN 0
#if BCI_BE_ACTIVE_FUN
static int bci_read_val(u8 reg);
static int bci_battery_temp(int volt);
#endif

static int twl4030_charge_send_event(bci_charge_device_info *di, bci_charge_event_id id);
static void twl4030_monitor_reset_update(bci_charge_device_info *di, bci_rst_update level);
static int twl4030_monitor_chgi_ref_set(bci_charge_device_info *di, int level);
static int twl4030_monitor_chgv_ref_set(bci_charge_device_info *di, int level);
static int twl4030_monitor_chgi_reg_get(u16 *val);
static int twl4030_monitor_chgi_reg_set(u16 val);
static int twl4030_itov_en_gain(int);
static int twl4030_presacler_en_mesvbus(int);
static int twl4030_presacler_en_mesvbat(int);
static int twl4030_presacler_en_mesvac(int);
static int bci_battery_cap(bci_charge_device_info *di, int rst);
static void bci_battery_update_cap(bci_charge_device_info *di, int rst);
static int bci_battery_voltage(int adc);
static int bci_backupbatt_voltage(int adc);
static int bci_charge_current(int adc);
static int bci_ac_voltage(int adc);
static int bci_usb_voltage(int adc);
static int bci_init_hardware(void);

/* Ptr to thermistor table */
int *therm_tbl;

/*the charge info*/
static struct bci_charge_device_info bci_charge_info;

/* ADC Calibration*/
/* Voltmv = Slope*Adc/1000 + Intercept 
  * NOTE: the Voltmv is the voltage after prescaler, which is form 0 to 1.5V
  * the actual voltage is Voltmv/R, which R is the prescaler divider ratio
  */
struct calibration_params{
    int      slope;               /*0.001mv/adc*/
    int      intercept;       /*uinit is mV*/
};

struct calibration_sample{
   int      volt;              /* voltage in mV                           */
   int      adc;              /* ADC value corresponding to the voltage  */   
} ;
/*The max sample point to calibrate ADC by fixed voltage input
  *Note: the value must be 2 at least
  */
#define ADC_VOLT_SAMPLE_NUM  (8)
#define ERROR_SAMPLE (0xFFFF)
static struct calibration_sample calib_table[ADC_VOLT_SAMPLE_NUM];
static int valid_samples = 0;

static struct calibration_params calib_params = {1450, 20};

/* the prescaler divider ratio for each ADC channle
  * N: numerator, D:denomenator
 */
#define BCI_VBAT_PSR_N  (25)  
#define BCI_VBAT_PSR_D  (100)

#define BCI_VBUS_PSR_N  (30)  
#define BCI_VBUS_PSR_D  (140)

#define BCI_VAC_PSR_N  (15)  
#define BCI_VAC_PSR_D  (100)

/*Get the voltage acorrding to the ADC value, which is the voltage after prescaler*/
#define adc2volt(adc)  ( (adc * (calib_params.slope)) /(1000) + (calib_params.intercept) )

static void bci_calib_save_point(int volt, int adc)
{
    int i = 0;

    BATDPRT("save point[%d , 0x%x].\n", volt, adc);
    
    if( (adc < 100) || (adc > 0x3FF) || 
        (volt < 2800) || (volt > 4500) ) {
            BATPRT("Invalid calibration sample point from comand line [%d , 0x%x].\n", volt, adc);
    }
     
    for(i = 0; i < ADC_VOLT_SAMPLE_NUM; i++ ){
        if(calib_table[i].adc == 0){
            calib_table[i].volt = volt;
            calib_table[i].adc = adc;
            valid_samples++;
            break;
        }
    }

}

/*  Get the sample point from Comand line
  *  Formats: adc_calibration=<Vol,0xADC>,<Vol,0xADC>
  */
static int __init bci_calib_get_samples(char *str)
{
    int volt = 0, adc = 0, flag = 0;
    char *begin=NULL, *end=NULL;
    
    if (str == NULL || *str == '\0')
        return 1;

    while(*str){
        if(*str == '<'){
            begin = str;
        }
        
        if(*str == '>'){
            end = str;
        }
        
        /*parse one ponit*/
        if(begin && end){
            begin++;
            end --;
            while(end >= begin){
                if(isspace(*begin)){
                    begin++;
                }else if(*begin == ','){
                    flag = 1;
                    begin++;
                }else{
                    if(flag){
                        adc = simple_strtol (begin, &begin, 0);
                    }else{
                        volt = simple_strtol (begin, &begin, 0);
                    }
                }
            }
            
            bci_calib_save_point(volt, adc);
            begin = NULL;
            end = NULL;
            flag = 0;

        }
        
        str++;
    }
    
    return 1;
}
__setup("adc_calibrate=", bci_calib_get_samples);

#define NANME_LENGTH (20)
#define MAC_ADDR_LEN (6)
static unsigned char wifi_mac[NANME_LENGTH] = "unknow";
int get_wifi_mac(unsigned char *addr, int len)
{
    int i = 0, count = 0;
    unsigned char *head;
    unsigned char buf[NANME_LENGTH];

    memcpy(buf, wifi_mac, NANME_LENGTH);
    if(!strcmp(buf, "unknow")){
        return 0;
    }

    count = 0; //the number of bytes according to the MAC address 
    head = buf;
    while(count < len){
        if(buf[i] == ':'){
            buf[i] = '\0';
            addr[count++] = (unsigned char)simple_strtoul(head, NULL, 16);
            head = buf + i + 1;
        }else if(buf[i] == '\0'){
            addr[count++] = (unsigned char)simple_strtoul(head, NULL, 16);
            break;
        }
        i++;
    }

    return count;
}
EXPORT_SYMBOL(get_wifi_mac);

int static wifi_mac_check(const char *str)
{
    int i = 0;
    int ret = 0, step = 0;

    /*ignore the last char for echo test*/
    for(i=0; i<strlen(str) - 1; i++){
        step = (i+1) % 3;
        if(step){
            if((str[i] >= '0' && str[i] <= '9') ||(str[i] >= 'a' && str[i] <= 'f') || (str[i] >= 'A' && str[i] <= 'F') ){
                continue;
            }else{
                ret = -1;
                printk("Invalid MAC address(MAC[%d] = %c)\n", i, str[i]);
                break;
            }
        }else{
            if(str[i] == ':'){
                continue;
            }else{
                ret = -1;
                printk("Invalid MAC address(MAC[%d] = %c)\n", i, str[i]);
                break;
            }
        }
    }

    return ret;
}
static int __init bci_calib_get_wifi_mac(char *str)
{
    int len = 0;
    
    if (str == NULL || *str == '\0')
        return 1;

    if(wifi_mac_check(str) < 0){
        return 1;
    }
    
    while( *(str+len) && (len < NANME_LENGTH -1)){
        wifi_mac[len] = *(str + len);
        len++;
    }
    
    return 1;
}
__setup("wifi_mac=", bci_calib_get_wifi_mac);

static void bci_calib_init_samples(void)
{
    int i, j;
    int tmpadc, tmpvolt;

    /* Check for data integrality */
    for(i = 0; i < ADC_VOLT_SAMPLE_NUM; i++){
        if( (calib_table[i].adc < 100) || (calib_table[i].adc > 0x3FF) || 
            (calib_table[i].volt < 2800) || (calib_table[i].volt > 4500) ){
            
            BATPRT("Error calibration sample point %d:[%d , 0x%x].\n", i,  calib_table[i].volt, calib_table[i].adc);
            calib_table[i].adc = ERROR_SAMPLE;
            calib_table[i].volt = ERROR_SAMPLE;
        }
    }
    
    /* Sort the data by battery value increasingly */
    for(i = 0; i < ADC_VOLT_SAMPLE_NUM; i++){
        for(j = i + 1; j <ADC_VOLT_SAMPLE_NUM; j++){
            if(calib_table[i].volt > calib_table[j].volt){
                tmpvolt = calib_table[i].volt;
                tmpadc = calib_table[i].adc;
                calib_table[i].volt = calib_table[j].volt;
                calib_table[i].adc = calib_table[j].adc;
                calib_table[j].volt = tmpvolt;
                calib_table[j].adc = tmpadc;
            }
        }
    }
    
}

static int bci_calib_caculate_slope(void)
{
    int volt1, volt2;
    int adc1,  adc2;
    int slope= 0, intercept = 0;
    int slope_sum = 0, intercept_sum = 0;
    int samples = 0;
    int count = 0;
    int i = 0, j = 0;

    for(i = 0; i < ADC_VOLT_SAMPLE_NUM; i++){
        if( calib_table[i].volt == ERROR_SAMPLE ){
            break;
        }
    }

    if(i < 1){
        BATPRT("Valid sample is less 2, so can not calibrate ADC.\n");
        return -1;
    }

    /*get the number of valid sample points*/
    samples = i;

    /*Calibrate the ADC by any two samples, and get the average*/
    /*Voltmv = Slope*Adc/1000 + Intercept*/ 
    for(i = 0; i < samples; i++){
        volt1 = calib_table[i].volt * BCI_VBAT_PSR_N / BCI_VBAT_PSR_D;
        adc1 = calib_table[i].adc;

        for(j = i + 1; j < samples; j++){
            volt2 = calib_table[j].volt * BCI_VBAT_PSR_N / BCI_VBAT_PSR_D;
            adc2 = calib_table[j].adc;

            slope = ((volt2 - volt1) * 1000)  / (adc2 - adc1);
            intercept = volt1 - ( (adc1 * slope) / 1000);

            slope_sum += slope;
            intercept_sum += intercept;
            count++;
        }
    }

     /*get the slope and intercept, amplifed the slope 1000 times to revert the bit after decimal point*/
    calib_params.slope = slope_sum / count;
    calib_params.intercept = intercept_sum / count;

    BATDPRT("ADC Calibration: slope=%d, intercept=%d.\n", calib_params.slope, calib_params.intercept);
    
 #if 0
    /*Calibrate the ADC between Max and Min samples*/
    /*convert the input Voltage to the presacled Voltage*/
    volt2 = calib_table[ADC_VOLT_SAMPLE_NUM-1].volt  * BCI_VBAT_PSR_N / BCI_VBAT_PSR_D;
    adc2 = calib_table[ADC_VOLT_SAMPLE_NUM-1].adc;

    volt1 = calib_table[0].volt * BCI_VBAT_PSR_N / BCI_VBAT_PSR_D;
    adc1 = calib_table[0].adc;
    
    /*get the slope and intercept, amplifed the slope 1000 times to revert the bit after decimal point*/
    calib_params.slope = ((volt2 - volt1) * 1000)  / (adc2 - adc1);
    calib_params.intercept = volt1 - (adc1 * (calib_params.slope) / 1000);
#endif

    return 0;
    
}

static int bci_calib_init(void)
{
    int ret = -1;

    bci_calib_init_samples();
    
    ret = bci_calib_caculate_slope();

    return ret;
}


static DEFINE_SPINLOCK(status_lock);
void bci_status_set(const char *name, int on)
{
    unsigned long irqflags;
    int i = 0;

    spin_lock_irqsave(&status_lock, irqflags);
    for(i =0; i < CUR_ESTM_ID_MAX; i++){
        if(!strncmp(status_table[i].name, name, strlen(status_table[i].name))){
            break;
        }
    }
    
    if(i < CUR_ESTM_ID_MAX){
        status_table[i].on = on;
    }
    spin_unlock_irqrestore(&status_lock, irqflags);
}
EXPORT_SYMBOL_GPL(bci_status_set);

static ssize_t bci_status_on_show_attrs(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    unsigned long irqflags;
    int i = 0;
    int ret = 0;
    
    spin_lock_irqsave(&status_lock, irqflags);

    for(i =0; i < CUR_ESTM_ID_MAX; i++){
        if(status_table[i].on){
            ret += sprintf(buf + ret, " %s ", status_table[i].name);
        }
    }
    
    ret += sprintf(buf + ret, "\n"); 
    spin_unlock_irqrestore(&status_lock, irqflags);
    
    return ret;
}

static ssize_t bci_status_on_store_attrs(struct device *dev, struct device_attribute *attr,
                const char *buf, size_t size)
{
    if( size >= 4096 )
		return -EINVAL;

    BATDPRT("set status point %s on.\n", buf);
    
    bci_status_set(buf, 1);

    return size;
}

static ssize_t bci_status_off_show_attrs(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    unsigned long irqflags;
    int i = 0;
    int ret = 0;
    
    spin_lock_irqsave(&status_lock, irqflags);

    for(i =0; i < CUR_ESTM_ID_MAX; i++){
        if(!status_table[i].on){
            ret += sprintf(buf + ret, " %s ", status_table[i].name);
        }
    }
    
    ret += sprintf(buf + ret, "\n"); 
    
    spin_unlock_irqrestore(&status_lock, irqflags);
    
    return ret;
}

static ssize_t bci_status_off_store_attrs(struct device *dev, struct device_attribute *attr,
                const char *buf, size_t size)
{

    if( size >= 4096 )
		return -EINVAL;

    BATDPRT("set status point %s off.\n", buf);
    
    bci_status_set(buf, 0);

    return size;
}

static int bci_discharge_current(bci_charge_device_info *di)
{
    unsigned long irqflags;
    int i = 0;
    int sum = BCI_BASE_CURRENT;
    
    spin_lock_irqsave(&status_lock, irqflags);
    for(i =0; i < CUR_ESTM_ID_MAX; i++){
        if(status_table[i].on){
            sum += status_table[i].cur;
            BATDPRT("%s=%d ", status_table[i].name, status_table[i].cur);
        }
    }
    BATDPRT("\n");
    spin_unlock_irqrestore(&status_lock, irqflags);

    return sum;
}

static unsigned long bci_discharge_map(void)
{
    unsigned long irqflags;
    unsigned long map = 0;
    int i = 0;
   
    spin_lock_irqsave(&status_lock, irqflags);
    for(i =0; i < CUR_ESTM_ID_MAX; i++){
        if(status_table[i].on){
            map |= (1 << i);
        }
    }
    spin_unlock_irqrestore(&status_lock, irqflags);

    return map;
}

int bci_cap_get_sample(int vbat, const struct cap_curve *curve)
{
    int cap, i;
    const struct curve_sample *table = curve->table;

    if(vbat >= table[0].volt){
        cap = table[0].cap;
    }else if(vbat <= table[BATT_VOLT_SAMPLE_NUM-1].volt){
        cap = table[BATT_VOLT_SAMPLE_NUM-1].cap;
    }else{
        for(i = 1; i < BATT_VOLT_SAMPLE_NUM-1; i++){
            if(vbat >= table[i].volt){
                break;
            }
        }
        
         /* linear interpolation.(based on voltage) */
        cap = ( ( (table[i-1].cap - table[i].cap) * (vbat - table[i].volt) ) / (table[i-1].volt - table[i].volt) ) + table[i].cap;
    }

    return cap;
}

/*convers state id to index of the state table*/
static int id_to_idx(int id)
{
    int index = -1;

    while(id){
        id = id >> 1;
        index++;
    }
    
    return index;
}

/*Get the capacity by Vbat and Discharging current*/
static int bci_cap_discharge(bci_charge_device_info *di, int rst)
{
    int i = 0;
    int cur1 = 0, cur2 = 0;
    int cap, cap1 = 0, cap2 = 0;
    int vbat = di->mbat_param.volt;
    int discrg_cur;

    /*use the big one to caculate the capacity*/
    discrg_cur = bci_discharge_current(di);
    if(di->update_cur > discrg_cur){
        discrg_cur = di->update_cur;
    }
    /* select the proper discharge curve and get capacity by linear interpolation. */
    if(discrg_cur <= discharge_curve[0].cur){
        i = 0;
        cur2 = discrg_cur;
        cap2 = bci_cap_get_sample(vbat, &discharge_curve[i]);
    }else if(discrg_cur >= discharge_curve[DISCHRG_CURVE_NUM - 1].cur){
        i = DISCHRG_CURVE_NUM - 1;
        cur2 = discrg_cur;
        cap2 = bci_cap_get_sample(vbat, &discharge_curve[i]);
    }else{
        for(i=1; i < DISCHRG_CURVE_NUM; i++){
            if(discrg_cur < discharge_curve[i].cur){
                break;
            }
        }

        cur1 = discharge_curve[i-1].cur;
        cap1 = bci_cap_get_sample(vbat, &discharge_curve[i-1]);
    
        cur2 = discharge_curve[i].cur;
        cap2 = bci_cap_get_sample(vbat, &discharge_curve[i]);
    }

    cap = (cap2-cap1)*(discrg_cur-cur1)/(cur2-cur1) + cap1;

    BATDPRT("Current=%d,  Vbat=%d.\n", discrg_cur, vbat);
    BATDPRT("Capacity:%d, Curve:%d, Current=%d, Vbat=%d, Cap1=%d, Cap2=%d.\n", cap, i, discrg_cur, vbat, cap1, cap2);
    /*permillage*/
    return cap;
}

void static bci_charge_full_mark(bci_charge_device_info *di)
{
    if(di->mbat_param.volt >= CHARGE_FULL_VOL_1){
        di->mbat_param.status = POWER_SUPPLY_STATUS_FULL;
        di->charger_param.full = 1;
        di->charger_param.timeout = 0;
        di->charger_param.judge = 0;
        BATPRT("I am realy full\n");
    }
}

void static bci_charge_full_check(bci_charge_device_info *di)
{
    /*Really full ?*/
    if(!di->charger_param.judge){
        di->charger_param.judge = 1;
        di->charger_param.timeout = 0;
    }else{
        di->charger_param.timeout++;
        if(CHARGE_FULL_TIMES == di->charger_param.timeout){
            /*stop charge so step into complete4*/
            twl4030_charge_send_event(di, _TIMEOUT_3);
        }
        BATDPRT("Full check times=%d, required times =%d\n", di->charger_param.timeout, CHARGE_FULL_TIMES);
    }
}

void static bci_charge_full_clear(bci_charge_device_info *di)
{
     if(di->charger_param.judge){
            di->charger_param.judge = 0;
            di->charger_param.timeout = 0;
      }
}

#define SNIFFER_DEBOUNCE_TIMES (3)
/*The irq for charge plug or unplug is not stable enough, so check the VAC each time to sniff charger */
static void bci_charge_sniffer(bci_charge_device_info *di)
{
    /*first update will not stable enough to sniff*/
    int vac = di->charger_param.vac;
    int vbat = di->mbat_param.volt;
    static int plug_times = 0;
    static int unplug_times = 0;
    
    /*The smaple VAC is not stable while plugging, so we check it after off mode */
    if(vac < vbat - CHARGE_SNIFFER_VOL_GAP_2 && di->cidx > BCI_CHGS_OFF_MODE){
        if(unplug_times >= SNIFFER_DEBOUNCE_TIMES){
            unplug_times = 0;
            BATPRT("Sinffer send BCI_EVENT_CHG_UNPLUG(Vac=%d, Vbat=%d).\n", vac, vbat);
            twl4030_charge_send_event(di, _CHG_UNPLUG);
        }
        unplug_times++;
        plug_times = 0;
    }else if(vac > vbat + CHARGE_SNIFFER_VOL_GAP_1 && !di->charger_param.online){
        if(plug_times >= SNIFFER_DEBOUNCE_TIMES){
            plug_times = 0;
            BATPRT("Sinffer send BCI_EVENT_CHG_PLUG(Vac=%d, Vbat=%d).\n", vac, vbat);
            twl4030_charge_send_event(di, _CHG_PLUG);
        }
        plug_times++;
        unplug_times = 0;
    }else{
        plug_times = unplug_times = 0;
    }
}

static int bci_cap_charge(bci_charge_device_info *di, int rst)
{
    int cap = 250;
    int vbat = di->mbat_param.volt;
    int ichg = di->charger_param.ichg;

    cap = bci_cap_get_sample(vbat, &charge_curve[0]);

    /*judge full after BCI_CHGS_QUK_CHG_2*/
    if(di->cidx < id_to_idx(BCI_CHGS_QUK_CHG_2)){
        goto done;
    }

    spin_lock(&di->lock);
    
    /*Voltage or Current has been fit to judge full*/
    if(vbat >= CHARGE_FULL_VOL_1 && ichg <= CHARGE_FULL_CUR_1 && ichg > 0){
        bci_charge_full_check(di);
    }if(vbat >= CHARGE_FULL_VOL_2 && ichg <= CHARGE_FULL_CUR_2 && ichg > 0){
        bci_charge_full_check(di);
    }else{
       /* Once ichg is more or vbat is lower, recheck full*/
       bci_charge_full_clear(di);
    }
    spin_unlock(&di->lock);

done:
    /*permillage*/
    return cap;
}

/*
 * Sets and clears bits on an given register on a given module
 */
static inline int clear_n_set(u8 mod_no, u8 clear, u8 set, u8 reg)
{
	int ret;
	u8 val = 0;

	/* Gets the initial register value */
	ret = twl_i2c_read_u8(mod_no, &val, reg);
	if (ret)
		return ret;

	/* Clearing all those bits to clear */
	val &= ~(clear);

	/* Setting all those bits to set */
	val |= set;

	/* Update the register */
	ret = twl_i2c_write_u8(mod_no, val, reg);
	if (ret)
		return ret;

	return 0;
}

/*
 * Enable/Disable AC AUTO Charge funtionality.
 */
static int twl4030_charge_en_autoac(int enable)
{
	int ret;

	if (enable) {
		/* forcing the field BCIAUTOAC (BOOT_BCI[0) to 1 */
		ret = clear_n_set(TWL4030_MODULE_PM_MASTER, BCIAUTOWEN,
			    (CONFIG_DONE | BCIAUTOAC),
			    REG_BOOT_BCI);
		if (ret)
			return ret;
	} else {
		/* forcing the field BCIAUTOAC (BOOT_BCI[0) to 0*/
		ret = clear_n_set(TWL4030_MODULE_PM_MASTER, BCIAUTOWEN | BCIAUTOAC,
			    CONFIG_DONE,
			    REG_BOOT_BCI);
		if (ret)
			return ret;
	}
    
	return 0;
}

/*
 * Enable/Disable USB AUTO Charge funtionality.
 */
static int twl4030_charge_en_autousb(int enable)
{
	int ret;

	if (enable) {
		/* forcing the field BCIAUTOAC (BOOT_BCI[0) to 1 */
		ret = clear_n_set(TWL4030_MODULE_PM_MASTER, BCIAUTOWEN,
			    (CONFIG_DONE | BCIAUTOUSB),
			    REG_BOOT_BCI);
		if (ret)
			return ret;
	} else {
		/* forcing the field BCIAUTOAC (BOOT_BCI[0) to 0*/
		ret = clear_n_set(TWL4030_MODULE_PM_MASTER, BCIAUTOWEN |BCIAUTOUSB,
			    CONFIG_DONE,
			    REG_BOOT_BCI);
		if (ret)
			return ret;
	}
    
	return 0;
}

/*
 * Enable/Disable Constant Voltage mode.
 */
static int twl4030_charge_cv_en(int enable)
{
	int ret;

	if (enable) {
		/* forcing the field BCIAUTOAC (BOOT_BCI[0) to 1 */
		ret = clear_n_set(TWL4030_MODULE_PM_MASTER, BCIAUTOWEN,
			    (CONFIG_DONE | CVENAC),
			    REG_BOOT_BCI);
		if (ret)
			return ret;
	} else {
		/* forcing the field BCIAUTOAC (BOOT_BCI[0) to 0*/
		ret = clear_n_set(TWL4030_MODULE_PM_MASTER, BCIAUTOWEN | CVENAC ,
			    CONFIG_DONE,
			    REG_BOOT_BCI);
		if (ret)
			return ret;
	}
    
	return 0;
}

/*
 * check whether CV mode is enabled.
 */
static int twl4030_charge_cv_check(void)
{
       u8 val = 0;
	int ret = 0;

       ret = twl_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &val, REG_BOOT_BCI);
	if (ret)
		return DISABLE;

       if(val & CVENAC){
            ret = ENABLE;
       }else{
            ret = DISABLE;
       }
       
       return ret;     
}

/*
 * Enable/Disable AC PATH Charge funtionality in software-controlled mode.
 */
static int twl4030_charge_en_acpath(int enable)
{
    int ret;

    /* firstly must set to be OFFMODE*/
    ret = twl_i2c_write_u8(TWL4030_MODULE_MAIN_CHARGE, 
                EKEY_OFFMODE,
                REG_BCIMDKEY);

    if(ret){
       return ret;
    }
   
    if (enable) {
        ret = twl_i2c_write_u8(TWL4030_MODULE_MAIN_CHARGE, 
                    EKEY_ACPATH,
                    REG_BCIMDKEY);

        if (ret){
            return ret;
        }
    }

    return 0;
}

/*
 * Enable/Disable AC PATH Charge funtionality in software-controlled mode.
 */
static int twl4030_charge_en_usbpath(int enable)
{
    int ret;

    /* firstly must set to be OFFMODE*/
    ret = twl_i2c_write_u8(TWL4030_MODULE_MAIN_CHARGE, 
                EKEY_OFFMODE,
                REG_BCIMDKEY);

    if(ret){
        return ret;
    }
   
   if (enable) {
        ret = twl_i2c_write_u8(TWL4030_MODULE_MAIN_CHARGE, 
                    EKEY_USBPATH,
                    REG_BCIMDKEY);

        if (ret){
            return ret;
        }
    }

    return 0;
}

/*
 * set watchdog which is auto enable when LINECHEN has been set .
 */
static int twl4030_charge_set_watchdog(u8 cmd)
{

    return twl_i2c_write_u8(TWL4030_MODULE_MAIN_CHARGE,
			      cmd,
			      REG_WDKEY);
}

/*check whether any charged has been plugged*/
static int twl4030_charge_plugged(void)
{
    int ret = 0;
    u8 chg_sts;

    /* read charger power supply status */
    ret = twl_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &chg_sts,
                REG_STS_HW_CONDITIONS);

    if (ret < 0){
        BATPRT("%s: Fail to read REG_STS_HW_CONDITIONS.\n", __FUNCTION__);
        return ret;
    }

    if (chg_sts & STS_CHG) { 
    /* If the AC charger have been connected */
        ret = 1;
    }

    return ret;
}

/*
 * Enable/Disable hardware charger presence event notifications.
 * the irq will be trigged whenever charger state has been changed, 
 * but we have to check STS_CHG to judge whether the state is plugged or unplugged  
 */
static int twl4030_charge_en_charger_presence(int enable)
{
	int ret;
	u8 pres = 0, edge = 0;

	pres |= CHG_PRES;
	edge |= (CHG_PRES_RISING | CHG_PRES_FALLING);

	if (enable) {
		/* unmask USB_PRES and CHG_PRES interrupt for INT1 */
		ret = clear_n_set(TWL4030_MODULE_INT, pres, 0, REG_PWR_IMR1);
		if (ret)
		    return ret;
		/*
		 * configuring interrupt edge detection
		 * for USB_PRES and CHG_PRES
		 */
		ret = clear_n_set(TWL4030_MODULE_INT, 0, edge, REG_PWR_EDR1);
		if (ret)
		    return 0;
	} else {
		/* mask USB_PRES and CHG_PRES interrupt for INT1 */
		ret = clear_n_set(TWL4030_MODULE_INT, 0, pres, REG_PWR_IMR1);
		if (ret)
		    return ret;
	}

	return 0;
}

static int twl4030_charge_stop(bci_charge_device_info *di)
{
    int ret = 0;

    ret += twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_STOP);
    ret += twl4030_charge_set_watchdog(WDKEY_STOP);
    ret += twl4030_charge_en_acpath(DISABLE);
    ret += twl4030_charge_cv_en(DISABLE);
    ret += twl4030_presacler_en_mesvbat(ENABLE);
    ret += twl4030_presacler_en_mesvac(ENABLE);

    BATDPRT("stop charge.\n");
    mdelay(10);
    
    del_timer(&di->timer1);
    del_timer(&di->timer2);
    del_timer(&di->timer3);

    if(di->mbat_param.status != POWER_SUPPLY_STATUS_FULL){
        di->mbat_param.status = POWER_SUPPLY_STATUS_DISCHARGING;
    }

    if(ret < 0){
        BATPRT("error when twl4030 stop charger.\n");
    }
    return ret;
    
}

static int twl4030_charge_clear_ref(bci_charge_device_info *di)
{
    int ret = 0;

    ret += twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_STOP);
    ret += twl4030_charge_set_watchdog(WDKEY_STOP);
    twl4030_monitor_reset_update(di, RST_UPDATE_LV1);
    if(ret < 0){
        BATPRT("error when twl4030 clear ref.\n");
    }
    return ret;
}

static int twl4030_charge_start(bci_charge_device_info *di)
{
    int ret = 0;

    ret += twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_LOW);
    ret += twl4030_monitor_chgv_ref_set(di, VCHG_LEVEL_START);
    ret += twl4030_charge_en_acpath(ENABLE);
    mdelay(10);

    /*FIXME: maybe watchdog is need when host is crash, who can stop charging*/
    ret += twl4030_charge_set_watchdog(WDKEY_STOP);

    del_timer(&di->timer1);
    del_timer(&di->timer2);
    del_timer(&di->timer3);
    
    di->charger_param.timeout = 0;
    di->charger_param.judge = 0;
    di->charger_param.full = 0;
 
    if(di->mbat_param.status != POWER_SUPPLY_STATUS_FULL){
        di->mbat_param.status = POWER_SUPPLY_STATUS_CHARGING;
    }

    if(ret < 0){
        BATPRT("error when twl4030 start charger.\n");
    }
    return ret;
    
}

static int  twl4030_charge_send_event(bci_charge_device_info *di, bci_charge_event_id id)
{
    int ret = 0;
    unsigned long flags;
    bci_charge_state_dsp *dsp = NULL;
    bci_charge_event *event = NULL;

    if(di->monitor_thread == NULL || di->process_thread == NULL){
        return ret;
    }
    
    spin_lock_irqsave(&di->lock, flags);
    dsp = state_table + di->cidx;
    
    /*check whether the event is cared by current charge state*/
    if(dsp->mask & BCI_EVENT(id)){
        event = kzalloc(sizeof(*event), GFP_KERNEL);
        if(!event){
            BATPRT("No memory to create new event.\n");
            ret = -ENOMEM;
            goto send_event_error;
        }
        /*insert a new event to the list tail and wakeup the process thread*/
        BATDPRT("send   Event_0x%08x to State_%s.\n", id, dsp->name);
        event->id = id;
        if(BCI_EVENT(id) & BCI_CRITICAL_EVENT){
            list_add(&event->list, &di->event_q);
        }else{
            list_add_tail(&event->list, &di->event_q);
        }
        
        wake_up(&di->wait);
    }else{
        //BATDPRT("ignore Event_0x%08x to State_%s.\n", id, dsp->name);
    }
send_event_error:
    spin_unlock_irqrestore(&di->lock, flags);
    return ret;
}

static int twl4030_charge_has_event(bci_charge_device_info *di)
{
    unsigned long flags = 0;
    int ret = 0;

    if(di->monitor_thread == NULL || di->process_thread == NULL){
        return ret;
    }
     
    spin_lock_irqsave(&di->lock, flags);
    if(!list_empty(&di->event_q)){
        ret = 1;
    }
    spin_unlock_irqrestore(&di->lock, flags);

    return ret;
}

static bci_charge_event_id twl4030_charge_recv_event(bci_charge_device_info *di)
{
    unsigned long flags = 0;
    bci_charge_event *event = NULL;
    bci_charge_event_id id = 0;

    if(di->monitor_thread == NULL || di->process_thread == NULL){
        return id;
    }
     
    spin_lock_irqsave(&di->lock, flags);
    if(!list_empty(&di->event_q)){
        event = list_first_entry(&di->event_q, bci_charge_event, list);
        id = event->id;
        list_del(&event->list);
        kfree(event);
    }
    spin_unlock_irqrestore(&di->lock, flags);

    return id;
}
    
/*
 * Report and clear the charger presence event.
 */
static inline int twl4030_charge_presence(struct bci_charge_device_info *di)
{
    int ret = 0, i=0;
    u8 chg_sts, set = 0, clear = 0;

    /* read charger power supply status, sometime I2C will fail to read, so try many times */
    do{
        i++;
        ret = twl_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &chg_sts,
                REG_STS_HW_CONDITIONS);
    }while(ret < 0 && i < 3);
    /*FIXME: I2C will fail to read somtime, just try detect charge by voltage*/
    if (ret < 0){
        BATPRT("%s: Fail to read REG_STS_HW_CONDITIONS.\n", __FUNCTION__);
        goto try_detect;
    }

    if (chg_sts & STS_CHG) { /* If the AC charger have been connected */
        /* configuring falling edge detection for CHG_PRES */
        BATDPRT("IRQ: Charge plugged.\n");
        set = CHG_PRES_FALLING;
        clear = CHG_PRES_RISING;
        ret = ENABLE;
    } else { /* If the AC charger have been disconnected */
        /* configuring rising edge detection for CHG_PRES */
        BATDPRT("IRQ: Charge unplugged.\n");
        set = CHG_PRES_RISING;
        clear = CHG_PRES_FALLING;
        ret = DISABLE;
    }

    /* detect all interrupt because the state will not stable when plug or unplug*/
    clear_n_set(TWL4030_MODULE_INT, 0, clear | set, REG_PWR_EDR1);
    twl4030_presacler_en_mesvbat(ENABLE);
    twl4030_presacler_en_mesvac(ENABLE);
    /*skip the abnormal irq*/
    if( (ret == ENABLE && !di->charger_param.online) || (ret == DISABLE && di->charger_param.online)){
try_detect:
        schedule_delayed_work(&di->debounce_work, msecs_to_jiffies(CHARGE_DEBOUNCE_TIME));
    }
    return ret;
}

static void twl4030_charge_present_task(struct work_struct *work)
{
	struct bci_charge_device_info *di = container_of(work, struct bci_charge_device_info,
					       present_task);
       twl4030_charge_presence(di);
}
/*
  * enable the write to the monitor or ref register 
  */
static inline int twl4030_monitor_reg_select(u8 mfkey)
{
    return twl_i2c_write_u8(TWL4030_MODULE_MAIN_CHARGE, 
                    mfkey, 
                    REG_BCIMFKEY);
}

#if BCI_BE_ACTIVE_FUN
/*
 * check the paramters accoding to the threshold values, and generate the events if any 
 * Or < 0 on failure.
 */
#define CHECK_TIMES (4)
static int check_event[BCI_EVENT_NUM][CHECK_TIMES];
static int bci_set_event(bci_charge_event_id id)
{
    int i, idx;

    idx = id_to_idx(id);

    for(i=0; i<CHECK_TIMES; i++){
        if(check_event[idx][i] == DISABLE){
            check_event[idx][i] = ENABLE;
            i++;
            break;
        }
    }

    return i;
}
static void bci_clear_event(bci_charge_event_id id)
{
    int i, idx;

    idx = id_to_idx(id);

    for(i=0; i<CHECK_TIMES; i++){
        check_event[idx][i] = DISABLE;
    }

}

static inline bci_charge_event_id bci_check_event(bci_charge_event_id id)
{
     if(CHECK_TIMES == bci_set_event(id)){
        bci_clear_event(id);
        return id;
     }else{
        return 0;
     }
}

/*
 * Enable/Disable BCI auto monitor the VBAT level 1.
 */
static int twl4030_monitor_vbat_lv1_en(int enable)
{
    int ret;

    ret = twl4030_monitor_reg_select(MFKEY_EN1);
    if(ret){
        return ret;
    }
       
    if (enable) {
        ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                           0,
                           VBATOV1EN,
                           REG_BCIMFEN1);
        if (ret){
    	    return ret;
        }
    } else {
        ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                           VBATOV1EN,
                           0,
                           REG_BCIMFEN1);
        if (ret){
            return ret;
        }
    }

    return 0;
}

/*
 * set the value of VBAT level 1.
 */
static int twl4030_monitor_vbat_lv1_set(u8 level)
{
       int ret;

       ret = twl4030_monitor_reg_select(MFKEY_TH1);
       if(ret){
           return ret;
       }

       ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                          VBATOV1TH,
                          level << VBATOV1TH_SHIFT,
                          REG_BCIMFTH1);
	return ret;
}

/*
 * Enable/Disable BCI auto monitor the VBAT level 2.
 */
static int twl4030_monitor_vbat_lv2_en(int enable)
{
       int ret;

       ret = twl4030_monitor_reg_select(MFKEY_EN1);
       if(ret){
           return ret;
       }
       
	if (enable) {
		ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                                   0,
                                   VBATOV2EN,
                                   REG_BCIMFEN1);
		if (ret)
			return ret;
	} else {
		ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                                   VBATOV2EN,
                                   0,
                                   REG_BCIMFEN1);
		if (ret)
			return ret;
	}
    
	return 0;
}

/*
 * set the value of VBAT level 2 .
 */
static int twl4030_monitor_vbat_lv2_set(u8 level)
{
       int ret;

       ret = twl4030_monitor_reg_select(MFKEY_TH1);
       if(ret){
           return ret;
       }

       ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                          VBATOV1TH,
                          level << VBATOV1TH_SHIFT,
                          REG_BCIMFTH1);
	return ret;
}

/*
 * Enable/Disable BCI auto monitor the VBAT level 3.
 */
static int twl4030_monitor_vbat_lv3_en(int enable)
{
       int ret;

       ret = twl4030_monitor_reg_select(MFKEY_EN1);
       if(ret){
           return ret;
       }
       
	if (enable) {
		ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                                   0,
                                   VBATOV3EN,
                                   REG_BCIMFEN1);
		if (ret)
			return ret;
	} else {
		ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                                   VBATOV3EN,
                                   0,
                                   REG_BCIMFEN1);
		if (ret)
			return ret;
	}
    
	return 0;
}

/*
 * set the value of VBAT level 3.
 */
static int twl4030_monitor_vbat_lv3_set(u8 level)
{
      int ret;

      ret = twl4030_monitor_reg_select(MFKEY_TH2);
      if(ret){
           return ret;
      }

      ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                         VBATOV2TH,
                         level << VBATOV3TH_SHIFT,
                         REG_BCIMFTH2);
      return ret;
}

/*
 * Enable/Disable BCI auto monitor the VBAT level 4.
 */
static int twl4030_monitor_vbat_lv4_en(int enable)
{
        int ret;

        ret = twl4030_monitor_reg_select(MFKEY_EN1);
        if(ret){
           return ret;
        }
       
	if (enable) {
		ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                                   0,
                                   VBATOV4EN,
                                   REG_BCIMFEN1);
		if (ret)
			return ret;
	} else {
		ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                                   VBATOV4EN,
                                   0,
                                   REG_BCIMFEN1);
		if (ret)
			return ret;
	}
    
	return 0;
}

/*
 * set the value of VBAT level 4.
 */
static int twl4030_monitor_vbat_lv4_set(u8 level)
{
       int ret;

       ret = twl4030_monitor_reg_select(MFKEY_TH2);
       if(ret){
           return ret;
       }

       ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                          VBATOV2TH,
                          level << VBATOV4TH_SHIFT,
                          REG_BCIMFTH2);
       return ret;
}

/*
 * Enable/Disable BCI auto monitor the VAC overvoltage.
 */
static int twl4030_monitor_vac_ov_en(int enable)
{
        int ret;

        ret = twl4030_monitor_reg_select(MFKEY_EN2);
        if(ret){
           return ret;
        }
       
	if (enable) {
		ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                                   0,
                                   ACCHGOVEN,
                                   REG_BCIMFEN2);
		if (ret)
			return ret;
	} else {
		ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                                   ACCHGOVEN,
                                   0,
                                   REG_BCIMFEN2);
		if (ret)
			return ret;
	}
    
	return 0;
}

/*
 * set the value of VAC overvoltage.
 */
static int twl4030_monitor_vac_ov_set(u8 level)
{
       int ret;

       ret = twl4030_monitor_reg_select(MFKEY_TH3);
       if(ret){
           return ret;
       }

       ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                          ACCHGOVTH,
                          level,
                          REG_BCIMFTH3);
       return ret;
}

/*
 * Enable/Disable BCI auto monitor the VBUS overvoltage.
 */
static int twl4030_monitor_vbus_ov_en(int enable)
{
        int ret;

        ret = twl4030_monitor_reg_select(MFKEY_EN2);
        if(ret){
           return ret;
        }
       
	if (enable) {
		ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                                   0,
                                   VBUSOVEN,
                                   REG_BCIMFEN2);
		if (ret)
			return ret;
	} else {
		ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                                   VBUSOVEN,
                                   0,
                                   REG_BCIMFEN2);
		if (ret)
			return ret;
	}
    
	return 0;
}

/*
 * set the value of VBUS overvoltage.
 */
static int twl4030_monitor_vbus_ov_set(u8 level)
{
       int ret;

       ret = twl4030_monitor_reg_select(MFKEY_TH3);
       if(ret){
           return ret;
       }

       ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                          VBUSOVTH,
                          level,
                          REG_BCIMFTH3);
       return ret;
}

/*
 * Enable/Disable BCI auto monitor the VBAT overvoltage.
 */
static int twl4030_monitor_vbat_ov_en(int enable)
{
       int ret;

       ret = twl4030_monitor_reg_select(MFKEY_EN4);
       if(ret){
           return ret;
       }
       
	if (enable) {
		ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                                   0,
                                   VBATOVEN,
                                   REG_BCIMFEN4);
		if (ret)
		   return ret;
	} else {
		ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                                   VBATOVEN,
                                   0,
                                   REG_BCIMFEN4);
		if (ret)
		   return ret;
	}
    
	return 0;
}

/*
 * set the value of VBAT overvoltage.
 */
static int twl4030_monitor_vbat_ov_set(u8 level)
{
       int ret;

       ret = twl4030_monitor_reg_select(MFKEY_TH3);
       if(ret){
           return ret;
       }

       ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                          VBATOVTH,
                          level,
                          REG_BCIMFTH3);
       return ret;
}

/*
 * Enable/Disable BCI auto monitor the TBAT out of range 1 temperature.
 */
static int twl4030_monitor_tbat_or1_en(int enable)
{
       int ret;

       ret = twl4030_monitor_reg_select(MFKEY_EN2);
       if(ret){
           return ret;
       }
       
	if (enable) {
		ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                                   0,
                                   TBATOR1EN,
                                   REG_BCIMFEN2);
		if (ret)
		   return ret;
	} else {
		ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                                   TBATOR1EN,
                                   0,
                                   REG_BCIMFEN2);
		if (ret)
		   return ret;
	}
    
	return 0;
}

/*
 * set the range 1 of TBAT temperature.
 */
static int twl4030_monitor_tbat_or1_set(u8 high, u8 low)
{
	int ret;

       ret = twl4030_monitor_reg_select(MFKEY_TH4);
       if(ret){
           return ret;
       }

       ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                          TBATOR1LTH,
                          low,
                          REG_BCIMFTH4);
       if(ret){
           return ret;
       }

       ret = twl4030_monitor_reg_select(MFKEY_TH5);
       if(ret){
           return ret;
       }

       ret = clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                          TBATOR1HTH,
                          high,
                          REG_BCIMFTH5);
       if(ret){
           return ret;
       }
       
	return 0;
}

/*
 * Enable/Disable BCI auto monitor the TBAT out of range 2 temperature.
 */
static int twl4030_monitor_tbat_or2_en(int enable)
{
	int ret;

       ret = twl4030_monitor_reg_select(MFKEY_EN2);
       if(ret){
           return ret;
       }
       
	if (enable) {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           0,
			      TBATOR2EN,
			      REG_BCIMFEN2);
		if (ret)
			return ret;
	} else {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           TBATOR2EN,
			      0,
			      REG_BCIMFEN2);
		if (ret)
			return ret;
	}
    
	return 0;
}

/*
 * set the range 2 of TBAT temperature.
 */
static int twl4030_monitor_tbat_or2_set(u8 high, u8 low)
{
	int ret;

       ret = twl4030_monitor_reg_select(MFKEY_TH6);
       if(ret){
           return ret;
       }

       ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           TBATOR2LTH,
			      low,
			      REG_BCIMFTH6);
       if(ret){
           return ret;
       }

       ret = twl4030_monitor_reg_select(MFKEY_TH7);
       if(ret){
           return ret;
       }

       ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           TBATOR2HTH,
			      high,
			      REG_BCIMFTH7);
       if(ret){
           return ret;
       }
       
	return 0;
}

/*
 * Enable/Disable low threshold for charge current monitor.
 */
static int twl4030_monitor_ichg_eoc_en(int enable)
{
	int ret;

       ret = twl4030_monitor_reg_select(MFKEY_EN3);
       if(ret){
           return ret;
       }
       
	if (enable) {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           0,
			      ICHGEOCEN,
			      REG_BCIMFEN3);
		if (ret)
			return ret;
	} else {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           ICHGEOCEN,
			      0,
			      REG_BCIMFEN3);
		if (ret)
			return ret;
	}
    
	return 0;
}

/*
 * set the low threshold for charge current.
 */
static int twl4030_monitor_ichg_eoc_set(u8 level)
{
	int ret;

       ret = twl4030_monitor_reg_select(MFKEY_TH8);

       if(ret){
           return ret;
       }

       ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           ICHGEOCTH,
			      level << (ICHGEOCTH_SHIFT),
			      REG_BCIMFTH8);
       if(ret){
           return ret;
       }
       
	return 0;
}

/*
 * Enable/Disable monitor low threshold for charge current .
 */
static int twl4030_monitor_ichg_low_en(int enable)
{
	int ret;

       ret = twl4030_monitor_reg_select(MFKEY_EN3);
       
       if(ret){
           return ret;
       }
       
	if (enable) {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           0,
			      ICHGLOWEN,
			      REG_BCIMFEN3);
		if (ret)
			return ret;
	} else {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           ICHGLOWEN,
			      0,
			      REG_BCIMFEN3);
		if (ret)
			return ret;
	}
    
	return 0;
}


/*
 * set the low threshold for charge current.
 */
static int twl4030_monitor_ichg_low_set(u8 level)
{
	int ret;

       ret = twl4030_monitor_reg_select(MFKEY_TH8);

       if(ret){
           return ret;
       }

       ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           ICHGLOWTH,
			      level << (ICHGLOWTH_SHIFT),
			      REG_BCIMFTH8);
       if(ret){
           return ret;
       }
       
	return 0;
}

/*
 * Enable/Disable monitor high threshold for charge current .
 */
static int twl4030_monitor_ichg_high_en(int enable)
{
	int ret;

       ret = twl4030_monitor_reg_select(MFKEY_EN3);
       
       if(ret){
           return ret;
       }
       
	if (enable) {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           0,
			      ICHGHIGEN,
			      REG_BCIMFEN3);
		if (ret)
			return ret;
	} else {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           ICHGHIGEN,
			      0,
			      REG_BCIMFEN3);
		if (ret)
			return ret;
	}
    
	return 0;
}


/*
 * set the high threshold for charge current.
 */
static int twl4030_monitor_ichg_high_set(u8 level)
{
	int ret;

       ret = twl4030_monitor_reg_select(MFKEY_TH9_REF);

       if(ret){
           return ret;
       }

       ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           ICHGHIGHTH,
			      level << (ICHGHIGHTH_SHIFT),
			      REG_BCIMFTH9);
       if(ret){
           return ret;
       }
       
	return 0;
}

/*
 * Enable/Disable type detection .
 */
static int twl4030_monitor_type_en(int enable)
{
	int ret = 0;
       
	if (enable) {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           0,
			      TYPEN,
			      REG_BCICTL1);
		if (ret)
			return ret;
	} else {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           TYPEN,
			      0,
			      REG_BCICTL1);
		if (ret)
			return ret;
	}
    
	return 0;
}

/*
 * Enable/Disable temperature detection .
 */
static int twl4030_monitor_ith_en(int enable)
{
	int ret = 0;
       
	if (enable) {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           0,
			      ITHEN,
			      REG_BCICTL1);
		if (ret)
			return ret;
	} else {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           ITHEN,
			      0,
			      REG_BCICTL1);
		if (ret)
			return ret;
	}
    
	return 0;
}
static int bci_read_val(u8 reg)
{
	int ret, temp;
	u8 val;

	/* reading MSB */
	ret = twl_i2c_read_u8(TWL4030_MODULE_MAIN_CHARGE, &val,
		reg + 1);
	if (ret)
		return ret;

	temp = ((int)(val & BCI_MSB_VALUE_MASK)) << BCI_REG_SIZE;

	/* reading LSB */
	ret = twl_i2c_read_u8(TWL4030_MODULE_MAIN_CHARGE, &val,
		reg);
	if (ret)
		return ret;

	return temp | val;
}

/*
 * Return battery temperature
 * Or < 0 on failure.
 */
static int bci_battery_temp(int volt)
{
	u8 val;
	int temp, curr, res, ret;

	volt = (volt * TEMP_STEP_SIZE) / TEMP_PSR_R;

	/* Getting and calculating the supply current in micro ampers */
	ret = twl_i2c_read_u8(TWL4030_MODULE_MAIN_CHARGE, &val,
		 REG_BCICTL2);
	if (ret)
		return 0;

	curr = ((val & ITHSENS) + 1) * 10;

	/* Getting and calculating the thermistor resistance in ohms*/
	res = volt * 1000 / curr;

	/*calculating temperature*/
	for (temp = 58; temp >= 0; temp--) {
		int actual = therm_tbl[temp];
		if ((actual - res) >= 0)
			break;
	}

	/* Negative temperature */
	if (temp < 3) {
		if (temp == 2)
			temp = -1;
		else if (temp == 1)
			temp = -2;
		else
			temp = -3;
	}

	/* Return temp in tenths of digree celsius as per linux power class */
	return (temp + 1) * 10;
}

/*
 * set the charge voltage reference.
 */
static int twl4030_monitor_chgv_set(int level)
{
	int ret;
       u8 lsb=0, msb=0;

       ret = twl4030_monitor_reg_select(MFKEY_TH9_REF);

       if(ret){
           return ret;
       }

       lsb = (u8)(level & CHGV_LSB);
       msb = (u8)((level >> BCI_REG_SIZE) & CHGV_MSB);
       ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           CHGV_LSB,
			      lsb,
			      REG_BCIVREF1);
       if(ret){
           return ret;
       }

       ret = twl4030_monitor_reg_select(MFKEY_TH9_REF);

       if(ret){
           return ret;
       }

       ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           CHGV_MSB,
			      msb,
			      REG_BCIVREF2);
       if(ret){
           return ret;
       }
       
	return 0;
}

/*
 * get watchdog status.
 */
static int twl4030_charge_watchdog_get(u8 *status)
{
       return twl_i2c_read_u8(TWL4030_MODULE_MAIN_CHARGE, status, REG_BCIWD);
}

#endif

/*
 * Enable/Disable monitor battery presence .
 */
static int twl4030_monitor_bat_presence_en(int enable)
{
	int ret;

       ret = twl4030_monitor_reg_select(MFKEY_EN4);
       
       if(ret){
           return ret;
       }
       
	if (enable) {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           0,
			      BATSTSMCHGEN,
			      REG_BCIMFEN4);
		if (ret)
			return ret;
	} else {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           BATSTSMCHGEN,
			      0,
			      REG_BCIMFEN4);
		if (ret)
			return ret;
	}
    
	return 0;
}


/* Stop the monitor function by set to reset value, 
  * which must be used after AutoCharge has been disabled
  */
static int twl4030_stop_all_monitor(void)
{
    int ret = 0;
    
    ret += twl4030_monitor_reg_select(MFKEY_EN1);
    ret += clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                   0xFF,
                   0x00,
                   REG_BCIMFEN1);

    ret += twl4030_monitor_reg_select(MFKEY_EN2);
    ret += clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                   0xFF,
                   0x00,
                   REG_BCIMFEN2);

    ret += twl4030_monitor_reg_select(MFKEY_EN3);
    ret += clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                   0xFF,
                   0x00,
                   REG_BCIMFEN3);

    ret += twl4030_monitor_reg_select(MFKEY_EN4);
    ret += clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                   0xFF,
                   0x68,
                   REG_BCIMFEN2);

    return ret;

}

/* FIXME: must not call the function many times continuely before suspend with charging,
  * otherwise it's seen to mask all the interrupt from TLW5030, which make TWL5030 can not
  * wakeup Ap.
  */
static int twl4030_enable_vbat_monitor(int vbat)
{
     int ret = 0;
     bci_vbat_monitor level;
     int max, min, unit;
     int val, offset;
     u8 key, reg;

     BATPRT("I will back when %d mV\n", vbat);
     if(vbat > 3761){
        level = MONITOR_VBAT_LV4;
     }else if(vbat > 3029){
        level = MONITOR_VBAT_LV2;
     }else if(vbat > 2636){
        level = MONITOR_VBAT_LV1;
     }else{
        level = -1;
     }
     switch(level){
        case MONITOR_VBAT_LV1:
            key = MFKEY_TH1;
            reg = REG_BCIMFTH1;
            max = 2988;
            min = 2636;
            unit = 234;
            break;

        case MONITOR_VBAT_LV2:
            key = MFKEY_TH1;
            reg = REG_BCIMFTH1;
            max = 3732;
            min = 3029;
            unit = 468;
            break;

        case MONITOR_VBAT_LV3:
        case MONITOR_VBAT_LV4:
            key = MFKEY_TH2;
            reg = REG_BCIMFTH2;
            max = 4113;
            min = 3761;
            unit = 234;
            break;
        default:
            BATPRT("error vbat monitor number %d.\n", vbat);
            return -1;
     }

     if(vbat > max){
            vbat = max;
     }else if(vbat < min){
            vbat = min;
     }
     val = ((vbat - min) * 10  / unit) & (0x0F);
     offset = (level % 2) ? 0 : 4;

     /*set monitor VBAT*/
     ret += twl4030_monitor_reg_select(key);
     ret += clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
                   (0x0F << offset),
                   (val << offset),
                   reg);

     /*enable the monitor*/
     ret += twl4030_monitor_reg_select(MFKEY_EN1);
     ret += clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
               0xAA,
               0x03 << ((4 - level) * 2),
               REG_BCIMFEN1);

     /*set irq edge*/
     ret += clear_n_set( TWL4030_MODULE_INTERRUPTS,
                   VBATLVL_EDRFALL,
                   VBATLVL_EDRRISIN,
                   REG_BCIEDR3);

     /*unmask BCI VBATLVL_IMR1*/
     ret += clear_n_set( TWL4030_MODULE_INTERRUPTS,
                   VBATLVL_IMR1,
                   0x00,
                   REG_BCIIMR2A);

     BATDPRT("%s done\n", __FUNCTION__);
     return ret;
}

static int twl4030_disable_vbat_monitor(void)
{
     int ret = 0;

     /*enable the monitor*/
     ret += twl4030_monitor_reg_select(MFKEY_EN1);
     ret += clear_n_set( TWL4030_MODULE_MAIN_CHARGE,
               0xFF,
               0x55,
               REG_BCIMFEN1);

     /*mask BCI VBATLVL_IMR1*/
     ret += clear_n_set( TWL4030_MODULE_INTERRUPTS,
                   0x00,
                   VBATLVL_IMR1,
                   REG_BCIIMR2A);

     BATDPRT("%s done\n", __FUNCTION__);
     return ret;

}
/*
 * Enable/Disable CGAIN .
 */
static int twl4030_itov_en_gain(int enable)
{
	int ret = 0;

	if (enable) {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           0,
			      CGAIN,
			      REG_BCICTL1);
		if (ret)
			return ret;
	} else {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           CGAIN,
			      0,
			      REG_BCICTL1);
		if (ret)
			return ret;
	}

	return 0;
}

/*
 * Enable/Disable resistive divider for VBUS .
 */
static int twl4030_presacler_en_mesvbus(int enable)
{
	int ret = 0;
       
	if (enable) {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           0,
			      MESVBUS,
			      REG_BCICTL1);
		if (ret)
			return ret;
	} else {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           MESVBUS,
			      0,
			      REG_BCICTL1);
		if (ret)
			return ret;
	}
    
	return 0;
}

/*
 * Enable/Disable resistive divider for VBAT .
 */
static int twl4030_presacler_en_mesvbat(int enable)
{
	int ret = 0;
       
	if (enable) {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           0,
			      MESBAT,
			      REG_BCICTL1);
		if (ret)
			return ret;
	} else {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           MESBAT,
			      0,
			      REG_BCICTL1);
		if (ret)
			return ret;
	}
    
	return 0;
}

/*
 * Enable/Disable resistive divider for VAC .
 */
static int twl4030_presacler_en_mesvac(int enable)
{
	int ret = 0;
       
	if (enable) {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           0,
			      MESVAC,
			      REG_BCICTL1);
		if (ret)
			return ret;
	} else {
		ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           MESVAC,
			      0,
			      REG_BCICTL1);
		if (ret)
			return ret;
	}
    
	return 0;
}

/*
 * set the charge current reference.
 * the DAC is 10bit, and the bit0-bit9 has been used to measure the votalge from 750mV to 1500mV
 * which can be translated into the ICHG, so the current value in LSB is 
 *    ((1500 - 750) / (512)) /(0.44) = 3.33mA when CGAIN = 1 or
 *    ((1500 - 750) / (512)) /(0.88) = 1.665mA when CGAIN = 0 
 */
static int twl4030_monitor_chgi_ref_set(bci_charge_device_info *di, int level)
{
	int ret, temp;
       u8 lsb=0, msb=0;
	u8 val;

       BATDPRT("%s : %d.\n", __FUNCTION__, level);
       switch(level){
            case ICHG_LEVEL_STOP:
                temp = 0;
                break;
            case ICHG_LEVEL_LOW:
                temp = config_table[di->charger_param.type].chgi_low;
                break;
            case ICHG_LEVEL_HIGH:
                temp = config_table[di->charger_param.type].chgi_high;
                break;
             default:
                BATPRT("%s: invalid level %d.\n",__FUNCTION__ ,level);
                return -EINVAL;
       }
       di->charger_param.ichg_level = level;
       level = temp;
	ret = twl_i2c_read_u8(TWL4030_MODULE_MAIN_CHARGE, &val, REG_BCICTL1);
	if (ret)
		return ret;

	if (val & CGAIN){
            /* slope of 3.33 mA/LSB */
           level = (level * BCI_CHGI_REF_STEP) / (BCI_CHGI_REF_PSR_R1); 
	}
	else{
            /* slope of 1.665 mA/LSB */
	     level = (level * BCI_CHGI_REF_STEP) / (BCI_CHGI_REF_PSR_R2); 
	}

       if(level > BCI_CHGI_REF_MASK){
            level = BCI_CHGI_REF_MASK;
       }

       level |= BCI_CHGI_REF_OFF;
       
       ret = twl4030_monitor_reg_select(MFKEY_TH9_REF);

       if(ret){
           BATPRT("fail to select MFKEY_TH9_REF.\n");
           return ret;
       }

       lsb = (u8)(level & CHGI_LSB);
       msb = (u8)((level >> BCI_REG_SIZE) & CHGI_MSB);
       BATDPRT("set CHGI level:0x%x , LSB:0x%x , MSB;0x%x.\n",level, lsb, msb);
       ret = twl_i2c_write_u8(TWL4030_MODULE_MAIN_CHARGE,
			      lsb,
			      REG_BCIIREF1);
       if(ret){
           return ret;
       }

       ret = twl4030_monitor_reg_select(MFKEY_TH9_REF);

       if(ret){
           BATPRT("fail to select MFKEY_TH9_REF.\n");
           return ret;
       }
       
       ret = twl_i2c_write_u8(TWL4030_MODULE_MAIN_CHARGE,
			      msb,
			      REG_BCIIREF2);
       if(ret){
           return ret;
       }
       
	return 0;
}

static int twl4030_monitor_chgi_reg_get(u16 *val)
{
     u8 lsb=0, msb=0;
     int ret;
     
     ret = twl4030_monitor_reg_select(MFKEY_TH9_REF);

      if(ret){
           BATPRT("fail to select MFKEY_TH9_REF.\n");
           return ret;
      }

      ret = twl_i2c_read_u8(TWL4030_MODULE_MAIN_CHARGE,
			      &lsb,
			      REG_BCIIREF1);
       if(ret){
           return ret;
       }

       ret = twl4030_monitor_reg_select(MFKEY_TH9_REF);

       if(ret){
           BATPRT("fail to select MFKEY_TH9_REF.\n");
           return ret;
       }
       
       ret = twl_i2c_read_u8(TWL4030_MODULE_MAIN_CHARGE,
			      &msb,
			      REG_BCIIREF2);
       if(ret){
           return ret;
       }

       *val = (lsb | ( (msb) << BCI_REG_SIZE));
       //BATDPRT("get CHGI REG: LSB=0x%x , MSB=0x%x.\n",lsb, msb);
       return ret;
}

static int twl4030_monitor_chgi_reg_set(u16 val)
{
     u8 lsb=0, msb=0;
     int ret = 0;

     lsb = (u8)(val & CHGI_LSB);
     msb = (u8)(val >> BCI_REG_SIZE);
     ret = twl4030_monitor_reg_select(MFKEY_TH9_REF);

      ret = twl_i2c_write_u8(TWL4030_MODULE_MAIN_CHARGE,
			      lsb,
			      REG_BCIIREF1);
       if(ret){
           return ret;
       }

       ret = twl4030_monitor_reg_select(MFKEY_TH9_REF);

       if(ret){
           BATPRT("fail to select MFKEY_TH9_REF.\n");
           return ret;
       }
       
       ret = twl_i2c_write_u8(TWL4030_MODULE_MAIN_CHARGE,
			      msb,
			      REG_BCIIREF2);
       if(ret){
           return ret;
       }
       
       //BATDPRT("Set CHGI REG: LSB=0x%x , MSB=0x%x.\n",lsb, msb);
       
       return ret;
}
/*
 * set the charge current reference.
 * the DAC is 10bit, and the bit0-bit9 has been used to measure the votalge from 750mV to 1500mV
 * which can be translated into the CHGV, so the voltage value in LSB is 
 *    ((1500 - 750) / (512)) /(0.25) = 5.85mV, 0.25 is the precaler ratio.
 */
static int twl4030_monitor_chgv_ref_set(bci_charge_device_info *di, int level)
{
	int ret, temp;
       u8 lsb=0, msb=0;
       BATDPRT("%s : %d.\n", __FUNCTION__, level);
       switch(level){
            case VCHG_LEVEL_STOP:
                temp = 0;
                break;
            case VCHG_LEVEL_START:
                temp = config_table[di->charger_param.type].chgv;
                break;
             default:
                BATPRT("%s: invalid level %d.\n", __FUNCTION__, level);
                return -EINVAL;
       }
       di->charger_param.vchg_level = level;
	level = temp;
       level -= BCI_CHGV_REF_BASE;
       if(level <= 0){
           level = BCI_CHGV_REF_BASE;
       }
       
       level = (level * BCI_CHGV_REF_STEP) / (BCI_CHGV_REF_PSR_R) ;

       if(level > BCI_CHGV_REF_MASK){
            level = BCI_CHGV_REF_MASK;
       }

       level |= BCI_CHGV_REF_OFF;
       
       ret = twl4030_monitor_reg_select(MFKEY_TH9_REF);
       if(ret){
           return ret;
       }

       lsb = (u8)(level & CHGV_LSB);
       msb = (u8)((level >> BCI_REG_SIZE) & CHGV_MSB);
       ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           CHGV_LSB,
			      lsb,
			      REG_BCIVREF1);
       if(ret){
           return ret;
       }

       ret = twl4030_monitor_reg_select(MFKEY_TH9_REF);
       if(ret){
           return ret;
       }
       
       ret = clear_n_set(TWL4030_MODULE_MAIN_CHARGE,
                           CHGV_MSB,
			      msb,
			      REG_BCIVREF2);
       if(ret){
           return ret;
       }
       
	return 0;
}

/*Only one ADC channel can be get by this function*/
static int twl4030_monitor_get_adc(bci_madc_ch channel)
{
    int ret;
    struct twl4030_madc_request req;

    memset(&req, 0, sizeof(req));

    req.channels = (1 << channel);
    req.type = TWL4030_MADC_WAIT;
    req.do_avg = 1;
    req.method = TWL4030_MADC_SW1;
    req.active = 0;
    req.func_cb = NULL;
    ret = twl4030_madc_conversion(&req);
    if(ret < 0){
        BATPRT("Fail to conversion the bci parameter %d.\n", ret);
        return ret;
    }

    return  req.rbuf[channel];
}

static int twl4030_monitor_init_params(bci_charge_device_info *di)
{
    int ret = 0;
    int try = 10;
    struct twl4030_madc_request req;

    while(try--)
    {
        memset(&req, 0, sizeof(req));

        req.channels = (1 << MADC_CH_BTYPE)  |   /*Battery type*/
                               (1 << MADC_CH_BTEMP) |   /*Battery temperature*/
                               (1 << MADC_CH_VBUS)   |  /*VBUS: usb voltage*/
                               (1 << MADC_CH_VBBAT) |  /*VBKP:Backup battery voltage*/
                               (1 << MADC_CH_ICHG)   |  /*ICHG:Battery charger current*/
                               (1 << MADC_CH_VAC)    |  /*VAC:Battery charger voltage*/
                               (1 << MADC_CH_VBAT) ;    /*VBAT:main battery voltage*/


        req.type = TWL4030_MADC_WAIT;
        req.do_avg = 1;
        req.method = TWL4030_MADC_SW1;
        req.active = 0;
        req.func_cb = NULL;
        ret = twl4030_madc_conversion(&req);
        if(ret < 0){
            BATPRT("Fail to conversion the bci parameter %d.\n", ret);
            continue;
        }else{
            break;
        }
    }

    di->mbat_param.volt = bci_battery_voltage(req.rbuf[MADC_CH_VBAT]);
#ifdef BCI_SUPPORT_TEMP
    di->mbat_param.temp = bci_battery_temp(req.rbuf[MADC_CH_BTEMP]);
#endif
    di->bbat_param.volt = bci_backupbatt_voltage(req.rbuf[MADC_CH_VBBAT]);
    di->charger_param.ichg = bci_charge_current(req.rbuf[MADC_CH_ICHG]);
    di->charger_param.vac = bci_ac_voltage(req.rbuf[MADC_CH_VAC]);
    di->charger_param.vbus = bci_usb_voltage(req.rbuf[MADC_CH_VBUS]);
    di->mbat_param.cap = bci_battery_cap(di, RST_UPDATE_LV2);
    
    BATDPRT("INIT value: ichg:%d, vbat:%d, vac:%d, vbus:%d, cap:%d.\n", di->charger_param.ichg, 
                             di->mbat_param.volt, di->charger_param.vac, di->charger_param.vbus, di->mbat_param.cap);
    return ret;
}
/*
 * update the parameters of charge and battery 
 * Or < 0 on failure.
 */
static int twl4030_avg_caculate(int *p, int num)
{
    int i=0, j=0;
    int idx;
    int sum=0;
    int temp = 0;

    if(num <= 0){
        return 0;
    }

    for(i=0; i<num; i++){
        if(*(p+i) < 0){
            *(p+i) = 0;
        }
    }

    if(num == 1){
        return *p;
    }else if(num == 2){
        return (*p + *(p+1)) >> 1;
    }else{
        /*sort the data by values increasingly*/
        for(i = 0; i < num; i++){
            for(j = i+1; j < num; j++){
                if(p[i] > p[j]){
                    temp = p[i];
                    p[i] = p[j];
                    p[j] = temp;
                }
            }
        }

        temp = p[num>>1];
        idx = num >> 1;
        i = idx - (AVG_NUM >> 1);
        j = i + AVG_NUM;
        /*caculate the average*/
        if(i > 0 && j < num){
            sum = 0;
            idx = i;
            for(i=0; i < AVG_NUM; i++){
                sum += p[idx + i];
            }

            temp = sum / AVG_NUM ;
        }
  
        return temp;
    }
}
 
static void twl4030_monitor_event_generate(bci_charge_device_info *di)
{
    int i = 0;
    int vbat = 0;
    int ichg = 0;
    int vac = 0;
#ifdef BCI_SUPPORT_TEMP
    int temp = 0;
#endif
    unsigned long long  mask = 0;
    charge_type type = 0;
    bci_charge_config *pfg = NULL;

    if(!di->monitor_thread || !di->process_thread){
        return ;
    }

    /*not be in charge, do nothing*/
    if(di->cidx == id_to_idx(BCI_CHGS_NO_CHARGING_DEV)){
        return ;
    }

    type = di->charger_param.type;
    pfg = &(config_table[type]);

    /*the event according to VBAT*/
    vbat = di->mbat_param.volt;
    if(vbat <= pfg->vbat_th_1){
        mask |= BCI_EVENT(_VBAT_DOWN_TH_1);
        mask |= BCI_EVENT(_VBAT_DOWN_TH_2);
        mask |= BCI_EVENT(_VBAT_DOWN_TH_3);
        mask |= BCI_EVENT(_VBAT_DOWN_TH_4);
        mask |= BCI_EVENT(_VBAT_DOWN);
    }else if(vbat <= pfg->vbat_th_2){
        mask |= BCI_EVENT(_VBAT_OVER_TH_1);
        mask |= BCI_EVENT(_VBAT_DOWN_TH_2);
        mask |= BCI_EVENT(_VBAT_DOWN_TH_3);
        mask |= BCI_EVENT(_VBAT_DOWN_TH_4);
        mask |= BCI_EVENT(_VBAT_DOWN);
    }else if(vbat <= pfg->vbat_th_3){
        mask |= BCI_EVENT(_VBAT_OVER_TH_1);
        mask |= BCI_EVENT(_VBAT_OVER_TH_2);
        mask |= BCI_EVENT(_VBAT_DOWN_TH_3);
        mask |= BCI_EVENT(_VBAT_DOWN_TH_4);
        mask |= BCI_EVENT(_VBAT_DOWN);
    }else if(vbat <= pfg->vbat_th_4){
        mask |= BCI_EVENT(_VBAT_OVER_TH_1);
        mask |= BCI_EVENT(_VBAT_OVER_TH_2);
        mask |= BCI_EVENT(_VBAT_OVER_TH_3);
        mask |= BCI_EVENT(_VBAT_DOWN_TH_4);
        mask |= BCI_EVENT(_VBAT_DOWN);
    }else if(vbat <= pfg->vbat_over){
        mask |= BCI_EVENT(_VBAT_OVER_TH_1);
        mask |= BCI_EVENT(_VBAT_OVER_TH_2);
        mask |= BCI_EVENT(_VBAT_OVER_TH_3);
        mask |= BCI_EVENT(_VBAT_OVER_TH_4);
        mask |= BCI_EVENT(_VBAT_DOWN);
    }else{
        mask |= BCI_EVENT(_VBAT_OVER_TH_1);
        mask |= BCI_EVENT(_VBAT_OVER_TH_2);
        mask |= BCI_EVENT(_VBAT_OVER_TH_3);
        mask |= BCI_EVENT(_VBAT_OVER_TH_4);
        mask |= BCI_EVENT(_VBAT_OVER);
    }

    /*the event according to VAC*/
    vac = di->charger_param.vac;
    if(vac <= pfg->vac_over){
        mask |= BCI_EVENT(_VAC_DOWN);
    }else{
        mask |= BCI_EVENT(_VAC_OVER);
    }

    /*the event according to ICHG*/
    ichg = di->charger_param.ichg;
    if(ichg < 0){
        /*is discharging, no evne need be sent*/
    }else if(ichg <= pfg->ichg_th_1){
        mask |= BCI_EVENT(_ICHG_DOWN_TH_1);
        mask |= BCI_EVENT(_ICHG_DOWN_TH_2);
        mask |= BCI_EVENT(_ICHG_DOWN);
    }else if(ichg <= pfg->ichg_th_2){
        mask |= BCI_EVENT(_ICHG_OVER_TH_1);
        mask |= BCI_EVENT(_ICHG_DOWN_TH_2);
        mask |= BCI_EVENT(_ICHG_DOWN);
    }else if(ichg <= pfg->ichg_over){
        mask |= BCI_EVENT(_ICHG_OVER_TH_1);
        mask |= BCI_EVENT(_ICHG_OVER_TH_2);
        mask |= BCI_EVENT(_ICHG_DOWN);
    }else{
        mask |= BCI_EVENT(_ICHG_OVER_TH_1);
        mask |= BCI_EVENT(_ICHG_OVER_TH_2);
        mask |=BCI_EVENT(_ICHG_OVER);
    }

#ifdef BCI_SUPPORT_TEMP
    /*the event according to TEMPERATURE*/
    temp = di->mbat_param.temp;
    if(temp<= pfg->temp_normal_high && temp >= pfg->temp_normal_low){
        mask |= BCI_EVENT(_TEMP_DOWN);
    }else if(temp <= pfg->temp_over_low || temp >= pfg->temp_over_high){
        mask |= BCI_EVENT(_TEMP_OVER);
    }
#endif

    /*check whether the event is valid in current charge state*/
    for(i = 0; i < BCI_EVENT_NUM; i++){
        if(mask & BCI_EVENT(i)){
            twl4030_charge_send_event(di, i); 
        }
    }
}

static int twl4030_monitor_update_params(bci_charge_device_info *di)
{
    int ret = 0;
    u16 icharge=0;
    struct twl4030_madc_request req;
    static int mvbat_avg[AVG_NUM];
    static int bvbat_avg[AVG_NUM];
    static int ichg_avg[AVG_NUM];
    static int vac_avg[AVG_NUM];
    static int vbus_avg[AVG_NUM];
#ifdef BCI_SUPPORT_TEMP
    static int temp_avg[AVG_NUM];
#endif
    static int count = 0;
    static int index = 0;
    static int update = 0;

    /*Reupdate the patameters at once */
    if(di->update_reset > RST_UPDATE_NONE){
        index = 0;
        count = 0;
        if(!di->cap_init || di->update_reset < update){
            /*LV2 is priorer than LV1*/
            di->update_reset = RST_UPDATE_NONE;
            return 0;
        }
        update = di->update_reset;
        di->update_reset = RST_UPDATE_NONE;
        return 0;
    }

    memset(&req, 0, sizeof(req));
    req.type = TWL4030_MADC_WAIT;
    req.do_avg = 1;
    req.method = TWL4030_MADC_SW1;
    req.active = 0;
    req.func_cb = NULL;

    if(!di->charger_param.online){
         req.channels = (1 << MADC_CH_BTYPE)  |   /*Battery type*/
                           (1 << MADC_CH_BTEMP) |   /*Battery temperature*/
                           (1 << MADC_CH_VBUS)   |  /*VBUS: usb voltage*/
                           (1 << MADC_CH_VBBAT) |  /*VBKP:Backup battery voltage*/
                           (1 << MADC_CH_ICHG)   |  /*ICHG:Battery charger current*/ 
                           (1 << MADC_CH_VAC)    |  /*VAC:Battery charger voltage*/
                           (1 << MADC_CH_VBAT) ;    /*VBAT:main battery voltage*/

        ret = twl4030_madc_conversion(&req);
        if(ret < 0){
              BATPRT("Fail to conversion the bci parameter %d.\n", ret);
              return ret;
        }
    }else{
        req.channels = (1 << MADC_CH_ICHG);
        ret = twl4030_madc_conversion(&req);
        if(ret < 0){
               BATPRT("IGHG Fail to conversion the bci parameter %d.\n", ret);
               return ret;
        }
        ret = twl4030_monitor_chgi_reg_get(&icharge);
        if(ret < 0){
               BATPRT("Fail to get ICHG Reg %d.\n", ret);
               return ret;
        }
        //set ICHG to be 0mA
        twl4030_monitor_chgi_reg_set(0x200);
        mdelay(10);
        req.channels = (1 << MADC_CH_BTYPE)  |   /*Battery type*/
                           (1 << MADC_CH_BTEMP) |   /*Battery temperature*/
                           (1 << MADC_CH_VBUS)   |  /*VBUS: usb voltage*/
                           (1 << MADC_CH_VBBAT) |  /*VBKP:Backup battery voltage*/
                           (1 << MADC_CH_VAC)    |  /*VAC:Battery charger voltage*/
                           (1 << MADC_CH_VBAT) ;    /*VBAT:main battery voltage*/
        ret = twl4030_madc_conversion(&req);
        twl4030_monitor_chgi_reg_set(icharge);
        if(ret < 0){
               BATPRT("VBAT Fail to conversion the bci parameter %d.\n", ret);
               return ret;
        }
    }

    mvbat_avg[index] = bci_battery_voltage(req.rbuf[MADC_CH_VBAT]);
    bvbat_avg[index] = bci_backupbatt_voltage(req.rbuf[MADC_CH_VBBAT]);
    ichg_avg[index] = bci_charge_current(req.rbuf[MADC_CH_ICHG]);
    vac_avg[index] = bci_ac_voltage(req.rbuf[MADC_CH_VAC]);
    vbus_avg[index] = bci_usb_voltage(req.rbuf[MADC_CH_VBUS]);
#ifdef BCI_SUPPORT_TEMP
    temp_avg[index] = bci_battery_temp(req.rbuf[MADC_CH_BTEMP]);
#endif
    
    count++;
    if(count >= AVG_NUM){
        di->mbat_param.volt = twl4030_avg_caculate(mvbat_avg, AVG_NUM);
#ifdef BCI_SUPPORT_TEMP
        di->mbat_param.temp = twl4030_avg_caculate(temp_avg, AVG_NUM);
#endif
        di->bbat_param.volt = twl4030_avg_caculate(bvbat_avg, AVG_NUM);
        di->charger_param.ichg = twl4030_avg_caculate(ichg_avg, AVG_NUM);
        di->charger_param.vbus = twl4030_avg_caculate(vbus_avg, AVG_NUM);
        /*Update the parameters to be dangerous value, not average value*/
        if(di->cidx > BCI_CHGS_OFF_MODE && vac_avg[index]  >= config_table[di->charger_param.type].vac_over){
            di->charger_param.vac = vac_avg[index];
            BATPRT("VAC is %d over voltage %d\n", vac_avg[index], config_table[di->charger_param.type].vac_over);
        }else{
            di->charger_param.vac = twl4030_avg_caculate(vac_avg, AVG_NUM);
        }

#if defined(BCI_LEVEL_WAKEUP_DEBUG)
       if(ldbg_full){
            di->mbat_param.volt = ldbg_volt;
            di->charger_param.ichg = ldbg_ichg;
       }
#endif
        bci_battery_update_cap(di, update);
        bci_charge_sniffer(di);
        /*the update reset has already been completed*/
        if(update){
            update = RST_UPDATE_NONE;
            di->update_cur = 0;
            di->update_cur_map = 0;
            di->update_gab = BCI_PARAM_UPDATE_INTERVAL;
            BATDPRT("Reset Update VBat=%d, VAC=%d, IChg=%d, Cap=%d\n", \
                   di->mbat_param.volt, di->charger_param.vac, di->charger_param.ichg, di->mbat_param.cap);
        }

        /*send event after the all parameters was ready again*/
        twl4030_monitor_event_generate(di);
        
    }

    index++;
    index = index % AVG_NUM;

    return ret;
}

/* update the parameter again using new sample value.
  * avoid averaging parameters by the middle value in history
  */
static void twl4030_monitor_reset_update(bci_charge_device_info *di, bci_rst_update level)
{
    if(di->monitor_thread){ 
        di->update_reset = level;
        switch(level){
            case RST_UPDATE_LV2:
                di->update_gab = BCI_CAP_UPATE_INTERVAL;
                /*record discharge current to calculate the capacity, otherwise the current might be changed by App*/
                di->update_cur = bci_discharge_current(di);
                di->update_cur_map = bci_discharge_map();
                break;
            case RST_UPDATE_LV1:
                di->update_gab = BCI_PARAM_RESET_INTERVAL;
                break;
            default:
                BATPRT("unknow update reset level %d.\n", di->update_reset);
        }
        wake_up_process(di->monitor_thread);
    }
}
/*
 * Settup the twl4030 BCI module to enable backup
 * battery charging.
 */
static int bci_backupbatt_charge_setup(void)
{
	int ret = 0;

       /* Starting backup batery charge */
	ret = clear_n_set( TWL4030_MODULE_PM_RECEIVER, 
	                              0xFF, 
	                              BBCHEN | BBSEL_3200mV | BBISEL_150uA,
		                       REG_BB_CFG);
       
	return ret;
}

/*
 * Return the battery backup voltage
 * Or < 0 on failure.
 */
static int bci_backupbatt_voltage(int volt)
{
	return  (volt * BK_VOLT_STEP_SIZE) / BK_VOLT_PSR_R;
}

static void bci_battery_cap_full_check(bci_charge_device_info *di)
{
   unsigned long flags = 0;
   int vbat, ichg;
    
    /*judge full after BCI_CHGS_QUK_CHG_2*/
   if(di->cidx < id_to_idx(BCI_CHGS_QUK_CHG_2)){
        return ;
   }

   spin_lock_irqsave(&di->lock, flags);
   if(di->charger_param.online){
        vbat = di->mbat_param.volt;
        ichg = di->charger_param.ichg;
        /*Voltage or Current has been fit to judge full*/
        if(vbat >= CHARGE_FULL_VOL_1 && ichg <= CHARGE_FULL_CUR_1 && ichg > 0){
            bci_charge_full_check(di);
        }if(vbat >= CHARGE_FULL_VOL_2 && ichg <= CHARGE_FULL_CUR_2 && ichg > 0){
            bci_charge_full_check(di);
        }else{
            /* Once ichg is more or vbat is lower, recheck full*/
            //bci_charge_full_clear(di);
        }
   }
   spin_unlock_irqrestore(&di->lock, flags);
}
/*
 * Return battery capacity
 * Or < 0 on failure.
 */
static int bci_battery_cap(bci_charge_device_info *di, int update)
{
   int cap;

   bci_battery_cap_full_check(di);
   cap = bci_cap_discharge(di, update);

/*Only use the discharge curve to caculate the cap, because we get he voltage of vbat with chrage disabled*/
#if 0
#if defined(CONFIG_VIATELECOM_DISCHARGE_CURVE)
    if(di->charger_param.online){
         cap = bci_cap_charge(di, update);
    }else{
        cap = bci_cap_discharge(di, update);
    }
#else
    cap = bci_cap_charge(di, update);
#endif
#endif

    return cap;

}

 /*
 * Update the battery capacity
 */
extern void print_active_lock(void);
static void bci_battery_update_cap(bci_charge_device_info *di, int update)
{
    int cap, rcap;
    int reset = 0;
    int report = 0;
    int pcap = di->mbat_param.cap;
    int prcap = di->mbat_param.real_cap;
    int step = CAP_DISCHARGE_MAX_STEP;
    static int shutdown_count = 0;
    static int dischg_count = 0;
    static int cap_avg[CAP_AVG_NUM];
    static int idx = 0;
    int rper, pper;
    /*get the real cap according to the Vbat*/
    cap = pcap;
    rcap = bci_battery_cap(di, update);
    di->mbat_param.real_cap = rcap;

    /*caculate the average cap value*/
    switch(update){
        case RST_UPDATE_LV2:
            reset = 1;
        case RST_UPDATE_LV1:
            idx = 0;
            break;
    }

    if(reset){
        if(di->update_cur_map & (1 << CUR_ESTM_PHONE)){
            rper = 20;
            pper = 80;
        }else{
            rper = 50;
            pper = 50;
        }
        /*caculate the Cap with privilage*/
        cap = (rcap * rper + pcap * pper)  / (rper + pper);
        report = 1;
        BATPRT("reset update Cap=%d: rcap=%d(\%d), pcap=%d(\%d)\n",cap, rcap, rper, pcap, pper);
    }else{
        cap_avg[idx] = rcap;
        idx++;
        if(idx >= CAP_AVG_NUM){
            idx = 0;
            cap = twl4030_avg_caculate(cap_avg, CAP_AVG_NUM);
            report = 1;
            BATDPRT("Cap average update\n");
        }
    }

    if(report){
        print_active_lock( );
        /*update the first cap, which pcap is none*/
        if(!di->cap_init){
            di->cap_init = 1;
            di->update_cur = 0;
            di->update_gab = BCI_PARAM_UPDATE_INTERVAL;
            bci_status_set("BOOT", 0);
            BATPRT("Cap init %d\n", cap);
            goto cap_end;
        }

        if(reset){
            /*policy to update the display cap*/
            if(di->charger_param.online){
                if(cap - pcap < 0){
                    cap = pcap;
                }
            }else{
                if(cap - pcap > 0){
                    cap = pcap;
                }
            }
        }else{
             /*policy to update the display cap, make the debouce cap is no more than CAP_MAX_STEP*/
            if(di->charger_param.online){
                step = CAP_CHARGE_MAX_STEP;
                /* If the voltage of battery has been discreased while charging many times, 
                  * then maybe the current of discharge was more than charge
                  */
                if(di->mbat_param.volt >= di->mbat_param.pvolt){
                    dischg_count = 0;
                }else{
                    dischg_count++;
                }
                if(cap - pcap > step){
                    cap = pcap + step;
                }else if(pcap - cap > 0){
                    if(dischg_count < 3){
                        cap = pcap;
                    }else{
                        if(pcap - cap > step){
                            cap = pcap - step;
                        }
                    }
                }
            }else{
                step = CAP_DISCHARGE_MAX_STEP;
                if(cap < 200){
                    step = 2 * CAP_DISCHARGE_MAX_STEP;
                }
                if(pcap - cap > step){
                    cap = pcap - step;
                }else if(cap - pcap > 0){
                    cap = pcap;
                }
            }
        }
        di->mbat_param.pvolt = di->mbat_param.volt;
    }

cap_end:
    /*judge whether the cap is too less to shut down*/
    if(rcap < 10){
        /*the upate times must not be too faster*/
        if(BCI_PARAM_UPDATE_INTERVAL == di->update_gab){
            shutdown_count++;
        }
        wake_lock(&di->cap_lock);
    }else{
        wake_unlock(&di->cap_lock);
        shutdown_count = 0;
    }

#ifdef CONFIG_BOOTCASE_VIATELECOM
    /*boot by charger, then the cap will always enough to work*/
    if(!strncmp(get_bootcase(), "charger", strlen("charger"))){
        if(di->charger_param.online){
            shutdown_count = 0;
            if(cap < 10){
                cap = 10;
            }
        }
    }
#endif

    if(shutdown_count > CAP_AVG_NUM){
        cap = 0;
        di->mbat_param.status = POWER_SUPPLY_STATUS_DISCHARGING;
        di->charger_param.online = 0;
    }else if(shutdown_count >= (CAP_AVG_NUM / 2)){
        cap = 10;
    }

    if(POWER_SUPPLY_STATUS_FULL == di->mbat_param.status){
        cap = 1000;
    }

    BATDPRT("online=%d, shutdown_count=%d, real_cap=%d, pre_real_cap=%d, cap=%d, pre_cap=%d.\n", di->charger_param.online, shutdown_count, rcap, prcap, cap, pcap);
    di->mbat_param.cap = cap;
}
/*check whether the battery is packed, maybe we can just use MADC to check*/
static int bci_battery_packed(void)
{
    int ret = 0;
    u8 batstsmchg, batstspchg;

    /* check for the battery presence in main charge*/
    ret = twl_i2c_read_u8(TWL4030_MODULE_MAIN_CHARGE,
    		&batstsmchg, REG_BCIMFSTS3);
    if (ret)
    	return ret;

    /* check for the battery presence in precharge */
    ret = twl_i2c_read_u8(TWL4030_MODULE_PRECHARGE,
    		&batstspchg, REG_BCIMFSTS1);
    if (ret)
    	return ret;

    if ((batstspchg & BATSTSPCHG) || (batstsmchg & BATSTSMCHG)) {
    	ret = 1;
    }

    return ret;
}

/*
 * This function handles the twl4030 battery presence interrupt
 */
static int bci_battery_presence(struct bci_charge_device_info *di)
{
	int ret;
	u8 batstsmchg, batstspchg;

	/* check for the battery presence in main charge*/
	ret = twl_i2c_read_u8(TWL4030_MODULE_MAIN_CHARGE,
			&batstsmchg, REG_BCIMFSTS3);
	if (ret)
		return ret;

	/* check for the battery presence in precharge */
	ret = twl_i2c_read_u8(TWL4030_MODULE_PRECHARGE,
			&batstspchg, REG_BCIMFSTS1);
	if (ret)
		return ret;

	/*
	 * REVISIT: Physically inserting/removing the batt
	 * does not seem to generate an int on 3430ES2 SDP.
	 */
	if ((batstspchg & BATSTSPCHG) || (batstsmchg & BATSTSMCHG)) {
		/* In case of the battery insertion event */
              BATDPRT("Battery packed.\n");
              twl4030_charge_send_event(di, _BAT_PACK);
		ret = clear_n_set(TWL4030_MODULE_INTERRUPTS, BATSTS_EDRRISIN,
			BATSTS_EDRFALLING, REG_BCIEDR2);
		if (ret)
			return ret;
	} else {
		/* In case of the battery removal event */
              BATDPRT("Battery unpacked.\n");
              twl4030_charge_send_event(di, _BAT_OPEN);
		ret = clear_n_set(TWL4030_MODULE_INTERRUPTS, BATSTS_EDRFALLING,
			BATSTS_EDRRISIN, REG_BCIEDR2);
		if (ret)
			return ret;
	}

	return 0;
}

/*
 * This function handles the twl4030 battery voltage level interrupt.
 */
static int bci_battery_level_evt(struct bci_charge_device_info *di)
{
	int ret;
	u8 mfst = 0;

	/* checking for threshold event */
	ret = twl_i2c_read_u8(TWL4030_MODULE_MAIN_CHARGE,
			&mfst, REG_BCIMFSTS2);
	if (ret)
		return ret;

#if defined(BCI_VBAT_LEVEL_WAKEUP)
	/* REVISIT could use a bitmap */
       /* FIXME: TWL5030 will not set the VBATOV bit in REG_BCIMFSTS2
         * when VBAT translate from high to low, but the irq will be generated
         */
       BATPRT("I am back for recharge dinner.\n");

       /*lock system to check VBAT, wait BCI_EVENT_VBAT_DOWN_TH_3 for recharge*/
       if(id_to_idx(BCI_CHGS_CPL_CHG_4) == di->cidx){
            wake_lock(&di->charge_lock);
       }
#if 0
	if (mfst & VBATOV4) {
		BATDPRT("Vbat down threshold4.\n");
              twl4030_charge_send_event(di, _VBAT_DOWN_TH_4);
	} else if (mfst & VBATOV3) {
		BATDPRT("Vbat down threshold3.\n");
              twl4030_charge_send_event(di, _VBAT_DOWN_TH_3);
	} else if (mfst & VBATOV2) {
		BATDPRT("Vbat down threshold2.\n");
              twl4030_charge_send_event(di, _VBAT_DOWN_TH_2);
	} else if(mfst & VBATOV1){
		BATDPRT("Vbat down threshold1.\n");
              twl4030_charge_send_event(di, _VBAT_DOWN_TH_1);
	}
#endif

#endif
	return 0;
}

/*
 * Interrupt service routine
 *
 * Attends to BCI interruptions events,
 * specifically BATSTS (battery connection and removal)
 * VBATOV (main battery voltage threshold) events
 *
 * Detect whether main battery has been inserted or removed.
 */
static irqreturn_t bci_battery_interrupt(int irq, void *di)
{
	u8 isr1a_val, isr2a_val;
	u8 clear_2a, clear_1a;
	int ret;

#ifdef CONFIG_LOCKDEP
	/* WORKAROUND for lockdep forcing IRQF_DISABLED on us, which
	 * we don't want and can't tolerate.  Although it might be
	 * friendlier not to borrow this thread context...
	 */
	local_irq_enable();
#endif

	ret = twl_i2c_read_u8(TWL4030_MODULE_INTERRUPTS, &isr1a_val,
				REG_BCIISR1A);
	if (ret)
		return IRQ_NONE;

	ret = twl_i2c_read_u8(TWL4030_MODULE_INTERRUPTS, &isr2a_val,
				REG_BCIISR2A);
	if (ret)
		return IRQ_NONE;

       /*handle the irq event*/
       BATDPRT("BCI irq: ISR1A=0x%x ISR2A=0x%x.\n", isr1a_val, isr2a_val);

	clear_2a = (isr2a_val & VBATLVL_ISR1) ? (VBATLVL_ISR1) : 0;
	clear_1a = (isr1a_val & BATSTS_ISR1) ? (BATSTS_ISR1) : 0;

	/* cleaning BCI interrupt status flags */
	ret = twl_i2c_write_u8(TWL4030_MODULE_INTERRUPTS,
			clear_1a , REG_BCIISR1A);
	if (ret)
		return IRQ_NONE;

	ret = twl_i2c_write_u8(TWL4030_MODULE_INTERRUPTS,
			clear_2a , REG_BCIISR2A);
	if (ret)
		return IRQ_NONE;

	if (isr1a_val & BATSTS_ISR1){
              /* battery connetion or removal event */
		bci_battery_presence(di);
	}
	else if (isr2a_val & VBATLVL_ISR1){
              /* battery voltage threshold event*/
		bci_battery_level_evt(di);
	}
	else{
		return IRQ_NONE;
	}

	return IRQ_HANDLED;
}

/*
 * Interrupt service routine
 *
 * Attends to TWL 4030 power module interruptions events, specifically
 * USB_PRES (USB charger presence) CHG_PRES (AC charger presence) events
 *
 */
static irqreturn_t bci_charge_interrupt(int irq, void *_di)
{
	struct bci_charge_device_info *di = (struct bci_charge_device_info *)(_di);

       wake_lock_timeout(&di->plug_lock, 10*HZ);
       schedule_work(&di->present_task);
	return IRQ_HANDLED;
}

static int  bci_init_hardware(void)
{
    int ret = 0;
    
     /*disable auto charge instead of software-controled*/
    ret += twl4030_charge_en_autoac(DISABLE);
    ret += twl4030_charge_en_autousb(DISABLE);
    ret += twl4030_charge_en_acpath(DISABLE);
    ret += twl4030_charge_en_usbpath(DISABLE);

    /*FIXME: we must set CV disable before disable itov gain, otherwise the gain can not be disabled*/
    ret += twl4030_charge_cv_en(DISABLE);

    /*Stop all monitor function in BCI to save power when use manual charge*/
    ret += twl4030_stop_all_monitor();

    /* To decrease the power consumption, we maybe need clear DEFAULT_MADC_CLK_EN, 
      * and enable the MADC manually by set HFCLKIN_FREQ before start the ADC.
      */
    ret += twl4030_presacler_en_mesvbus(ENABLE);
    ret += twl4030_presacler_en_mesvbat(ENABLE);
    ret += twl4030_presacler_en_mesvac(ENABLE);
    ret += twl4030_itov_en_gain(DISABLE);
    
    /*  BCI only monitor the battery presence, 
      *  which can taken place by using MADC to get the differnt value of battery ID 
      *  pin between packed and umpacked 
      */
    ret += twl4030_monitor_bat_presence_en(ENABLE);

    /*stop the watchdog*/
    ret += twl4030_charge_set_watchdog(WDKEY_STOP);

    /*enable the AC charege present irq in Power Managment*/
    ret += twl4030_charge_en_charger_presence(ENABLE);

    /* enabling GPCH09 for read back battery voltage */
    ret += bci_backupbatt_charge_setup();

    if(ret < 0){
        BATPRT("error in %s( ).\n", __FUNCTION__);
    }
    
    return ret;
}

static void bci_release_eventq(bci_charge_device_info *di)
{
    bci_charge_event *event = NULL;
    unsigned long flags;
    
    spin_lock_irqsave(&di->lock, flags);
    while(!list_empty(&di->event_q)){
        event = list_first_entry(&di->event_q, bci_charge_event, list);
        list_del(&event->list);
        kfree(event);
    }
    spin_unlock_irqrestore(&di->lock, flags);
}

int bci_report_charge_type(charge_type type)
{
    int ret = 0;
    struct bci_charge_device_info *di = &bci_charge_info;

    if(type < TYPE_UNKNOW || type >= CHARGE_TYPE_MAX){
        return ret;
    }

#if  defined(CONFIG_BOOTCASE_VIATELECOM) && defined(CONFIG_TWL4030_BCI_CHARGE_TYPE)
       /*disable USB when boot by charger, then do nothing*/
       if(!strncmp(get_bootcase(), "charger", strlen("charger"))){
           return 0;
       }
#endif

    if(di->charger_param.type != type){
        di->charger_param.type = type;
        twl4030_charge_send_event(di, _CHG_TYPE_REP);
        BATDPRT("report charge type: %d.\n", type);
    }

    return ret;
}

/*
 * Return VAC
 * Or < 0 on failure.
 */
static int bci_ac_voltage(int adc)
{
    return adc2volt(adc) * BCI_VAC_PSR_D / BCI_VAC_PSR_N;
}

/*
 * Return VBUS
 * Or < 0 on failure.
 */
static int bci_usb_voltage(int adc)
{
    return adc2volt(adc) * BCI_VBUS_PSR_D / BCI_VBUS_PSR_N;
}

/*
 * Return battery voltage
 * Or < 0 on failure.
 */
//#define ADTOVOL(x)       ((int)(x * 1000 / 168))
static int bci_battery_voltage(int adc)
{
    return adc2volt(adc) * BCI_VBAT_PSR_D / BCI_VBAT_PSR_N;
}

/*
 * Return the ICHG
 * Or < 0 on failure.
 */
static int bci_charge_current(int adc)
{
    int ret;
    u8 val;

    ret = twl_i2c_read_u8(TWL4030_MODULE_MAIN_CHARGE, &val,
                REG_BCICTL1);
    if (ret){
        return ret;
    }

    /*the voltage is 750mV(ADC value is 0x200) when ICHG is 0 */
    if (val & CGAIN){ /* slope of 0.44 mV/mA */
        ret = (adc2volt(adc) -750) * 100 / 44;
    }else{ /* slope of 0.88 mV/mA */
        ret = (adc2volt(adc) -750) * 100 / 88;
    }

    return ret;
}


static int bci_handle_critical_event(bci_charge_device_info *di, bci_charge_event_id id)
{
    int ret = 0;

    if(!(BCI_EVENT(id) & BCI_CRITICAL_EVENT)){
        return ret;
    }

    /*check whether the event can be processed in current charge state*/
    if(!(BCI_EVENT(id) & (state_table[di->cidx].mask))){
        return ret;
    }

    switch(id){
        case _VAC_OVER:
            BATDPRT("Critical event: %s.\n", "BCI_EVENT_VAC_OVER");
            ret = twl4030_charge_stop(di);
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_VAC_OVER);
            break;
            
        case _VBUS_OVER:
            BATDPRT("Critical event: %s.\n", "BCI_EVENT_VBUS_OVER");
            ret = twl4030_charge_stop(di);
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_VBUS_OVER);
            break;

        case _ICHG_OVER:
            BATDPRT("Critical event: %s.\n", "BCI_EVENT_ICHG_OVER");
            ret = twl4030_charge_stop(di);
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_ICHG_OVER);
            break;

        case _VBAT_OVER:
            BATDPRT("Critical event: %s.\n", "BCI_EVENT_VBAT_OVER");
            ret = twl4030_charge_stop(di);
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_OFF_MODE);
            break;

        case _CHG_UNPLUG:
            BATDPRT("Critical event: %s.\n", "BCI_EVENT_CHG_UNPLUG");
            wake_unlock(&di->charge_lock);
            /*update the ICHG*/
            ret = twl4030_charge_stop(di);

#if defined(CONFIG_TWL4030_BCI_CHARGE_TYPE)
            cancel_delayed_work(&di->ac_detect_work);
#endif

            twl4030_monitor_reset_update(di, RST_UPDATE_LV1);
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_NO_CHARGING_DEV);
            di->mbat_param.status = POWER_SUPPLY_STATUS_DISCHARGING;
            di->charger_param.full = 0;
            di->charger_param.online = DISABLE;
            BATPRT("Unplug.\n");
            di->charger_param.type = TYPE_UNKNOW;
            power_supply_changed(&di->mbat);
            power_supply_changed(&di->ac);
            power_supply_changed(&di->usb);
            break;

        case _BAT_OPEN:
            BATDPRT("Critical event: %s.\n", "BCI_EVENT_BAT_OPEN");
            twl4030_charge_stop(di);
     
            if(twl4030_charge_cv_check() && (di->charger_param.type == TYPE_AC)){
                twl4030_charge_en_acpath(ENABLE);
                twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_STOP);
                twl4030_monitor_chgv_ref_set(di, VCHG_LEVEL_START);
                di->pidx = id_to_idx(BCI_CHGS_BAT_OPEN);
                di->cidx = id_to_idx(BCI_CHGS_CV_MODE);
            }else{
                twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_STOP);
                twl4030_monitor_chgv_ref_set(di, VCHG_LEVEL_STOP);
                twl4030_charge_en_acpath(ENABLE);
                di->pidx = di->cidx;
                di->cidx = id_to_idx(BCI_CHGS_BAT_OPEN);
            }
            di->mbat_param.status = POWER_SUPPLY_STATUS_UNKNOWN;
            power_supply_changed(&di->mbat);
            power_supply_changed(&di->ac);
            power_supply_changed(&di->usb);
            break;

        case _CHG_TYPE_REP:
            BATPRT("Charger type is %d\n", di->charger_param.type);
            twl4030_monitor_chgi_ref_set(di, di->charger_param.ichg_level);
            twl4030_monitor_chgv_ref_set(di, di->charger_param.vchg_level);
            power_supply_changed(&di->mbat);
            power_supply_changed(&di->ac);
            power_supply_changed(&di->usb);
            break;
            
        default:
            BATPRT("Unknow critical Evnet_0x%08x.\n", id);
    }

    if(ret < 0){
        BATPRT("error to process critical Event_0x%08x.\n", id);
    }
    
    return ret;
}

static int bci_handle_no_charge_device(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;
    
    switch(id){
        case _CHG_PLUG:
            wake_lock(&di->charge_lock);
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_OFF_MODE);
            BATPRT("Plug.\n");
            di->charger_param.online = ENABLE;
            di->mbat_param.status = POWER_SUPPLY_STATUS_CHARGING;
            power_supply_changed(&di->mbat);
            power_supply_changed(&di->ac);
            power_supply_changed(&di->usb);
#if defined(CONFIG_TWL4030_BCI_CHARGE_TYPE)
            if(di->charger_param.type == TYPE_UNKNOW){
                wake_lock_timeout(&di->plug_lock, HZ * (CHARGE_TYPE_TIMEOUT + 1));
                schedule_delayed_work(&di->ac_detect_work, (HZ * CHARGE_TYPE_TIMEOUT));
            }
#endif
            /*check the VBAT as soon as possilbe to swith to next states*/
            twl4030_monitor_event_generate(di);
            break;
 
        default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}

static int bci_handle_off_mode(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;
    
    switch(id){
        case _VBAT_DOWN:
            twl4030_charge_start(di);
            /*update the ICHG to avoid go into QUICK_CHG_3 by error ICHG average value */
            twl4030_monitor_reset_update(di, RST_UPDATE_LV1);
            mod_timer(&di->timer3, jiffies + HZ*(config_table[di->charger_param.type].timeout_3));
            /*skip the standby mode*/
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_1);
            /*check the VBAT as soon as possilbe to swith to next states*/
            twl4030_monitor_event_generate(di);
            /*update the ICHG to avoid go into QUICK_CHG_3 by error ICHG average value */
            twl4030_monitor_reset_update(di, RST_UPDATE_LV1);
            break;

        default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}

static int bci_handle_detect_device(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;
    
    switch(id){
        case _CHG_TYPE_REP:
            break;
        default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
  
    }

    return ret;
}

static int bci_handle_constant_voltage_mode(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;

    switch(id){
        case _BAT_PACK:
            twl4030_charge_stop(di);
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_OFF_MODE);
            di->mbat_param.status = POWER_SUPPLY_STATUS_UNKNOWN;
            power_supply_changed(&di->mbat);
            power_supply_changed(&di->ac);
            power_supply_changed(&di->usb);
            break;
        default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}

static int bci_handle_battery_open_mode(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
     int ret = 0;
    
     switch(id){
        case _BAT_PACK:
            twl4030_charge_stop(di);
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_OFF_MODE);
            di->mbat_param.status = POWER_SUPPLY_STATUS_UNKNOWN;
            /*check the VBAT as soon as possilbe to swith to next states*/
            twl4030_monitor_event_generate(di);
            power_supply_changed(&di->mbat);
            power_supply_changed(&di->ac);
            power_supply_changed(&di->usb);
            break;
            
        default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}

/*Skip the standby mode*/
static int bci_handle_standby_mode(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;

    return ret;
}
static int bci_handle_vbus_over_voltage(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;
    
    switch(id){
        case _VBUS_DOWN:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_OFF_MODE);
            /*check the VBAT as soon as possilbe to swith to next states*/
            twl4030_monitor_event_generate(di);
            break;
        default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}

static int bci_handle_vac_over_voltage(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;
    
    switch(id){
        case _VAC_DOWN:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_OFF_MODE);
            /*check the VBAT as soon as possilbe to swith to next states*/
            twl4030_monitor_event_generate(di);
            break;
        default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}

static int bci_handle_ichg_over_current(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;
     
    switch(id){
        case _ICHG_DOWN:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_OFF_MODE);
            /*check the VBAT as soon as possilbe to swith to next states*/
            twl4030_monitor_event_generate(di);
            break;
        default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}

static int bci_handle_quick_charge_1(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;

    switch(id){
        case _VBAT_OVER_TH_1:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_2);
            twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_HIGH);
            break;
            
         case _TIMEOUT_3:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_CPL_CHG_4);
            bci_charge_full_mark(di);
            twl4030_charge_clear_ref(di);
            mod_timer(&di->timer1, jiffies + HZ*(config_table[di->charger_param.type].timeout_1) );
            break;

         case _TEMP_OVER:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_4);
            break;
            
         default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}

static int bci_handle_quick_charge_2(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
     int ret = 0;
     
    switch(id){
        case _VBAT_DOWN_TH_1:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_1);
            twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_LOW);
            break;

         case _TIMEOUT_3:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_CPL_CHG_4);
            bci_charge_full_mark(di);
            twl4030_charge_clear_ref(di);
            BATDPRT("quick_charge_2:Start timer1: delay=%d.\n",(config_table[di->charger_param.type].timeout_1) );
            mod_timer(&di->timer1, jiffies + HZ*(config_table[di->charger_param.type].timeout_1) );
            break;
            
         case _VBAT_OVER_TH_4:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_3);
            mod_timer(&di->timer2, jiffies + HZ*(config_table[di->charger_param.type].timeout_2) );
            break;
            
         case _TEMP_OVER:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_5);
            twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_LOW);
            break;
            
         default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}
static int bci_handle_quick_charge_3(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;
   
    switch(id){
        case _VBAT_DOWN_TH_1:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_1);
            del_timer(&di->timer2);
            break;

         case _TIMEOUT_2:
         case _TIMEOUT_3:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_CPL_CHG_4);
            twl4030_charge_clear_ref(di);
            bci_charge_full_mark(di);
            del_timer(&di->timer2);
            del_timer(&di->timer3);
            BATDPRT("quick_charge_3:Start timer1: delay=%d.\n",(config_table[di->charger_param.type].timeout_1) );
            mod_timer(&di->timer1, jiffies + HZ*(config_table[di->charger_param.type].timeout_1) );
            break;

         case _ICHG_DOWN_TH_1:
 #if 0  /*only use complete charge 4, skip other complete states*/
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_CPL_CHG_1);
            twl4030_charge_clear_ref(di);
            mod_timer(&di->timer1, jiffies + HZ*(config_table[di->charger_param.type].timeout_1) );
#endif
            break;

         case _VBAT_DOWN_TH_4:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_2);
            del_timer(&di->timer2);
            break;
            
         case _TEMP_OVER:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_6);
            twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_LOW);
            break;
            
         default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}

static int bci_handle_quick_charge_4(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;

    switch(id){
        case _TIMEOUT_3:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_CPL_CHG_4);
            twl4030_charge_clear_ref(di);
            bci_charge_full_mark(di);
            mod_timer(&di->timer1, jiffies + HZ*(config_table[di->charger_param.type].timeout_1) );
            break;

         case _TEMP_DOWN:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_1);
            break;

         case _VBAT_DOWN_TH_2:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_STP_CHG_1);
            twl4030_charge_clear_ref(di);
            break;
            
         default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}

static int bci_handle_quick_charge_5(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;

    switch(id){
        case _TIMEOUT_3:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_CPL_CHG_4);
            twl4030_charge_clear_ref(di);
            bci_charge_full_mark(di);
            mod_timer(&di->timer1, jiffies + HZ*(config_table[di->charger_param.type].timeout_1) );
            break;

        case _TEMP_DOWN:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_2);
            twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_HIGH);
            break;

         case _VBAT_DOWN_TH_2:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_STP_CHG_2);
             twl4030_charge_clear_ref(di);
            break;

         default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}

static int bci_handle_quick_charge_6(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;
    
    switch(id){
        case _TIMEOUT_2:
        case _TIMEOUT_3:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_CPL_CHG_4);
            twl4030_charge_clear_ref(di);
            bci_charge_full_mark(di);
            del_timer(&di->timer2);
            del_timer(&di->timer3);
            mod_timer(&di->timer1, jiffies + HZ*(config_table[di->charger_param.type].timeout_1) );
            break;

        case _TEMP_DOWN:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_3);
            twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_HIGH);
            break;

         case _VBAT_DOWN_TH_2:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_STP_CHG_3);
            twl4030_charge_clear_ref(di);
            break;
            
         default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}
static int bci_handle_stop_charge_1(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;
   
    switch(id){
        case _TIMEOUT_3:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_CPL_CHG_4);
            twl4030_charge_clear_ref(di);
            mod_timer(&di->timer1, jiffies + HZ*(config_table[di->charger_param.type].timeout_1) );
            break;

         case _TEMP_DOWN:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_1);
            twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_LOW);
            break;

         case _VBAT_OVER_TH_2:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_4);
            twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_LOW);
            break;
            
         default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}

static int bci_handle_stop_charge_2(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;
    
    switch(id){
        case _TIMEOUT_3:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_CPL_CHG_4);
            twl4030_charge_clear_ref(di);
            mod_timer(&di->timer1, jiffies + HZ*(config_table[di->charger_param.type].timeout_1) );
            break;

         case _TEMP_DOWN:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_2);
            twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_HIGH);
            break;

         case _VBAT_OVER_TH_2:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_5);
            twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_LOW);
            break;
            
         default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}
static int bci_handle_stop_charge_3(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;
    
    switch(id){
       case _TIMEOUT_2:
       case _TIMEOUT_3:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_CPL_CHG_4);
            twl4030_charge_clear_ref(di);
            del_timer(&di->timer2);
            del_timer(&di->timer3);
            mod_timer(&di->timer1, jiffies + HZ*(config_table[di->charger_param.type].timeout_1) );
            break;

         case _TEMP_DOWN:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_3);
            twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_HIGH);
            break;

         case _VBAT_OVER_TH_2:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_6);
            twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_LOW);
            break;
            
         default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}
static int bci_handle_complete_charge_1(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;
    
    switch(id){
        case _TIMEOUT_2:
        case _TIMEOUT_3:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_CPL_CHG_4);
            del_timer(&di->timer2);
            del_timer(&di->timer3);
            mod_timer(&di->timer1, jiffies + HZ*(config_table[di->charger_param.type].timeout_1) );
            break;
            
         case _TIMEOUT_1:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_CPL_CHG_3);
            break;
            
         default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}

static int bci_handle_complete_charge_2(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;
    
    switch(id){
        case _VBAT_DOWN_TH_3:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_2);
            ret = twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_HIGH);
            mod_timer(&di->timer3, jiffies + HZ*(config_table[di->charger_param.type].timeout_3) );
            break;
            
         default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}
static int bci_handle_complete_charge_3(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
    int ret = 0;

    switch(id){
        case _VBAT_DOWN_TH_3:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_2);
            ret = twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_HIGH);
            del_timer(&di->timer2);
            break;

         case _VBAT_DOWN_TH_4:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_CPL_CHG_2);
            del_timer(&di->timer2);
            del_timer(&di->timer3);
            break;
            
         case _TIMEOUT_2:
         case _TIMEOUT_3:
            bci_charge_full_mark(di);
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_CPL_CHG_4);
            del_timer(&di->timer2);
            del_timer(&di->timer3);
            mod_timer(&di->timer1, jiffies + HZ*(config_table[di->charger_param.type].timeout_1) );
            break;
            
         default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}
static int bci_handle_complete_charge_4(bci_charge_device_info *di, bci_charge_event_id id, void *data)
{
     int ret = 0;
     
     switch(id){
         case _VBAT_DOWN_TH_3:
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_QUK_CHG_2);
            BATDPRT("recharge by lower battery.\n");
 #if defined(BCI_VBAT_LEVEL_WAKEUP)
            if(di->charger_param.full){
                BATPRT("Lock baby, I'am hungery, it's time to dinner again.\n");
                bci_charge_full_clear(di);
                wake_lock(&di->charge_lock);
            }
 #endif
            ret = twl4030_monitor_chgi_ref_set(di, ICHG_LEVEL_HIGH);
            mod_timer(&di->timer3, jiffies + HZ*(config_table[di->charger_param.type].timeout_3) );
            break;

         case _VBAT_DOWN_TH_4:
#if 0  /*if lower than TH_3, go to quick charge again, skip TH_4*/
            di->pidx = di->cidx;
            di->cidx = id_to_idx(BCI_CHGS_CPL_CHG_2);
            del_timer(&di->timer1);
#endif
            break;

         case _VBAT_OVER_TH_4:
         case _TIMEOUT_1:
            BATDPRT("complete_charge_4: try to sleep.\n");
 #if defined(BCI_VBAT_LEVEL_WAKEUP)
            if(di->charger_param.full){
                BATDPRT("Unlock baby,  I'am full, maybe it's time to bed.\n");
                wake_unlock(&di->charge_lock);
            }
 #endif
            break;
            
         default:
            BATPRT("unknow Event_0x%08x to State_%s.\n", id, state_table[di->cidx].name);
    }

    return ret;
}

void static bci_timer1_out_handle(unsigned long data)
{
    struct bci_charge_device_info *di = (struct bci_charge_device_info *)data; 
    BATDPRT("Timer 1 time out.\n");
    twl4030_charge_send_event(di, _TIMEOUT_1);
}

void static bci_timer2_out_handle(unsigned long data)
{
    struct bci_charge_device_info *di = (struct bci_charge_device_info *)data;
    BATDPRT("Timer 2 time out.\n");
    twl4030_charge_send_event(di, _TIMEOUT_2);
}

void static bci_timer3_out_handle(unsigned long data)
{
    struct bci_charge_device_info *di = (struct bci_charge_device_info *)data;
    BATDPRT("Timer 3 time out.\n");
    twl4030_charge_send_event(di, _TIMEOUT_3);
}

#if defined(CONFIG_TWL4030_BCI_CHARGE_TYPE)
static void bci_charge_ac_detect_work (struct work_struct *work)
{
    struct bci_charge_device_info	*di = container_of(work, struct bci_charge_device_info,
					    ac_detect_work.work);
    charge_type type;

    type = di->charger_param.type;
    BATDPRT("Ac detect work time out.\n");
    if(type == TYPE_UNKNOW){
        bci_report_charge_type(TYPE_AC);
    }
}
#endif

static void bci_debounce_work (struct work_struct *work)
{
    struct bci_charge_device_info *di = container_of(work, struct bci_charge_device_info,
					    debounce_work.work);
    struct twl4030_madc_request req;
    int vac, vbat;
    int ret = 0;

    down(&di->sem);
    memset(&req, 0, sizeof(req));
    req.type = TWL4030_MADC_WAIT;
    req.do_avg = 1;
    req.method = TWL4030_MADC_SW1;
    req.active = 0;
    req.func_cb = NULL;

    /*get Vac and Vbat*/
    req.channels = (1 << MADC_CH_VAC) | (1 << MADC_CH_VBAT);
    ret = twl4030_madc_conversion(&req);
    if(ret < 0){
        BATDPRT("Fail to conversion the bci parameter %d.\n", ret);
        schedule_delayed_work(&di->debounce_work, msecs_to_jiffies(CHARGE_DEBOUNCE_TIME));
        up(&di->sem);
        return ;
    }

    vbat = bci_battery_voltage(req.rbuf[MADC_CH_VBAT]);
    vac = bci_ac_voltage(req.rbuf[MADC_CH_VAC]);

     BATDPRT("%s:Vac=%d, Vbat=%d.\n", __FUNCTION__, vac, vbat);
     
     if(vac < vbat + CHARGE_SNIFFER_VOL_GAP_2){
        BATDPRT("Debounce: send BCI_EVENT_CHG_UNPLUG(Vac=%d, Vbat=%d).\n", vac, vbat);
        twl4030_charge_send_event(di, _CHG_UNPLUG);
    }else if(vac > vbat + CHARGE_SNIFFER_VOL_GAP_1){
        BATDPRT("Debounce: send BCI_EVENT_CHG_PLUG(Vac=%d, Vbat=%d).\n", vac, vbat);
        twl4030_charge_send_event(di, _CHG_PLUG); 
    }else{
        BATDPRT("Debounce: suspect voltage so try again(Vac=%d, Vbat=%d).\n", vac, vbat);
        schedule_delayed_work(&di->debounce_work, msecs_to_jiffies(CHARGE_DEBOUNCE_TIME / 2));
    }
    up(&di->sem);
}

static int bci_monitor_thread(void *data)
{
    struct bci_charge_device_info *di = (struct bci_charge_device_info *)data; 
    static int old_cap = 0, old_vbat = 0;
    static int old_temp = 0, old_present = 0;
    static int old_status = 0;
    
    di->monitor_thread = current;
    daemonize("BCI Monitor");

    BATDPRT("BCI monitor thread start now.\n");
    
    while(1){
        schedule_timeout_uninterruptible(msecs_to_jiffies(di->update_gab));

        /*Driver is exist*/
        if(!di->monitor_thread){
		break;
        }
        
        down(&di->sem);
        /*FIXME: the mesvac will be disabled sometime, so set it before madc everytime*/
        twl4030_presacler_en_mesvbat(ENABLE);
        twl4030_presacler_en_mesvac(ENABLE);

        twl4030_monitor_update_params(di);
        
        /*check whether paramters changed*/
        if( old_cap != di->mbat_param.cap || 
             old_vbat != di->mbat_param.volt ||
             old_temp != di->mbat_param.temp ||
             old_present != di->mbat_param.pret  ||
             old_status != di->mbat_param.status){
             
            old_cap = di->mbat_param.cap;
            old_vbat = di->mbat_param.volt;
            old_temp = di->mbat_param.temp;
            old_present = di->mbat_param.pret;
            old_status = di->mbat_param.status;
            power_supply_changed(&di->mbat);
        }
        
        up(&di->sem);
    }

    BATDPRT("BCI monitor thread exit.\n");
    return 0;
}

static int bci_process_thread(void *data)
{
    struct bci_charge_device_info *di = (struct bci_charge_device_info *)data; 
    bci_charge_event_id id = 0;
    bci_charge_state_dsp *dsp = NULL;
    
    di->process_thread = current;
    daemonize("BCI Process");
    BATDPRT("BCI process thread start now.\n");

    while(1){        
        /*sleep until receive an evnet or thread exist*/
        wait_event(di->wait, (twl4030_charge_has_event(di)) || (!di->process_thread) );
        /*thread is existed*/
        if(!di->process_thread){
		break;
        }

        id = twl4030_charge_recv_event(di);

        dsp = state_table + di->cidx;

        if(dsp->handle){
            BATDPRT("process Event_0x%08x in State_%s,\n", id, dsp->name);
            down(&di->sem);
            if(BCI_EVENT(id) & BCI_CRITICAL_EVENT){
                bci_handle_critical_event(di, id);
            }else if (dsp->handle){
                dsp->handle(di, id, dsp->data);
            }
            up(&di->sem);
            BATDPRT("translate into State_%s .\n", state_table[di->cidx].name);
        }
    }

    BATDPRT("BCI process thread exit.\n");
    return 0;
}

static int bci_get_mbat_property(struct power_supply *psy,
              enum power_supply_property psp,
              union power_supply_propval *val)
{
       struct bci_charge_device_info *di = container_of(psy, 
    	                                                            struct bci_charge_device_info, mbat);
       int ret = 0;
       
       if(!di){
           return -EINVAL;
       } 
       
       down(&di->sem);

	switch (psp) {
           case POWER_SUPPLY_PROP_STATUS:
			val->intval = di->mbat_param.status;
			break;
                      
           case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			/*amplify 1000 according to android*/
			val->intval = di->mbat_param.volt * 1000;
        		break;
 
           case POWER_SUPPLY_PROP_CURRENT_NOW:
                     val->intval = di->charger_param.ichg;
                     break;
 
           case POWER_SUPPLY_PROP_TEMP:
        		val->intval = di->mbat_param.temp;
        		//val->intval = di->mbat_param.real_cap;;
        		break;
                
           case POWER_SUPPLY_PROP_HEALTH:
        	 	val->intval = di->mbat_param.health;
        	 	break;
        		
           case POWER_SUPPLY_PROP_PRESENT:
        	 	val->intval = di->mbat_param.pret;
        	 	break;
        		
           case POWER_SUPPLY_PROP_TECHNOLOGY:
        	 	val->intval = di->mbat_param.tech;
        	 	break;
        		
           case POWER_SUPPLY_PROP_CAPACITY:
                     val->intval = di->mbat_param.cap / 10;
        		break;
                
           default:
                     ret = -EINVAL;
	}

       BATDPRT("main battery get property[%d] = %d.\n", psp, val->intval);
       up(&di->sem);
       
       return ret;
}

static int bci_get_bbat_property(struct power_supply *psy,
              enum power_supply_property psp,
              union power_supply_propval *val)
{
   struct bci_charge_device_info *di = container_of(psy, 
                                          struct bci_charge_device_info, bbat);
   int ret = 0;
    
   if(!di){
      return -EINVAL;
   }
       
   down(&di->sem);

   switch (psp) {
      case POWER_SUPPLY_PROP_VOLTAGE_NOW:
         val->intval = di->bbat_param.volt * 1000;
         break;

      default:
         ret = -EINVAL;
   }

   //BATDPRT("backup battery get property[%d] = %d.\n", psp, val->intval);
   up(&di->sem);
   return ret;
}

static int bci_get_ac_property(struct power_supply *psy,
              enum power_supply_property psp,
              union power_supply_propval *val)
{
   struct bci_charge_device_info *di = container_of(psy, 
                                          struct bci_charge_device_info, ac);
   int ret = 0;
       
   if(!di){
      return -EINVAL;
   }
       
   down(&di->sem);
       
   switch (psp) {
      case POWER_SUPPLY_PROP_ONLINE:
         val->intval = (di->charger_param.online) && 
                              ((di->charger_param.type == TYPE_AC) || (di->charger_param.type == TYPE_UNKNOW));
         break;

      case POWER_SUPPLY_PROP_VOLTAGE_NOW:
         val->intval = di->charger_param.vac * 1000;
         break;

      case POWER_SUPPLY_PROP_CURRENT_NOW:
         val->intval = di->charger_param.ichg;
         break;
                    
      default:
         ret = -EINVAL;
   }

   BATDPRT("AC charge get property[%d] = %d.\n", psp, val->intval);
   up(&di->sem);
   return ret;
}

static int bci_get_usb_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
   struct bci_charge_device_info *di = container_of( psy, 
                                          struct bci_charge_device_info, usb);
   int ret = 0;
       
   if(!di){
      return -EINVAL;
   }    
   down(&di->sem);

   switch (psp) {
      case POWER_SUPPLY_PROP_ONLINE:
         val->intval = (di->charger_param.online) && ((di->charger_param.type == TYPE_USB) );
         break;

      case POWER_SUPPLY_PROP_VOLTAGE_NOW:
         val->intval = di->charger_param.vbus * 1000;
         break;
               
      case POWER_SUPPLY_PROP_CURRENT_NOW:
         val->intval = di->charger_param.ichg;
         break;
              
      default:
         ret = -EINVAL;
   }

   //BATDPRT("USB charge get property[%d] = %d.\n", psp, val->intval);
   up(&di->sem);
   return ret;
}

static enum power_supply_property bci_mbat_props[] = {
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_VOLTAGE_NOW,
    POWER_SUPPLY_PROP_CURRENT_NOW,
    POWER_SUPPLY_PROP_CAPACITY,
    POWER_SUPPLY_PROP_TEMP,
    POWER_SUPPLY_PROP_HEALTH,
    POWER_SUPPLY_PROP_PRESENT,
    POWER_SUPPLY_PROP_TECHNOLOGY,
};

static enum power_supply_property bci_bbat_props[] = {
    POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

static enum power_supply_property bci_ac_charge_props[] = {
    POWER_SUPPLY_PROP_VOLTAGE_NOW,
    POWER_SUPPLY_PROP_CURRENT_NOW,
    POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property bci_usb_charge_props[] = {
    POWER_SUPPLY_PROP_VOLTAGE_NOW,
    POWER_SUPPLY_PROP_CURRENT_NOW,
    POWER_SUPPLY_PROP_ONLINE,
};

static ssize_t bci_charge_info_show_attrs(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    int ret = 0;
    unsigned long flags=0;
    bci_charge_event *event = NULL;
    struct bci_charge_device_info *di = &bci_charge_info;
    
    ret += sprintf(buf, "******   Manual Charge Information  ******\n");
    ret += sprintf(buf + ret, " Current  state: %s\n", state_table[di->cidx].name);
    ret += sprintf(buf + ret, " Previous state: %s\n", state_table[di->pidx].name);
    ret += sprintf(buf + ret, " Event Queue:");
    spin_lock_irqsave(&di->lock, flags);
    while(!list_empty(&di->event_q)){
        event = list_first_entry(&di->event_q, bci_charge_event, list);
        ret += sprintf(buf + ret, "[0x%x] - ", event->id);
    }
    spin_unlock_irqrestore(&di->lock, flags);
    ret += sprintf(buf + ret, "\n");
    ret += sprintf(buf + ret, " Main battery param:");
    ret += sprintf(buf + ret, " VBAT:%d mV,  PVBAT:%d mV, CAP:%d, REAL_CAP:%d, HEALTH:%d, STATUS:%d\n", di->mbat_param.volt, di->mbat_param.pvolt, di->mbat_param.cap,
                    di->mbat_param.real_cap, di->mbat_param.health, di->mbat_param.status);
    ret += sprintf(buf + ret, " Back battery param:");
    ret += sprintf(buf + ret, " VBBAT:%d mV\n", di->bbat_param.volt);
    ret += sprintf(buf + ret, " Charger param:");
    ret += sprintf(buf + ret, " TYPE:%d, ONLINE:%d, VAC:%d mV, VBUS:%d mV, ICHG:%d mA\n", di->charger_param.type, di->charger_param.online,
                    di->charger_param.vac, di->charger_param.vbus, di->charger_param.ichg);
    ret += sprintf(buf + ret, " ADC param:");
    ret += sprintf(buf + ret, " Slope:%d, Intercept:%d.\n", calib_params.slope, calib_params.intercept);

    ret += sprintf(buf + ret, " Discharge Current: %d mA.\n", bci_discharge_current(di));

    return ret;
}

static ssize_t bci_pmureg_show_attrs(struct device *dev,
                struct device_attribute *attr, char *buf)
{

    int ret = 0;
    
    ret += sprintf(buf, "Command format:\n");
    ret += sprintf(buf + ret, " w 0x(mod) 0x(reg) 0x(val)\n");
    ret += sprintf(buf + ret, " r 0x(mod) 0x(reg)\n");
    ret += sprintf(buf + ret, " d (dump all reg in twl5030)\n");
    return ret;
}

static ssize_t bci_pmureg_store_attrs(struct device *dev, struct device_attribute *attr,
                const char *buf, size_t size)
{
    u8 mod;
    u8 reg;
    u8 val = 0;
    char cmd = 0;
    int ret = 0;

    if( size >= 4096 )
        return -EINVAL;
	
    sscanf(buf, "%c", &cmd); 

    if(cmd == 'w'){
        sscanf(buf, "%c 0x%hhx 0x%hhx 0x%hhx", &cmd, &mod, &reg, &val); 
        
        ret = twl_i2c_write_u8(mod, val, reg);
        if(ret < 0){
            printk(KERN_ERR "fail to write 0x%02x to reg(0x%02x) in mod(0x%02x).\n", val, reg, mod);
        }else{
            printk(KERN_INFO "write 0x%02x to reg(0x%02x) in mod(0x%02x).\n", val, reg, mod);
        }
    }else if(cmd == 'r'){
        sscanf(buf , "%c 0x%hhx 0x%hhx", &cmd, &mod, &reg);
        
        ret = twl_i2c_read_u8(mod, &val , reg);
        if(ret < 0){
            printk(KERN_ERR "fail to read reg(0x%02x) in mod(0x%02x).\n", reg, mod);
        }else{
            printk(KERN_INFO "read 0x%02x from reg(0x%02x) in mod(0x%02x).\n", val , reg, mod);
        }
    }else if(cmd == 'd'){
        int i, j;
        //mode which base address is 0
        int reg_mod[] = {
            TWL4030_MODULE_USB, //
            TWL4030_MODULE_AUDIO_VOICE,
            TWL4030_MODULE_MADC,
            TWL4030_MODULE_SECURED_REG
        };
        /*If want dump PHY, make sure that PHY had been powered on and enabled I2C access*/
        for(i=0; i<ARRAY_SIZE(reg_mod); i++){
            for(j= 0; j<0xFF; j++){
                 ret = twl_i2c_read_u8((u8)i, &val , (u8)j);
                 if(ret < 0){
                      printk(KERN_ERR "fail to read reg(0x%02x) in mod(0x%02x).\n", j, i);
                 }else{
                      printk(KERN_INFO "Slave %d Reg[0x%02x] = 0x%02x.\n", i, j, val);
                 }
            }
        }       
    }else{
         printk(KERN_ERR "Error cmd.\n");
    }
   
    return size;
}

static ssize_t bci_charge_config_show_attrs(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    int ret = 0;
    int i = 0;
    bci_charge_config *pfg = NULL;
    
    ret += sprintf(buf, "******   BCI Charge Config  ******\n");
    for(i = 0; i<CHARGE_TYPE_MAX; i++){
        pfg = config_table + i;
        ret += sprintf(buf + ret,"index: %d\n", i);
        ret += sprintf(buf + ret,"name: %s\n", pfg->name);
        ret += sprintf(buf + ret," 0) chgv: %d\n", pfg->chgv);
        ret += sprintf(buf + ret," 1) chgi_low : %d\n", pfg->chgi_low);
        ret += sprintf(buf + ret," 2) chgi_high: %d\n", pfg->chgi_high);
        ret += sprintf(buf + ret," 3) vac_over: %d\n", pfg->vac_over);
        ret += sprintf(buf + ret," 4) vbat_over: %d\n", pfg->vbat_over);
        ret += sprintf(buf + ret," 5) vbus_over: %d\n", pfg->vbus_over);
        ret += sprintf(buf + ret," 6) ichg_over: %d\n", pfg->ichg_over);
        ret += sprintf(buf + ret," 7) ichg_th_1: %d\n", pfg->ichg_th_1);
        ret += sprintf(buf + ret," 8) ichg_th_2: %d\n", pfg->ichg_th_2);
        ret += sprintf(buf + ret," 9)  vbat_th_1: %d\n", pfg->vbat_th_1);
        ret += sprintf(buf + ret," 10) vbat_th_2: %d\n", pfg->vbat_th_2);
        ret += sprintf(buf + ret," 11) vbat_th_3: %d\n", pfg->vbat_th_3);
        ret += sprintf(buf + ret," 12) vbat_th_4: %d\n", pfg->vbat_th_4);
        ret += sprintf(buf + ret," 13) timeout_1: %d\n", pfg->timeout_1);
        ret += sprintf(buf + ret," 14) timeout_2: %d\n", pfg->timeout_2);
        ret += sprintf(buf + ret," 15) timeout_3: %d\n", pfg->timeout_3);
        ret += sprintf(buf + ret," 16) temp_normal_low : %d\n", pfg->temp_normal_low);
        ret += sprintf(buf + ret," 17) temp_normal_high: %d\n", pfg->temp_normal_high);
        ret += sprintf(buf + ret," 18) temp_over_low : %d\n", pfg->temp_over_low);
        ret += sprintf(buf + ret," 19) temp_over_high: %d\n", pfg->temp_over_high);
        ret += sprintf(buf + ret," 20) update_gab: %d mS\n", bci_charge_info.update_gab);
        ret += sprintf(buf + ret," 21) ready: %d \n", bci_charge_info.ready);
    }
    
    return ret;
}

static ssize_t bci_charge_config_store_attrs(struct device *dev, struct device_attribute *attr,
                const char *buf, size_t size)
{
    int index;
    int param;
    int val;

    if( size >= 4096 )
		return -EINVAL;
	
    sscanf(buf, "%d %d %d", &index, &param, &val); 

    if(index<0 || index>CHARGE_TYPE_MAX){
        printk("Error charge index %d.\n", index);
        return size;
    }

    if(param<0 || param>21){
        printk("Error parameter index %d.\n", param);
        return size;
    }

    if(val < 0){
        printk("Error input value %d.\n", val);
        return size;
    }
 
    switch(param){
        case 0:
            config_table[index].chgv = val;
            printk("%s: chgv = %d\n", config_table[index].name, val);
            break;
        case 1:
            config_table[index].chgi_low = val;
            printk("%s: chgi_low = %d\n", config_table[index].name, val);
            break;
        case 2:
            config_table[index].chgi_high = val;
            printk("%s: chgi_high = %d\n", config_table[index].name, val);
            break;
        case 3:
            config_table[index].vac_over = val;
            printk("%s: vac_over = %d\n", config_table[index].name, val);
            break;
        case 4:
            config_table[index].vbat_over = val;
            printk("%s: vbat_over = %d\n", config_table[index].name, val);
            break;
        case 5:
            config_table[index].vbus_over = val;
            printk("%s: vbus_over = %d\n", config_table[index].name, val);
            break;
        case 6:
            config_table[index].ichg_over = val;
            printk("%s: ichg_over = %d\n", config_table[index].name, val);
            break;
        case 7:
            config_table[index].ichg_th_1 = val;
            printk("%s: ichg_th_1 = %d\n", config_table[index].name, val);
            break;
        case 8:
            config_table[index].ichg_th_2 = val;
            printk("%s: ichg_th_2 = %d\n", config_table[index].name, val);
            break;
        case 9:
            config_table[index].vbat_th_1 = val;
            printk("%s: vbat_th_1 = %d\n", config_table[index].name, val);
            break;
        case 10:
            config_table[index].vbat_th_2 = val;
            printk("%s: vbat_th_2 = %d\n", config_table[index].name, val);
            break;
        case 11:
            config_table[index].vbat_th_3 = val;
            printk("%s: vbat_th_3 = %d\n", config_table[index].name, val);
            break;
        case 12:
            config_table[index].vbat_th_4 = val;
            printk("%s: vbat_th_4 = %d\n", config_table[index].name, val);
            break;
        case 13:
            config_table[index].timeout_1 = val;
            printk("%s: timeout_1 = %d\n", config_table[index].name, val);
            break;
        case 14:
            config_table[index].timeout_2 = val;
            printk("%s: timeout_2 = %d\n", config_table[index].name, val);
            break;
        case 15:
            config_table[index].timeout_3 = val;
            printk("%s: timeout_3 = %d\n", config_table[index].name, val);
            break;
        case 16:
            config_table[index].temp_normal_low = val;
            printk("%s: temp_normal_low = %d\n", config_table[index].name, val);
            break;
        case 17:
            config_table[index].temp_normal_high = val;
            printk("%s: temp_normal_high = %d\n", config_table[index].name, val);
            break;
        case 18:
            config_table[index].temp_over_low = val;
            printk("%s: temp_over_low = %d\n", config_table[index].name, val);
            break;
        case 19:
            config_table[index].temp_over_high = val;
            printk("%s: temp_over_high = %d\n", config_table[index].name, val);
            break;
        case 20:
            bci_charge_info.update_gab = val;
            printk("%s: update_gab = %d mS\n", config_table[index].name, bci_charge_info.update_gab);
            break;
        case 21:
            bci_charge_info.ready= val;
            printk("%s: ready = %d\n", config_table[index].name, bci_charge_info.ready);
            break;
        default:
            printk("Command: charge_index parameter_index value\n");
    }
    
    return size;
}

static ssize_t bci_adc_show_attrs(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    int adc1, adc2;
    int ret = 0;

    adc1 = twl4030_monitor_get_adc(MADC_CH_VBAT);
    adc2 = twl4030_monitor_get_adc(MADC_CH_VAC);
    ret += sprintf(buf, "VBat=%d(0x%x), VAc=%d(0x%x).\n", bci_battery_voltage(adc1), adc1, bci_ac_voltage(adc2), adc2);

    return ret;
}

static ssize_t bci_vbat_monitor_store_attrs(struct device *dev, struct device_attribute *attr,
                const char *buf, size_t size)
{
    struct bci_charge_device_info *di = &bci_charge_info;

    if ( !strncmp(buf, "on", strlen("on"))) {
        twl4030_enable_vbat_monitor(4000);
        //twl4030_enable_vbat_monitor(3900);
    }else if( !strncmp(buf, "off", strlen("off"))){
        twl4030_disable_vbat_monitor();
   }else if( !strncmp(buf, "lock", strlen("lock"))){
        wake_lock(&di->charge_lock);
   }else if( !strncmp(buf, "unlock", strlen("unlock"))){
        wake_unlock(&di->charge_lock);
   }

    return size;
}

static ssize_t bci_vbat_monitor_show_attrs(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    int ret = 0;

    ret += sprintf(buf + ret, "on/off\n");

    return ret;
}

#if defined(BCI_LEVEL_WAKEUP_DEBUG)
static ssize_t bci_level_wakeup_store_attrs(struct device *dev, struct device_attribute *attr,
                const char *buf, size_t size)
{
    struct bci_charge_device_info *di = &bci_charge_info;

    if ( !strncmp(buf, "on", strlen("on"))) {
        ldbg_full = 1;
        ldbg_volt = CHARGE_FULL_VOL_2;
        ldbg_ichg = CHARGE_FULL_CUR_2;
    }else if( !strncmp(buf, "off", strlen("off"))){
        ldbg_full = 0;
        ldbg_volt = CHARGE_FULL_VOL_1;
        ldbg_ichg = CHARGE_FULL_CUR_2;
   }

    return size;
}

static ssize_t bci_level_wakeup_show_attrs(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    int ret = 0;

    ret += sprintf(buf + ret, "%s: VBat =%d, IChg=%d\n", ldbg_full?"On":"Off", ldbg_volt, ldbg_ichg);

    return ret;
}
#endif

static ssize_t bci_calibration_show_attrs(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    int i;
    int ret = 0;
    if(valid_samples < 2){
        return sprintf(buf, "unknow" );
    }
    
    ret += sprintf(buf, "Voltmv = (Slope * Adc) / 1000 + Intercept.\n" );
    ret += sprintf(buf + ret, "Slope=%d, Intercept=%d.\n", calib_params.slope, calib_params.intercept);
    ret += sprintf(buf + ret, "Command: index volt 0xadc.\n");

    for(i=0; i<valid_samples; i++){
        ret += sprintf(buf + ret, "%d: volt =%d, adc=0x%x.\n", i, calib_table[i].volt, calib_table[i].adc);
    }
    
    return ret;
}

static ssize_t bci_calibration_store_attrs(struct device *dev, struct device_attribute *attr,
                const char *buf, size_t size)
{
    int index, volt, adc;

    if( size >= 4096 )
		return -EINVAL;
    
    sscanf(buf, "%d %d 0x%x", &index, &volt, &adc); 

    BATDPRT("input command: index=%d, volt=%d, adc=0x%x.\n", index, volt, adc);
    
    if(index >= ADC_VOLT_SAMPLE_NUM){
        printk("Invalid index input: index=%d, index:[0, %d).\n", index, ADC_VOLT_SAMPLE_NUM);
        return -EINVAL;
    }

    if( (adc < 100) || (adc > 0x3FF) ){
        printk("Invalid adc input: adc=%d, adc:[100, 0x3FF].\n", adc);
        return -EINVAL;
    }

    if( (volt < 2800) || (volt > 4500) ){
        printk("Invalid volt input: volt=%d, volt:[2800, 4500].\n", volt);
        return -EINVAL;
    }

    calib_table[index].volt = volt;
    calib_table[index].adc = adc;

   if( bci_calib_init() < 0){
        printk("Failt to run bci_calib_init().\n");
        return -EINVAL;
   }

   return size;
}

static ssize_t bci_wifi_mac_show_attrs(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    int ret = 0;

    ret += sprintf(buf, "%s", wifi_mac);
    return ret;
}
static ssize_t bci_wifi_mac_store_attrs(struct device *dev, struct device_attribute *attr,
                const char *buf, size_t size)
{
    int len = 0;

    if( size >= 4096 || buf == NULL || *buf == '\0')
		return -EINVAL;
    
    if(wifi_mac_check(buf) < 0 ){
        return -EINVAL;
    }
    
    while( *(buf+len) && (len < NANME_LENGTH -1)){
        wifi_mac[len] = *(buf + len);
        len++;
    }

    return size;
}

static int bci_charge_init(bci_charge_device_info *di)
{
    int ret = 0;
    int bat_in = 0;
    int chg_in = 0;
    
    bci_calib_init();
    mdelay(3);
    di->ready = 0;
    di->update_reset = RST_UPDATE_NONE;
    di->cap_init = 0;
    di->update_gab = BCI_CAP_INIT_INTERVAL;
    di->mbat.name = "battery";
    di->mbat.type = POWER_SUPPLY_TYPE_BATTERY;
    di->mbat_param.temp = 220; //set the fix value 22.C
    di->mbat.properties = bci_mbat_props;
    di->mbat.num_properties = ARRAY_SIZE(bci_mbat_props);
    di->mbat.get_property = bci_get_mbat_property;
    di->mbat.external_power_changed = NULL;

    di->mbat_param.health = POWER_SUPPLY_HEALTH_GOOD;
    di->mbat_param.tech = POWER_SUPPLY_TECHNOLOGY_LION;
    di->mbat_param.status = POWER_SUPPLY_STATUS_DISCHARGING;
    di->mbat_param.cap = 50;

    /*Android only care one battery, AC and USB power supply*/
    di->bbat.name = "twl4030_bci_bk_battery";
    di->bbat.type = POWER_SUPPLY_TYPE_UPS;
    di->bbat.properties = bci_bbat_props;
    di->bbat.num_properties = ARRAY_SIZE(bci_bbat_props);
    di->bbat.get_property = bci_get_bbat_property;
    di->bbat.external_power_changed = NULL;

    di->ac.name = "ac";
    di->ac.type = POWER_SUPPLY_TYPE_MAINS;
    di->ac.properties = bci_ac_charge_props;
    di->ac.num_properties = ARRAY_SIZE(bci_ac_charge_props);
    di->ac.get_property = bci_get_ac_property;
    di->ac.external_power_changed = NULL;

    di->usb.name = "usb";
    di->usb.type = POWER_SUPPLY_TYPE_USB;
    di->usb.properties = bci_usb_charge_props;
    di->usb.num_properties = ARRAY_SIZE(bci_usb_charge_props);
    di->usb.get_property = bci_get_usb_property;
    di->usb.external_power_changed = NULL;

    /*do not init the charge type, because report type maybe called before probe function*/
    //di->charger_param.type = TYPE_UNKNOW;
    di->charger_param.full = 0;
    di->cidx = di->pidx = id_to_idx(BCI_CHGS_NO_CHARGING_DEV);
    INIT_LIST_HEAD(&di->event_q);
    spin_lock_init(&di->lock);
    wake_lock_init(&di->charge_lock, WAKE_LOCK_SUSPEND, "charge");
    wake_lock_init(&di->plug_lock, WAKE_LOCK_SUSPEND, "plug");
    wake_lock_init(&di->cap_lock, WAKE_LOCK_SUSPEND, "capacity");

    init_MUTEX(&di->sem);
    init_waitqueue_head(&di->wait);

    INIT_DELAYED_WORK(&di->ac_detect_work, bci_charge_ac_detect_work);
    setup_timer(&di->timer1, bci_timer1_out_handle, (unsigned long)di);
    setup_timer(&di->timer2, bci_timer2_out_handle, (unsigned long)di);
    setup_timer(&di->timer3, bci_timer3_out_handle, (unsigned long)di);

    ret = power_supply_register(di->dev, &di->mbat);
    if(ret){
        BATPRT("failed to register main battery in power supply.\n");
        goto mbat_reg_fail;
    }

    ret = power_supply_register(di->dev, &di->bbat);
    if(ret){
        BATPRT("failed to register backup battery in power supply.\n");
        goto bbat_reg_fail;
    }

    ret = power_supply_register(di->dev, &di->ac);
    if(ret){
        BATPRT("failed to register AC charger in power supply.\n");
        goto ac_reg_fail;
    }

    ret = power_supply_register(di->dev, &di->usb);
    if(ret){
	BATPRT("failed to register USB charge in power supply.\n");
        goto usb_reg_fail;
    }

    /*check the charge and battery to init the state*/
    bat_in = bci_battery_packed();
    chg_in = twl4030_charge_plugged();
    BATDPRT("Battery: %s, Charge: %s, Type:%d.\n", bat_in ?"Packed":"Unpacked", chg_in?"Plugged":"Unplugged", di->charger_param.type);

    /*Disable Auto charger which been enabled in Uboot*/
    ret = bci_init_hardware();
    if(ret < 0){
        return ret;
    }
    mdelay(3);
    twl4030_monitor_init_params(di);

    if(chg_in){
        bci_handle_no_charge_device(di, _CHG_PLUG, NULL);
    }

    return ret;
    
usb_reg_fail:
    power_supply_unregister(&di->ac);
ac_reg_fail:
    power_supply_unregister(&di->bbat);
bbat_reg_fail:
    power_supply_unregister(&di->mbat);
mbat_reg_fail:
    return ret;
}

static struct device_attribute bci_test_attrs[] = {
    __ATTR(config, 0666, bci_charge_config_show_attrs, bci_charge_config_store_attrs),
    __ATTR(info, 0444, bci_charge_info_show_attrs, NULL),
    __ATTR(pmureg, 0666, bci_pmureg_show_attrs, bci_pmureg_store_attrs),
    __ATTR(adc, 0444, bci_adc_show_attrs, NULL),
    __ATTR(monitor, 0666, bci_vbat_monitor_show_attrs, bci_vbat_monitor_store_attrs),
    __ATTR(calibration, 0666, bci_calibration_show_attrs, bci_calibration_store_attrs),
    __ATTR(wifi_mac, 0666, bci_wifi_mac_show_attrs, bci_wifi_mac_store_attrs),
    __ATTR(status_on, 0666, bci_status_on_show_attrs, bci_status_on_store_attrs),
    __ATTR(status_off, 0666, bci_status_off_show_attrs, bci_status_off_store_attrs),
#if defined(BCI_LEVEL_WAKEUP_DEBUG)
     __ATTR(wakeuplevel, 0666, bci_level_wakeup_show_attrs, bci_level_wakeup_store_attrs),
#endif
};

static int __init twl4030_bci_charge_probe(struct platform_device *pdev)
{
    struct twl4030_bci_platform_data *pdata = pdev->dev.platform_data;
    struct bci_charge_device_info *di = NULL;
    int irq;
    int i, ret=0;

    BATDPRT("%s has been called.\n", __FUNCTION__);
    therm_tbl = pdata->battery_tmp_tbl;

    di = &bci_charge_info;
    //di = memset(di, 0, sizeof(*di));
    di->dev = &pdev->dev;
    
    ret = bci_charge_init(di);
    if(ret){
       BATPRT("Fail to init the BCI state ret = %d\n", ret);
       goto bci_init_fail;
    }
    
    platform_set_drvdata(pdev, di);

    /* REVISIT do we need to request both IRQs ?? */
    /* request BCI interruption*/
    irq = platform_get_irq(pdev, 1);
    ret = request_irq(irq, bci_battery_interrupt, 
                    0, pdev->name, di);

    if(ret){
    	BATPRT("could not request irq %d, status %d\n", irq, ret);
       goto bat_irq_fail;
    }

    irq = platform_get_irq(pdev, 0);

    /* request Power interruption */
    ret = request_irq(irq, bci_charge_interrupt,
                    0, pdev->name, di);
    
    if(ret){
        BATPRT("could not request irq %d, status %d\n", irq, ret);
        goto chg_irq_fail;
    }

    INIT_WORK(&di->present_task, twl4030_charge_present_task);
    INIT_DELAYED_WORK(&di->debounce_work, bci_debounce_work);

    ret = kernel_thread(bci_monitor_thread, di, 0);
    if(ret < 0){
        BATPRT("Fail to create BCI monitor thread.\n");
        goto monitor_thread_fail;
    }

    ret = kernel_thread(bci_process_thread, di, 0);
    if(ret < 0){
        BATPRT("Fail to start the BCI process thread.\n");
        goto process_thread_fail;
    }

    for (i = 0; i < ARRAY_SIZE(bci_test_attrs); i++) {
        ret = device_create_file(&pdev->dev, &bci_test_attrs[i]);
        if(ret < 0){
            BATPRT("Fail to create sysfs for %d attrs.\n", i);
        }
    }

    BATPRT("probe manual charge driver ok.\n");
    return 0;
    
process_thread_fail:
    if(di->monitor_thread){
        release_thread(di->monitor_thread);
        di->monitor_thread = NULL;
    }
monitor_thread_fail:
    free_irq(irq, di);
chg_irq_fail:
    irq = platform_get_irq(pdev, 1);
    free_irq(irq, di);
bat_irq_fail:
bci_init_fail:
    kfree(di);
    
    return ret;
}

static int __exit twl4030_bci_charge_remove(struct platform_device *pdev)
{
	struct bci_charge_device_info *di = (struct bci_charge_device_info *)platform_get_drvdata(pdev);
	int irq;
    
	twl4030_charge_stop(di);
    
	irq = platform_get_irq(pdev, 0);
	free_irq(irq, di);

	irq = platform_get_irq(pdev, 1);
	free_irq(irq, di);
	
	bci_release_eventq(di);
	di->monitor_thread = NULL;
	di->monitor_thread = NULL;
	wake_up(&di->wait);
	power_supply_unregister(&di->mbat);
	power_supply_unregister(&di->bbat);
	power_supply_unregister(&di->ac);
	power_supply_unregister(&di->usb);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM
static int twl4030_bci_charge_suspend(struct platform_device *pdev,
	pm_message_t state)
{
#if defined(BCI_VBAT_LEVEL_WAKEUP)
       int vbat, vbat_th;
       struct bci_charge_device_info *di = platform_get_drvdata(pdev);

       if(di->charger_param.full){
            vbat = di->mbat_param.volt;
            vbat_th = config_table[di->charger_param.type].vbat_th_3;
            if(vbat > vbat_th){
                vbat = vbat_th - 50;
            }else{
                BATPRT("Maybe something wrong, the VBat(%d) is lower than VBat_Th3(%d) after full charge.\n", vbat, vbat_th);
                vbat = vbat - 50;
            }
            /*set the threshold that wakeup system to charge again after full*/
            twl4030_enable_vbat_monitor(vbat);
            //twl4030_enable_vbat_monitor(3900);
       }else{
            /*FIXME:set the threshold that wakeup system to warnning low battery
              * but TWL5030 can not be wakeup by VBAT level if no AC charger input
              * becasuse TWL5030 will not sleep if AC charger input
              */
            //twl4030_enable_vbat_monitor(3450);
       }
#endif

	return 0;
}

static int twl4030_bci_charge_resume(struct platform_device *pdev)
{
	struct bci_charge_device_info *di = platform_get_drvdata(pdev);
       /*update the all parameters again*/
       twl4030_monitor_reset_update(di, RST_UPDATE_LV2);

#if defined(BCI_VBAT_LEVEL_WAKEUP)
       //twl4030_disable_vbat_monitor();
       twl4030_disable_vbat_monitor();
       /*wake the system to check whether recharge*/
       if(di->charger_param.full){
            wake_lock_timeout(&di->charge_lock, 15*HZ);     
       }
#endif

     return 0;
}
#else
#define twl4030_bci_charge_suspend	NULL
#define twl4030_bci_charge_resume	NULL
#endif /* CONFIG_PM */

static struct platform_driver twl4030_bci_charge_driver = {
	.probe		= twl4030_bci_charge_probe,
	.remove		= __exit_p(twl4030_bci_charge_remove),
	.suspend	       = twl4030_bci_charge_suspend,
	.resume		= twl4030_bci_charge_resume,
	.driver		= {
		.name	= "twl4030_bci",
	},
};

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:twl4030_bci");
MODULE_AUTHOR("VIA telecom Inc");

static int __init twl4030_charge_init(void)
{
	return platform_driver_register(&twl4030_bci_charge_driver);
}
module_init(twl4030_charge_init);

static void __exit twl4030_charge_exit(void)
{
	platform_driver_unregister(&twl4030_bci_charge_driver);
}
module_exit(twl4030_charge_exit);
