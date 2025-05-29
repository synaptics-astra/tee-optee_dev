/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_APIFUNCS_WRAPPER_H_
#define _VPP_APIFUNCS_WRAPPER_H_

#ifndef CONFIG_AMP_TEE_ENABLE
#include "avpll.h"
#else

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
        AVPLL_A = 0,
        AVPLL_B,
        AVPLL_MAX
}AVPLL_TREE;
int AVPLL_GetVcoFreq(int hdmiMode, int freqIndex);
int AVPLL_SetVideoFreq(int avpllGroupId, int freqIndex, int hdmiMode, int frameRate, float overSampleRate, int chId);
int AVPLL_GetChannelOutputFreq(int AVPLLIndex, int chID);
int AVPLL_SetClockFreq(int avpllGroupId, int vco_freq_index, unsigned int target_freq, int chId);
void diag_clock_change_otherClk(unsigned int index, unsigned int pllSwitch, unsigned int pllSelect, unsigned int divider);
void VPP_Enable_INT(UINT32 DhubSemMap, UINT32 Enable);
int VPP_Receive_CECData(VPP_CEC_RX_MSG_BUF *rx_buf);

#ifdef __cplusplus__
}
#endif
#endif //TEE_ENABLE
#endif
