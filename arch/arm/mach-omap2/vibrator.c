#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/wakelock.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/clk.h>

#include <../../../drivers/staging/android/timed_output.h>

#ifndef CONFIG_MACH_OMAP_KUNLUN_P2
#define VIBRATOR_GPIO_CTL  // yx add vibrator ctl gpio15  2011.04.08
#endif

#ifndef VIBRATOR_GPIO_CTL
#include <linux/i2c/twl.h>

#else
#include <linux/module.h>
#include <linux/earlysuspend.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/gpio.h>

static int vibartor_gpio_ctl = 28;
#endif

#define TWL_LEDEN                   (0x00)
#define TWL_CODEC_MODE              (0x01)
#define TWL_VIBRATOR_CFG            (0x05)
#define TWL_VIBRA_CTL               (0x45)
#define TWL_VIBRA_SET               (0x46)

#define TWL_CODEC_MODE_CODECPDZ		(0x01 << 1)
#define TWL_VIBRA_CTL_VIBRA_EN		(0x01)
#define TWL_VIBRA_CTL_VIBRA_DIS    	(0x00)

#define VIBRATOR_ON                 (0x01)
#define VIBRATOR_OFF                (0x00)

#define MAX_TIMEOUT                 (10000)

static struct vibrator {
    struct wake_lock wklock;
    struct hrtimer timer;
    struct mutex lock;
    struct work_struct work;
} vibdata;


static int vibrator_hardward_init(void)
{
    int ret = 0;
#ifndef VIBRATOR_GPIO_CTL
    u8 reg_value;

    ret = twl_i2c_read_u8(TWL4030_MODULE_LED, &reg_value,
		TWL_LEDEN);

    reg_value &= 0xFC;
    ret = twl_i2c_write_u8(TWL4030_MODULE_LED, reg_value,
		TWL_LEDEN);

    ret = twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE, TWL_CODEC_MODE_CODECPDZ,
		TWL_CODEC_MODE);
#else

   gpio_direction_output(vibartor_gpio_ctl,0);
#endif

    return ret;
}

static void vibartor_hardware_on(void)
{
#ifndef VIBRATOR_GPIO_CTL
    twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE, TWL_VIBRA_CTL_VIBRA_EN,
		TWL_VIBRA_CTL);
#else
		gpio_set_value(vibartor_gpio_ctl,1);
#endif



}

static void vibartor_hardware_off(void)
{
#ifndef VIBRATOR_GPIO_CTL
    twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE, TWL_VIBRA_CTL_VIBRA_DIS,
			TWL_VIBRA_CTL);
#else
		gpio_set_value(vibartor_gpio_ctl,0);

#endif
    wake_unlock(&vibdata.wklock);

}



static int vibrator_get_time(struct timed_output_dev *dev)
{
    int lefttime = 0;

    if(hrtimer_active(&vibdata.timer))
    {
        ktime_t r = hrtimer_get_remaining(&vibdata.timer);
        lefttime = r.tv.sec * 1000 + r.tv.nsec / 1000000;
    }

    return lefttime;
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
    mutex_lock(&vibdata.lock);
    hrtimer_cancel(&vibdata.timer);
    cancel_work_sync(&vibdata.work);

    if (value) {
            wake_lock(&vibdata.wklock);
            vibartor_hardware_on();

            if (value > 0) {
                if (value > MAX_TIMEOUT)
                    value = MAX_TIMEOUT;

                hrtimer_start(&vibdata.timer,
                    ns_to_ktime((u64)value * NSEC_PER_MSEC),
                    HRTIMER_MODE_REL);
            }
    } else
        vibartor_hardware_off();

    mutex_unlock(&vibdata.lock);
}

static void vibrator_work(struct work_struct *work)
{
    vibartor_hardware_off();
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	schedule_work(&vibdata.work);
	return HRTIMER_NORESTART;
}

static struct timed_output_dev kunlun_vibartor = {
    .name = "vibrator",
    .get_time = vibrator_get_time,
    .enable = vibrator_enable,
};

void __init kunlun_init_vibrator(void)
{
    int ret = 0;
    hrtimer_init(&vibdata.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    vibdata.timer.function = vibrator_timer_func;

    INIT_WORK(&vibdata.work, vibrator_work);

    wake_lock_init(&vibdata.wklock, WAKE_LOCK_SUSPEND, "vibrator");
    mutex_init(&vibdata.lock);
#ifndef CONFIG_MACH_OMAP_KUNLUN_P2
    vibrator_hardward_init();
#endif
    ret = timed_output_dev_register(&kunlun_vibartor);

    if (ret < 0) {
        mutex_destroy(&vibdata.lock);
        wake_lock_destroy(&vibdata.wklock);
    }
}
