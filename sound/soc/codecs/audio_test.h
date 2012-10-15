#ifndef __AUDIO_TEST_H__
#define __AUDIO_TEST_H__


#include <linux/ioctl.h>
#include <linux/mfd/twl4030-codec.h>

#define TWL4030_REG_SW_SHADOW		0x4A
#define TWL4030_CACHEREGNUM	(TWL4030_REG_SW_SHADOW + 1)

//#define AUDIO_PLAY _IOWR('A',0x0,int)

enum {
	AUDIO_PATH_DEBUG = _IOR('A', 0x00, int),
	PATH_SPEAKER = _IOR('A', 0x01, int),
	PATH_HEADSET = _IOR('A', 0x02, int),
	PATH_MIC1 = _IOR('A', 0x03, int),
	PATH_MIC2 = _IOR('A', 0x04, int),	
	SEL_VOICE = _IOR('A', 0x05, int),
	EARPIECE_VOICE = _IOR('A', 0x06, int),
	HEADSET_VOICE = _IOR('A', 0x07, int),
	PATH_EARPIECE = _IOR('A', 0x08, int),
	HSP = _IOR('A', 0x09, int),
	MIC1_SPEAKER_LOOPBACK = _IOR('A', 0x0A, int),
	MIC1_EARPIECE_LOOPBACK = _IOR('A', 0x0B, int),
	MIC2_HEADSET_LOOPBACK = _IOR('A', 0x0C, int),
	MIC1_HEADSET_LOOPBACK = _IOR('A', 0x0D, int),
	PATH_AUX = _IOR('A', 0x0E, int),
	DISABLE_MIC = _IOR('A', 0x0F, int),
	ENABLE_MIC = _IOR('A', 0x10, int),
    MUTE_OUTPUT = _IOR('A', 0x11, int),
    UNMUTE_OUTPUT = _IOR('A', 0x12, int),
    DISABLE_AUX = _IOR('A', 0x13, int),
    ENABLE_AUX  = _IOR('A', 0x14, int),
    SAVE_ENV	= _IOR('A', 0x15, int),
    RESTORE_ENV	= _IOR('A', 0x16, int),
};

/* Sample rate */
#define NARROWBAND_FREQ				8000
#define WIDEBAND_FREQ				16000
#define AUDIO_RATE_DEFAULT			8000
#define VOICE_RATE_DEFAULT			NARROWBAND_FREQ
#define AUDIO_APLL_DEFAULT			APLL_CTL_FREQ_26_0MHZ


#define AUDIO_MODE				1
#define	VOICE_MODE				2
#define LOOPBACK_MODE           3

/* Input/output volumes */
#define INPUT_VOLUME				0x11
#define INPUT_HEADSET_MIC			0x12
#define INPUT_MAIN_MIC				0x13
#define INPUT_SUB_MIC				0x14
#define INPUT_AUX				0x15
#define INPUT_CARKIT				0x16

#define OUTPUT_VOLUME				0x21
#define OUTPUT_STEREO_HEADSET			0x22
#define OUTPUT_HANDS_FREE_CLASSD		0x23
#define OUTPUT_MONO_EARPIECE			0x24
#define OUTPUT_SIDETONE				0x25
#define OUTPUT_CARKIT				0x26
#define OUTPUT_DOWNLINK				0x27

/* BitField Definitions */

/* CODEC_MODE Fields */
#define BIT_CODEC_MODE_OPT_MODE                  (0x000)
#define BIT_CODEC_MODE_OPT_MODE_M                (0x00000001)

#define CODEC_OPTION_1                           (0x1)
#define CODEC_OPTION_2                           (0x0)

#define BIT_CODEC_MODE_CODECPDZ                  (0x001)
#define BIT_CODEC_MODE_CODECPDZ_M                (0x00000002)
#define BIT_CODEC_MODE_SPARE                     (0x002)
#define BIT_CODEC_MODE_SPARE_M                   (0x00000004)
#define BIT_CODEC_MODE_SEL_16K                   (0x003)
#define BIT_CODEC_MODE_SEL_16K_M                 (0x00000008)

#define VOICE_MODE_RATE_08_000K                  (0x0)
#define VOICE_MODE_RATE_16_000K                  (0x1)

#define BIT_CODEC_MODE_APLL_RATE                 (0x004)
#define BIT_CODEC_MODE_APLL_RATE_M               (0x000000F0)

