/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef _AVPLL_H_
#define _AVPLL_H_

#ifdef __cplusplus
extern "C" {
#endif

// audio channel frequency
typedef enum
{
    AUDIO_FREQ_8P4672_MHZ=0,
    AUDIO_FREQ_11P2896_MHZ,
    AUDIO_FREQ_16P9344_MHZ,
    AUDIO_FREQ_22P5792_MHZ,
    AUDIO_FREQ_12P288_MHZ,
    AUDIO_FREQ_24P576_MHZ,
    AUDIO_FREQ_16P384_MHZ,
    AUDIO_FREQ_65P536_MHZ,
    AUDIO_FREQ_90P3168_MHZ,
    AUDIO_FREQ_98P304_MHZ,

    AUDIO_FREQ_22P636_MHZ,
    AUDIO_FREQ_24P636_MHZ,
    AUDIO_FREQ_16P424_MHZ,
    AUDIO_FREQ_90P544_MHZ,
    AUDIO_FREQ_98P544_MHZ,
    AUDIO_FREQ_65P696_MHZ,

    AUDIO_FREQ_22P522_MHZ,
    AUDIO_FREQ_24P514_MHZ,
    AUDIO_FREQ_16P342_MHZ,
    AUDIO_FREQ_90P088_MHZ,
    AUDIO_FREQ_98P056_MHZ,
    AUDIO_FREQ_65P368_MHZ,
} AUDIO_CLK_FREQ;

typedef enum
{
        AVPLL_VCO_FREQ_1_080G=0,
        AVPLL_VCO_FREQ_1_260G,
        AVPLL_VCO_FREQ_1_350G,
        AVPLL_VCO_FREQ_1_485G,
        AVPLL_VCO_FREQ_1_512G,
        AVPLL_VCO_FREQ_1_620G,
        AVPLL_VCO_FREQ_1_856G,
        AVPLL_VCO_FREQ_2_227G,
        AVPLL_VCO_FREQ_1_353G,
        AVPLL_VCO_FREQ_2_970G,
        AVPLL_VCO_FREQ_MAX
}AVPLL_VCO_FREQ;

typedef enum
{
    AVPLL_A = 0,
    AVPLL_B,
    AVPLL_MAX
}AVPLL_TREE;

typedef enum
{
    AVPLL_E_OK = 0,
    AVPLL_E_BADPARAM = 2,
    AVPLL_E_MAX
}AVPLL_ERROR_CODES;

int AVPLL_SetVideoFreq(int avpllGroupId, int freqIndex, int hdmiMode, int frameRate, float overSampleRate, int chId);
int AVPLL_GetChannelOutputFreq(int AVPLLIndex, int chID);
int AVPLL_GetVcoFreq(int hdmiMode, int freqIndex);

#ifdef __cplusplus
}
#endif

#endif

