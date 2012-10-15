#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include "audio_test.h"
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include "audio_test.h"

#define DRVNAME "audio_test"
#define MAX_PINS 32
#define APOLLO_CP_ENABLE_GPIO 22
static int  major=0;
static struct device *audiotest_device;
struct cdev apollo_audio_switch_cdev;	/* use 1 cdev for all pins */
void __iomem *io_base;
void __iomem *io_base_mux;

static unsigned int reg_hs_sel = 0;
static unsigned int reg_hfl_ctl = 0;
static unsigned int reg_hfr_ctl = 0;


unsigned char codec_reg[TWL4030_CACHEREGNUM] = {0};

static int save_codec_env(void)
{
	int reg = 0;
	
	for(reg = TWL4030_REG_CODEC_MODE;reg < TWL4030_CACHEREGNUM;reg++)
	{
		codec_readAreg(reg,&(codec_reg[reg]));
	}

    return 0;
}

static int restore_codec_env(void)
{
	int reg = 0;
	
	for(reg = TWL4030_REG_CODEC_MODE;reg < TWL4030_CACHEREGNUM;reg++)
	{
		codec_writeAreg(reg,codec_reg[reg]);
	}

    return 0;
}

int twl4030_codec_tog_on(void)
{
    unsigned char data = 0;
    int ret = 0;

    codec_readAreg(TWL4030_REG_CODEC_MODE,&data);

    data &= ~BIT_CODEC_MODE_CODECPDZ_M;
    codec_writeAreg(TWL4030_REG_CODEC_MODE, data);

    udelay(10);			/* 10 ms delay for power settling */
    data |= BIT_CODEC_MODE_CODECPDZ_M;
    codec_writeAreg(TWL4030_REG_CODEC_MODE, data);

    udelay(10);			/* 10 ms delay for power settling */

    return ret;
}

void codec_disable_outputs(void)
{
    unsigned char opt = 0;

    /* Turn RX filters off */
    codec_readAreg(TWL4030_REG_OPTION,&opt);

    opt &= ~(BIT_OPTION_ARXL1_VRX_EN_M | BIT_OPTION_ARXR1_EN_M
	     | BIT_OPTION_ARXL2_EN_M | BIT_OPTION_ARXR2_EN_M);
    codec_writeAreg(TWL4030_REG_OPTION, opt);

    /* Turn DACs off */
    codec_writeAreg(TWL4030_REG_AVDAC_CTL, 0x00);

    /* Headset */
    codec_writeAreg(TWL4030_REG_HS_SEL, 0x00);

    /* Hands free */
    codec_writeAreg(TWL4030_REG_HFL_CTL, 0x00);

    codec_writeAreg(TWL4030_REG_HFR_CTL, 0x00);

    /* Earpiece */
    codec_writeAreg(TWL4030_REG_EAR_CTL, 0x00);

    /* Carkit */
    codec_writeAreg( TWL4030_REG_PRECKL_CTL, 0x00);

    codec_writeAreg(TWL4030_REG_PRECKR_CTL, 0x00);

}


void switch_outs(int codec_mode, int source)
{
    unsigned char option = 0;
    unsigned char ear_ctl = 0;
    unsigned char hs_sel = 0;
    unsigned char hs_pop = 0;
    unsigned char hfl_ctl = 0;
    unsigned char hfr_ctl = 0;
    unsigned char preckl_ctl = 0;
    unsigned char preckr_ctl = 0;
    unsigned char dac = 0;
    unsigned char data = 0;
    unsigned char misc = 0;
	
    audio_debug("switch_outs\n");
    codec_disable_outputs();
    codec_readAreg(TWL4030_REG_OPTION,&option);
    option &= ~(BIT_OPTION_ARXR2_EN_M | BIT_OPTION_ARXL2_EN_M);

    codec_readAreg(TWL4030_REG_AVDAC_CTL,&dac);
    dac &= ~(BIT_AVDAC_CTL_VDAC_EN_M | BIT_AVDAC_CTL_ADACL2_EN_M
	     | BIT_AVDAC_CTL_ADACR2_EN_M);

    switch (source) {
	/* Headset */
    case OUTPUT_STEREO_HEADSET:
	if (codec_mode == AUDIO_MODE) {
	    option |= BIT_OPTION_ARXR2_EN_M | BIT_OPTION_ARXL2_EN_M;
	    hs_sel = BIT_HS_SEL_HSOR_AR2_EN_M | BIT_HS_SEL_HSOL_AL2_EN_M;
	    dac |= BIT_AVDAC_CTL_ADACL2_EN_M | BIT_AVDAC_CTL_ADACR2_EN_M;
        codec_readAreg(TWL4030_REG_MISC_SET_1,&misc);
        misc &= ~(TWL4030_FMLOOP_EN);
        codec_writeAreg( TWL4030_REG_MISC_SET_1, misc);
	} else if (codec_mode == VOICE_MODE) {
	    option |= BIT_OPTION_ARXL1_VRX_EN_M;
	    hs_sel = BIT_HS_SEL_HSOL_VOICE_EN_M;
	    dac |= BIT_AVDAC_CTL_VDAC_EN_M;
	} else if (codec_mode == LOOPBACK_MODE) {
	    option |= BIT_OPTION_ARXL1_VRX_EN_M;
	    hs_sel = BIT_HS_SEL_HSOL_VOICE_EN_M;
	    dac = 0;
        codec_readAreg(TWL4030_REG_MISC_SET_1,&misc);
        misc |= TWL4030_FMLOOP_EN;
        codec_writeAreg(TWL4030_REG_MISC_SET_1, misc);
        codec_writeAreg(TWL4030_REG_VDL_APGA_CTL, 0x25);
	}
	hs_pop = BIT_HS_POPN_SET_VMID_EN_M;
	break;
	/* Hands free */
    case OUTPUT_HANDS_FREE_CLASSD:
	if (codec_mode == AUDIO_MODE) {
	    option |= BIT_OPTION_ARXR2_EN_M | BIT_OPTION_ARXL2_EN_M;
	    hfl_ctl = (HANDS_FREEL_AL2 << BIT_HFL_CTL_HFL_INPUT_SEL)
		| BIT_HFL_CTL_HFL_REF_EN_M;
	    hfr_ctl = (HANDS_FREER_AR2 << BIT_HFR_CTL_HFR_INPUT_SEL)
		| BIT_HFR_CTL_HFR_REF_EN_M;
	    dac |= BIT_AVDAC_CTL_ADACL2_EN_M | BIT_AVDAC_CTL_ADACR2_EN_M;
	} else if (codec_mode == VOICE_MODE) {
	    option |= BIT_OPTION_ARXL1_VRX_EN_M;
	    hfl_ctl = (HANDS_FREEL_VOICE << BIT_HFL_CTL_HFL_INPUT_SEL)
		| BIT_HFL_CTL_HFL_REF_EN_M;
	    dac |= BIT_AVDAC_CTL_VDAC_EN_M;
	} else if (codec_mode == LOOPBACK_MODE) {
	    option |= BIT_OPTION_ARXL1_VRX_EN_M;
	    hfl_ctl = (HANDS_FREEL_VOICE << BIT_HFL_CTL_HFL_INPUT_SEL)
		| BIT_HFL_CTL_HFL_REF_EN_M;
	    dac = 0;
        codec_readAreg(TWL4030_REG_MISC_SET_1,&misc );
        misc |= TWL4030_FMLOOP_EN;
        codec_writeAreg(TWL4030_REG_MISC_SET_1, misc);
        codec_writeAreg(TWL4030_REG_VDL_APGA_CTL, 0x25);
	}
	
	break;
	/* Earpiece */
    case OUTPUT_MONO_EARPIECE:
	if (codec_mode == AUDIO_MODE) {
	    option |= BIT_OPTION_ARXL2_EN_M;
	    ear_ctl = BIT_EAR_CTL_EAR_AL2_EN_M;
	    dac |= BIT_AVDAC_CTL_ADACL2_EN_M;
	} else if (codec_mode == VOICE_MODE) {
	    option |= BIT_OPTION_ARXL1_VRX_EN_M;
	    ear_ctl = BIT_EAR_CTL_EAR_VOICE_EN_M;
	    dac |= BIT_AVDAC_CTL_VDAC_EN_M;
	} else if (codec_mode == LOOPBACK_MODE) {
	    option |= BIT_OPTION_ARXL1_VRX_EN_M;
	    ear_ctl = BIT_EAR_CTL_EAR_VOICE_EN_M;
	    dac = 0;
	    codec_readAreg(TWL4030_REG_MISC_SET_1,&misc);
        misc |= TWL4030_FMLOOP_EN;
        codec_writeAreg(TWL4030_REG_MISC_SET_1, misc);
        codec_writeAreg(TWL4030_REG_VDL_APGA_CTL, 0x0f);
	}
	break;
	/* Carkit */
    case OUTPUT_CARKIT:
	if (codec_mode == AUDIO_MODE) {
	    option |= BIT_OPTION_ARXL2_EN_M | BIT_OPTION_ARXR2_EN_M;
	    preckl_ctl = BIT_PRECKL_CTL_PRECKL_AL2_EN_M
		| BIT_PRECKL_CTL_PRECKL_EN_M;
	    preckr_ctl = BIT_PRECKR_CTL_PRECKR_AR2_EN_M
		| BIT_PRECKR_CTL_PRECKR_EN_M;
	    dac |= BIT_AVDAC_CTL_ADACL2_EN_M | BIT_AVDAC_CTL_ADACR2_EN_M;
	} else if (codec_mode == VOICE_MODE) {
	    option |= BIT_OPTION_ARXL1_VRX_EN_M;
	    preckl_ctl = BIT_PRECKL_CTL_PRECKL_VOICE_EN_M
		| BIT_PRECKL_CTL_PRECKL_EN_M;
	    dac |= BIT_AVDAC_CTL_VDAC_EN_M;
	}
	break;
    }

    codec_writeAreg(TWL4030_REG_OPTION, option);

    if (ear_ctl) {
	codec_readAreg(TWL4030_REG_EAR_CTL,&data);

	data &= ~(BIT_EAR_CTL_EAR_AR1_EN_M
		  | BIT_EAR_CTL_EAR_AL2_EN_M
		  | BIT_EAR_CTL_EAR_AL1_EN_M | BIT_EAR_CTL_EAR_VOICE_EN_M);
	data |= ear_ctl;
	data |=0x30;
	codec_writeAreg(TWL4030_REG_EAR_CTL, data);
    }

    if (hs_sel) {
	codec_writeAreg(TWL4030_REG_HS_SEL, hs_sel);

    }

    if (hs_pop) {
	codec_writeAreg(TWL4030_REG_HS_POPN_SET, hs_pop);

	hs_pop |= BIT_HS_POPN_SET_RAMP_EN_M;
	udelay(1);		/* Require a short delay before enabling ramp */
	codec_writeAreg(TWL4030_REG_HS_POPN_SET, hs_pop);

    }

    if (hfl_ctl) {
	codec_writeAreg(TWL4030_REG_HFL_CTL, hfl_ctl);

	hfl_ctl |= BIT_HFL_CTL_HFL_RAMP_EN_M;
	codec_writeAreg(TWL4030_REG_HFL_CTL, hfl_ctl);

	hfl_ctl |= BIT_HFL_CTL_HFL_LOOP_EN_M;
	codec_writeAreg(TWL4030_REG_HFL_CTL, hfl_ctl);

	hfl_ctl |= BIT_HFL_CTL_HFL_HB_EN_M;
	codec_writeAreg(TWL4030_REG_HFL_CTL, hfl_ctl);

    }

    if (hfr_ctl) {
	codec_writeAreg(TWL4030_REG_HFR_CTL, hfr_ctl);

	hfr_ctl |= BIT_HFR_CTL_HFR_RAMP_EN_M;
	codec_writeAreg(TWL4030_REG_HFR_CTL, hfr_ctl);

	hfr_ctl |= BIT_HFR_CTL_HFR_LOOP_EN_M;
	codec_writeAreg(TWL4030_REG_HFR_CTL, hfr_ctl);

	hfr_ctl |= BIT_HFR_CTL_HFR_HB_EN_M;
	codec_writeAreg(TWL4030_REG_HFR_CTL, hfr_ctl);

    }

    if (preckl_ctl) {
	codec_writeAreg(TWL4030_REG_PRECKL_CTL, preckl_ctl);

    }

    if (preckr_ctl) {
	codec_writeAreg(TWL4030_REG_PRECKR_CTL, preckr_ctl);

    }

    codec_writeAreg(TWL4030_REG_AVDAC_CTL, dac);

}