#define AUDIO_MODE_RATE_08_000                   (0x0)
#define AUDIO_MODE_RATE_11_025                   (0x1)
#define AUDIO_MODE_RATE_12_000                   (0x2)
#define AUDIO_MODE_RATE_16_000                   (0x4)
#define AUDIO_MODE_RATE_22_050                   (0x5)
#define AUDIO_MODE_RATE_24_000                   (0x6)
#define AUDIO_MODE_RATE_32_000                   (0x8)
#define AUDIO_MODE_RATE_44_100                   (0x9)
#define AUDIO_MODE_RATE_48_000                   (0xA)
#define AUDIO_MODE_RATE_96_000                   (0xE)
/* OPTION Fields */
#define BIT_OPTION_ATXL1_EN                      (0x000)
#define BIT_OPTION_ATXL1_EN_M                    (0x00000001)
#define BIT_OPTION_ATXR1_EN                      (0x001)
#define BIT_OPTION_ATXR1_EN_M                    (0x00000002)
#define BIT_OPTION_ATXL2_VTXL_EN                 (0x002)
#define BIT_OPTION_ATXL2_VTXL_EN_M               (0x00000004)
#define BIT_OPTION_ATXR2_VTXR_EN                 (0x003)
#define BIT_OPTION_ATXR2_VTXR_EN_M               (0x00000008)
#define BIT_OPTION_ARXL1_VRX_EN                  (0x004)
#define BIT_OPTION_ARXL1_VRX_EN_M                (0x00000010)
#define BIT_OPTION_ARXR1_EN                      (0x005)
#define BIT_OPTION_ARXR1_EN_M                    (0x00000020)
#define BIT_OPTION_ARXL2_EN                      (0x006)
#define BIT_OPTION_ARXL2_EN_M                    (0x00000040)
#define BIT_OPTION_ARXR2_EN                      (0x007)
#define BIT_OPTION_ARXR2_EN_M                    (0x00000080)
/* MICBIAS_CTL Fields */
#define BIT_MICBIAS_CTL_MICBIAS1_EN              (0x000)
#define BIT_MICBIAS_CTL_MICBIAS1_EN_M            (0x00000001)
#define BIT_MICBIAS_CTL_MICBIAS2_EN              (0x001)
#define BIT_MICBIAS_CTL_MICBIAS2_EN_M            (0x00000002)
#define BIT_MICBIAS_CTL_HSMICBIAS_EN             (0x002)
#define BIT_MICBIAS_CTL_HSMICBIAS_EN_M           (0x00000004)
#define BIT_MICBIAS_CTL_MICBIAS1_CTL             (0x005)
#define BIT_MICBIAS_CTL_MICBIAS1_CTL_M           (0x00000020)
#define BIT_MICBIAS_CTL_MICBIAS2_CTL             (0x006)
#define BIT_MICBIAS_CTL_MICBIAS2_CTL_M           (0x00000040)
#define BIT_MICBIAS_CTL_SPARE                    (0x007)
#define BIT_MICBIAS_CTL_SPARE_M                  (0x00000080)
/* ANAMICL Fields */
#define BIT_ANAMICL_MAINMIC_EN                   (0x000)
#define BIT_ANAMICL_MAINMIC_EN_M                 (0x00000001)
#define BIT_ANAMICL_HSMIC_EN                     (0x001)
#define BIT_ANAMICL_HSMIC_EN_M                   (0x00000002)
#define BIT_ANAMICL_AUXL_EN                      (0x002)
#define BIT_ANAMICL_AUXL_EN_M                    (0x00000004)
#define BIT_ANAMICL_CKMIC_EN                     (0x003)
#define BIT_ANAMICL_CKMIC_EN_M                   (0x00000008)
#define BIT_ANAMICL_MICAMPL_EN                   (0x004)
#define BIT_ANAMICL_MICAMPL_EN_M                 (0x00000010)
#define BIT_ANAMICL_OFFSET_CNCL_SEL              (0x005)
#define BIT_ANAMICL_OFFSET_CNCL_SEL_M            (0x00000060)
#define BIT_ANAMICL_CNCL_OFFSET_START            (0x007)
#define BIT_ANAMICL_CNCL_OFFSET_START_M          (0x00000080)
#define OFFSET_CNCL_ARXL1_ARXR1			0
#define OFFSET_CNCL_ARXL2_ARXR2			1
#define OFFSET_CNCL_VRX				2
#define OFFSET_CNCL_ALL				3
/* ANAMICR Fields */
#define BIT_ANAMICR_SUBMIC_EN                    (0x000)
#define BIT_ANAMICR_SUBMIC_EN_M                  (0x00000001)
#define BIT_ANAMICR_AUXR_EN                      (0x002)
#define BIT_ANAMICR_AUXR_EN_M                    (0x00000004)
#define BIT_ANAMICR_MICAMPR_EN                   (0x004)
#define BIT_ANAMICR_MICAMPR_EN_M                 (0x00000010)
/* AVADC_CTL Fields */
#define BIT_AVADC_CTL_ADCR_EN                    (0x001)
#define BIT_AVADC_CTL_ADCR_EN_M                  (0x00000002)
#define BIT_AVADC_CTL_AVADC_CLK_PRIORITY         (0x002)
#define BIT_AVADC_CTL_AVADC_CLK_PRIORITY_M       (0x00000004)
#define BIT_AVADC_CTL_ADCL_EN                    (0x003)
#define BIT_AVADC_CTL_ADCL_EN_M                  (0x00000008)
/* ADCMICSEL Fields */
#define BIT_ADCMICSEL_TX1IN_SEL                  (0x000)
#define BIT_ADCMICSEL_TX1IN_SEL_M                (0x00000001)
#define BIT_ADCMICSEL_DIGMIC0_EN                 (0x001)
#define BIT_ADCMICSEL_DIGMIC0_EN_M               (0x00000002)
#define BIT_ADCMICSEL_TX2IN_SEL                  (0x002)
#define BIT_ADCMICSEL_TX2IN_SEL_M                (0x00000004)
#define BIT_ADCMICSEL_DIGMIC1_EN                 (0x003)
#define BIT_ADCMICSEL_DIGMIC1_EN_M               (0x00000008)
/* DIGMIXING Fields */
#define BIT_DIGMIXING_VTX_MIXING                 (0x002)
#define BIT_DIGMIXING_VTX_MIXING_M               (0x0000000C)
#define BIT_DIGMIXING_ARX2_MIXING                (0x004)
#define BIT_DIGMIXING_ARX2_MIXING_M              (0x00000030)
#define BIT_DIGMIXING_ARX1_MIXING                (0x006)
#define BIT_DIGMIXING_ARX1_MIXING_M              (0x000000C0)
#define INPUT_MIN_GAIN				0x00
#define INPUT_MAX_GAIN				0x1F
/* ATXL1PGA Fields */
#define BIT_ATXL1PGA_ATXL1PGA_GAIN               (0x000)
#define BIT_ATXL1PGA_ATXL1PGA_GAIN_M             (0x0000001F)
/* ATXR1PGA Fields */
#define BIT_ATXR1PGA_ATXR1PGA_GAIN               (0x000)
#define BIT_ATXR1PGA_ATXR1PGA_GAIN_M             (0x0000001F)
/* AVTXL2PGA Fields */
#define BIT_AVTXL2PGA_AVTXL2PGA_GAIN             (0x000)
#define BIT_AVTXL2PGA_AVTXL2PGA_GAIN_M           (0x0000001F)
/* AVTXR2PGA Fields */
#define BIT_AVTXR2PGA_AVTXR2PGA_GAIN             (0x000)
#define BIT_AVTXR2PGA_AVTXR2PGA_GAIN_M           (0x0000001F)
/* AUDIO_IF Fields */
#define BIT_AUDIO_IF_AIF_EN                      (0x000)
#define BIT_AUDIO_IF_AIF_EN_M                    (0x00000001)
#define BIT_AUDIO_IF_CLK256FS_EN                 (0x001)
#define BIT_AUDIO_IF_CLK256FS_EN_M               (0x00000002)
#define BIT_AUDIO_IF_AIF_TRI_EN                  (0x002)
#define BIT_AUDIO_IF_AIF_TRI_EN_M                (0x00000004)

#define AUDIO_DATA_FORMAT_I2S                    (0x0)
#define AUDIO_DATA_FORMAT_LJUST                  (0x1)
#define AUDIO_DATA_FORMAT_RJUST                  (0x2)
#define AUDIO_DATA_FORMAT_TDM                    (0x3)

#define BIT_AUDIO_IF_AIF_FORMAT                  (0x003)
#define BIT_AUDIO_IF_AIF_FORMAT_M                (0x00000018)

