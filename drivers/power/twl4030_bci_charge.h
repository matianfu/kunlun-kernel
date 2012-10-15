#ifndef TWL4030_BCI_CHARGE_H
#define TWL4030_BCI_CHARGE_H

#include <mach/charge.h>

#define BCI_REG_SIZE    (8)
#define BCI_MSB_VALUE_MASK (0x03)

#define T2_BATTERY_VOLT		0x04
#define T2_BATTERY_TEMP		0x06
#define T2_BATTERY_CUR		0x08

/* charger constants */
#define NO_PW_CONN		0
#define AC_PW_CONN		0x01
#define USB_PW_CONN		0x02

/* TWL4030_MODULE_USB */
#define REG_POWER_CTRL		0x0AC
#define OTG_EN			0x020
#define REG_PHY_CLK_CTRL	0x0FE
#define REG_PHY_CLK_CTRL_STS	0x0FF
#define PHY_DPLL_CLK		0x01

/* Boot BCI flag bits */
#define BCIAUTOWEN		0x020
#define CONFIG_DONE		0x010
#define CVENAC                  0x004
#define BCIAUTOUSB		0x002
#define BCIAUTOAC		0x001
#define BCIMSTAT_MASK		0x03F

/* Boot BCI register */
#define REG_BOOT_BCI		0x007
#define REG_CTRL1		0x00
#define REG_SW1SELECT_MSB	0x07
#define SW1_CH9_SEL		0x02
#define REG_CTRL_SW1		0x012
#define SW1_TRIGGER		0x020
#define EOC_SW1			0x002
#define REG_GPCH9		0x049
#define REG_STS_HW_CONDITIONS	0x0F
#define STS_VBUS		0x080
#define STS_CHG			0x02
#define REG_BCIMSTATEC		0x02
#define REG_BCIMFSTS2		0x00E
#define REG_BCIMFSTS3 		0x00F
#define REG_BCIMFSTS4		0x010

#define REG_BCIMFSTS1		0x001
#define USBFASTMCHG		0x004
#define BATSTSPCHG		0x004
#define BATSTSMCHG		0x040
#define VBATOV4			0x020
#define VBATOV3			0x010
#define VBATOV2			0x008
#define VBATOV1			0x004
#define REG_BB_CFG		0x012

#define BBCHEN		0x010
#define BBSEL_2500mV	0x00
#define BBSEL_3000mV	0x04
#define BBSEL_3100mV	0x08
#define BBSEL_3200mV	0x0C
#define BBISEL_25uA	0x00
#define BBISEL_150uA	0x01
#define BBISEL_500uA	0x02
#define BBISEL_1000uA	0x03

/* Power supply charge interrupt */
#define REG_PWR_ISR1		0x00
#define REG_PWR_IMR1		0x01
#define REG_PWR_EDR1		0x05
#define REG_PWR_SIH_CTRL	0x007

#define USB_PRES		0x004
#define CHG_PRES		0x002

#define USB_PRES_RISING		0x020
#define USB_PRES_FALLING	0x010
#define CHG_PRES_RISING		0x008
#define CHG_PRES_FALLING	0x004
#define AC_STATEC		0x20
#define COR			0x004

/* interrupt status registers */
#define REG_BCIISR1A		0x0
#define REG_BCIISR2A		0x01

/* Interrupt flags bits BCIISR1 */
#define BATSTS_ISR1		0x080
#define VBATLVL_ISR1		0x001

/* Interrupt mask registers for int1*/
#define REG_BCIIMR1A		0x002
#define REG_BCIIMR2A		0x003

/* Interrupt masks for BCIIMR1 */
#define BATSTS_IMR1		0x080
#define VBATLVL_IMR1		0x001

/* Interrupt edge detection register */
#define REG_BCIEDR1		0x00A
#define REG_BCIEDR2		0x00B
#define REG_BCIEDR3		0x00C

/* BCIEDR2 */
#define	BATSTS_EDRRISIN		0x080
#define BATSTS_EDRFALLING	0x040

/* BCIEDR3 */
#define	VBATLVL_EDRRISIN	0x02
#define	VBATLVL_EDRFALL		0x01

/*REG_BCIVBAT1*/
#define REG_BCIVBAT1            (0x004)
#define BCIVBAT_LSB      	(0xFF)

/*REG_BCIVBAT2*/
#define REG_BCIVBAT2            (0x005)
#define BCIVBAT_MSB     	(0x03)

/*REG_BCITBAT1*/
#define REG_BCITBAT1            (0x006)
#define BCITBAT_LSB      	(0xFF)

/*REG_BCITBAT2*/
#define REG_BCITBAT2            (0x007)
#define BCITBAT_MSB     	(0x03)

/*REG_BCIICHG1*/
#define REG_BCIICHG1            (0x008)
#define BCIICHG_LSB      	(0xFF)

/*REG_BCIICHG2*/
#define REG_BCIICHG2            (0x009)
#define BCIICHG_MSB     	(0x03)

/*REG_BCIVAC1*/
#define REG_BCIVAC1		(0x00A)
#define BCIVAC_LSB      	(0xFF)

/*REG_BCIVAC2*/
#define REG_BCIVAC2		(0x00B)
#define BCIVAC_MSB		(0x03)

/*REG_BCIVBUS1*/
#define REG_BCIVBUS1            (0x00C)
#define BCIVBUS_LSB      	(0xFF)

/*REG_BCIVAC2*/
#define REG_BCIVBUS2            (0x00D)
#define BCIVBUS_MSB		(0x03)

/*BCIMDKEY:
  *cannot be accessed  in AUTOAC or AUTOUSB charge mode,
  *and must set EKEY_OFFMODE before enble other EKEY.
  */
#define REG_BCIMDKEY            (0x001)
#define EKEY_ACPATH     	(0x25)
#define EKEY_USBPATH		(0x26)
#define EKEY_PULSE		(0x27)
#define EKEY_ACACESS		(0x28)
#define EKEY_USBACESS		(0x29)
#define EKEY_OFFMODE		(0x2a)