/*
 * void_disable_inputs - Reset all the inputs
 */
void codec_disable_inputs(void)
{
    u8 opt = 0;

    audio_debug("KERNEL DEBUGcodec_disable_inputs\n");
    /* Turn TX filters off */
    codec_readAreg(TWL4030_REG_OPTION,&opt);

    opt &= ~(BIT_OPTION_ATXL1_EN_M | BIT_OPTION_ATXR1_EN_M
	     | BIT_OPTION_ATXL2_VTXL_EN_M | BIT_OPTION_ATXR2_VTXR_EN_M);
    codec_writeAreg(TWL4030_REG_OPTION, opt);

    /* Turn ADCs off */
    codec_writeAreg(TWL4030_REG_AVADC_CTL, 0x00);

    /* Turn mics bias off */
    codec_writeAreg(TWL4030_REG_MICBIAS_CTL, 0x00);

    codec_writeAreg(TWL4030_REG_ANAMICL, 0x00);

    codec_writeAreg(TWL4030_REG_ANAMICR, 0x00);

    codec_writeAreg(TWL4030_REG_ADCMICSEL, 0x00);

    codec_writeAreg(TWL4030_REG_DIGMIXING, 0x00);

}

void switch_inputs(int codec_mode, int source)
{
    unsigned char micbias = 0;
    unsigned char mic_l = 0;
    unsigned char mic_r = 0;
    unsigned char adc = 0;
    unsigned char option = 0;

    audio_debug("switch_input\n");

    codec_disable_inputs();

    codec_readAreg(TWL4030_REG_OPTION,&option);

    codec_readAreg(TWL4030_REG_AVADC_CTL,&adc);

    option &= ~(BIT_OPTION_ATXL1_EN_M
		| BIT_OPTION_ATXR1_EN_M
		| BIT_OPTION_ATXL2_VTXL_EN_M | BIT_OPTION_ATXR2_VTXR_EN_M);
    adc &= ~(BIT_AVADC_CTL_ADCL_EN_M | BIT_AVADC_CTL_ADCR_EN_M);

    switch (source) {
	/* Headset mic */
    case INPUT_HEADSET_MIC:
	micbias = BIT_MICBIAS_CTL_HSMICBIAS_EN_M;
	mic_l = BIT_ANAMICL_HSMIC_EN_M | BIT_ANAMICL_MICAMPL_EN_M;
	adc |= BIT_AVADC_CTL_ADCL_EN_M;
	if (codec_mode == AUDIO_MODE) {
	    option |= BIT_OPTION_ATXL1_EN_M;
	} else if (codec_mode == VOICE_MODE) {
	    option |= BIT_OPTION_ATXL2_VTXL_EN_M;
	    mic_l |= (OFFSET_CNCL_VRX << BIT_ANAMICL_OFFSET_CNCL_SEL);
	}

	break;
	/* Main mic */
    case INPUT_MAIN_MIC:
	mic_l = BIT_ANAMICL_MAINMIC_EN_M | BIT_ANAMICL_MICAMPL_EN_M;
	micbias = BIT_MICBIAS_CTL_MICBIAS1_EN_M;
	adc |= BIT_AVADC_CTL_ADCL_EN_M;
	if (codec_mode == AUDIO_MODE) {
	    option |= BIT_OPTION_ATXL1_EN_M;
	} else if (codec_mode == VOICE_MODE) {
	    option |= BIT_OPTION_ATXL2_VTXL_EN_M;
	    mic_l |= (OFFSET_CNCL_VRX << BIT_ANAMICL_OFFSET_CNCL_SEL);
	}

	break;
	/* Sub mic */
    case INPUT_SUB_MIC:
	micbias = BIT_MICBIAS_CTL_MICBIAS2_EN_M;
	mic_r = BIT_ANAMICR_SUBMIC_EN_M | BIT_ANAMICR_MICAMPR_EN_M;
	adc |= BIT_AVADC_CTL_ADCR_EN_M;
	if (codec_mode == AUDIO_MODE)
	    option |= BIT_OPTION_ATXR1_EN_M;
	else if (codec_mode == VOICE_MODE)
	    option |= BIT_OPTION_ATXR2_VTXR_EN_M;

	break;
	/* Auxiliar input */
    case INPUT_AUX:
	mic_l = BIT_ANAMICL_AUXL_EN_M | BIT_ANAMICL_MICAMPL_EN_M;
	mic_r = BIT_ANAMICR_AUXR_EN_M | BIT_ANAMICR_MICAMPR_EN_M;
	adc |= BIT_AVADC_CTL_ADCL_EN_M | BIT_AVADC_CTL_ADCR_EN_M;
	if (codec_mode == AUDIO_MODE) {
	    option |= BIT_OPTION_ATXL1_EN_M | BIT_OPTION_ATXR1_EN_M;
	} else if (codec_mode == VOICE_MODE) {
	    option |= BIT_OPTION_ATXL2_VTXL_EN_M | BIT_OPTION_ATXR2_VTXR_EN_M;
	    mic_l |= (OFFSET_CNCL_VRX << BIT_ANAMICL_OFFSET_CNCL_SEL);
	}
	break;
	/* Carkit */
    case INPUT_CARKIT:
	mic_l = BIT_ANAMICL_CKMIC_EN_M | BIT_ANAMICL_MICAMPL_EN_M;
	adc |= BIT_AVADC_CTL_ADCL_EN_M;
	if (codec_mode == AUDIO_MODE) {
	    option |= BIT_OPTION_ATXL1_EN_M;
	} else if (codec_mode == VOICE_MODE) {
	    option |= BIT_OPTION_ATXL2_VTXL_EN_M;
	    mic_l |= (OFFSET_CNCL_VRX << BIT_ANAMICL_OFFSET_CNCL_SEL);
	}
	break;
    }

    codec_writeAreg(TWL4030_REG_OPTION, option);

    codec_writeAreg(TWL4030_REG_MICBIAS_CTL, micbias);

    codec_writeAreg(TWL4030_REG_ANAMICL, mic_l);

    codec_writeAreg(TWL4030_REG_ANAMICR, mic_r);

    codec_writeAreg(TWL4030_REG_AVADC_CTL, adc);

    /* Use ADC TX2 routed to digi mic - we dont use tx2 path
     * - route it to digi mic1
     */
    codec_writeAreg(TWL4030_REG_ADCMICSEL, 0x00);

    codec_writeAreg(TWL4030_REG_DIGMIXING, 0x00);

    codec_writeAreg(TWL4030_REG_ANAMIC_GAIN, 0x2d);
}


void twl4030_codec_on(void)
{
	u8 data = 0;

	codec_readAreg(TWL4030_REG_CODEC_MODE,&data);

	if (data & BIT_CODEC_MODE_CODECPDZ_M)
		return;

	data |= BIT_CODEC_MODE_CODECPDZ_M;
 	codec_writeAreg(TWL4030_REG_CODEC_MODE, data);
}



void twl4030_codec_off(void)
{
	u8 data = 0;
	
    audio_debug("twl4030_codec_off\n");
	codec_readAreg(TWL4030_REG_CODEC_MODE,&data );

	if (!(data & BIT_CODEC_MODE_CODECPDZ_M))
         {
		return;
         }

	data &= ~BIT_CODEC_MODE_CODECPDZ_M;
 	codec_writeAreg(TWL4030_REG_CODEC_MODE, data);
}