#define AUDIO_DATA_WIDTH_16SAMPLE_16DATA         (0x0)
#define AUDIO_DATA_WIDTH_32SAMPLE_16DATA         (0x2)
#define AUDIO_DATA_WIDTH_32SAMPLE_24DATA         (0x3)

#define BIT_AUDIO_IF_DATA_WIDTH                  (0x005)
#define BIT_AUDIO_IF_DATA_WIDTH_M                (0x00000060)
#define BIT_AUDIO_IF_AIF_SLAVE_EN                (0x007)
#define BIT_AUDIO_IF_AIF_SLAVE_EN_M              (0x00000080)
/* VOICE_IF Fields */
#define BIT_VOICE_IF_VIF_EN                      (0x000)
#define BIT_VOICE_IF_VIF_EN_M                    (0x00000001)
#define BIT_VOICE_IF_VIF_SUB_EN                  (0x001)
#define BIT_VOICE_IF_VIF_SUB_EN_M                (0x00000002)
#define BIT_VOICE_IF_VIF_TRI_EN                  (0x002)
#define BIT_VOICE_IF_VIF_TRI_EN_M                (0x00000004)
#define BIT_VOICE_IF_VIF_FORMAT                  (0x003)
#define BIT_VOICE_IF_VIF_FORMAT_M                (0x00000008)
#define BIT_VOICE_IF_VIF_SWAP                    (0x004)
#define BIT_VOICE_IF_VIF_SWAP_M                  (0x00000010)
#define BIT_VOICE_IF_VIF_DOUT_EN                 (0x005)
#define BIT_VOICE_IF_VIF_DOUT_EN_M               (0x00000020)
#define BIT_VOICE_IF_VIF_DIN_EN                  (0x006)
#define BIT_VOICE_IF_VIF_DIN_EN_M                (0x00000040)
#define BIT_VOICE_IF_VIF_SLAVE_EN                (0x007)
#define BIT_VOICE_IF_VIF_SLAVE_EN_M              (0x00000080)
/* Analog gain range */
#define OUTPUT_MIN_GAIN                          0x00
#define OUTPUT_MAX_GAIN                          0x3F
#define AUDIO_OUTPUT_COARSE_GAIN_LOW             (0x0)
#define AUDIO_OUTPUT_COARSE_GAIN_6DB             (0x1)
#define AUDIO_OUTPUT_COARSE_GAIN_12DB            (0x2)

/* ARXR1PGA Fields */
#define BIT_ARXR1PGA_ARXR1PGA_FGAIN              (0x000)
#define BIT_ARXR1PGA_ARXR1PGA_FGAIN_M            (0x0000003F)
#define BIT_ARXR1PGA_ARXR1PGA_CGAIN              (0x006)
#define BIT_ARXR1PGA_ARXR1PGA_CGAIN_M            (0x000000C0)
/* ARXL1PGA Fields */
#define BIT_ARXL1PGA_ARXL1PGA_FGAIN              (0x000)
#define BIT_ARXL1PGA_ARXL1PGA_FGAIN_M            (0x0000003F)
#define BIT_ARXL1PGA_ARXL1PGA_CGAIN              (0x006)
#define BIT_ARXL1PGA_ARXL1PGA_CGAIN_M            (0x000000C0)
/* ARXR2PGA Fields */
#define BIT_ARXR2PGA_ARXR2PGA_FGAIN              (0x000)
#define BIT_ARXR2PGA_ARXR2PGA_FGAIN_M            (0x0000003F)
#define BIT_ARXR2PGA_ARXR2PGA_CGAIN              (0x006)
#define BIT_ARXR2PGA_ARXR2PGA_CGAIN_M            (0x000000C0)
/* ARXL2PGA Fields */
#define BIT_ARXL2PGA_ARXL2PGA_FGAIN              (0x000)
#define BIT_ARXL2PGA_ARXL2PGA_FGAIN_M            (0x0000003F)
#define BIT_ARXL2PGA_ARXL2PGA_CGAIN              (0x006)
#define BIT_ARXL2PGA_ARXL2PGA_CGAIN_M            (0x000000C0)
/* VRXPGA Fields */
#define BIT_VRXPGA_VRXPGA_GAIN                   (0x000)
#define BIT_VRXPGA_VRXPGA_GAIN_M                 (0x0000003F)
/* Voice gain range */
#define VOICE_MIN_GAIN				0x00
#define VOICE_MAX_GAIN				0x31
/* VSTPGA Fields */
#define BIT_VSTPGA_VSTPGA_GAIN                   (0x000)
#define BIT_VSTPGA_VSTPGA_GAIN_M                 (0x0000003F)
/* Sidetone gain range */
#define SIDETONE_MIN_GAIN			0x00
#define SIDETONE_MAX_GAIN			0x29
/* VRX2ARXPGA Fields */
#define BIT_VRX2ARXPGA_VRX2ARXPGA_GAIN           (0x000)
#define BIT_VRX2ARXPGA_VRX2ARXPGA_GAIN_M         (0x0000001F)
/* AVDAC_CTL Fields */
#define BIT_AVDAC_CTL_ADACR1_EN                  (0x000)
#define BIT_AVDAC_CTL_ADACR1_EN_M                (0x00000001)
#define BIT_AVDAC_CTL_ADACL1_EN                  (0x001)
#define BIT_AVDAC_CTL_ADACL1_EN_M                (0x00000002)
#define BIT_AVDAC_CTL_ADACR2_EN                  (0x002)
#define BIT_AVDAC_CTL_ADACR2_EN_M                (0x00000004)
#define BIT_AVDAC_CTL_ADACL2_EN                  (0x003)
#define BIT_AVDAC_CTL_ADACL2_EN_M                (0x00000008)
#define BIT_AVDAC_CTL_VDAC_EN                    (0x004)
#define BIT_AVDAC_CTL_VDAC_EN_M                  (0x00000010)
/* ARX2VTXPGA Fields */
#define BIT_ARX2VTXPGA_ARX2VTXPGA_GAIN           (0x000)
#define BIT_ARX2VTXPGA_ARX2VTXPGA_GAIN_M         (0x0000003F)

/* Digital gain range */
#define OUTPUT_ANALOG_GAIN_MIN			0x12
#define OUTPUT_ANALOG_GAIN_MAX			0x00