/*
  *BCIMFKEY:
  *to protect the register for monitor,
  *and must write the MFKEY value before write the monitor registers
  */
#define REG_BCIMFKEY		(0x011)
#define MFKEY_EN1   (0x57)
#define MFKEY_EN2   (0x73)
#define MFKEY_EN3   (0x9c)
#define MFKEY_EN4   (0x3e)
#define MFKEY_TH1   (0xd2)
#define MFKEY_TH2   (0x7f)
#define MFKEY_TH3   (0x6d)
#define MFKEY_TH4   (0xea)
#define MFKEY_TH5   (0xc4)
#define MFKEY_TH6   (0xbc)
#define MFKEY_TH7   (0xc3)
#define MFKEY_TH8   (0xf4)
#define MFKEY_TH9_REF   (0xe7)

#define REG_BCIMFEN1        (0x012)
#define VBATOV1EN   (0x80)
#define VBATOV1CF   (0x40)
#define VBATOV2EN   (0x20)
#define VBATOV2CF   (0x10)
#define VBATOV3EN   (0x08)
#define VBATOV3CF   (0x04)
#define VBATOV4EN   (0x02)
#define VBATOV4CF   (0x01)

#define REG_BCIMFEN2	(0x013)
#define ACCHGOVEN	(0x80)
#define ACCHGOVCF	(0x40)
#define VBUSOVEN	(0x20)
#define VBUSOVCF	(0x10)
#define TBATOR1EN	(0x08)
#define TBATOR1CF	(0x04)
#define TBATOR2EN	(0x02)
#define TBATOR2CF	(0x01)

#define REG_BCIMFEN3	(0x014)
#define ICHGEOCEN	(0x80)
#define ICHGEOCCF	(0x40)
#define ICHGLOWEN	(0x20)
#define ICHGLOWCF	(0x10)
#define ICHGHIGEN	(0x08)
#define ICHGHIGCF	(0x04)
#define HSMODE		(0x02)
#define HSEN		(0x01)

#define REG_BCIMFEN4    (0x015)
#define VBUSVT  	(0xC0)
#define VBUSVT_4P2V     (0x00)
#define VBUSVT_4P3V     (0x40)
#define VBUSVT_4P4V     (0x80)
#define VBUSVT_4P5V     (0xC0)
#define VBATVT  	(0x30)
#define VBATVT_3P9V     (0x00)
#define VBATVT_4P1V     (0x10)
#define VBATVT_4P2V     (0x20)
#define VBATVT_4P3V     (0x30)
#define WATCHDOGEN     	(0x08)
#define BATSTSMCHGEN  	(0x04)
#define VBATOVEN        (0x02)
#define VBATOVCF        (0x01)

#define REG_BCIMFTH1    (0x016)
#define VBATOV2TH   	(0xF0)
#define VBATOV2TH_SHIFT (4)
#define VBATOV1TH   	(0x0F)
#define VBATOV1TH_SHIFT (0)

#define REG_BCIMFTH2    (0x017)
#define VBATOV4TH   	(0xF0)
#define VBATOV4TH_SHIFT (4)
#define VBATOV3TH   	(0x0F)
#define VBATOV3TH_SHIFT (0)

#define REG_BCIMFTH3    (0x018)
#define ITBCIGROUP  	(0x80)
#define ITBCIFSM      	(0x40)
#define VBATOVTH     	(0x30)
#define VBATOVTH_4P55V  (0x00)
#define VBATOVTH_4P75V  (0x10)
#define VBATOVTH_4P95V  (0x20)
#define VBATOVTH_5P05V  (0x30)
#define ACCHGOVTH   	(0x0C)
#define ACCHGOVTH_5P5V  (0x00)
#define ACCHGOVTH_6P0V  (0x04)
#define ACCHGOVTH_6P5V  (0x08)
#define ACCHGOVTH_6P8V  (0x0C)
#define VBUSOVTH    	(0x03)
#define VBUSOVTH_5P5V   (0x00)
#define VBUSOVTH_6P0V   (0x01)
#define VBUSOVTH_6P5V   (0x02)
#define VBUSOVTH_6P8V   (0x03)

#define REG_BCIMFTH4   	(0x019)
#define TBATOR1LTH  	(0xFF)

#define REG_BCIMFTH5    (0x01A)
#define TBATOR1HTH  	(0xFF)

#define REG_BCIMFTH6    (0x01B)
#define TBATOR2LTH  	(0xFF)

#define REG_BCIMFTH7    (0x01C)
#define TBATOR2HTH  	(0xFF)

#define REG_BCIMFTH8    (0x01D)
#define ICHGEOCTH   	(0xF0)
#define ICHGEOCTH_SHIFT (4)
#define ICHGLOWTH  	(0x0F)
#define ICHGLOWTH_SHIFT (0)

#define REG_BCIMFTH9    (0x01E)
#define ICHGHIGHTH   	(0xF0)
#define ICHGHIGHTH_SHIFT	(4)
#define HSVOLTTH       	(0x0C)
#define HSVOLTTH_3P95V  (0x00)
#define HSVOLTTH_4P00V  (0x02)
#define HSVOLTTH_4P05V  (0x04)
#define HSVOLTTH_4P10V  (0x0C)
#define HSCURTH        	(0x03)
#define HSCURTH_P360A   (0x00)
#define HSCURTH_P420A   (0x01)
#define HSCURTH_P480A   (0x02)
#define HSCURTH_P541A   (0x03)

#define REG_BCITIMER1   (0x01F)
#define TOTALTMOUT  	(0x20)
#define CHGTMOUT      	(0x10)
#define EOCTMOUT      	(0x08)
#define TOTALTMST     	(0x04)
#define CHGTMST         (0x02)
#define EOCTMST         (0x01)

#define REG_BCITIMER2	(0x020)
#define TOTALTMTH   	(0x30)
#define CHGTMTH       	(0x0C)
#define EOCTMTH       	(0x03)