static int twl4030_cleanup(void)
{
	int ret = 0;

	audio_debug("KERNEL DEBUG twl4030_cleanup\n");
	ret = codec_writeAreg(TWL4030_REG_OPTION, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_OPTION, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_MICBIAS_CTL, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_ANAMICL, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_ANAMICR, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_AVADC_CTL, 0x00);
	if (ret)
		goto cleanup_exit;
	
	ret = codec_writeAreg(TWL4030_REG_ADCMICSEL, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_DIGMIXING, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_ATXL1PGA, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_ATXR1PGA, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_AVTXL2PGA, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_AVTXR2PGA, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_AUDIO_IF, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_VOICE_IF, 0x00);
	if (ret)
		goto cleanup_exit;
	
	ret = codec_writeAreg(TWL4030_REG_ARXL1PGA, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_ARXR1PGA, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_ARXL2PGA, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_ARXR2PGA, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_AVDAC_CTL, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_ARXL1_APGA_CTL, 0x00);
	if (ret)
		goto cleanup_exit;

	ret =codec_writeAreg(TWL4030_REG_ARXR1_APGA_CTL, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_ARXL2_APGA_CTL, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_ARXR2_APGA_CTL, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_ALC_CTL, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_BTPGA, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_DTMF_FREQSEL, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_DTMF_TONOFF, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_DTMF_PGA_CTL1, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_DTMF_PGA_CTL2, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_DTMF_WANONOFF, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_VRXPGA, 0x00);
	if (ret)
		goto cleanup_exit;

	ret = codec_writeAreg(TWL4030_REG_VDL_APGA_CTL, 0x00);
cleanup_exit:
	if (ret)
		printk(KERN_ERR "Cleanup error %d\n", ret);

	return ret;
}


/*
 * twl4030_set_codec_mode - Set code operational mode: audio, voice
 */
int twl4030_set_codec_mode(int codec_mode)
{
	u8 data = 0;
	int ret = 0;

	codec_readAreg(TWL4030_REG_CODEC_MODE,&data);

	/* Option 1: Audio mode */
	if (codec_mode == AUDIO_MODE) {
		data |= BIT_CODEC_MODE_OPT_MODE_M;
	/* Option 2: Voice/audio mode */
	} else if (codec_mode == VOICE_MODE) {
		data &= ~BIT_CODEC_MODE_OPT_MODE_M;
	} else {
		printk(KERN_ERR "Invalid codec mode\n");
		return -EPERM;
	}
	codec_writeAreg(TWL4030_REG_CODEC_MODE, data);

	return ret;
}


/*
 * twl4030_configure_codec_interface - Configure the codec's data path
 */
static int twl4030_configure_codec_interface(int codec_mode)
{
	int reg = 0;
	u8 data = 0;
	u8 data_width = 0;
	int ret = 0;
	
	/* Check sample width */
#if 0
	if (twl4030_local.bitspersample ==
				AUDIO_SAMPLE_DATA_WIDTH_16) {
		/* Data width 16-bits */
		data_width = AUDIO_DATA_WIDTH_16SAMPLE_16DATA;
	} else if (twl4030_local.bitspersample ==
				AUDIO_SAMPLE_DATA_WIDTH_24) {
		/* Data width 24-bits */
		data_width = AUDIO_DATA_WIDTH_32SAMPLE_24DATA;
	} else {
		printk(KERN_ERR "Unknown sample width %d\n",
			twl4030_local.bitspersample);
		return -EPERM;
	}
#endif
	data_width = AUDIO_DATA_WIDTH_16SAMPLE_16DATA;
	/* No need to set BIT_AUDIO_IF_CLK256FS_EN_M -not using it as CLKS!! */
	/* configure the audio IF of codec- Application Mode */
	if (codec_mode == AUDIO_MODE) {
		reg = TWL4030_REG_AUDIO_IF;
		data = data_width << BIT_AUDIO_IF_DATA_WIDTH
			| AUDIO_DATA_FORMAT_I2S << BIT_AUDIO_IF_AIF_FORMAT
			| BIT_AUDIO_IF_AIF_EN_M;
	} else if (codec_mode == VOICE_MODE) {
		reg = TWL4030_REG_VOICE_IF;
		data = BIT_VOICE_IF_VIF_SLAVE_EN_M
			| BIT_VOICE_IF_VIF_DIN_EN_M
			| BIT_VOICE_IF_VIF_DOUT_EN_M
			| BIT_VOICE_IF_VIF_SUB_EN_M
			| BIT_VOICE_IF_VIF_EN_M;
	} else {
		printk(KERN_ERR "Invalid codec mode\n");
		return -EPERM;
	}

	ret = codec_writeAreg(reg, data);
	if (ret)
		printk(KERN_ERR "Failed configuring codec interface %d\n", ret);

	return ret;
}

int twl4030_set_voice_samplerate(long sample_rate)
{
	u8 data = 0;
	int ret = 0;
	
	codec_readAreg(TWL4030_REG_CODEC_MODE,&data);

	if (sample_rate == NARROWBAND_FREQ) {
		data &= ~BIT_CODEC_MODE_SEL_16K_M;
	} else if (sample_rate == WIDEBAND_FREQ) {
		data |= BIT_CODEC_MODE_SEL_16K_M;
	} else  {
		printk(KERN_ERR "Invalid sample rate: %ld", sample_rate);
		return -EPERM;
	}

	ret = codec_writeAreg(TWL4030_REG_CODEC_MODE, data);
	if (ret) {
		printk(KERN_ERR "Failed to write codec mode %d\n", ret);
		return ret;
	}

	/* Set system master clock */
	ret = codec_writeAreg(TWL4030_REG_APLL_CTL, BIT_APLL_CTL_APLL_EN_M |
				AUDIO_APLL_DEFAULT << BIT_APLL_CTL_APLL_INFREQ);
	if (ret) {
		printk(KERN_ERR "Unable to set APLL input frequency %d\n", ret);
		return ret;
	}

//	twl4030_local.voice_samplerate = sample_rate;

	return ret;
}

static u8 compute_gain(int volume, int min_gain, int max_gain)
{
	int gain = 0;

	gain = ((max_gain - min_gain) * volume + AUDIO_MAX_VOLUME * min_gain) /
		AUDIO_MAX_VOLUME;

	return (u8) gain;
}

/*
 * twl4030_set_digital_volume - Set digital volume for input/output sources
 */
int twl4030_set_digital_volume(int source, int volume)
{
	u8 reg = 0;
	u8 data = 0;
	u8 coarse_gain = 0;
	u8 fine_gain = 0;
	int ret = 0;
	
	if (volume > AUDIO_MAX_VOLUME) {
		printk(KERN_ERR "Invalid volume: %d\n", volume);
		return -EPERM;
	}

	if (is_input(source)) {
		fine_gain = compute_gain(volume,
				INPUT_MIN_GAIN, INPUT_MAX_GAIN);
	} else if (is_output(source)) {
		fine_gain = compute_gain(volume,
				OUTPUT_MIN_GAIN, OUTPUT_MAX_GAIN);
		coarse_gain = AUDIO_DEF_COARSE_VOLUME_LEVEL;
	} else if (is_voice(source)) {
		fine_gain = compute_gain(volume,
				VOICE_MIN_GAIN, VOICE_MAX_GAIN);
	} else if (is_sidetone(source)) {
		fine_gain = compute_gain(volume,
				SIDETONE_MIN_GAIN, SIDETONE_MAX_GAIN);
	}

	switch (source) {
	case TXL1:
		reg = TWL4030_REG_ATXL1PGA;
		data = fine_gain << BIT_ATXL1PGA_ATXL1PGA_GAIN;
		break;
	case TXR1:
		reg = TWL4030_REG_ATXR1PGA;
		data = fine_gain << BIT_ATXR1PGA_ATXR1PGA_GAIN;
		break;
	case TXL2:
		reg = TWL4030_REG_AVTXL2PGA;
		data = fine_gain << BIT_AVTXL2PGA_AVTXL2PGA_GAIN;
		break;
	case TXR2:
		reg = TWL4030_REG_AVTXR2PGA;
		data = fine_gain << BIT_AVTXR2PGA_AVTXR2PGA_GAIN;
		break;
	case RXL1:
		reg = TWL4030_REG_ARXL1PGA;
		data = coarse_gain << BIT_ARXL1PGA_ARXL1PGA_CGAIN
			| fine_gain << BIT_ARXL1PGA_ARXL1PGA_FGAIN;
		break;
	case RXR1:
		reg = TWL4030_REG_ARXR1PGA;
		data = coarse_gain << BIT_ARXR1PGA_ARXR1PGA_CGAIN
			| fine_gain << BIT_ARXR1PGA_ARXR1PGA_FGAIN;
		break;
	case RXL2:
		reg = TWL4030_REG_ARXL2PGA;
		data = coarse_gain << BIT_ARXL2PGA_ARXL2PGA_CGAIN
			| fine_gain << BIT_ARXL2PGA_ARXL2PGA_FGAIN;
		break;
	case RXR2:
		reg = TWL4030_REG_ARXR2PGA;
		data = coarse_gain << BIT_ARXR2PGA_ARXR2PGA_CGAIN
			| fine_gain << BIT_ARXR2PGA_ARXR2PGA_FGAIN;
		break;
	case VDL:
		reg = TWL4030_REG_VRXPGA;
		data = fine_gain << BIT_VRXPGA_VRXPGA_GAIN;
		break;
	case VST:
		reg = TWL4030_REG_VSTPGA;
		data = fine_gain << BIT_VSTPGA_VSTPGA_GAIN;
		break;
	default:
		printk(KERN_ERR "Invalid source type\n");
		return -EPERM;
	}

	ret = codec_writeAreg(reg, data);
	if (ret)
		printk(KERN_ERR "Error setting digital volume %d\n", ret);

	return ret;
}


/*
 * twl4030_set_analog_volume - Set analog volume for input/output sources
 */