/* ARXL1_APGA_CTL Fields */
#define BIT_ARXL1_APGA_CTL_ARXL1_PDZ             (0x000)
#define BIT_ARXL1_APGA_CTL_ARXL1_PDZ_M           (0x00000001)
#define BIT_ARXL1_APGA_CTL_ARXL1_DA_EN           (0x001)
#define BIT_ARXL1_APGA_CTL_ARXL1_DA_EN_M         (0x00000002)
#define BIT_ARXL1_APGA_CTL_ARXL1_FM_EN           (0x002)
#define BIT_ARXL1_APGA_CTL_ARXL1_FM_EN_M         (0x00000004)
#define BIT_ARXL1_APGA_CTL_ARXL1_GAIN_SET        (0x003)
#define BIT_ARXL1_APGA_CTL_ARXL1_GAIN_SET_M      (0x000000F8)
/* ARXR1_APGA_CTL Fields */
#define BIT_ARXR1_APGA_CTL_ARXR1_PDZ             (0x000)
#define BIT_ARXR1_APGA_CTL_ARXR1_PDZ_M           (0x00000001)
#define BIT_ARXR1_APGA_CTL_ARXR1_DA_EN           (0x001)
#define BIT_ARXR1_APGA_CTL_ARXR1_DA_EN_M         (0x00000002)
#define BIT_ARXR1_APGA_CTL_ARXR1_FM_EN           (0x002)
#define BIT_ARXR1_APGA_CTL_ARXR1_FM_EN_M         (0x00000004)
#define BIT_ARXR1_APGA_CTL_ARXR1_GAIN_SET        (0x003)
#define BIT_ARXR1_APGA_CTL_ARXR1_GAIN_SET_M      (0x000000F8)
/* ARXL2_APGA_CTL Fields */
#define BIT_ARXL2_APGA_CTL_ARXL2_PDZ             (0x000)
#define BIT_ARXL2_APGA_CTL_ARXL2_PDZ_M           (0x00000001)
#define BIT_ARXL2_APGA_CTL_ARXL2_DA_EN           (0x001)
#define BIT_ARXL2_APGA_CTL_ARXL2_DA_EN_M         (0x00000002)
#define BIT_ARXL2_APGA_CTL_ARXL2_FM_EN           (0x002)
#define BIT_ARXL2_APGA_CTL_ARXL2_FM_EN_M         (0x00000004)
#define BIT_ARXL2_APGA_CTL_ARXL2_GAIN_SET        (0x003)
#define BIT_ARXL2_APGA_CTL_ARXL2_GAIN_SET_M      (0x000000F8)
/* ARXR2_APGA_CTL Fields */
#define BIT_ARXR2_APGA_CTL_ARXR2_PDZ             (0x000)
#define BIT_ARXR2_APGA_CTL_ARXR2_PDZ_M           (0x00000001)
#define BIT_ARXR2_APGA_CTL_ARXR2_DA_EN           (0x001)
#define BIT_ARXR2_APGA_CTL_ARXR2_DA_EN_M         (0x00000002)
#define BIT_ARXR2_APGA_CTL_ARXR2_FM_EN           (0x002)
#define BIT_ARXR2_APGA_CTL_ARXR2_FM_EN_M         (0x00000004)
#define BIT_ARXR2_APGA_CTL_ARXR2_GAIN_SET        (0x003)
#define BIT_ARXR2_APGA_CTL_ARXR2_GAIN_SET_M      (0x000000F8)
/* ATX2ARXPGA Fields */
#define BIT_ATX2ARXPGA_ATX2ARXR_PGA              (0x000)
#define BIT_ATX2ARXPGA_ATX2ARXR_PGA_M            (0x00000007)
#define BIT_ATX2ARXPGA_ATX2ARXL_PGA              (0x003)
#define BIT_ATX2ARXPGA_ATX2ARXL_PGA_M            (0x00000038)
/* BT_IF Fields */
#define BIT_BT_IF_BT_EN                          (0x000)
#define BIT_BT_IF_BT_EN_M                        (0x00000001)
#define BIT_BT_IF_BT_TRI_EN                      (0x002)
#define BIT_BT_IF_BT_TRI_EN_M                    (0x00000004)
#define BIT_BT_IF_BT_SWAP                        (0x004)
#define BIT_BT_IF_BT_SWAP_M                      (0x00000010)
#define BIT_BT_IF_BT_DOUT_EN                     (0x005)
#define BIT_BT_IF_BT_DOUT_EN_M                   (0x00000020)
#define BIT_BT_IF_BT_DIN_EN                      (0x006)
#define BIT_BT_IF_BT_DIN_EN_M                    (0x00000040)
#define BIT_BT_IF_SPARE                          (0x007)
#define BIT_BT_IF_SPARE_M                        (0x00000080)
/* BTPGA Fields */
#define BIT_BTPGA_BTRXPGA_GAIN                   (0x000)
#define BIT_BTPGA_BTRXPGA_GAIN_M                 (0x0000000F)
#define BIT_BTPGA_BTTXPGA_GAIN                   (0x004)
#define BIT_BTPGA_BTTXPGA_GAIN_M                 (0x000000F0)
/* BTSTPGA Fields */
#define BIT_BTSTPGA_BTSTPGA_GAIN                 (0x000)
#define BIT_BTSTPGA_BTSTPGA_GAIN_M               (0x0000003F)
/* EAR_CTL Fields */
#define BIT_EAR_CTL_EAR_VOICE_EN                 (0x000)
#define BIT_EAR_CTL_EAR_VOICE_EN_M               (0x00000001)
#define BIT_EAR_CTL_EAR_AL1_EN                   (0x001)
#define BIT_EAR_CTL_EAR_AL1_EN_M                 (0x00000002)
#define BIT_EAR_CTL_EAR_AL2_EN                   (0x002)
#define BIT_EAR_CTL_EAR_AL2_EN_M                 (0x00000004)
#define BIT_EAR_CTL_EAR_AR1_EN                   (0x003)
#define BIT_EAR_CTL_EAR_AR1_EN_M                 (0x00000008)
#define BIT_EAR_CTL_EAR_GAIN                     (0x004)
#define BIT_EAR_CTL_EAR_GAIN_M                   (0x00000030)
#define BIT_EAR_CTL_SPARE                        (0x006)
#define BIT_EAR_CTL_SPARE_M                      (0x00000040)
#define BIT_EAR_CTL_EAR_OUTLOW_EN                (0x007)
#define BIT_EAR_CTL_EAR_OUTLOW_EN_M              (0x00000080)
/* Earpiece gain range */
#define EARPHONE_GAIN_MIN			0x03
#define EARPHONE_GAIN_MAX			0x01