#define REG_WDKEY       (0x021)
#define WDKEY   	(0xFF)
#define WDKEY_1S    	(0xAA)
#define WDKEY_2S    	(0x65)
#define WDKEY_4S    	(0xDB)
#define WDKEY_8S    	(0xBD)
#define WDKEY_STOP    	(0xF3)
#define WDKEY_WOVF   	(0x33)

#define REG_BCIWD       (0x022)
#define WOVF    (0x08)
#define WEN     (0x04)
#define WSTS    (0x03)
#define WSTS_1S (0x00)
#define WSTS_2S (0x01)
#define WSTS_4S (0x02)
#define WSTS_8S (0x03)

#define REG_BCICTL1	(0x023)
#define CGAIN       	(0x20)
#define TYPEN       	(0x10)
#define ITHEN	 	(0x08)
#define MESVBUS  	(0x04)
#define MESBAT    	(0x02)
#define MESVAC    	(0x01)

#define REG_BCICTL2	(0x024)
#define TRIMEN      	(0x10)
#define CVEN          	(0x08)
#define ITHSENS    	(0x07)

#define REG_BCIVREF1       (0x025)
#define CHGV_LSB    (0xFF)

#define REG_BCIVREF2       (0x026)
#define CHGV_MSB    (0x03)

#define REG_BCIIREF1       (0x027)
#define CHGI_LSB    (0xFF)

#define REG_BCIIREF2       (0x028)
#define CHGI_MSB    (0x03)

#define REG_BCIPWM2       (0x029)
#define PWMDTYCY_MSB    (0x06)
#define PWM     (0x01)

#define REG_BCIPWM1       (0x02A)
#define PWMDTYCY_LSB     (0xFF)

#define REG_BCIVREFCOMB1        (0x02F)
#define CHGVCOMB_LSB    (0xFF)

#define REG_BCIVREFCOMB2        (0x030)
#define CHGVCOMB_MSB    (0x03)

#define REG_BCIIREFCOMB1        (0x031)
#define CHGICOMB_LSB    (0xFF)

#define REG_BCIIREFCOMB2        (0x032)
#define CHGICOMB_MSB    (0x03)

/* Step size and prescaler ratio */
#define VAC_STEP_SIZE   978
#define VAC_PSR_R           100

#define VBUS_STEP_SIZE  684
#define VBUS_PSR_R         100

#define TEMP_STEP_SIZE		147
#define TEMP_PSR_R		100

#define VOLT_STEP_SIZE		588
#define VOLT_PSR_R		100

#define CURR_STEP_SIZE		147
#define CURR_PSR_R1		44
#define CURR_PSR_R2		80

#define BK_VOLT_STEP_SIZE	441
#define BK_VOLT_PSR_R		100

/*the capacity is caclulated based on the right value between now and previou capacity*/
#define BCI_CAP_R_NOW   (20)
#define BCI_CAP_R_PRE    (80)

#define BCI_CHGI_REF_OFF    (0x200)
#define BCI_CHGI_REF_MASK (0x1FF)
#define BCI_CHGI_REF_STEP               (1000)
#define BCI_CHGI_REF_PSR_R1	    (3330)
#define BCI_CHGI_REF_PSR_R2	    (1665)

#define BCI_CHGV_REF_OFF      (0x200)
#define BCI_CHGV_REF_BASE    (3000)  //mV
#define BCI_CHGV_REF_MASK   (0x1FF)
#define BCI_CHGV_REF_STEP    (100)
#define BCI_CHGV_REF_PSR_R	 (585)

#define ENABLE		(1)
#define DISABLE		(0)

typedef enum {
    MADC_CH_BTYPE  = 0,
    MADC_CH_BTEMP  = 1,
    MADC_CH_VBUS   = 8,
    MADC_CH_VBBAT  = 9,
    MADC_CH_ICHG   = 10,
    MADC_CH_VAC    = 11,
    MADC_CH_VBAT   = 12,
}bci_madc_ch;

typedef enum {
    MONITOR_VBAT_LV1  = 1,
    MONITOR_VBAT_LV2  = 2,
    MONITOR_VBAT_LV3  = 3,
    MONITOR_VBAT_LV4  = 4,
}bci_vbat_monitor;

typedef enum {
    RST_UPDATE_NONE = 0,
    RST_UPDATE_LV1  = 1,
    RST_UPDATE_LV2  = 2,
}bci_rst_update;

typedef enum {
    _CHG_PLUG = 0,
    _CHG_UNPLUG,

    /*the charge type notification after charge plug in*/
    _CHG_TYPE_REP,

    /*battery is removed or inserted*/
    _BAT_OPEN,
    _BAT_PACK,

    /*AC charge volatge is over*/
    _VAC_OVER,
    _VAC_DOWN,

    /*USB charge volatge is over*/
    _VBUS_OVER,
    _VBUS_DOWN,

    /*battery volatge for QUK(1)->QUK(2), QUK(3)->QUK(1), such as 2.9V*/
    _VBAT_OVER_TH_1,
    _VBAT_DOWN_TH_1,

    /*battery volatge for QUK(4, 5, 6)<->STP(1,2,3), such as 3.45V*/
    _VBAT_OVER_TH_2,
    _VBAT_DOWN_TH_2,

    /*battery volatge for CPL(2,3,4)->QUK(2), such as 3.90V*/
    _VBAT_OVER_TH_3,
    _VBAT_DOWN_TH_3,

    /*battery volatge for CPL(3,4)->CPL(2), such as 3.95V*/
    _VBAT_OVER_TH_4,
    _VBAT_DOWN_TH_4,

    /*the threshhold volatge for VBAT , such as 4.55V*/
    _VBAT_OVER,
    _VBAT_DOWN,

    /*the charge current for QUK(3)->CPL(1), such as 80mA*/
    _ICHG_OVER_TH_1,
    _ICHG_DOWN_TH_1,

    /*the charge current for QUK(2)->QUK(3), such as 240mA*/
    _ICHG_OVER_TH_2,
    _ICHG_DOWN_TH_2,

    /*the threshhold current for ICHG, such as 800mA*/
    _ICHG_OVER,
    _ICHG_DOWN,

    _TIMEOUT_TYPE,
     /*short delay timerout such 4s*/
    _TIMEOUT_1,
    /*medial delay timerout such as 120min*/
    _TIMEOUT_2,
    /*long delay timerout such as 240min*/
    _TIMEOUT_3,

    _TEMP_OVER,
    _TEMP_DOWN,

    BCI_EVENT_NUM
}bci_charge_event_id;