static int twl4030_set_analog_volume(int source, int volume)
{
	u8 gain = 0;
	u8 data = 0;
	u8 reg = 0;
	int ret = 0;

	if (volume > AUDIO_MAX_VOLUME) {
		printk(KERN_ERR "Invalid volume: %d\n", volume);
		return -EPERM;
	}

	gain = compute_gain(volume, OUTPUT_ANALOG_GAIN_MIN,
				OUTPUT_ANALOG_GAIN_MAX);
	switch (source) {
	case RXL1:
		reg = TWL4030_REG_ARXL1_APGA_CTL;
		data = gain << BIT_ARXL1_APGA_CTL_ARXL1_GAIN_SET
			| BIT_ARXL1_APGA_CTL_ARXL1_DA_EN_M
			| BIT_ARXL1_APGA_CTL_ARXL1_PDZ_M;
		break;
	case RXR1:
		reg = TWL4030_REG_ARXR1_APGA_CTL;
		data = gain << BIT_ARXR1_APGA_CTL_ARXR1_GAIN_SET
			| BIT_ARXR1_APGA_CTL_ARXR1_DA_EN_M
			| BIT_ARXR1_APGA_CTL_ARXR1_PDZ_M;
		break;
	case RXL2:
		reg = TWL4030_REG_ARXL2_APGA_CTL;
		data = gain << BIT_ARXL2_APGA_CTL_ARXL2_GAIN_SET
			| BIT_ARXL2_APGA_CTL_ARXL2_DA_EN_M
			| BIT_ARXL2_APGA_CTL_ARXL2_PDZ_M;
		break;
	case RXR2:
		reg = TWL4030_REG_ARXR2_APGA_CTL;
		data = gain << BIT_ARXR2_APGA_CTL_ARXR2_GAIN_SET
			| BIT_ARXR2_APGA_CTL_ARXR2_DA_EN_M
			| BIT_ARXR2_APGA_CTL_ARXR2_PDZ_M;
		break;
	case VDL:
		reg = TWL4030_REG_VDL_APGA_CTL;
		data = gain << BIT_VDL_APGA_CTL_VDL_GAIN_SET
			| BIT_VDL_APGA_CTL_VDL_DA_EN_M
			| BIT_VDL_APGA_CTL_VDL_PDZ_M;
		break;
	default:
		printk(KERN_ERR "Invalid source type\n");
		return -EPERM;
	}

	ret = codec_writeAreg(reg, data);
	if (ret)
		printk(KERN_ERR "Error setting analog volume %d\n", ret);

	return ret;
}


/*
 * twl4030_set_volume -Set the gain of the requested device
 */
int twl4030_setvolume(int source, int volume_l, int volume_r)
{
	u8 gain_l = 0;
	u8 gain_r = 0;
	u8 data = 0;
	int ret = 0;
	
	if ((volume_l > AUDIO_MAX_VOLUME) || (volume_r > AUDIO_MAX_VOLUME)) {
		printk(KERN_ERR "Invalid volume values: %d, %d\n",
			volume_l, volume_r);
		return -EPERM;
	}

	switch (source) {
	case OUTPUT_VOLUME:
		ret = twl4030_set_digital_volume(RXL2, volume_l);
		if (ret) {
			printk(KERN_ERR "RXL2 digital volume error %d\n", ret);
			return ret;
		}
		ret = twl4030_set_digital_volume(RXR2, volume_r);
		if (ret) {
			printk(KERN_ERR "RXR2 digital volume error %d\n", ret);
			return ret;
		}
		ret = twl4030_set_analog_volume(RXL2, volume_l);
		if (ret) {
			printk(KERN_ERR "RXL2 analog volume %d\n", ret);
			return ret;
		}
		ret = twl4030_set_analog_volume(RXR2, volume_r);
		if (ret) {
			printk(KERN_ERR "RXR2 analog volume %d\n", ret);
			return ret;
		}
//		twl4030_local.master_play_vol = WRITE_LEFT_VOLUME(volume_l) |
//						WRITE_RIGHT_VOLUME(volume_r);
		break;
	case OUTPUT_STEREO_HEADSET:
		gain_l = compute_gain(volume_l, HEADSET_GAIN_MIN,
					HEADSET_GAIN_MAX);
		gain_r = compute_gain(volume_r, HEADSET_GAIN_MIN,
					HEADSET_GAIN_MAX);
		/* Handle mute request */
		gain_l = (volume_l == 0) ? 0 : gain_l;
		gain_r = (volume_r == 0) ? 0 : gain_r;
		ret = codec_writeAreg(TWL4030_REG_HS_GAIN_SET,
				  (gain_l << BIT_HS_GAIN_SET_HSL_GAIN) |
				  (gain_r << BIT_HS_GAIN_SET_HSR_GAIN));
		if (ret) {
			printk(KERN_ERR "Headset volume error %d\n", ret);
			return ret;
		}
//		twl4030_local.headset_vol = WRITE_LEFT_VOLUME(volume_l) |
//					WRITE_RIGHT_VOLUME(volume_r);
		break;
	case OUTPUT_HANDS_FREE_CLASSD:
		/* ClassD no special gain */
//		twl4030_local.classd_vol = WRITE_LEFT_VOLUME(volume_l) |
//					WRITE_RIGHT_VOLUME(volume_r);
		break;
	case OUTPUT_MONO_EARPIECE:
		printk("!@!@!@!@!@!@!@twl4030_setvolume OUTPUT_MONO_EARPIECE\n");
		gain_l = compute_gain(volume_l, EARPHONE_GAIN_MIN,
					EARPHONE_GAIN_MAX);
		gain_l = (volume_l == 0) ? 0 : gain_l;
		//ret = audio_twl4030_read(REG_EAR_CTL, &data);
		codec_readAreg(TWL4030_REG_EAR_CTL,&data);
		if (ret) {
			printk(KERN_ERR "Earpiece volume error %d\n", ret);
			return ret;
		}
		data &= ~BIT_EAR_CTL_EAR_GAIN_M;
		data |= gain_l << BIT_EAR_CTL_EAR_GAIN;
		ret = codec_writeAreg(TWL4030_REG_EAR_CTL, data);
		if (ret) {
			printk(KERN_ERR "Earpiece volume error %d\n", ret);
			return ret;
		}
		//twl4030_local.ear_vol = WRITE_LEFT_VOLUME(volume_l);
		break;
	case OUTPUT_SIDETONE:
		ret = twl4030_set_digital_volume(VST, volume_l);
		if (ret) {
			printk(KERN_ERR "Sidetone volume error %d\n", ret);
			return ret;
		}
		//twl4030_local.sidetone_vol = WRITE_LEFT_VOLUME(volume_l);
		break;
	case OUTPUT_CARKIT:
		gain_l = compute_gain(volume_l, CARKIT_MIN_GAIN,
					CARKIT_MAX_GAIN);
		gain_r = compute_gain(volume_r, CARKIT_MIN_GAIN,
					CARKIT_MAX_GAIN);
		gain_l = (volume_l == 0) ? 0 : gain_l;
		gain_r = (volume_r == 0) ? 0 : gain_r;
		/* Left gain */
		//ret = audio_twl4030_read(REG_PRECKL_CTL, &data);
		codec_readAreg(TWL4030_REG_PRECKL_CTL,&data);
		if (ret) {
			printk(KERN_ERR "Carkit output volume error %d\n", ret);
			return ret;
		}
		data &= ~BIT_PRECKL_CTL_PRECKL_GAIN_M;
		data |=  gain_l << BIT_PRECKL_CTL_PRECKL_GAIN;
		ret = codec_writeAreg(TWL4030_REG_PRECKL_CTL, data);
		if (ret) {
			printk(KERN_ERR "Carkit output volume error %d\n", ret);
			return ret;
		}
		//twl4030_local.carkit_vol = WRITE_LEFT_VOLUME(volume_l);
		/* Right gain */
		//ret = audio_twl4030_read(REG_PRECKR_CTL, &data);
		codec_readAreg(TWL4030_REG_PRECKR_CTL,&data);
		if (ret) {
			printk(KERN_ERR "Carkit output volume error %d\n", ret);
			return ret;
		}
		data &= ~BIT_PRECKR_CTL_PRECKR_GAIN_M;
		data |= gain_r << BIT_PRECKR_CTL_PRECKR_GAIN;
		ret = codec_writeAreg(TWL4030_REG_PRECKR_CTL, data);
		if (ret) {
			printk(KERN_ERR "Carkit output volume error %d\n", ret);
			return ret;
		}
		//twl4030_local.carkit_vol |= WRITE_RIGHT_VOLUME(volume_r);
		break;
	case OUTPUT_DOWNLINK:
		ret = twl4030_set_digital_volume(VDL, volume_l);
		if (ret) {
			printk(KERN_ERR "Downlink volume error %d\n", ret);
			return ret;
		}
		ret = twl4030_set_analog_volume(VDL, volume_l);
		if (ret) {
			printk(KERN_ERR "Downlink volume error %d\n", ret);
			return ret;
		}
		//twl4030_local.downlink_vol = WRITE_LEFT_VOLUME(volume_l);
		break;
#if 0
	case INPUT_VOLUME:
		ret = twl4030_set_digital_volume(TXL1, volume_l);
		if (ret) {
			printk(KERN_ERR "TXL1 digital volume error %d\n", ret);
			return ret;
		}
		ret = twl4030_set_digital_volume(TXR1, volume_r);
		if (ret) {
			printk(KERN_ERR "TXR1 digital volume error %d\n", ret);
			return ret;
		}
		ret = twl4030_set_digital_volume(TXL2, volume_l);
		if (ret) {
			printk(KERN_ERR "TXL2 digital volume error %d\n", ret);
			return ret;
		}
		ret = twl4030_set_digital_volume(TXR2, volume_r);
		if (ret) {
			printk(KERN_ERR "TXR2 digital volume error %d\n", ret);
			return ret;
		}
		twl4030_local.master_rec_vol = WRITE_LEFT_VOLUME(volume_l) |
						WRITE_RIGHT_VOLUME(volume_r);
		break;
	case INPUT_HEADSET_MIC:
		gain_l = compute_gain(volume_l, MICROPHONE_MIN_GAIN,
					MICROPHONE_MAX_GAIN);
		ret = audio_twl4030_read(REG_ANAMIC_GAIN, &data);
		if (ret) {
			printk(KERN_ERR "Headset mic volume error %d\n", ret);
			return ret;
		}
		data &= ~BIT_ANAMIC_GAIN_MICAMPL_GAIN_M;
		data |= gain_l << BIT_ANAMIC_GAIN_MICAMPL_GAIN;
		ret = audio_twl4030_write(REG_ANAMIC_GAIN, data);
		if (ret) {
			printk(KERN_ERR "Headset mic volume error %d\n", ret);
			return ret;
		}
		twl4030_local.headset_mic_vol = WRITE_LEFT_VOLUME(volume_l);
		break;
	case INPUT_MAIN_MIC:
		gain_l = compute_gain(volume_l, MICROPHONE_MIN_GAIN,
					MICROPHONE_MAX_GAIN);
		ret = audio_twl4030_read(REG_ANAMIC_GAIN, &data);
		if (ret) {
			printk(KERN_ERR "Main mic volume error %d\n", ret);
			return ret;
		}
		data &= ~BIT_ANAMIC_GAIN_MICAMPL_GAIN_M;
		data |= gain_l << BIT_ANAMIC_GAIN_MICAMPL_GAIN;
		ret = audio_twl4030_write(REG_ANAMIC_GAIN, data);
		if (ret) {
			printk(KERN_ERR "Main mic volume error %d\n", ret);
			return ret;
		}
		twl4030_local.main_mic_vol = WRITE_LEFT_VOLUME(volume_l);
		break;
	case INPUT_SUB_MIC:
		gain_r = compute_gain(volume_r, MICROPHONE_MIN_GAIN,
					MICROPHONE_MAX_GAIN);
		ret = audio_twl4030_read(REG_ANAMIC_GAIN, &data);
		if (ret) {
			printk(KERN_ERR "Sub mic volume error %d\n", ret);
			return ret;
		}
		data &= ~BIT_ANAMIC_GAIN_MICAMPR_GAIN_M;
		data |= gain_r << BIT_ANAMIC_GAIN_MICAMPR_GAIN;
		ret = audio_twl4030_write(REG_ANAMIC_GAIN, data);
		if (ret) {
			printk(KERN_ERR "Sub mic volume error %d\n", ret);
			return ret;
		}
		twl4030_local.sub_mic_vol = WRITE_RIGHT_VOLUME(volume_r);
		break;
	case INPUT_AUX:
		gain_l = compute_gain(volume_l, MICROPHONE_MIN_GAIN,
					MICROPHONE_MAX_GAIN);
		gain_r = compute_gain(volume_r, MICROPHONE_MIN_GAIN,
					MICROPHONE_MAX_GAIN);
		ret = audio_twl4030_write(REG_ANAMIC_GAIN,
				gain_l << BIT_ANAMIC_GAIN_MICAMPL_GAIN
				| gain_r << BIT_ANAMIC_GAIN_MICAMPR_GAIN);
		if (ret) {
			printk(KERN_ERR "Aux input volume error %d\n", ret);
			return ret;
		}
		twl4030_local.aux_vol = WRITE_LEFT_VOLUME(volume_l) |
					WRITE_RIGHT_VOLUME(volume_r);
		break;
	case INPUT_CARKIT:
		gain_l = compute_gain(volume_l, MICROPHONE_MIN_GAIN,
					MICROPHONE_MAX_GAIN);
		gain_r = compute_gain(volume_r, MICROPHONE_MIN_GAIN,
					MICROPHONE_MAX_GAIN);
		ret = audio_twl4030_write(REG_ANAMIC_GAIN,
				gain_l << BIT_ANAMIC_GAIN_MICAMPL_GAIN
				| gain_r << BIT_ANAMIC_GAIN_MICAMPR_GAIN);
		if (ret) {
			printk(KERN_ERR "Carkit input volume error %d\n", ret);
			return ret;
		}
		twl4030_local.carkit_mic_vol = WRITE_LEFT_VOLUME(volume_l) |
						WRITE_RIGHT_VOLUME(volume_r);
		break;
#endif
	default:
		printk(KERN_WARNING "Invalid source specified\n");
		ret = -EPERM;
		break;
	}

	return ret;
}