/* HS_GAIN_SET Fields */
#define BIT_HS_GAIN_SET_HSL_GAIN                 (0x000)
#define BIT_HS_GAIN_SET_HSL_GAIN_M               (0x00000003)
#define BIT_HS_GAIN_SET_HSR_GAIN                 (0x002)
#define BIT_HS_GAIN_SET_HSR_GAIN_M               (0x0000000C)
#define BIT_HS_GAIN_SET_SPARE                    (0x006)
#define BIT_HS_GAIN_SET_SPARE_M                  (0x00000040)
/* Headset gain range */
#define HEADSET_GAIN_MIN			0x03
#define HEADSET_GAIN_MAX			0x01

/* HS_SEL Fields */
#define BIT_HS_SEL_HSOL_VOICE_EN                 (0x000)
#define BIT_HS_SEL_HSOL_VOICE_EN_M               (0x00000001)
#define BIT_HS_SEL_HSOL_AL1_EN                   (0x001)
#define BIT_HS_SEL_HSOL_AL1_EN_M                 (0x00000002)
#define BIT_HS_SEL_HSOL_AL2_EN                   (0x002)
#define BIT_HS_SEL_HSOL_AL2_EN_M                 (0x00000004)
#define BIT_HS_SEL_HSOR_VOICE_EN                 (0x003)
#define BIT_HS_SEL_HSOR_VOICE_EN_M               (0x00000008)
#define BIT_HS_SEL_HSOR_AR1_EN                   (0x004)
#define BIT_HS_SEL_HSOR_AR1_EN_M                 (0x00000010)
#define BIT_HS_SEL_HSOR_AR2_EN                   (0x005)
#define BIT_HS_SEL_HSOR_AR2_EN_M                 (0x00000020)
#define BIT_HS_SEL_HS_OUTLOW_EN                  (0x006)
#define BIT_HS_SEL_HS_OUTLOW_EN_M                (0x00000040)
#define BIT_HS_SEL_HSR_INV_EN                    (0x007)
#define BIT_HS_SEL_HSR_INV_EN_M                  (0x00000080)
/* HS_POPN_SET Fields */
#define BIT_HS_POPN_SET_RAMP_EN                  (0x001)
#define BIT_HS_POPN_SET_RAMP_EN_M                (0x00000002)
#define BIT_HS_POPN_SET_RAMP_DELAY               (0x002)
#define BIT_HS_POPN_SET_RAMP_DELAY_M             (0x0000001C)
#define BIT_HS_POPN_SET_EXTMUTE                  (0x005)
#define BIT_HS_POPN_SET_EXTMUTE_M                (0x00000020)
#define BIT_HS_POPN_SET_VMID_EN                  (0x006)
#define BIT_HS_POPN_SET_VMID_EN_M                (0x00000040)
/* PREDL_CTL Fields */
#define BIT_PREDL_CTL_PREDL_VOICE_EN             (0x000)
#define BIT_PREDL_CTL_PREDL_VOICE_EN_M           (0x00000001)
#define BIT_PREDL_CTL_PREDL_AL1_EN               (0x001)
#define BIT_PREDL_CTL_PREDL_AL1_EN_M             (0x00000002)
#define BIT_PREDL_CTL_PREDL_AL2_EN               (0x002)
#define BIT_PREDL_CTL_PREDL_AL2_EN_M             (0x00000004)
#define BIT_PREDL_CTL_PREDL_AR2_EN               (0x003)
#define BIT_PREDL_CTL_PREDL_AR2_EN_M             (0x00000008)
#define BIT_PREDL_CTL_PREDL_GAIN                 (0x004)
#define BIT_PREDL_CTL_PREDL_GAIN_M               (0x00000030)
#define BIT_PREDL_CTL_PREDL_OUTLOW_EN            (0x007)
#define BIT_PREDL_CTL_PREDL_OUTLOW_EN_M          (0x00000080)
/* PREDR_CTL Fields */
#define BIT_PREDR_CTL_PREDR_VOICE_EN             (0x000)
#define BIT_PREDR_CTL_PREDR_VOICE_EN_M           (0x00000001)
#define BIT_PREDR_CTL_PREDR_AR1_EN               (0x001)
#define BIT_PREDR_CTL_PREDR_AR1_EN_M             (0x00000002)
#define BIT_PREDR_CTL_PREDR_AR2_EN               (0x002)
#define BIT_PREDR_CTL_PREDR_AR2_EN_M             (0x00000004)
#define BIT_PREDR_CTL_PREDR_AL2_EN               (0x003)
#define BIT_PREDR_CTL_PREDR_AL2_EN_M             (0x00000008)
#define BIT_PREDR_CTL_PREDR_GAIN                 (0x004)
#define BIT_PREDR_CTL_PREDR_GAIN_M               (0x00000030)
#define BIT_PREDR_CTL_PREDR_OUTLOW_EN            (0x007)
#define BIT_PREDR_CTL_PREDR_OUTLOW_EN_M          (0x00000080)
/* PRECKL_CTL Fields */
#define BIT_PRECKL_CTL_PRECKL_VOICE_EN           (0x000)
#define BIT_PRECKL_CTL_PRECKL_VOICE_EN_M         (0x00000001)
#define BIT_PRECKL_CTL_PRECKL_AL1_EN             (0x001)
#define BIT_PRECKL_CTL_PRECKL_AL1_EN_M           (0x00000002)
#define BIT_PRECKL_CTL_PRECKL_AL2_EN             (0x002)
#define BIT_PRECKL_CTL_PRECKL_AL2_EN_M           (0x00000004)
#define BIT_PRECKL_CTL_PRECKL_GAIN               (0x004)
#define BIT_PRECKL_CTL_PRECKL_GAIN_M             (0x00000030)
#define BIT_PRECKL_CTL_PRECKL_EN                 (0x006)
#define BIT_PRECKL_CTL_PRECKL_EN_M               (0x00000040)
/* PRECKR_CTL Fields */
#define BIT_PRECKR_CTL_PRECKR_VOICE_EN           (0x000)
#define BIT_PRECKR_CTL_PRECKR_VOICE_EN_M         (0x00000001)
#define BIT_PRECKR_CTL_PRECKR_AR1_EN             (0x001)
#define BIT_PRECKR_CTL_PRECKR_AR1_EN_M           (0x00000002)
#define BIT_PRECKR_CTL_PRECKR_AR2_EN             (0x002)
#define BIT_PRECKR_CTL_PRECKR_AR2_EN_M           (0x00000004)
#define BIT_PRECKR_CTL_PRECKR_GAIN               (0x004)
#define BIT_PRECKR_CTL_PRECKR_GAIN_M             (0x00000030)
#define BIT_PRECKR_CTL_PRECKR_EN                 (0x006)
#define BIT_PRECKR_CTL_PRECKR_EN_M               (0x00000040)
/* Carkit gain range */
#define CARKIT_MIN_GAIN				0x03
#define CARKIT_MAX_GAIN				0x01

