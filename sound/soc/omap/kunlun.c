/*
 * kunlun.c  --  SoC audio for Kunlun
 *
 * Author: Misael Lopez Cruz <x0052729@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/clk.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/gpio.h>
#include <mach/board-kunlun.h>
#include <plat/mcbsp.h>

/* Register descriptions for twl4030 codec part */
#include <linux/mfd/twl4030-codec.h>

#include "../../../arch/arm/mach-omap2/mux.h"
#include "omap-mcbsp.h"
#include "omap-pcm.h"

#define KUNLUN_BT_MCBSP_GPIO		164
#define KUNLUN_HEADSET_MUX_GPIO		(OMAP_MAX_GPIO_LINES + 6)

/* McBSP mux config group */
enum {
	KUNLUN_ASOC_MUX_MCBSP2_SLAVE = 0,
	KUNLUN_ASOC_MUX_MCBSP2_MASTER,
	KUNLUN_ASOC_MUX_MCBSP3_SLAVE,
	KUNLUN_ASOC_MUX_MCBSP3_MASTER,
	KUNLUN_ASOC_MUX_MCBSP3_TRISTATE,
	KUNLUN_ASOC_MUX_MCBSP4_SLAVE,
	KUNLUN_ASOC_MUX_MCBSP4_MASTER,
};