static int twl4030_set_all_volumes(void)
{
	int ret = 0;
	
	audio_debug("twl4030_set_all_volumes111!!!!!!!!!!!!!!\n");
	ret = twl4030_setvolume(OUTPUT_VOLUME, 90,90);
	if (ret)
		goto set_volumes_exit;

	ret = twl4030_setvolume(OUTPUT_STEREO_HEADSET,90,90);
	if (ret)
		goto set_volumes_exit;

	ret = twl4030_setvolume(OUTPUT_HANDS_FREE_CLASSD,90,90);
	if (ret)
		goto set_volumes_exit;

	ret = twl4030_setvolume(OUTPUT_MONO_EARPIECE,90, 0);
	if (ret)
		goto set_volumes_exit;

	ret = twl4030_setvolume(OUTPUT_CARKIT,90,90);
	if (ret)
		goto set_volumes_exit;

	ret = twl4030_setvolume(OUTPUT_SIDETONE,90,0);
	if (ret)
		goto set_volumes_exit;

	ret = twl4030_setvolume(OUTPUT_DOWNLINK,90, 0);
set_volumes_exit:
	if (ret)
		printk(KERN_ERR "Failed setting all volumes %d\n", ret);

	return ret;
}


/*
 * twl4030_configure - Configure codec according to specified mode
 */
int twl4030_configure(int codec_mode)
{
	int ret = 0;
	
	ret = twl4030_cleanup();
	if (ret)
		goto configure_exit;

	ret = twl4030_set_codec_mode(codec_mode);
	if (ret)
		goto configure_exit;

	ret = twl4030_configure_codec_interface(codec_mode);
	if (ret)
		goto configure_exit;

#if 0
	if (codec_mode == AUDIO_MODE)
		ret = twl4030_set_audio_samplerate(twl4030_local.
				audio_samplerate);
	else if (codec_mode == VOICE_MODE)
		ret = twl4030_set_voice_samplerate(twl4030_local.
				voice_samplerate);
	#endif
	if (codec_mode == VOICE_MODE)
		{
	ret = twl4030_set_voice_samplerate(NARROWBAND_FREQ);
	if (ret)
		goto configure_exit;
		}

	/* Set the gains - we do not know the defaults
	 * attempt to set the volume of all the devices
	 * only those enabled get set.
	 */
	ret = twl4030_set_all_volumes();
	if (ret)
		goto configure_exit;

	/* Do not bypass the high pass filters */
	codec_writeAreg(TWL4030_REG_MISC_SET_2, 0x00);
#if 0
	/* Switch on the input/output required */
	ret = twl4030_select_source(twl4030_local.input_src);
	if (ret)
		goto configure_exit;

	ret = twl4030_select_source(twl4030_local.output_src);
	#endif
configure_exit:
	if (ret)
		printk(KERN_ERR "Codec configuration failed %d\n", ret);

	return ret;
}