#define HANDS_FREEL_VOICE                        (0x0)
#define HANDS_FREEL_AL1                          (0x1)
#define HANDS_FREEL_AL2                          (0x2)
#define HANDS_FREEL_AR2                          (0x3)
#define HANDS_FREER_VOICE                        (0x0)
#define HANDS_FREER_AR1                          (0x1)
#define HANDS_FREER_AR2                          (0x2)
#define HANDS_FREER_AL2                          (0x3)
/* HFL_CTL Fields */
#define BIT_HFL_CTL_HFL_INPUT_SEL                (0x000)
#define BIT_HFL_CTL_HFL_INPUT_SEL_M              (0x00000003)
#define BIT_HFL_CTL_HFL_HB_EN                    (0x002)
#define BIT_HFL_CTL_HFL_HB_EN_M                  (0x00000004)
#define BIT_HFL_CTL_HFL_LOOP_EN                  (0x003)
#define BIT_HFL_CTL_HFL_LOOP_EN_M                (0x00000008)
#define BIT_HFL_CTL_HFL_RAMP_EN                  (0x004)
#define BIT_HFL_CTL_HFL_RAMP_EN_M                (0x00000010)
#define BIT_HFL_CTL_HFL_REF_EN                   (0x005)
#define BIT_HFL_CTL_HFL_REF_EN_M                 (0x00000020)
/* HFR_CTL Fields */
#define BIT_HFR_CTL_HFR_INPUT_SEL                (0x000)
#define BIT_HFR_CTL_HFR_INPUT_SEL_M              (0x00000003)
#define BIT_HFR_CTL_HFR_HB_EN                    (0x002)
#define BIT_HFR_CTL_HFR_HB_EN_M                  (0x00000004)
#define BIT_HFR_CTL_HFR_LOOP_EN                  (0x003)
#define BIT_HFR_CTL_HFR_LOOP_EN_M                (0x00000008)
#define BIT_HFR_CTL_HFR_RAMP_EN                  (0x004)
#define BIT_HFR_CTL_HFR_RAMP_EN_M                (0x00000010)
#define BIT_HFR_CTL_HFR_REF_EN                   (0x005)
#define BIT_HFR_CTL_HFR_REF_EN_M                 (0x00000020)
/* ALC_CTL Fields */
#define BIT_ALC_CTL_ALC_WAIT                     (0x000)
#define BIT_ALC_CTL_ALC_WAIT_M                   (0x00000007)
#define BIT_ALC_CTL_MAINMIC_ALC_EN               (0x003)
#define BIT_ALC_CTL_MAINMIC_ALC_EN_M             (0x00000008)
#define BIT_ALC_CTL_SUBMIC_ALC_EN                (0x004)
#define BIT_ALC_CTL_SUBMIC_ALC_EN_M              (0x00000010)
#define BIT_ALC_CTL_ALC_MODE                     (0x005)
#define BIT_ALC_CTL_ALC_MODE_M                   (0x00000020)
#define BIT_ALC_CTL_SPARE1                       (0x006)
#define BIT_ALC_CTL_SPARE1_M                     (0x00000040)
#define BIT_ALC_CTL_SPARE2                       (0x007)
#define BIT_ALC_CTL_SPARE2_M                     (0x00000080)
/* ALC_SET1 Fields */
#define BIT_ALC_SET1_ALC_MIN_LIMIT               (0x000)
#define BIT_ALC_SET1_ALC_MIN_LIMIT_M             (0x00000007)
#define BIT_ALC_SET1_ALC_MAX_LIMIT               (0x003)
#define BIT_ALC_SET1_ALC_MAX_LIMIT_M             (0x00000038)
/* ALC_SET2 Fields */
#define BIT_ALC_SET2_ALC_RELEASE                 (0x000)
#define BIT_ALC_SET2_ALC_RELEASE_M               (0x00000007)
#define BIT_ALC_SET2_ALC_ATTACK                  (0x003)
#define BIT_ALC_SET2_ALC_ATTACK_M                (0x00000038)
#define BIT_ALC_SET2_ALC_STEP                    (0x006)
#define BIT_ALC_SET2_ALC_STEP_M                  (0x00000040)
/* BOOST_CTL Fields */
#define BIT_BOOST_CTL_EFFECT                     (0x000)
#define BIT_BOOST_CTL_EFFECT_M                   (0x00000003)
/* SOFTVOL_CTL Fields */
#define BIT_SOFTVOL_CTL_SOFTVOL_EN               (0x000)
#define BIT_SOFTVOL_CTL_SOFTVOL_EN_M             (0x00000001)
#define BIT_SOFTVOL_CTL_SOFTVOL_SET              (0x005)
#define BIT_SOFTVOL_CTL_SOFTVOL_SET_M            (0x000000E0)
/* DTMF_FREQSEL Fields */
#define BIT_DTMF_FREQSEL_FREQSEL                 (0x000)
#define BIT_DTMF_FREQSEL_FREQSEL_M               (0x0000001F)
#define BIT_DTMF_FREQSEL_SPARE1                  (0x006)
#define BIT_DTMF_FREQSEL_SPARE1_M                (0x00000040)
#define BIT_DTMF_FREQSEL_SPARE2                  (0x007)
#define BIT_DTMF_FREQSEL_SPARE2_M                (0x00000080)
/* DTMF_TONEXT1H Fields */
#define BIT_DTMF_TONEXT1H_EXT_TONE1H             (0x000)
#define BIT_DTMF_TONEXT1H_EXT_TONE1H_M           (0x000000FF)
/* DTMF_TONEXT1L Fields */
#define BIT_DTMF_TONEXT1L_EXT_TONE1L             (0x000)
#define BIT_DTMF_TONEXT1L_EXT_TONE1L_M           (0x000000FF)
/* DTMF_TONEXT2H Fields */
#define BIT_DTMF_TONEXT2H_EXT_TONE2H             (0x000)
#define BIT_DTMF_TONEXT2H_EXT_TONE2H_M           (0x000000FF)
/* DTMF_TONEXT2L Fields */
#define BIT_DTMF_TONEXT2L_EXT_TONE2L             (0x000)
#define BIT_DTMF_TONEXT2L_EXT_TONE2L_M           (0x000000FF)
/* DTMF_TONOFF Fields */
#define BIT_DTMF_TONOFF_TONE_ON_TIME             (0x000)
#define BIT_DTMF_TONOFF_TONE_ON_TIME_M           (0x0000000F)
#define BIT_DTMF_TONOFF_TONE_OFF_TIME            (0x004)
#define BIT_DTMF_TONOFF_TONE_OFF_TIME_M          (0x000000F0)
/* DTMF_WANONOFF Fields */
#define BIT_DTMF_WANONOFF_WAMBLE_ON_TIME         (0x000)
#define BIT_DTMF_WANONOFF_WAMBLE_ON_TIME_M       (0x0000000F)
#define BIT_DTMF_WANONOFF_WAMBLE_OFF_TIME        (0x004)
#define BIT_DTMF_WANONOFF_WAMBLE_OFF_TIME_M      (0x000000F0)
/* I2S_RX_SCRAMBLE_H Fields */
#define BIT_I2S_RX_SCRAMBLE_H_I2S_RX_SCRAMBLE_H  (0x000)
#define BIT_I2S_RX_SCRAMBLE_H_I2S_RX_SCRAMBLE_H_M (0x000000FF)
/* I2S_RX_SCRAMBLE_M Fields */
#define BIT_I2S_RX_SCRAMBLE_M_I2S_RX_SCRAMBLE_M  (0x000)
#define BIT_I2S_RX_SCRAMBLE_M_I2S_RX_SCRAMBLE_M_M (0x000000FF)
/* I2S_RX_SCRAMBLE_L Fields */
#define BIT_I2S_RX_SCRAMBLE_L_I2S_RX_SCRAMBLE_L  (0x000)
#define BIT_I2S_RX_SCRAMBLE_L_I2S_RX_SCRAMBLE_L_M (0x000000FF)
/* APLL_CTL Fields */
#define APLL_CTL_FREQ_19_2MHZ                    (0x5)
#define APLL_CTL_FREQ_26_0MHZ                    (0x6)
#define APLL_CTL_FREQ_38_4MHZ                    (0xF)
#define BIT_APLL_CTL_APLL_INFREQ                 (0x000)
#define BIT_APLL_CTL_APLL_INFREQ_M               (0x0000000F)
#define BIT_APLL_CTL_APLL_EN                     (0x004)
#define BIT_APLL_CTL_APLL_EN_M                   (0x00000010)
/* DTMF_CTL Fields */
#define BIT_DTMF_CTL_TONE_EN                     (0x000)
#define BIT_DTMF_CTL_TONE_EN_M                   (0x00000001)
#define BIT_DTMF_CTL_DUAL_TONE_EN                (0x001)
#define BIT_DTMF_CTL_DUAL_TONE_EN_M              (0x00000002)
#define BIT_DTMF_CTL_TONE_PATTERN                (0x002)
#define BIT_DTMF_CTL_TONE_PATTERN_M              (0x00000004)
#define BIT_DTMF_CTL_TONE_MODE                   (0x003)
#define BIT_DTMF_CTL_TONE_MODE_M                 (0x00000008)
/* DTMF_PGA_CTL2 Fields */
#define BIT_DTMF_PGA_CTL2_TONE3_GAIN             (0x000)
#define BIT_DTMF_PGA_CTL2_TONE3_GAIN_M           (0x00000007)
#define BIT_DTMF_PGA_CTL2_TONE4_GAIN             (0x004)
#define BIT_DTMF_PGA_CTL2_TONE4_GAIN_M           (0x00000070)
#define BIT_DTMF_PGA_CTL2_TONE_18DB_ATT          (0x007)
#define BIT_DTMF_PGA_CTL2_TONE_18DB_ATT_M        (0x00000080)
/* DTMF_PGA_CTL1 Fields */
#define BIT_DTMF_PGA_CTL1_TONE1_GAIN             (0x000)
#define BIT_DTMF_PGA_CTL1_TONE1_GAIN_M           (0x0000000F)
#define BIT_DTMF_PGA_CTL1_TONE2_GAIN             (0x004)
#define BIT_DTMF_PGA_CTL1_TONE2_GAIN_M           (0x000000F0)
/* MISC_SET_1 Fields */
#define BIT_MISC_SET_1_DIGMIC_LR_SWAP_EN         (0x000)
#define BIT_MISC_SET_1_DIGMIC_LR_SWAP_EN_M       (0x00000001)
#define BIT_MISC_SET_1_SPARE1                    (0x001)
#define BIT_MISC_SET_1_SPARE1_M                  (0x00000002)
#define BIT_MISC_SET_1_SPARE2                    (0x002)
#define BIT_MISC_SET_1_SPARE2_M                  (0x00000004)
#define BIT_MISC_SET_1_FMLOOP_EN                 (0x005)
#define BIT_MISC_SET_1_FMLOOP_EN_M               (0x00000020)
#define BIT_MISC_SET_1_SCRAMBLE_EN               (0x006)
#define BIT_MISC_SET_1_SCRAMBLE_EN_M             (0x00000040)
#define BIT_MISC_SET_1_CLK64_EN                  (0x007)
#define BIT_MISC_SET_1_CLK64_EN_M                (0x00000080)
/* PCMBTMUX Fields */
#define BIT_PCMBTMUX_TNRXACT_BT                  (0x000)
#define BIT_PCMBTMUX_TNRXACT_BT_M                (0x00000001)
#define BIT_PCMBTMUX_TNTXACT_BT                  (0x001)
#define BIT_PCMBTMUX_TNTXACT_BT_M                (0x00000002)
#define BIT_PCMBTMUX_TNRXACT_PCM                 (0x002)
#define BIT_PCMBTMUX_TNRXACT_PCM_M               (0x00000004)
#define BIT_PCMBTMUX_TNTXACT_PCM                 (0x003)
#define BIT_PCMBTMUX_TNTXACT_PCM_M               (0x00000008)
#define BIT_PCMBTMUX_MUXRX_BT                    (0x005)
#define BIT_PCMBTMUX_MUXRX_BT_M                  (0x00000020)
#define BIT_PCMBTMUX_MUXRX_PCM                   (0x006)
#define BIT_PCMBTMUX_MUXRX_PCM_M                 (0x00000040)
#define BIT_PCMBTMUX_MUXTX_PCM                   (0x007)
#define BIT_PCMBTMUX_MUXTX_PCM_M                 (0x00000080)
/* RX_PATH_SEL Fields */
#define BIT_RX_PATH_SEL_RXR1_SEL                 (0x000)
#define BIT_RX_PATH_SEL_RXR1_SEL_M               (0x00000003)
#define BIT_RX_PATH_SEL_RXL1_SEL                 (0x002)
#define BIT_RX_PATH_SEL_RXL1_SEL_M               (0x0000000C)
#define BIT_RX_PATH_SEL_RXR2_SEL                 (0x004)
#define BIT_RX_PATH_SEL_RXR2_SEL_M               (0x00000010)
#define BIT_RX_PATH_SEL_RXL2_SEL                 (0x005)
#define BIT_RX_PATH_SEL_RXL2_SEL_M               (0x00000020)
/* VDL_APGA_CTL Fields */
#define BIT_VDL_APGA_CTL_VDL_PDZ                 (0x000)
#define BIT_VDL_APGA_CTL_VDL_PDZ_M               (0x00000001)
#define BIT_VDL_APGA_CTL_VDL_DA_EN               (0x001)
#define BIT_VDL_APGA_CTL_VDL_DA_EN_M             (0x00000002)
#define BIT_VDL_APGA_CTL_VDL_FM_EN               (0x002)
#define BIT_VDL_APGA_CTL_VDL_FM_EN_M             (0x00000004)
#define BIT_VDL_APGA_CTL_VDL_GAIN_SET            (0x003)
#define BIT_VDL_APGA_CTL_VDL_GAIN_SET_M          (0x000000F8)
/* VIBRA_CTL Fields */
#define BIT_VIBRA_CTL_VIVRA_EN                   (0x000)
#define BIT_VIBRA_CTL_VIVRA_EN_M                 (0x00000001)
#define BIT_VIBRA_CTL_VIBRA_DIR                  (0x001)
#define BIT_VIBRA_CTL_VIBRA_DIR_M                (0x00000002)
#define BIT_VIBRA_CTL_VIBRA_AUDIO_SEL            (0x002)
#define BIT_VIBRA_CTL_VIBRA_AUDIO_SEL_M          (0x0000000C)
#define BIT_VIBRA_CTL_VIBRA_SEL                  (0x004)
#define BIT_VIBRA_CTL_VIBRA_SEL_M                (0x00000010)
#define BIT_VIBRA_CTL_VIBRA_DIR_SEL              (0x005)
#define BIT_VIBRA_CTL_VIBRA_DIR_SEL_M            (0x00000020)
#define BIT_VIBRA_CTL_SPARE                      (0x006)
#define BIT_VIBRA_CTL_SPARE_M                    (0x00000040)
/* VIBRA_SET Fields */
#define BIT_VIBRA_SET_VIBRA_AVG_VAL              (0x000)
#define BIT_VIBRA_SET_VIBRA_AVG_VAL_M            (0x000000FF)
/* VIBRA_PWM_SET Fields */
#define BIT_VIBRA_PWM_SET_PWM_ON                 (0x000)
#define BIT_VIBRA_PWM_SET_PWM_ON_M               (0x000000FF)
#define MICAM_GAIN_MIN                           (0x0)
#define MICAM_GAIN_MAX                           (0x5)
/* ANAMIC_GAIN Fields */
#define BIT_ANAMIC_GAIN_MICAMPL_GAIN             (0x000)
#define BIT_ANAMIC_GAIN_MICAMPL_GAIN_M           (0x00000007)
#define BIT_ANAMIC_GAIN_MICAMPR_GAIN             (0x003)
#define BIT_ANAMIC_GAIN_MICAMPR_GAIN_M           (0x00000038)
/* Microphone amplifier gain range */
#define MICROPHONE_MIN_GAIN			0x00
#define MICROPHONE_MAX_GAIN			0x05
/* MISC_SET_2 Fields */
#define BIT_MISC_SET_2_SPARE1                    (0x000)
#define BIT_MISC_SET_2_SPARE1_M                  (0x00000001)
#define BIT_MISC_SET_2_VRX_HPF_BYP               (0x001)
#define BIT_MISC_SET_2_VRX_HPF_BYP_M             (0x00000002)
#define BIT_MISC_SET_2_VTX_HPF_BYP               (0x002)
#define BIT_MISC_SET_2_VTX_HPF_BYP_M             (0x00000004)
#define BIT_MISC_SET_2_ARX_HPF_BYP               (0x003)
#define BIT_MISC_SET_2_ARX_HPF_BYP_M             (0x00000008)
#define BIT_MISC_SET_2_VRX_3RD_HPF_BYP           (0x004)
#define BIT_MISC_SET_2_VRX_3RD_HPF_BYP_M         (0x00000010)
#define BIT_MISC_SET_2_ATX_HPF_BYP               (0x005)
#define BIT_MISC_SET_2_ATX_HPF_BYP_M             (0x00000020)
#define BIT_MISC_SET_2_VTX_3RD_HPF_BYP           (0x006)
#define BIT_MISC_SET_2_VTX_3RD_HPF_BYP_M         (0x00000040)
#define BIT_MISC_SET_2_SPARE2                    (0x007)
#define BIT_MISC_SET_2_SPARE2_M                  (0x00000080)