#define BCI_EVENT(id) (1<<(id))

typedef unsigned long long bci_charge_event_mask;

/*The critical event which must be processed in all charge states*/
#define BCI_CRITICAL_EVENT (  BCI_EVENT(_VAC_OVER) |BCI_EVENT(_VBUS_OVER)  | \
                                                   BCI_EVENT(_ICHG_OVER) |BCI_EVENT(_VBAT_OVER) | \
                                                   BCI_EVENT(_CHG_UNPLUG) | BCI_EVENT(_BAT_OPEN)| \
                                                   BCI_EVENT(_CHG_TYPE_REP) )

typedef struct bci_charge_event{
    bci_charge_event_id id;
    struct list_head list;
}bci_charge_event;

typedef enum{
    BCI_CHGS_NO_CHARGING_DEV    = 0x000001,
    BCI_CHGS_DETECT_DEV         = 0x000002,
    BCI_CHGS_OFF_MODE           = 0x000004,
    BCI_CHGS_CV_MODE            = 0x000008,
    BCI_CHGS_BAT_OPEN           = 0x000010,
    BCI_CHGS_STANDBY_MODE       = 0x000020,
    BCI_CHGS_VBUS_OVER          = 0x000040,
    BCI_CHGS_VAC_OVER           = 0x000080,
    BCI_CHGS_ICHG_OVER          = 0x000100,
    BCI_CHGS_QUK_CHG_1          = 0x000200,
    BCI_CHGS_QUK_CHG_2          = 0x000400,
    BCI_CHGS_QUK_CHG_3          = 0x000800,
    BCI_CHGS_QUK_CHG_4          = 0x001000,
    BCI_CHGS_QUK_CHG_5          = 0x002000,
    BCI_CHGS_QUK_CHG_6          = 0x004000,
    BCI_CHGS_STP_CHG_1          = 0x008000,
    BCI_CHGS_STP_CHG_2          = 0x010000,
    BCI_CHGS_STP_CHG_3          = 0x020000,
    BCI_CHGS_CPL_CHG_1          = 0x040000,
    BCI_CHGS_CPL_CHG_2          = 0x080000,
    BCI_CHGS_CPL_CHG_3          = 0x100000,
    BCI_CHGS_CPL_CHG_4          = 0x200000,

    /* the number of charge states */
    BCI_CHGS_NUM = 22
}bci_charge_state_id;


/*the parameters used to monitor the battery*/
typedef struct bci_battery_param{
    int volt;    //voltage uV
    int pvolt;    //previous voltage uV
    int temp; //temperation
    int tech;  //technology
    int cap;   //capacity percent report for app
    int real_cap; //the capacity detect right now
    int pret;  //present
    int health; //used for power supply describtion
    int status; //used for power supply describtion

}bci_battery_param;

enum {
    ICHG_LEVEL_STOP,
    ICHG_LEVEL_LOW,
    ICHG_LEVEL_HIGH,
};

enum {
    VCHG_LEVEL_STOP,
    VCHG_LEVEL_START,
};

/*the parameters used to monitor the charger*/
typedef struct bci_charge_param{
    charge_type type;
    int ichg_level; //config current to set
    int vchg_level; //config current to set
    int ichg; //current used to charge
    int vac; //voltage used to charge
    int vbus; //vbus of usb
    int online;  //whether charge is pluged
    int timeout; //full timer trigger times
    int judge; // begin full timeout judgement
    int full; //the cap has been full
}bci_charge_param;

/*all the informations used to describe battery and charger*/
typedef struct bci_charge_device_info {
    struct device		*dev;

    /*the information about battery*/
    bci_battery_param mbat_param; //main battery
    bci_battery_param bbat_param;  //backup battery
    bci_charge_param charger_param;

    /*the timer for delay detection*/
    struct timer_list  timer1;
    struct timer_list  timer2;
    struct timer_list  timer3;

    struct delayed_work	 debounce_work;

    struct power_supply	mbat;
    struct power_supply	bbat;
    struct power_supply ac;
    struct power_supply usb;

    /*current state index*/
    int cidx;
    /*preview state index*/
    int pidx;

    /*the interval to update the all parameters, which unit is ms*/
    int update_gab;

    /*start a new update for the parameters, ignore the history parameters.*/
    int update_reset;

    int update_cur;
    unsigned long update_cur_map;
 
    int cap_init;

    /*it is ready after init the battery and charge parameters*/
    int ready;
 
    /*the event queue of charge*/
    struct list_head event_q;

    /*the lock for event queue*/
    spinlock_t	lock;
    struct semaphore   sem;;

    /*used by process thread to wait for the new event*/
    wait_queue_head_t wait;

    /*do not go to sleep while in charging*/
    struct wake_lock charge_lock;
    struct wake_lock plug_lock;
    struct wake_lock cap_lock;
#if defined(CONFIG_TWL4030_BCI_CHARGE_TYPE)
    struct delayed_work	 ac_detect_work;
#endif
    struct work_struct present_task;
    /*monitor the all paramters about battery and charge*/
    struct task_struct	*monitor_thread;
    /*process the event to switch different states*/
    struct task_struct	*process_thread;
}bci_charge_device_info;