void show_reg(void)
{
    unsigned char value = 0;

	codec_readAreg(TWL4030_REG_CODEC_MODE,&value);
    printk("[0x%2x] CODEC_MODE = %08x\n",TWL4030_REG_CODEC_MODE,value);

	codec_readAreg(TWL4030_REG_OPTION,&value);
    printk("[0x%2x] OPTION = %08x\n",TWL4030_REG_OPTION,value);

	codec_readAreg(TWL4030_REG_MICBIAS_CTL,&value);
    printk("[0x%2x] MICBIAS_CTL = %08x\n",TWL4030_REG_MICBIAS_CTL,value);

	codec_readAreg(TWL4030_REG_ANAMICL,&value);
    printk("[0x%2x] ANAMICL = %08x\n",TWL4030_REG_ANAMICL,value);

	codec_readAreg(TWL4030_REG_ANAMICR,&value);
    printk("[0x%2x] ANAMICR = %08x\n",TWL4030_REG_ANAMICR,value);

	codec_readAreg(TWL4030_REG_AVADC_CTL,&value);
    printk("[0x%2x] AVADC_CTL = %08x\n",TWL4030_REG_AVADC_CTL,value);

	codec_readAreg(TWL4030_REG_ADCMICSEL,&value);
    printk("[0x%2x] ADCMICSEL = %08x\n",TWL4030_REG_ADCMICSEL,value);

	codec_readAreg(TWL4030_REG_DIGMIXING,&value);
    printk("[0x%2x] DIGMIXING = %08x\n",TWL4030_REG_DIGMIXING,value);

	codec_readAreg(TWL4030_REG_ATXL1PGA,&value);
    printk("[0x%2x] ATXL1PGA = %08x\n",TWL4030_REG_ATXL1PGA,value);

	codec_readAreg(TWL4030_REG_ATXR1PGA,&value);
    printk("[0x%2x] ATXR1PGA = %08x\n",TWL4030_REG_ATXR1PGA,value);

	codec_readAreg(TWL4030_REG_AVTXL2PGA,&value);
    printk("[0x%2x] AVTXL2PGA = %08x\n",TWL4030_REG_AVTXL2PGA,value);

	codec_readAreg(TWL4030_REG_AVTXR2PGA,&value);
    printk("[0x%2x] AVTXR2PGA = %08x\n",TWL4030_REG_AVTXR2PGA,value);

	codec_readAreg(TWL4030_REG_AUDIO_IF,&value);
    printk("[0x%2x] AUDIO_IF = %08x\n",TWL4030_REG_AUDIO_IF,value);

	codec_readAreg(TWL4030_REG_VOICE_IF,&value);
    printk("[0x%2x] VOICE_IF = %08x\n",TWL4030_REG_VOICE_IF,value);

	codec_readAreg(TWL4030_REG_ARXR1PGA,&value);
    printk("[0x%2x] ARXR1PGA = %08x\n",TWL4030_REG_ARXR1PGA,value);

	codec_readAreg(TWL4030_REG_ARXL1PGA,&value);
    printk("[0x%2x] ARXL1PGA = %08x\n",TWL4030_REG_ARXL1PGA,value);

	codec_readAreg(TWL4030_REG_ARXR2PGA,&value);
    printk("[0x%2x] ARXR2PGA = %08x\n",TWL4030_REG_ARXR2PGA,value);

	codec_readAreg(TWL4030_REG_ARXL2PGA,&value);
    printk("[0x%2x] ARXL2PGA = %08x\n",TWL4030_REG_ARXL2PGA,value);

	codec_readAreg(TWL4030_REG_VRXPGA,&value);
    printk("[0x%2x] VRXPGA = %08x\n",TWL4030_REG_VRXPGA,value);

	codec_readAreg(TWL4030_REG_VSTPGA,&value);
    printk("[0x%2x] VSTPGA = %08x\n",TWL4030_REG_VSTPGA,value);

	codec_readAreg(TWL4030_REG_VRX2ARXPGA,&value);
    printk("[0x%2x] VRX2ARXPGA = %08x\n",TWL4030_REG_VRX2ARXPGA,value);

	codec_readAreg(TWL4030_REG_AVDAC_CTL,&value);
    printk("[0x%2x] AVDAC_CTL = %08x\n",TWL4030_REG_AVDAC_CTL,value);

	codec_readAreg(TWL4030_REG_ARX2VTXPGA,&value);
    printk("[0x%2x] ARX2VTXPGA = %08x\n",TWL4030_REG_ARX2VTXPGA,value);

	codec_readAreg(TWL4030_REG_ARXL1_APGA_CTL,&value);
    printk("[0x%2x] ARXL1_APGA_CTL = %08x\n",TWL4030_REG_ARXL1_APGA_CTL,value);

	codec_readAreg(TWL4030_REG_ARXR1_APGA_CTL,&value);
    printk("[0x%2x] ARXR1_APGA_CTL = %08x\n",TWL4030_REG_ARXR1_APGA_CTL,value);

	codec_readAreg(TWL4030_REG_ARXL2_APGA_CTL,&value);
    printk("[0x%2x] ARXL2_APGA_CTL = %08x\n",TWL4030_REG_ARXL2_APGA_CTL,value);

	codec_readAreg(TWL4030_REG_ARXR2_APGA_CTL,&value);
    printk("[0x%2x] ARXR2_APGA_CTL = %08x\n",TWL4030_REG_ARXR2_APGA_CTL,value);

	codec_readAreg(TWL4030_REG_ATX2ARXPGA,&value);
    printk("[0x%2x] ATX2ARXPGA = %08x\n",TWL4030_REG_ATX2ARXPGA,value);

	codec_readAreg(TWL4030_REG_BT_IF,&value);
    printk("[0x%2x] BT_IF = %08x\n",TWL4030_REG_BT_IF,value);

	codec_readAreg(TWL4030_REG_BTPGA,&value);
    printk("[0x%2x] BTPGA = %08x\n",TWL4030_REG_BTPGA,value);

	codec_readAreg(TWL4030_REG_BTSTPGA,&value);
    printk("[0x%2x] BTSTPGA = %08x\n",TWL4030_REG_BTSTPGA,value);

	codec_readAreg(TWL4030_REG_EAR_CTL,&value);
    printk("[0x%2x] EAR_CTL = %08x\n",TWL4030_REG_EAR_CTL,value);

	codec_readAreg(TWL4030_REG_HS_SEL,&value);
    printk("[0x%2x] HS_SEL = %08x\n",TWL4030_REG_HS_SEL,value);

	codec_readAreg(TWL4030_REG_HS_GAIN_SET,&value);
    printk("[0x%2x] HS_GAIN_SET = %08x\n",TWL4030_REG_HS_GAIN_SET,value);

	codec_readAreg(TWL4030_REG_HS_POPN_SET,&value);
    printk("[0x%2x] HS_POPN_SET = %08x\n",TWL4030_REG_HS_POPN_SET,value);

	codec_readAreg(TWL4030_REG_PREDL_CTL,&value);
    printk("[0x%2x] PREDL_CTL = %08x\n",TWL4030_REG_PREDL_CTL,value);

	codec_readAreg(TWL4030_REG_PREDR_CTL,&value);
    printk("[0x%2x] PREDR_CTL = %08x\n",TWL4030_REG_PREDR_CTL,value);

	codec_readAreg(TWL4030_REG_PRECKL_CTL,&value);
    printk("[0x%2x] PRECKL_CTL = %08x\n",TWL4030_REG_PRECKL_CTL,value);

	codec_readAreg(TWL4030_REG_PRECKR_CTL,&value);
    printk("[0x%2x] PRECKR_CTL = %08x\n",TWL4030_REG_PRECKR_CTL,value);

	codec_readAreg(TWL4030_REG_HFL_CTL,&value);
    printk("[0x%2x] HFL_CTL = %08x\n",TWL4030_REG_HFL_CTL,value);

	codec_readAreg(TWL4030_REG_HFR_CTL,&value);
    printk("[0x%2x] HFR_CTL = %08x\n",TWL4030_REG_HFR_CTL,value);

	codec_readAreg(TWL4030_REG_ALC_CTL,&value);
    printk("[0x%2x] ALC_CTL = %08x\n",TWL4030_REG_ALC_CTL,value);

	codec_readAreg(TWL4030_REG_ALC_SET1,&value);
    printk("[0x%2x] ALC_SET1 = %08x\n",TWL4030_REG_ALC_SET1,value);

	codec_readAreg(TWL4030_REG_ALC_SET2,&value);
    printk("[0x%2x] ALC_SET2 = %08x\n",TWL4030_REG_ALC_SET2,value);

	codec_readAreg(TWL4030_REG_BOOST_CTL,&value);
    printk("[0x%2x] BOOST_CTL = %08x\n",TWL4030_REG_BOOST_CTL,value);

	codec_readAreg(TWL4030_REG_SOFTVOL_CTL,&value);
    printk("[0x%2x] SOFTVOL_CTL = %08x\n",TWL4030_REG_SOFTVOL_CTL,value);

	codec_readAreg(TWL4030_REG_DTMF_FREQSEL,&value);
    printk("[0x%2x] DTMF_FREQSEL = %08x\n",TWL4030_REG_DTMF_FREQSEL,value);

	codec_readAreg(TWL4030_REG_DTMF_TONEXT1H,&value);
    printk("[0x%2x] DTMF_TONEXT1H = %08x\n",TWL4030_REG_DTMF_TONEXT1H,value);

	codec_readAreg(TWL4030_REG_DTMF_TONEXT1L,&value);
    printk("[0x%2x] DTMF_TONEXT1L = %08x\n",TWL4030_REG_DTMF_TONEXT1L,value);

	codec_readAreg(TWL4030_REG_DTMF_TONEXT2H,&value);
    printk("[0x%2x] DTMF_TONEXT2H = %08x\n",TWL4030_REG_DTMF_TONEXT2H,value);

	codec_readAreg(TWL4030_REG_DTMF_TONEXT2L,&value);
    printk("[0x%2x] DTMF_TONEXT2L = %08x\n",TWL4030_REG_DTMF_TONEXT2L,value);

	codec_readAreg(TWL4030_REG_DTMF_TONOFF,&value);
    printk("[0x%2x] DTMF_TONOFF = %08x\n",TWL4030_REG_DTMF_TONOFF,value);

	codec_readAreg(TWL4030_REG_DTMF_WANONOFF,&value);
    printk("[0x%2x] DTMF_WANONOFF = %08x\n",TWL4030_REG_DTMF_WANONOFF,value);

	codec_readAreg(TWL4030_REG_I2S_RX_SCRAMBLE_H,&value);
    printk("[0x%2x] I2S_RX_SCRAMBLE_H = %08x\n",TWL4030_REG_I2S_RX_SCRAMBLE_H,value);

	codec_readAreg(TWL4030_REG_I2S_RX_SCRAMBLE_M,&value);
    printk("[0x%2x] CODEC_RX_SCRAMBLE_M = %08x\n",TWL4030_REG_I2S_RX_SCRAMBLE_M,value);

	codec_readAreg(TWL4030_REG_I2S_RX_SCRAMBLE_L,&value);
    printk("[0x%2x] CODEC_RX_SCRAMBLE_L = %08x\n",TWL4030_REG_I2S_RX_SCRAMBLE_L,value);

	codec_readAreg(TWL4030_REG_APLL_CTL,&value);
    printk("[0x%2x] APLL_CTL = %08x\n",TWL4030_REG_APLL_CTL,value);

	codec_readAreg(TWL4030_REG_DTMF_CTL,&value);
    printk("[0x%2x] DTMF_CTL = %08x\n",TWL4030_REG_DTMF_CTL,value);

	codec_readAreg(TWL4030_REG_DTMF_PGA_CTL2,&value);
    printk("[0x%2x] DTMF_PGA_CTL2 = %08x\n",TWL4030_REG_DTMF_PGA_CTL2,value);

	codec_readAreg(TWL4030_REG_DTMF_PGA_CTL1,&value);
    printk("[0x%2x] DTMF_PGA_CTL1 = %08x\n",TWL4030_REG_DTMF_PGA_CTL1,value);

	codec_readAreg(TWL4030_REG_MISC_SET_1,&value);
    printk("[0x%2x] MISC_SET_1 = %08x\n",TWL4030_REG_MISC_SET_1,value);

	codec_readAreg(TWL4030_REG_PCMBTMUX,&value);
    printk("[0x%2x] PCMBTMUX = %08x\n",TWL4030_REG_PCMBTMUX,value);

	codec_readAreg(TWL4030_REG_RX_PATH_SEL,&value);
    printk("[0x%2x] RX_PATH_SEL = %08x\n",TWL4030_REG_RX_PATH_SEL,value);

	codec_readAreg(TWL4030_REG_VDL_APGA_CTL,&value);
    printk("[0x%2x] VDL_APGA_CTL = %08x\n",TWL4030_REG_VDL_APGA_CTL,value);

	codec_readAreg(TWL4030_REG_VIBRA_CTL,&value);
    printk("[0x%2x] VIBRA_CTL = %08x\n",TWL4030_REG_VIBRA_CTL,value);

	codec_readAreg(TWL4030_REG_VIBRA_SET,&value);
    printk("[0x%2x] VIBRA_SET = %08x\n",TWL4030_REG_VIBRA_SET,value);

	codec_readAreg(TWL4030_REG_VIBRA_PWM_SET,&value);
    printk("[0x%2x] VIBRA_SET = %08x\n",TWL4030_REG_VIBRA_PWM_SET,value);

	codec_readAreg(TWL4030_REG_ANAMIC_GAIN,&value);
    printk("[0x%2x] ANAMIC_GAIN = %08x\n",TWL4030_REG_ANAMIC_GAIN,value);

	codec_readAreg(TWL4030_REG_MISC_SET_2,&value);
    printk("[0x%2x] MISC_SET_2 = %08x\n",TWL4030_REG_MISC_SET_2,value);

}