static int kunlun_asoc_mux_config(int group)
{
	switch (group) {
	case KUNLUN_ASOC_MUX_MCBSP2_SLAVE:
		omap_mux_init_signal("mcbsp2_fsx.mcbsp2_fsx",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("mcbsp2_clkx.mcbsp2_clkx",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("mcbsp2_dr.mcbsp2_dr",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("mcbsp2_dx.mcbsp2_dx",
						OMAP_PIN_OUTPUT);
		break;

	case KUNLUN_ASOC_MUX_MCBSP3_SLAVE:
		omap_mux_init_signal("mcbsp3_fsx.mcbsp3_fsx",
						OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("mcbsp3_clkx.mcbsp3_clkx",
						OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("mcbsp3_dr.mcbsp3_dr",
						OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("mcbsp3_dx.mcbsp3_dx",
						OMAP_PIN_OUTPUT);
		break;

	case KUNLUN_ASOC_MUX_MCBSP3_MASTER:
		omap_mux_init_signal("mcbsp_clks.mcbsp_clks",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("mcbsp3_fsx.mcbsp3_fsx",
						OMAP_PIN_OUTPUT);
		omap_mux_init_signal("mcbsp3_clkx.mcbsp3_clkx",
						OMAP_PIN_OUTPUT);
		omap_mux_init_signal("mcbsp3_dr.mcbsp3_dr",
						OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("mcbsp3_dx.mcbsp3_dx",
						OMAP_PIN_OUTPUT);
		break;

	case KUNLUN_ASOC_MUX_MCBSP3_TRISTATE:
		omap_mux_init_signal("mcbsp3_fsx.safe_mode",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("mcbsp3_clkx.safe_mode",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("mcbsp3_dr.safe_mode",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("mcbsp3_dx.safe_mode",
						OMAP_PIN_INPUT);
		break;

	case KUNLUN_ASOC_MUX_MCBSP4_MASTER:
		omap_mux_init_signal("mcbsp_clks.mcbsp_clks",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("mcbsp4_fsx.mcbsp4_fsx",
						OMAP_PIN_OUTPUT);
		omap_mux_init_signal("gpmc_ncs4.mcbsp4_clkx",
						OMAP_PIN_OUTPUT);
		omap_mux_init_signal("gpmc_ncs5.mcbsp4_dr",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("gpmc_ncs6.mcbsp4_dx",
						OMAP_PIN_OUTPUT);
		break;

	case KUNLUN_ASOC_MUX_MCBSP4_SLAVE:
		omap_mux_init_signal("mcbsp4_fsx.mcbsp4_fsx",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("gpmc_ncs4.mcbsp4_clkx",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("gpmc_ncs5.mcbsp4_dr",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("gpmc_ncs6.mcbsp4_dx",
						OMAP_PIN_OUTPUT);
		break;

	default:
		return -ENODEV;
	}
	return 0;
}

static int kunlun_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	/* Set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai,
				  SND_SOC_DAIFMT_I2S |
				  SND_SOC_DAIFMT_NB_IF |
				  SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0) {
		printk(KERN_ERR "can't set codec DAI configuration\n");
		return ret;
	}

	/* Set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai,
				  SND_SOC_DAIFMT_I2S |
				  SND_SOC_DAIFMT_NB_IF |
				  SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0) {
		printk(KERN_ERR "can't set cpu DAI configuration\n");
		return ret;
	}

	/* Set the codec system clock for DAC and ADC */
	ret = snd_soc_dai_set_sysclk(codec_dai, 0, 26000000,
					SND_SOC_CLOCK_IN);
	if (ret < 0) {
		printk(KERN_ERR "can't set codec system clock\n");
		return ret;
	}
#ifdef ENABLE_EXT_CLOCK
	/* Enable the 256 FS clock for HDMI */
	ret = twl4030_cpu_enable_ext_clock(codec_dai->codec, cpu_dai);
	if (ret < 0) {
		printk(KERN_ERR "can't set 256 FS clock\n");
		return ret;
	}
#endif
	/* Use external clock for McBSP2 */
	ret = snd_soc_dai_set_sysclk(cpu_dai, OMAP_MCBSP_SYSCLK_CLKS_EXT,
					0, SND_SOC_CLOCK_OUT);
	if (ret < 0) {
		printk(KERN_ERR "can't set cpu_dai system clock\n");
		return ret;
	}

	return 0;
}

static void kunlun_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	/* Use functional clock for McBSP2 */
	ret = snd_soc_dai_set_sysclk(cpu_dai, OMAP_MCBSP_SYSCLK_CLKS_FCLK,
					0, SND_SOC_CLOCK_OUT);
	if (ret < 0) {
		printk(KERN_ERR "can't set cpu_dai system clock\n");
		/* Ignore error and follow through */
	}
#ifdef ENABLE_EXT_CLOCK
	/* Disable the 256 FS clock used for HDMI */
	ret = twl4030_cpu_disable_ext_clock(codec_dai->codec, cpu_dai);
	if (ret < 0)
		printk(KERN_ERR "can't disable 256 FS clock\n");
#endif
}

static struct snd_soc_ops kunlun_ops = {
	.shutdown  = kunlun_shutdown,
	.hw_params = kunlun_hw_params,
};

static int kunlun_hw_voice_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	kunlun_asoc_mux_config(KUNLUN_ASOC_MUX_MCBSP3_SLAVE);

	/* Set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai,
				SND_SOC_DAIFMT_DSP_A |
				SND_SOC_DAIFMT_IB_NF |
				SND_SOC_DAIFMT_CBS_CFS);
	if (ret) {
		printk(KERN_ERR "can't set codec DAI configuration\n");
		return ret;
	}

	/* Set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai,
				SND_SOC_DAIFMT_DSP_A |
				SND_SOC_DAIFMT_IB_NF |
				SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0) {
		printk(KERN_ERR "can't set cpu DAI configuration\n");
		return ret;
	}

	/* set codec Voice IF to application mode*/
	ret = snd_soc_dai_set_tristate(codec_dai, 0);
	if (ret) {
		printk(KERN_ERR "can't disable codec VIF tristate\n");
		return ret;
	}

	/* Set the codec system clock for DAC and ADC */
	ret = snd_soc_dai_set_sysclk(codec_dai, 0, 26000000,
					SND_SOC_CLOCK_IN);
	if (ret < 0) {
		printk(KERN_ERR "can't set codec system clock\n");
		return ret;
	}
#ifdef ENABLE_EXT_CLOCK
	ret = twl4030_cpu_enable_ext_clock(codec_dai->codec, cpu_dai);
	if (ret < 0) {
		printk(KERN_ERR "can't set 256 FS clock\n");
		return ret;
	}
#endif
	return 0;
}

static void kunlun_voice_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	kunlun_asoc_mux_config(KUNLUN_ASOC_MUX_MCBSP3_TRISTATE);

	ret = snd_soc_dai_set_tristate(codec_dai, 1);
	if (ret) {
		printk(KERN_ERR "can't set codec VIF tristate\n");
		/* Ignore error and follow through */
	}
#ifdef ENABLE_EXT_CLOCK
	ret = twl4030_cpu_disable_ext_clock(codec_dai->codec, cpu_dai);
	if (ret)
		printk(KERN_ERR "can't set 256 FS clock\n");
#endif
}

static struct snd_soc_ops kunlun_voice_ops = {
	.shutdown  = kunlun_voice_shutdown,
	.hw_params = kunlun_hw_voice_params,
};

static int kunlun_pcm_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	//if (gpio_request(KUNLUN_BT_MCBSP_GPIO, "bt_mux") == 0) {
	//	gpio_direction_output(KUNLUN_BT_MCBSP_GPIO, 1);
	//	gpio_free(KUNLUN_BT_MCBSP_GPIO);
	//}

	kunlun_asoc_mux_config(KUNLUN_ASOC_MUX_MCBSP3_SLAVE);

	/* Set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai,
				SND_SOC_DAIFMT_DSP_B |
				SND_SOC_DAIFMT_NB_IF |
				SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0) {
		printk(KERN_ERR "can't set codec DAI configuration\n");
		return ret;
	}

	/* Set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai,
				SND_SOC_DAIFMT_DSP_B |
				SND_SOC_DAIFMT_NB_IF |
				SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0) {
		printk(KERN_ERR "can't set cpu DAI configuration\n");
		return ret;
	}

	/* Set the codec system clock for DAC and ADC */
	ret = snd_soc_dai_set_sysclk(codec_dai, 0, 26000000,
					SND_SOC_CLOCK_IN);
	if (ret < 0) {
		printk(KERN_ERR "can't set codec system clock\n");
		return ret;
	}

	ret = snd_soc_dai_set_tristate(codec_dai, 1);
	if (ret < 0) {
		printk(KERN_ERR "can't set codec VIF tristate\n");
		return ret;
	}
#ifdef ENABLE_EXT_CLOCK
	ret = twl4030_cpu_enable_ext_clock(codec_dai->codec, cpu_dai);
	if (ret < 0) {
		printk(KERN_ERR "can't set 256 FS clock\n");
		return ret;
	}
#endif
	return 0;
}

static void kunlun_pcm_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	kunlun_asoc_mux_config(KUNLUN_ASOC_MUX_MCBSP3_TRISTATE);
#ifdef ENABLE_EXT_CLOCK
	ret = twl4030_cpu_disable_ext_clock(codec_dai->codec, cpu_dai);
	if (ret < 0)
		printk(KERN_ERR "can't disable 256 FS clock\n");
#endif
}

static struct snd_soc_ops kunlun_pcm_ops = {
	.shutdown  = kunlun_pcm_shutdown,
	.hw_params = kunlun_pcm_hw_params,
};

static int kunlun_fm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int ret;

	/* Set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai,
				  SND_SOC_DAIFMT_I2S |
				  SND_SOC_DAIFMT_NB_NF |
				  SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0) {
		printk(KERN_ERR "can't set cpu DAI configuration\n");
		return ret;
	}

	/* Set the codec system clock for DAC and ADC */
	ret = snd_soc_dai_set_sysclk(codec_dai, 0, 26000000,
					SND_SOC_CLOCK_IN);
	if (ret < 0) {
		printk(KERN_ERR "can't set codec system clock\n");
		return ret;
	}
#ifdef ENABLE_EXT_CLOCK
	ret = twl4030_cpu_enable_ext_clock(codec_dai->codec, cpu_dai);
	if (ret < 0) {
		printk(KERN_ERR "can't set 256 FS clock\n");
		return ret;
	}
#endif
	return 0;
}

static void kunlun_fm_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;
#ifdef ENABLE_EXT_CLOCK
	ret = twl4030_cpu_disable_ext_clock(codec_dai->codec, cpu_dai);
	if (ret < 0)
		printk(KERN_ERR "can't disable 256 FS clock\n");
#endif
}

static struct snd_soc_ops kunlun_fm_ops = {
	.shutdown  = kunlun_fm_shutdown,
	.hw_params = kunlun_fm_hw_params,
};

/* Kunlun machine DAPM */
static const struct snd_soc_dapm_widget kunlun_twl4030_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Ext Mic", NULL),
	SND_SOC_DAPM_SPK("Ext Spk", NULL),
	SND_SOC_DAPM_SPK("EARPIECE PIN", NULL),
	SND_SOC_DAPM_MIC("Headset Mic", NULL),
	SND_SOC_DAPM_HP("Headset Stereophone", NULL),
	SND_SOC_DAPM_LINE("Aux In", NULL),
};

static const struct snd_soc_dapm_route audio_map[] = {
	/* External Mics: MAINMIC, SUBMIC with bias*/
	{"MAINMIC", NULL, "Mic Bias 1"},
	{"SUBMIC", NULL, "Mic Bias 2"},
	{"Mic Bias 1", NULL, "Ext Mic"},
	{"Mic Bias 2", NULL, "Ext Mic"},

	/* External Speakers: HFL, HFR */
	{"Ext Spk", NULL, "HFL"},
	{"Ext Spk", NULL, "HFR"},
	{"EARPIECE PIN", NULL, "EARPIECE"},

	/* Headset Stereophone:  HSOL, HSOR */
	{"Headset Stereophone", NULL, "HSOL"},
	{"Headset Stereophone", NULL, "HSOR"},

	/* Headset Mic: HSMIC with bias */
	{"HSMIC", NULL, "Headset Mic Bias"},
	{"Headset Mic Bias", NULL, "Headset Mic"},

	/* Aux In: AUXL, AUXR */
	{"Aux In", NULL, "AUXL"},
	{"Aux In", NULL, "AUXR"},
};

static int kunlun_twl4030_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	int ret;

	/* Add sidetone control for McBSP2 */
	ret = omap_mcbsp_st_add_controls(codec, OMAP_MCBSP2);
	if (ret) {
		printk(KERN_ERR "Failed to add sidetone control for mcbsp2\n");
		return ret;
	}

	/* Add Kunlun specific widgets */
	ret = snd_soc_dapm_new_controls(codec->dapm, kunlun_twl4030_dapm_widgets,
				ARRAY_SIZE(kunlun_twl4030_dapm_widgets));
	if (ret)
		return ret;

	/* Set up Kunlun specific audio path audio_map */
	snd_soc_dapm_add_routes(codec->dapm, audio_map, ARRAY_SIZE(audio_map));

	/* Kunlun connected pins */
	snd_soc_dapm_enable_pin(codec->dapm, "Ext Mic");
	snd_soc_dapm_enable_pin(codec->dapm, "Ext Spk");
	snd_soc_dapm_enable_pin(codec->dapm, "Headset Mic");
	snd_soc_dapm_enable_pin(codec->dapm, "Headset Stereophone");
	snd_soc_dapm_enable_pin(codec->dapm, "Aux In");
	snd_soc_dapm_enable_pin(codec->dapm, "EARPIECE PIN");

	/* TWL4030 not connected pins */
	snd_soc_dapm_nc_pin(codec->dapm, "CARKITMIC");
	snd_soc_dapm_nc_pin(codec->dapm, "DIGIMIC0");
	snd_soc_dapm_nc_pin(codec->dapm, "DIGIMIC1");
	snd_soc_dapm_nc_pin(codec->dapm, "OUTL");
	snd_soc_dapm_nc_pin(codec->dapm, "OUTR");
	//snd_soc_dapm_nc_pin(codec->dapm, "EARPIECE");
	snd_soc_dapm_nc_pin(codec->dapm, "PREDRIVEL");
	snd_soc_dapm_nc_pin(codec->dapm, "PREDRIVER");
	snd_soc_dapm_nc_pin(codec->dapm, "CARKITL");
	snd_soc_dapm_nc_pin(codec->dapm, "CARKITR");

	ret = snd_soc_dapm_sync(codec->dapm);

	return ret;
}

static int kunlun_voice_st_initialized;

static int kunlun_twl4030_voice_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	unsigned short reg;
	int ret;

	/* Enable voice interface */
	reg = codec->driver->read(codec, TWL4030_REG_VOICE_IF);
	reg |= TWL4030_VIF_DIN_EN | TWL4030_VIF_DOUT_EN | TWL4030_VIF_EN;
	codec->driver->write(codec, TWL4030_REG_VOICE_IF, reg);

	if (!kunlun_voice_st_initialized) {
		/* Add sidetone control for McBSP3 */
		ret = omap_mcbsp_st_add_controls(codec, OMAP_MCBSP3);
		if (ret) {
			printk(KERN_ERR
				"Failed to add sidetone control for mcbsp3\n");
			return ret;
		}
		kunlun_voice_st_initialized = 1;
	}
	kunlun_asoc_mux_config(KUNLUN_ASOC_MUX_MCBSP3_SLAVE);
	return 0;
}

/* Digital audio interface glue - connects codec <--> CPU */
static struct snd_soc_dai_link kunlun_dai[] = {
	{
		.name = "TWL4030_I2S",
		.stream_name = "TWL4030_I2S",
		.cpu_dai_name = "omap-mcbsp-dai.1",
		.codec_dai_name = "twl4030-hifi",
		.platform_name = "omap-pcm-audio",
		.codec_name = "twl4030-codec",
		.init = kunlun_twl4030_init,
		.ops = &kunlun_ops,
	},
	{
		.name = "TWL4030_VOICE",
		.stream_name = "TWL4030_Voice",
		.cpu_dai_name = "omap-mcbsp-dai.2",
		.codec_dai_name = "twl4030-voice",
		.platform_name = "omap-pcm-audio",
		.codec_name = "twl4030-codec",
		.init = kunlun_twl4030_voice_init,
		.ops = &kunlun_voice_ops,
	},
	{
		.name = "TWL4030_PCM",
		.stream_name = "TWL4030_PCM",
		.cpu_dai_name = "omap-mcbsp-dai.2",
		.codec_dai_name = "twl4030-voice",
		.platform_name = "omap-pcm-audio",
		.codec_name = "twl4030-codec",
		.init = kunlun_twl4030_voice_init,
		.ops = &kunlun_pcm_ops,
	},
	{
		.name = "TWL4030_FM",
		.stream_name = "TWL4030_FM",
		.cpu_dai_name = "omap-mcbsp-dai.3",
		.codec_dai_name = "twl4030-clock",
		.platform_name = "omap-pcm-audio",
		.codec_name = "twl4030-codec",
		.ops = &kunlun_fm_ops,
	},
};

/* Note: the hdmicbias will be disabled after soc device suspend,
 * which cause error interrupt for headset states
 */
#if defined(CONFIG_SWITCH_GPIO)
extern int gpio_switch_suspend(void);
extern int gpio_switch_resume(void);
int kunlun_headset_suspend(struct platform_device *pdev, pm_message_t state)
{
    return gpio_switch_suspend();
}
int kunlun_headset_resume(struct platform_device *pdev)
{
    return gpio_switch_resume();
}
#endif

/* Audio machine driver */
static struct snd_soc_card snd_soc_kunlun = {
	.name = "Kunlun",
	.long_name = "Kunlun (twl4030)",
	.dai_link = kunlun_dai,
	.num_links = ARRAY_SIZE(kunlun_dai),
#if defined(CONFIG_SWITCH_GPIO)
	.suspend_pre = kunlun_headset_suspend,
	.resume_post = kunlun_headset_resume,
#endif
};

static struct platform_device *kunlun_snd_device;

static int __init kunlun_soc_init(void)
{
	int ret;

	if (!machine_is_omap_kunlun()) {
		pr_debug("Not Kunlun!\n");
		return -ENODEV;
	}
	printk(KERN_INFO "Kunlun SoC init\n");

	kunlun_voice_st_initialized = 0;

	kunlun_asoc_mux_config(KUNLUN_ASOC_MUX_MCBSP2_SLAVE);

	kunlun_snd_device = platform_device_alloc("soc-audio", -1);
	if (!kunlun_snd_device) {
		printk(KERN_ERR "Platform device allocation failed\n");
		return -ENOMEM;
	}

	platform_set_drvdata(kunlun_snd_device, &snd_soc_kunlun);
	ret = platform_device_add(kunlun_snd_device);
	if (ret)
		goto err1;

	BUG_ON(gpio_request(KUNLUN_HEADSET_MUX_GPIO, "hs_mux") < 0);
	gpio_direction_output(KUNLUN_HEADSET_MUX_GPIO, 0);

	//BUG_ON(gpio_request(KUNLUN_HEADSET_EXTMUTE_GPIO, "ext_mute") < 0);
	/* Set EXTMUTE on for initial headset detection */
	//gpio_direction_output(KUNLUN_HEADSET_EXTMUTE_GPIO, 1);

	return 0;

err1:
	printk(KERN_ERR "Unable to add platform device\n");
	platform_device_put(kunlun_snd_device);

	return ret;
}
module_init(kunlun_soc_init);

static void __exit kunlun_soc_exit(void)
{
	gpio_free(KUNLUN_HEADSET_MUX_GPIO);

	platform_device_unregister(kunlun_snd_device);
}
module_exit(kunlun_soc_exit);

MODULE_AUTHOR("Misael Lopez Cruz <x0052729@ti.com>");
MODULE_DESCRIPTION("ALSA SoC Kunlun");
MODULE_LICENSE("GPL");

