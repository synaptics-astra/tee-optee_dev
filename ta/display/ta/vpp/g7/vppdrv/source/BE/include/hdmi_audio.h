/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef __HDMI_AUDIO_H__
#define __HDMI_AUDIO_H__

#include "hdmi_cmn.h"

/*-----------------------------------------------------------------------------
 * Structures
 *-----------------------------------------------------------------------------
 */
typedef struct tagHDMI_AUD_CHN_STS_DATA
{
    UINT8               NonPcm;
    UINT8               CategoryCode;
    UINT8               SrcNum;
    UINT8               ChnNum;
    UINT8               WordLength; // 24/20 bits
    INT                 SampFreq;
}HDMI_AUD_CHN_STS_DATA, *PHDMI_AUD_CHN_STS_DATA;

/*
 * N Value for 88.2 and 176.4 KHz would be multiples of 44.1 KHz.
 * N Value for 96 and 192 KHz would be multiples of 48 KHz.
 */
typedef struct tagHDMI_ACR_N_DATA
{
    UINT32  NVal_32KHz;
    UINT32  NVal_44_1KHz;
    UINT32  NVal_48KHz;
}HDMI_ACR_N_DATA;

/*-----------------------------------------------------------------------------
 * Function Prototypes
 *-----------------------------------------------------------------------------
 */
BOOL VppHdmiGetAcrNVal (
            CEA_AUDIO_SAMP_FREQ AudSampFreq,
            INT Resolution,
            UINT32 *pAcrN);

BOOL VppHdmiPackAudioChnSts(
            PHDMI_AUD_CHN_STS_DATA pAudioChnSts,
            UINT8 *pDataBuf);

#endif // __HDMI_AUDIO_H__