static int aux_input_count=0;
void audio_para(int para)
{
    unsigned int value = 0;

    if(para ==0){
        codec_readAreg(TWL4030_REG_ANAMICL,&value);
        value &= 0xfc;
        codec_writeAreg(TWL4030_REG_ANAMICL,value);

        codec_readAreg(TWL4030_REG_ANAMICR,&value);
        value &= 0xfe;
        codec_writeAreg(TWL4030_REG_ANAMICR,value);
    }else if(para == 1){
        codec_writeAreg(TWL4030_REG_MICBIAS_CTL,0x04);
    }else if(para == 2){    /* mute output  */
        codec_readAreg(TWL4030_REG_HS_SEL,&value);
        if( (value & 0x24) !=0){  /* audio is routing to headset */
            reg_hs_sel = value;
            value &= 0xdb; /*disable routing to headset */
            codec_writeAreg(TWL4030_REG_HS_SEL, value);
        }
        codec_readAreg(TWL4030_REG_HFL_CTL,&value);
        if( (value & 0x3c) !=0){  /* audio is routing to left speaker */
            reg_hfl_ctl = value;
            value &= 0xc3; /*disable routing to left speaker */
            codec_writeAreg(TWL4030_REG_HFL_CTL, value);
        }
        codec_readAreg(TWL4030_REG_HFR_CTL,&value);
        if( (value & 0x3c) !=0){  /* audio is routing to right speaker */
            reg_hfr_ctl = value;
            value &= 0xc3; /*disable routing to right speaker */
            codec_writeAreg(TWL4030_REG_HFR_CTL, value);
        }
    }else if(para == 3){    /* unmute output */
        if(reg_hs_sel!=0)
            codec_writeAreg(TWL4030_REG_HS_SEL, reg_hs_sel);
        if(reg_hfl_ctl!=0)
            codec_writeAreg(TWL4030_REG_HFL_CTL, reg_hfl_ctl);
        if(reg_hfr_ctl!=0)
            codec_writeAreg(TWL4030_REG_HFR_CTL, reg_hfr_ctl);
    }else if(para == 4){    /* disable AUX input */
        codec_readAreg(TWL4030_REG_ANAMICL,&value);
        value &= ~(BIT_ANAMICL_AUXL_EN_M | BIT_ANAMICL_MICAMPL_EN_M);
        codec_writeAreg(TWL4030_REG_ANAMICL,value);

        codec_readAreg(TWL4030_REG_ANAMICR,&value);
        value &= ~(BIT_ANAMICR_AUXR_EN_M | BIT_ANAMICR_MICAMPR_EN_M);
        codec_writeAreg(TWL4030_REG_ANAMICR,value);
    }else if(para == 5){    /* enable AUX input */
        codec_readAreg(TWL4030_REG_ANAMICL,&value);
        value = BIT_ANAMICL_AUXL_EN_M | BIT_ANAMICL_MICAMPL_EN_M;
        codec_writeAreg(TWL4030_REG_ANAMICL,value);

        codec_readAreg(TWL4030_REG_ANAMICR,&value);
        value = BIT_ANAMICR_AUXR_EN_M | BIT_ANAMICR_MICAMPR_EN_M;
        codec_writeAreg(TWL4030_REG_ANAMICR,value);

	/* add this to resolve left&&right volume isn't the same when runing FMRadio */
	if(aux_input_count==0){
		codec_writeAreg(TWL4030_REG_HS_SEL,0x36);
		reg_hs_sel = 0x36;
		codec_writeAreg(TWL4030_REG_ARXL1_APGA_CTL, 0x37);
		codec_writeAreg(TWL4030_REG_ARXR1_APGA_CTL, 0x37);
		aux_input_count++;
	}
    }

}


static int apollo_audio_ctl_ioctl (struct inode *inode, struct file *file,
			    unsigned int ioctl_num, unsigned long ioctl_param)
{
    unsigned char  value = 0;
	#if 1
    printk("apollo_audio_ctl_ioctl\n");
    switch(ioctl_num)
    {
    case PATH_EARPIECE:
        printk("audio test PATH_EARPIECE\n");
        twl4030_codec_tog_on();
        switch_outs(AUDIO_MODE,OUTPUT_MONO_EARPIECE);
        twl4030_codec_tog_on();
        break;
    case PATH_HEADSET:
        printk("audio test PATH_HEADSET\n");
        twl4030_codec_tog_on();
        switch_outs(AUDIO_MODE,OUTPUT_STEREO_HEADSET);
        twl4030_codec_tog_on();
	#if 0
        twl4030_i2c_read_u8(TWL4030_MODULE_GPIO, &value, REG_GPIODATADIR1);
        value |= 0x40;
        twl4030_i2c_write_u8(TWL4030_MODULE_GPIO, value,REG_GPIODATADIR1);
        twl4030_i2c_read_u8(TWL4030_MODULE_GPIO, &value, REG_GPIODATADIR1);
        printk("GPIODATADIR1 value = %08x\n",value);		
	#else
        printk("no support yet\n");			
	#endif	
        break;
    case PATH_SPEAKER:
        printk("audio test PATH_SPEAKER\n");
        twl4030_codec_tog_on();
        switch_outs(AUDIO_MODE,OUTPUT_HANDS_FREE_CLASSD);
        twl4030_codec_tog_on();
        break;
    case PATH_MIC1:
        printk("audio test switch PATH_MIC1\n");
        twl4030_codec_tog_on();
        switch_inputs(AUDIO_MODE,INPUT_MAIN_MIC);
        twl4030_codec_tog_on();
        break;
    case PATH_MIC2:
        printk("audio test switch PATH_MIC1\n");
        twl4030_codec_tog_on();
        switch_inputs(AUDIO_MODE,INPUT_HEADSET_MIC);
        twl4030_codec_tog_on();
        break;
    case SEL_VOICE:
        printk("audio test switch SEL_VOICE\n");
        //omap_set_gpio_direction(22, 0);
        //omap_set_gpio_dataout(22,0);
	#if 1
        if (gpio_request(APOLLO_CP_ENABLE_GPIO, "cp_enable") == 0) {
	    printk("cp_enable gpio_request successful \n");
		gpio_direction_output(APOLLO_CP_ENABLE_GPIO, 1);
		gpio_set_value(APOLLO_CP_ENABLE_GPIO,1);
		printk("gpio get value = %d\n", gpio_get_value(APOLLO_CP_ENABLE_GPIO));
		//gpio_free(APOLLO_CP_ENABLE_GPIO);
	    }
        //        omap_set_gpio_direction(22, 0);
        //omap_set_gpio_dataout(22,1);
        printk("+++++mcbsp3_dx = %08x,  mcbsp3_dr = %08x, mcbsp3_clkx = %08x, mcbsp3_fsx = %08x\n",__raw_readw(io_base_mux),__raw_readw(io_base_mux+ 0x2),__raw_readw(io_base_mux+ 0x4),__raw_readw(io_base_mux+ 0x6));
        __raw_writew(0x7 |__raw_readw(io_base_mux) , io_base_mux);
        __raw_writew(0x7|__raw_readw(io_base_mux+ 0x2) , io_base_mux+ 0x2);
        __raw_writew(0x7|__raw_readw(io_base_mux+ 0x4) , io_base_mux+ 0x4);
        __raw_writew(0x7|__raw_readw(io_base_mux+ 0x6) , io_base_mux+ 0x6);
        printk("-----mcbsp3_dx = %08x,  mcbsp3_dr = %08x, mcbsp3_clkx = %08x, mcbsp3_fsx = %08x\n",__raw_readw(io_base_mux),__raw_readw(io_base_mux+ 0x2),__raw_readw(io_base_mux+ 0x4),__raw_readw(io_base_mux+ 0x6));
        #endif
        twl4030_codec_on();
        twl4030_configure(VOICE_MODE);
        twl4030_codec_tog_on();
        switch_outs(VOICE_MODE,OUTPUT_HANDS_FREE_CLASSD);
        twl4030_codec_tog_on();
        twl4030_codec_tog_on();
        switch_inputs(VOICE_MODE,INPUT_MAIN_MIC);
        twl4030_codec_tog_on();
		show_reg();
        break;
    case EARPIECE_VOICE:
	    printk("EARPIECE_VOICE");
	    if (gpio_request(APOLLO_CP_ENABLE_GPIO, "cp_enable") == 0) {
		gpio_direction_output(APOLLO_CP_ENABLE_GPIO, 1);
		//gpio_free(APOLLO_CP_ENABLE_GPIO);
	    }
        //       omap_set_gpio_direction(22, 0);
        //omap_set_gpio_dataout(22,1);
        printk("+++++mcbsp3_dx = %08x,  mcbsp3_dr = %08x, mcbsp3_clkx = %08x, mcbsp3_fsx = %08x\n",__raw_readw(io_base_mux),__raw_readw(io_base_mux+ 0x2),__raw_readw(io_base_mux+ 0x4),__raw_readw(io_base_mux+ 0x6));
        __raw_writew(0x7 |__raw_readw(io_base_mux) , io_base_mux);
        __raw_writew(0x7|__raw_readw(io_base_mux+ 0x2) , io_base_mux+ 0x2);
        __raw_writew(0x7|__raw_readw(io_base_mux+ 0x4) , io_base_mux+ 0x4);
        __raw_writew(0x7|__raw_readw(io_base_mux+ 0x6) , io_base_mux+ 0x6);
		printk("-----mcbsp3_dx = %08x,  mcbsp3_dr = %08x, mcbsp3_clkx = %08x, mcbsp3_fsx = %08x\n",__raw_readw(io_base_mux),__raw_readw(io_base_mux+ 0x2),__raw_readw(io_base_mux+ 0x4),__raw_readw(io_base_mux+ 0x6));
		twl4030_configure(VOICE_MODE);
		twl4030_codec_tog_on();
		twl4030_codec_tog_on();
		switch_outs(VOICE_MODE, OUTPUT_MONO_EARPIECE);
		twl4030_codec_tog_on();
		twl4030_codec_tog_on();
        switch_inputs(VOICE_MODE,INPUT_MAIN_MIC);	
        twl4030_codec_tog_on();
	    show_reg();
	    break;

    case HEADSET_VOICE:
		printk("HEADSET_VOICE\n");
        if (gpio_request(APOLLO_CP_ENABLE_GPIO, "cp_enable") == 0) {
		gpio_direction_output(APOLLO_CP_ENABLE_GPIO, 1);
		gpio_set_value(APOLLO_CP_ENABLE_GPIO,1);
		gpio_free(APOLLO_CP_ENABLE_GPIO);
	}
	//  omap_set_gpio_direction(22, 0);
    //	omap_set_gpio_dataout(22,1);
        printk("+++++mcbsp3_dx = %08x,  mcbsp3_dr = %08x, mcbsp3_clkx = %08x, mcbsp3_fsx = %08x\n",__raw_readw(io_base_mux),__raw_readw(io_base_mux+ 0x2),__raw_readw(io_base_mux+ 0x4),__raw_readw(io_base_mux+ 0x6));
        __raw_writew(0x7 |__raw_readw(io_base_mux) , io_base_mux);
        __raw_writew(0x7|__raw_readw(io_base_mux+ 0x2) , io_base_mux+ 0x2);
        __raw_writew(0x7|__raw_readw(io_base_mux+ 0x4) , io_base_mux+ 0x4);
		__raw_writew(0x7|__raw_readw(io_base_mux+ 0x6) , io_base_mux+ 0x6);
		printk("-----mcbsp3_dx = %08x,  mcbsp3_dr = %08x, mcbsp3_clkx = %08x, mcbsp3_fsx = %08x\n",__raw_readw(io_base_mux),__raw_readw(io_base_mux+ 0x2),__raw_readw(io_base_mux+ 0x4),__raw_readw(io_base_mux+ 0x6));
        twl4030_codec_on();
	 	twl4030_configure(VOICE_MODE);
	 	twl4030_codec_tog_on();
		twl4030_codec_tog_on();
        switch_outs(VOICE_MODE, OUTPUT_STEREO_HEADSET);
        twl4030_codec_tog_on();
        twl4030_codec_tog_on();
        switch_inputs(VOICE_MODE,INPUT_HEADSET_MIC);
        twl4030_codec_tog_on();
		break;
    case HSP:
        printk("HSP\n");
        if (gpio_request(APOLLO_CP_ENABLE_GPIO, "cp_enable") == 0) {
		gpio_direction_output(APOLLO_CP_ENABLE_GPIO, 1);
		gpio_free(APOLLO_CP_ENABLE_GPIO);
		}
        //omap_set_gpio_direction(22, 0);
		//omap_set_gpio_dataout(22,1);
        printk("+++++mcbsp3_dx = %08x,  mcbsp3_dr = %08x, mcbsp3_clkx = %08x, mcbsp3_fsx = %08x\n",__raw_readw(io_base_mux),__raw_readw(io_base_mux+ 0x2),__raw_readw(io_base_mux+ 0x4),__raw_readw(io_base_mux+ 0x6));
		__raw_writew(0x7 |__raw_readw(io_base_mux) , io_base_mux);
		__raw_writew(0x7|__raw_readw(io_base_mux+ 0x2) , io_base_mux+ 0x2);
		__raw_writew(0x7|__raw_readw(io_base_mux+ 0x4) , io_base_mux+ 0x4);
		__raw_writew(0x7|__raw_readw(io_base_mux+ 0x6) , io_base_mux+ 0x6);
		printk("-----mcbsp3_dx = %08x,  mcbsp3_dr = %08x, mcbsp3_clkx = %08x, mcbsp3_fsx = %08x\n",__raw_readw(io_base_mux),__raw_readw(io_base_mux+ 0x2),__raw_readw(io_base_mux+ 0x4),__raw_readw(io_base_mux+ 0x6));
		twl4030_codec_off();
		break;
    case MIC1_SPEAKER_LOOPBACK:
        printk("MIC1_SPEAKER_LOOPBACK\n");
        twl4030_codec_on();
        twl4030_configure(VOICE_MODE);
        twl4030_codec_tog_on();
        switch_outs(LOOPBACK_MODE,OUTPUT_HANDS_FREE_CLASSD);
        twl4030_codec_tog_on();
        twl4030_codec_tog_on();
        switch_inputs(VOICE_MODE,INPUT_MAIN_MIC);
        twl4030_codec_tog_on();
        show_reg();
        break;
    case MIC1_EARPIECE_LOOPBACK:
        printk("MIC1_EARPIECE_LOOPBACK\n");
        //twl4030_codec_on();
        //twl4030_configure(VOICE_MODE);
        //twl4030_codec_tog_on();
        switch_outs(LOOPBACK_MODE,OUTPUT_MONO_EARPIECE);
        //twl4030_codec_tog_on();
        //twl4030_codec_tog_on();
        switch_inputs(VOICE_MODE,INPUT_MAIN_MIC);
        //twl4030_codec_tog_on();
        show_reg();
        break;
    case MIC2_HEADSET_LOOPBACK:
        printk("MIC2_HEADSET_LOOPBACK\n");
        twl4030_codec_on();
        twl4030_configure(VOICE_MODE);
        twl4030_codec_tog_on();
        switch_outs(LOOPBACK_MODE,OUTPUT_STEREO_HEADSET);
        twl4030_codec_tog_on();
        twl4030_codec_tog_on();
        switch_inputs(VOICE_MODE,INPUT_HEADSET_MIC);
        twl4030_codec_tog_on();
        show_reg();
        break;
    case AUDIO_PATH_DEBUG:
        printk("audio test debug\n");
		printk("gpio get value = %d\n", gpio_get_value(APOLLO_CP_ENABLE_GPIO));
        show_reg();
	//printk("mcbsp debug\n");
	//value1=__raw_readl(io_base+ 0x1c);
	//printk("mcbsp value1 = %08x\n", value1);
	//__raw_writel(0xc0, io_base+ 0x1c);
        break;
    case PATH_AUX:
        printk("audio test switch PATH_AUX\n");
        twl4030_codec_tog_on();
        switch_inputs(AUDIO_MODE,INPUT_AUX);
        twl4030_codec_tog_on();
        break;
    case DISABLE_MIC:
        audio_para(0);
        break;
    case ENABLE_MIC:
        audio_para(1);
        break;
    case MUTE_OUTPUT:
        audio_para(2);
        break;
    case UNMUTE_OUTPUT:
        audio_para(3);
        break;
    case DISABLE_AUX:
        audio_para(4);
        break;
    case ENABLE_AUX:
        audio_para(5);
        break;
	case SAVE_ENV:
		printk("audio test SAVE_ENV\n");
		save_codec_env();
		break;
	case RESTORE_ENV:
		printk("audio test RESTORE_ENV\n");
		restore_codec_env();
		break;
	default:
		printk("audio test no match case\n");
		break;
    }
	#endif
    return 0;
}