/* Volume calculation */
#define AUDIO_MAX_INPUT_VOLUME			100
#define AUDIO_MAX_OUTPUT_VOLUME			100
#define AUDIO_MAX_VOLUME			100
#define AUDIO_DEF_COARSE_VOLUME_LEVEL		AUDIO_OUTPUT_COARSE_GAIN_LOW
/* Input/output digital/analog gain controllers */
#define TXL1			0x11
#define TXR1			0x12
#define TXL2			0x13
#define TXR2			0x14

#define RXL1			0x21
#define RXR1			0x22
#define RXL2			0x23
#define	RXR2			0x24

#define VDL			0x31
#define VST			0x41

#define is_input(ARG)           (((ARG) >> 4) == 1)
#define is_output(ARG)          (((ARG) >> 4) == 2)
#define is_voice(ARG)		(((ARG) >> 4) == 3)
#define is_sidetone(ARG)	(((ARG) >> 4) == 4)
extern int audio_play_enable(void);
extern void show_reg(void);
extern void show_reg_real(void);
extern ssize_t twl4030_writeAreg(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count);

extern void switch_outs(int codec_mode, int source);
extern int twl4030_codec_tog_on(void);
extern  void codec_disable_outputs(void);
extern void switch_inputs(int codec_mode, int source);
extern void audio_para(int para);
extern void twl4030_codec_on(void);
extern void twl4030_codec_off(void);
extern int twl4030_configure(int codec_mode);
extern int codec_writeAreg(unsigned int reg,unsigned char value);
extern int codec_readAreg(unsigned int reg,unsigned char *value);

//#define VIA_DEBUG 1
#ifdef VIA_DEBUG
#undef audio_debug
#define audio_debug(format, arg...) printk(KERN_DEBUG format "\n" , ##arg)
#else
#undef dbg
#define audio_debug(format, arg...) do {} while (0)
#endif


#endif