/*the declaretion for state handle, which the number must equal to the states*/
static int bci_handle_no_charge_device(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_detect_device(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_off_mode(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_constant_voltage_mode(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_battery_open_mode(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_standby_mode(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_vbus_over_voltage(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_vac_over_voltage(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_ichg_over_current(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_quick_charge_1(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_quick_charge_2(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_quick_charge_3(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_quick_charge_4(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_quick_charge_5(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_quick_charge_6(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_stop_charge_1(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_stop_charge_2(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_stop_charge_3(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_complete_charge_1(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_complete_charge_2(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_complete_charge_3(bci_charge_device_info *di, bci_charge_event_id id, void *data);
static int bci_handle_complete_charge_4(bci_charge_device_info *di, bci_charge_event_id id, void *data);

typedef struct bci_charge_state_description{
    char name[40];
    bci_charge_state_id id;
    /*the mask about the event which has been cared in the state*/
    unsigned long long mask;
    /*state callback handle for events*/
    int (* handle)(bci_charge_device_info *, bci_charge_event_id , void *);
    void *data;
}bci_charge_state_dsp;

/*the table used to descript all charge state*/
bci_charge_state_dsp state_table[BCI_CHGS_NUM] = {
    [0] = {
        .name = "No charging device",
        .id = BCI_CHGS_NO_CHARGING_DEV,
        .mask = BCI_EVENT(_CHG_PLUG),
        .handle = bci_handle_no_charge_device,
    },
    [1] = {
        .name = "Detect device",
        .id = BCI_CHGS_DETECT_DEV,
        .mask = BCI_EVENT(_CHG_UNPLUG) | BCI_EVENT(_CHG_TYPE_REP),
        .handle = bci_handle_detect_device,
    },
    [2] = {
        .name = "Off mode",
        .id = BCI_CHGS_OFF_MODE,
        .mask = (BCI_CRITICAL_EVENT & (~ BCI_EVENT(_VBAT_OVER))) |
                     BCI_EVENT(_VBAT_DOWN),
        .handle = bci_handle_off_mode,
    },
    [3] = {
        .name = "Constant voltage mode",
        .id = BCI_CHGS_CV_MODE,
        .mask = (BCI_CRITICAL_EVENT & (~(BCI_EVENT(_BAT_OPEN) | BCI_EVENT(_VBAT_OVER)))) |
                     BCI_EVENT(_BAT_PACK),
        .handle = bci_handle_constant_voltage_mode,
    },
    [4] = {
        .name = "Battery open mode",
        .id = BCI_CHGS_BAT_OPEN,
        .mask = (BCI_CRITICAL_EVENT & (~(BCI_EVENT(_BAT_OPEN) | BCI_EVENT(_VBAT_OVER)))) ,
        .handle = bci_handle_battery_open_mode,
    },
    [5] = {
        .name = "Standby mode",
        .id = BCI_CHGS_STANDBY_MODE,
        .mask = BCI_CRITICAL_EVENT,
        .handle = bci_handle_standby_mode,
    },
    [6] = {
        .name = "VBUS over voltage",
        .id = BCI_CHGS_VBUS_OVER,
        .mask = BCI_EVENT(_CHG_UNPLUG) | BCI_EVENT(_VBUS_DOWN),
        .handle = bci_handle_vbus_over_voltage,
    },
    [7] = {
        .name = "VAC over voltage",
        .id = BCI_CHGS_VAC_OVER,
        .mask = BCI_EVENT(_CHG_UNPLUG) | BCI_EVENT(_VAC_DOWN),
        .handle = bci_handle_vac_over_voltage,
    },
    [8] = {
        .name = "ICHG over current",
        .id = BCI_CHGS_ICHG_OVER,
        .mask = BCI_EVENT(_CHG_UNPLUG) | BCI_EVENT(_ICHG_DOWN),
        .handle = bci_handle_ichg_over_current,
    },
    [9] = {
        .name = "Quick charge 1",
        .id = BCI_CHGS_QUK_CHG_1,
        .mask = BCI_CRITICAL_EVENT | BCI_EVENT(_VBAT_OVER_TH_1) |
                     BCI_EVENT(_TIMEOUT_3) | BCI_EVENT(_TEMP_OVER),
        .handle = bci_handle_quick_charge_1,
    },
    [10] = {
        .name = "Quick charge 2",
        .id = BCI_CHGS_QUK_CHG_2,
        .mask = BCI_CRITICAL_EVENT | BCI_EVENT(_VBAT_DOWN_TH_1) |
                     BCI_EVENT(_TIMEOUT_3) | BCI_EVENT(_VBAT_OVER_TH_4) |
                     BCI_EVENT(_TEMP_OVER),
        .handle = bci_handle_quick_charge_2,
    },
    [11] = {
        .name = "Quick charge 3",
        .id = BCI_CHGS_QUK_CHG_3,
        .mask = BCI_CRITICAL_EVENT | BCI_EVENT(_VBAT_DOWN_TH_1) |
                     BCI_EVENT(_TIMEOUT_3) | BCI_EVENT(_TIMEOUT_2) |
                     BCI_EVENT(_VBAT_DOWN_TH_4) | BCI_EVENT(_TEMP_OVER),
        .handle = bci_handle_quick_charge_3,
    },
    [12] = {
        .name = "Quick charge 4",
        .id = BCI_CHGS_QUK_CHG_4,
        .mask = BCI_CRITICAL_EVENT | BCI_EVENT(_TIMEOUT_3) |
                     BCI_EVENT(_TEMP_DOWN) | BCI_EVENT(_VBAT_DOWN_TH_2),
        .handle = bci_handle_quick_charge_4,
    },
    [13] = {
        .name = "Quick charge 5",
        .id = BCI_CHGS_QUK_CHG_5,
        .mask = BCI_CRITICAL_EVENT | BCI_EVENT(_TIMEOUT_3) |
                     BCI_EVENT(_TEMP_DOWN) | BCI_EVENT(_VBAT_DOWN_TH_2),
        .handle = bci_handle_quick_charge_5,
    },
    [14] = {
        .name = "Quick charge 6",
        .id = BCI_CHGS_QUK_CHG_6,
        .mask = BCI_CRITICAL_EVENT | BCI_EVENT(_TIMEOUT_3) |
                     BCI_EVENT(_TIMEOUT_2) | BCI_EVENT(_TEMP_DOWN) |
                     BCI_EVENT(_VBAT_DOWN_TH_2),
        .handle = bci_handle_quick_charge_6,
    },
    [15] = {
        .name = "Stop charge 1",
        .id = BCI_CHGS_STP_CHG_1,
        .mask = BCI_CRITICAL_EVENT | BCI_EVENT(_TIMEOUT_3) |
                     BCI_EVENT(_TEMP_DOWN) | BCI_EVENT(_VBAT_OVER_TH_2),
        .handle = bci_handle_stop_charge_1,
    },
    [16] = {
        .name = "Stop charge 2",
        .id = BCI_CHGS_STP_CHG_2,
        .mask = BCI_CRITICAL_EVENT | BCI_EVENT(_TIMEOUT_3) |
                     BCI_EVENT(_TEMP_DOWN) | BCI_EVENT(_VBAT_OVER_TH_2),
        .handle = bci_handle_stop_charge_2,
   },
   [17] = {
        .name = "Stop charge 3",
        .id = BCI_CHGS_STP_CHG_3,
        .mask = BCI_CRITICAL_EVENT | BCI_EVENT(_TIMEOUT_3) |
                     BCI_EVENT(_TIMEOUT_2) | BCI_EVENT(_TEMP_DOWN) |
                     BCI_EVENT(_VBAT_OVER_TH_2),
        .handle = bci_handle_stop_charge_3,
   },
   [18] = {
        .name = "Complete charge 1",
        .id = BCI_CHGS_CPL_CHG_1,
        .mask = BCI_CRITICAL_EVENT | BCI_EVENT(_TIMEOUT_3) |
                     BCI_EVENT(_TIMEOUT_2) | BCI_EVENT(_TIMEOUT_1),
        .handle = bci_handle_complete_charge_1,
  },
  [19] = {
        .name = "Complete charge 2",
        .id = BCI_CHGS_CPL_CHG_2,
        .mask = BCI_CRITICAL_EVENT | BCI_EVENT(_VBAT_DOWN_TH_3),
        .handle = bci_handle_complete_charge_2,
  },
  [20] = {
        .name = "Complete charge 3",
        .id = BCI_CHGS_CPL_CHG_3,
        .mask = BCI_CRITICAL_EVENT | BCI_EVENT(_VBAT_DOWN_TH_3) |
                     BCI_EVENT(_VBAT_DOWN_TH_4) | BCI_EVENT(_TIMEOUT_2) |
                     BCI_EVENT(_TIMEOUT_3),
        .handle = bci_handle_complete_charge_3,
  },
  [21] = {
        .name = "Complete charge 4",
        .id = BCI_CHGS_CPL_CHG_4,
        .mask = BCI_CRITICAL_EVENT | BCI_EVENT(_VBAT_DOWN_TH_3) |
                     BCI_EVENT(_VBAT_DOWN_TH_4) | BCI_EVENT(_VBAT_OVER_TH_4) |
                     BCI_EVENT(_TIMEOUT_1),
        .handle = bci_handle_complete_charge_4,
  },
};

typedef struct bci_charge_config{
    char name[20];
    /*the constant voltage used to CV mode and CV charge mV*/
    int chgv;

    /*charge current refernce mA*/
    int chgi_high;
    int chgi_low;

    /*the threshold of ICHG mA*/
    int ichg_th_1;
    int ichg_th_2;

    /*the threshold of VBAT mV*/
    int vbat_th_1;
    int vbat_th_2;
    int vbat_th_3;
    int vbat_th_4;

    /*the voltage for over-protection mV*/
    int vbat_over;
    int vac_over;
    int vbus_over;
    int ichg_over;

    /*timer out value Second*/
    int timeout_1;
    int timeout_2;
    int timeout_3;

    /*the range of normal and abnormal temperature */
    int temp_normal_low;
    int temp_normal_high;
    int temp_over_low;
    int temp_over_high;
}bci_charge_config;

#define BATT_VOLT_SAMPLE_NUM (13) 
#define DISCHRG_CURVE_NUM (4)
#define CHRG_CURVE_NUM (1)

struct curve_sample{
    int cap;    /*  capacity percent, Max is 1000, Min is 0 */
    int volt;    /* voltage, mV */
};
struct cap_curve{
    /*discharge current in mA*/
    int cur;
    /*sample in the discharge curve*/
    struct curve_sample table[BATT_VOLT_SAMPLE_NUM];
};

static const struct cap_curve charge_curve[CHRG_CURVE_NUM] = {

   [0] = {
        .cur = 500,  // current 0. must is the smallest
        .table = {
                [0] = {990  , 4150},
                [1] = {900  , 4010},
                [2] = {800  , 3913},
                [3] = {700  , 3839},
                [4] = {600  , 3780},
                [5] = {500  , 3723},
                [6] = {400  , 3675},
                [7] = {300  , 3643},
                [8] = {200  , 3584},
                [9] = {100  , 3530},
                [10] = {30  , 3500},
                [11] = {10  , 3301},
                [12] = {0  , 3300},
        },
  },

};

typedef enum{
    CUR_ESTM_BOOT,
    CUR_ESTM_LCD,
    CUR_ESTM_SPEAKER,
    CUR_ESTM_PHONE,
    CUR_ESTM_CAMERA,
    CUR_ESTM_WIFI,
    CUR_ESTM_DATA,
    CUR_ESTM_ID_MAX
}status_point_id;

struct status_point{
    status_point_id id;
    int on;
    int cur;
    char name[20];
};

#define CHARGE_SNIFFER_VOL_GAP_1 (400)
#define CHARGE_SNIFFER_VOL_GAP_2 (100)

/*the interval to update the all parameters, which unit is ms*/
#define BCI_PARAM_RESET_INTERVAL (100)
#define BCI_PARAM_UPDATE_INTERVAL (5000)
#define BCI_CAP_INIT_INTERVAL (10)
#define BCI_CAP_UPATE_INTERVAL (400)
#define CHARGE_DEBOUNCE_TIME (1000) //ms
#define CHARGE_TYPE_TIMEOUT (30) //second

#define AVG_NUM (5)
#define CAP_AVG_NUM (12)

#if defined(CONFIG_MACH_OMAP_KUNLUN_P2) //1000mAH
#define CHARGE_FULL_VOL_1 (4100)
#define CHARGE_FULL_CUR_1 (230)
#define CHARGE_FULL_VOL_2 (4160)
#define CHARGE_FULL_CUR_2 (280)
#define CHARGE_FULL_TIMES (5)

/*the max step than Cap can be varied after one update time*/
#define CAP_CHARGE_MAX_STEP (10)
#define CAP_DISCHARGE_MAX_STEP (8)

#define VBAT_TH_3  (4070)

#define CONFIG_TIMEOUT_2   (120 * 60)
#define CONFIG_TIMEOUT_3   (300 * 60)

#define BCI_BASE_CURRENT 250 // mA
static struct status_point status_table[CUR_ESTM_ID_MAX] = {
    [CUR_ESTM_BOOT] = {
            .id  = CUR_ESTM_BOOT,
            .on = 1,
            .cur = 150,//250,
            .name = "BOOT",
    },
    
    [CUR_ESTM_LCD] = {
            .id  = CUR_ESTM_LCD,
            .on = 1,
            .cur = 80,
            .name = "LCD",
    },
    
    [CUR_ESTM_SPEAKER] = {
            .id  = CUR_ESTM_SPEAKER,
            .on = 0,
            .cur = 0,
            .name = "SPEAKER",
    },
    
    [CUR_ESTM_PHONE] = {
            .id  = CUR_ESTM_PHONE,
            .on = 0,
            .cur = 150,//200,
            .name = "PHONE",
    },
    
    [CUR_ESTM_CAMERA] = {
            .id  = CUR_ESTM_CAMERA,
            .on = 0,
            .cur = 80,//100
            .name = "CAMERA",
    },
    
    [CUR_ESTM_WIFI] = {
            .id  = CUR_ESTM_WIFI,
            .on = 0,
            .cur = 0, //50
            .name = "WIFI",
    },
    [CUR_ESTM_DATA] = {
            .id  = CUR_ESTM_DATA,
            .on = 0,
            .cur = 80, //200
            .name = "DATA",
    },
};
static const struct  cap_curve discharge_curve[DISCHRG_CURVE_NUM] = {
   [0] = {
        .cur = 100,  /* current 0. must is the smallest*/
        .table = {
                [0] = {990  , 4170},
                [1] = {920  , 4050},
                [2] = {800  , 3955},
                [3] = {700  , 3896},
                [4] = {600  , 3844},
                [5] = {500  , 3783},
                [6] = {400  , 3752},
                [7] = {300  , 3742},
                [8] = {200  , 3700},
                [9] = {100  , 3571},
                [10] = {50  , 3536},
                [11] = {10  , 3500},
                [12] = {0    , 3430},
        },
   },

   [1] = {
        .cur = 200,  /* current 1. must is bigger than current 0 !!!!!*/
        .table = {
                [0] = {990  , 4160},
                [1] = {920  , 4040},
                [2] = {800  , 3944},
                [3] = {700  , 3878},
                [4] = {600  , 3822},
                [5] = {500  , 3765},
                [6] = {400  , 3726},
                [7] = {300  , 3700},
                [8] = {200  , 3669},
                [9] = {100  , 3543},
                [10] = {50  , 3515},
                [11] = {10  , 3470},
                [12] = {0    , 3420},
        },
   },

   [2] = {
        .cur = 400,  /* current 2, must is bigger than current 1 !!!!!. */
        .table = {
                [0] = {990  , 4150},
                [1] = {920  , 4030},
                [2] = {800  , 3913},
                [3] = {700  , 3839},
                [4] = {600  , 3780},
                [5] = {500  , 3723},
                [6] = {400  , 3675},
                [7] = {300  , 3643},
                [8] = {200  , 3584},
                [9] = {100  , 3530},
                [10] = {50  , 3500},
                [11] = {10  , 3450},
                [12] = {0    , 3410},
        },
   },

   [3] = {
        .cur = 600,  /* current 3, must is bigger than current 2 !!!!!. */
        .table = {
                [0] = {990  , 4140},
                [1] = {920  , 4020},
                [2] = {800  , 3882},
                [3] = {700  , 3801},
                [4] = {600  , 3738},
                [5] = {500  , 3683},
                [6] = {400  , 3624},
                [7] = {300  , 3576},
                [8] = {200  , 3500},
                [9] = {100  , 3450},
                [10] = {50  , 3420},
                [11] = {10  , 3410},
                [12] = {0    , 3400},
        },
    },
};
#elif defined(CONFIG_MACH_OMAP_KUNLUN_N710E) //1500mAH
#define CHARGE_FULL_VOL_1 (4100)
#define CHARGE_FULL_CUR_1 (230)
#define CHARGE_FULL_VOL_2 (4150)
#define CHARGE_FULL_CUR_2 (280)
#define CHARGE_FULL_TIMES (5)

/*the max step than Cap can be varied after one update time*/
#define CAP_CHARGE_MAX_STEP (8)
#define CAP_DISCHARGE_MAX_STEP (6)

#define VBAT_TH_3  (4100)
#define CONFIG_TIMEOUT_2   (180 * 60)
#define CONFIG_TIMEOUT_3   (360 * 60)

#define BCI_BASE_CURRENT 150 // mA
static struct status_point status_table[CUR_ESTM_ID_MAX] = {
    [CUR_ESTM_BOOT] = {
            .id  = CUR_ESTM_BOOT,
            .on = 1,
            .cur = 150,
            .name = "BOOT",
    },
    
    [CUR_ESTM_LCD] = {
            .id  = CUR_ESTM_LCD,
            .on = 1,
            .cur = 100,
            .name = "LCD",
    },
    
    [CUR_ESTM_SPEAKER] = {
            .id  = CUR_ESTM_SPEAKER,
            .on = 0,
            .cur = 0,
            .name = "SPEAKER",
    },
    
    [CUR_ESTM_PHONE] = {
            .id  = CUR_ESTM_PHONE,
            .on = 0,
            .cur = 150,//200,
            .name = "PHONE",
    },
    
    [CUR_ESTM_CAMERA] = {
            .id  = CUR_ESTM_CAMERA,
            .on = 0,
            .cur = 80,//100
            .name = "CAMERA",
    },
    
    [CUR_ESTM_WIFI] = {
            .id  = CUR_ESTM_WIFI,
            .on = 0,
            .cur = 0, //50
            .name = "WIFI",
    },
    [CUR_ESTM_DATA] = {
            .id  = CUR_ESTM_DATA,
            .on = 0,
            .cur = 80, //200
            .name = "DATA",
    },
};
static const struct  cap_curve discharge_curve[DISCHRG_CURVE_NUM] = {
   [0] = {
        .cur = 100,  /* current 0. must is the smallest*/
        .table = {
                [0] = {990  , 4167},
                [1] = {900  , 4049},
                [2] = {800  , 3970},
                [3] = {700  , 3897},
                [4] = {600  , 3830},
                [5] = {500  , 3780},
                [6] = {400  , 3751},
                [7] = {300  , 3730},
                [8] = {200  , 3705},
                [9] = {100  , 3663},
                [10] = {30  , 3598},
                [11] = {10  , 3500},
                [12] = {0    , 3450},
        },
   },

   [1] = {
        .cur = 200,  /* current 1. must is bigger than current 0 !!!!!*/
        .table = {
                [0] = {990  , 4156},
                [1] = {900  , 4036},
                [2] = {800  , 3954},
                [3] = {700  , 3876},
                [4] = {600  , 3811},
                [5] = {500  , 3764},
                [6] = {400  , 3735},
                [7] = {300  , 3712},
                [8] = {200  , 3687},
                [9] = {100  , 3641},
                [10] = {30  , 3551},
                [11] = {10  , 3500},
                [12] = {0    , 3450},
        },
   },

   [2] = {
        .cur = 400,  /* current 2, must is bigger than current 1 !!!!!. */
        .table = {
                [0] = {990  , 4120},
                [1] = {900  , 3982},
                [2] = {800  , 3898},
                [3] = {700  , 3819},
                [4] = {600  , 3761},
                [5] = {500  , 3715},
                [6] = {400  , 3685},
                [7] = {300  , 3662},
                [8] = {200  , 3637},
                [9] = {100  , 3589},
                [10] = {30  , 3451},
                [11] = {10  , 3440},
                [12] = {0    , 3430},
        },
   },

   [3] = {
        .cur = 600,  /* current 3, must is bigger than current 2 !!!!!. */
        .table = {
                [0] = {990  , 4100},
                [1] = {900  , 3937},
                [2] = {800  , 3846},
                [3] = {700  , 3770},
                [4] = {600  , 3710},
                [5] = {500  , 3665},
                [6] = {400  , 3631},
                [7] = {300  , 3608},
                [8] = {200  , 3581},
                [9] = {100  , 3536},
                [10] = {30  , 3420},
                [11] = {10  , 3410},
                [12] = {0    , 3400},
        },
    },
};
#endif

bci_charge_config config_table[CHARGE_TYPE_MAX] ={
    [TYPE_UNKNOW] = {
        .name             = "UNKNOW",
        .chgv             = 4230,
        .chgi_high        = 480,
        .chgi_low         = 300,
        .ichg_th_1        = 80,
        .ichg_th_2        = 300,
        .vbat_th_1        = 2900,
        .vbat_th_2        = 3450,
        .vbat_th_3        = VBAT_TH_3,
        .vbat_th_4        = 4150,
        .vbat_over        = 4500,
        .vac_over         = 6000,
        .vbus_over        = 5500,
        .ichg_over        = 900,
        .timeout_1        = 4,
        .timeout_2        = CONFIG_TIMEOUT_2,
        .timeout_3        = CONFIG_TIMEOUT_3,
    },
    [TYPE_AC] = {
        .name             = "AC",
        .chgv             = 4230,
        .chgi_high        = 500,
        .chgi_low         = 300,
        .ichg_th_1        = 80,
        .ichg_th_2        = 300,
        .vbat_th_1        = 2900,
        .vbat_th_2        = 3450,
        .vbat_th_3        = VBAT_TH_3,
        .vbat_th_4        = 4150,
        .vbat_over        = 4500,
        .vac_over         = 6000,
        .vbus_over        = 5500,
        .ichg_over        = 900,
        .timeout_1        = 4,
        .timeout_2        = CONFIG_TIMEOUT_2,
        .timeout_3        = CONFIG_TIMEOUT_3,
    },
    [TYPE_USB] = {
        .name             = "USB",
        .chgv             = 4230,
        .chgi_high        = 480,
        .chgi_low         = 300,
        .ichg_th_1        = 80,
        .ichg_th_2        = 300,
        .vbat_th_1        = 2900,
        .vbat_th_2        = 3450,
        .vbat_th_3        = VBAT_TH_3,
        .vbat_th_4        = 4150,
        .vbat_over        = 4500,
        .vac_over         = 6000,
        .vbus_over        = 5500,
        .ichg_over        = 900,
        .timeout_1        = 4,
        .timeout_2        = CONFIG_TIMEOUT_2,
        .timeout_3        = CONFIG_TIMEOUT_3,
    },
};

#endif