static int apollo_audio_ctl_open(struct inode *inode, struct file *file)
{

    printk("apollo_audio_ctl_open\n");
    return 0;
}

static int apollo_audio_ctl_release(struct inode *inode, struct file *file)
{

    printk("apollo_audio_ctl_release\n");
    return 0;
}

ssize_t twl4030_writeAreg(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int		rc = 0;
    unsigned char reg = 0,value = 0;
    char *temp,*temp1;

	/* Remove a trailing newline */
	if (count > 0 && buf[count-1] == '\n')
		((char *) buf)[count-1] = 0;
	printk("buf:%s  count = %d \n", buf,count);
    temp = kzalloc(100 * sizeof(char), GFP_KERNEL);
    temp1 = temp;
    if(!temp)
    {
	    printk("can't  malloc memory \n");
	    return (rc < 0 ? rc : count);
    }
    reg = simple_strtoul(buf, &temp, 16) & 0xff;
    if(buf !=NULL)
    {
	    printk("temp:%s  \n", temp);
        while((temp) && (*temp == ' '))
            temp ++;
	    printk("temp:%s\n", temp);
        codec_readAreg(reg,&value);
        printk("read :[0x%x]= 0x%x\n",reg,value);

        value = simple_strtoul(temp, NULL, 16) & 0xff;
        printk("value = 0x%x \n",value);
        codec_writeAreg(reg,value);
        printk("write :[0x%x]= 0x%x\n",reg,value);

        codec_readAreg(reg,&value);
        printk("read :[0x%2x] = 0x%x\n",reg,value);
    }
    else
    {
        printk("param error,please Enter again,like this(reg and value in hex):echo reg value >reg\n");
    }
    kfree(temp1);
	return (rc < 0 ? rc : count);
}



static const struct file_operations apollo_audio_switch_fileops = {
  .owner = THIS_MODULE,
  //.llseek = no_llseek,
  .open = apollo_audio_ctl_open,
  .ioctl = apollo_audio_ctl_ioctl,
  .release = apollo_audio_ctl_release,
};
static DEVICE_ATTR(reg, 0777, show_reg, twl4030_writeAreg);

static int __init apollo_audio_switch_init (void)
{
    int rc = 0;
    dev_t devid;
    struct class *my_class;
    if (major) {
        devid = MKDEV (major, 0);
        rc = register_chrdev_region (devid, MAX_PINS, "audio_test");
    } else {
        rc = alloc_chrdev_region (&devid, 0, MAX_PINS, "audio_test");
        major = MAJOR (devid);
	devid = MKDEV (major, 0);
    }
    if (rc < 0) {
        printk ("apollo audio_switch chrdev_region err: %d\n" ,rc);
    }
	printk("major  = %d\n", major);

	io_base = ioremap(0x49022000, SZ_4K);
	io_base_mux = ioremap(0x4800216c, SZ_1K);
	printk("io_base = %p, io_base_mux = %p\n",io_base,io_base_mux);

    cdev_init (&apollo_audio_switch_cdev, &apollo_audio_switch_fileops);
    cdev_add (&apollo_audio_switch_cdev, devid, MAX_PINS);
    my_class = class_create(THIS_MODULE, "my_class");
    audiotest_device = device_create( my_class, audiotest_device, devid, NULL,"audiotest");
  #if 1  
	rc = device_create_file(audiotest_device, &dev_attr_reg);
	if (rc != 0) {
		printk("device_create_file failed: %d\n", rc);
	}
  #endif  

   return 0;			/* succeed */
}

static void __exit
apollo_audio_switch_cleanup (void)
{
    cdev_del (&apollo_audio_switch_cdev);
    /* cdev_put(&apollo_audio_switch_cdev); */
    unregister_chrdev_region (MKDEV (major, 0), MAX_PINS);
}

module_init (apollo_audio_switch_init);
module_exit (apollo_audio_switch_cleanup);
