/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef __VPP_BE_HDMITX_PRV_H__
#define __VPP_BE_HDMITX_PRV_H__

/**********************************************************************
* Private Definitions and Prototypes                                  *
***********************************************************************/
#define HDMITX_RETCODE_CHK(RetCode) \
    if (RetCode != MV_VPP_OK) \
        return (RetCode);

/**********************************************************************
* Static Function Prototypes                                          *
***********************************************************************/
INT HDMITX_UpdatePHYSettings (VPP_OBJ *vpp_obj);

/**********************************************************************
* Register access typedefs and macros                                 *
***********************************************************************/
/* ACR Control Register */
typedef union tagHDMITX_ACR_CTRL
{
    UINT32 u32;
    struct
    {
        UINT32 ACREnable     : 1;
        UINT32 CTSSelect     : 1;
        UINT32 MClkSelect    : 2;
        UINT32 ACRPriority   : 1;
        UINT32 Res           : 27;
    };
} HDMITX_ACR_CTRL;

/* Audio Control Register */
typedef union tagHDMITX_AUD_CTRL
{
    UINT32 u32;
    struct
    {
        UINT32 AudEnable     : 1;
        UINT32 I2SChnEnable  : 4;
        UINT32 I2SMode       : 1;
        UINT32 I2SDbgEnable  : 1;
        UINT32 ValidityBit   : 1;
        UINT32 Layout        : 1;
        UINT32 MuteAudio     : 1;
        UINT32 HBRPacketEn   : 1;
        UINT32 HBRPacketMode : 1;
        UINT32 SPBits        : 16;
        UINT32 Res           : 4;
    };
} HDMITX_AUD_CTRL;

typedef union tagHDMITX_AUD_CTRL1
{
    UINT32 u32;
    struct
    {
        UINT32 Memsize       : 9;
        UINT32 OVFClear      : 1;
        UINT32 UNFClear      : 1;
        UINT32 UVCLoadCtrl   : 1;
        UINT32 FifoRst       : 1;
        UINT32 Irst          : 1;
        UINT32 Aud3DEn       : 1;
        UINT32 MaxNOP        : 2;
        UINT32 NoOfChan      : 6;
        UINT32 MstEn         : 1;
        UINT32 NumStreams    : 2;
        UINT32 SPFlat        : 4;
        UINT32 Res           : 2;
    };
} HDMITX_AUD_CTRL1;

/* I2S Control Register */
typedef union tagHDMITX_I2S_CTRL
{
    UINT32 u32;
    struct
    {
        UINT32 ClkDelayMode      : 1;
        UINT32 ByteShift         : 1;
        UINT32 DataJustification : 1;
        UINT32 LeftChannelWs     : 1;
        UINT32 DataLatchingEdge  : 1;
        UINT32 DataLen           : 6;
        UINT32 Res               : 21;
    };
} HDMITX_I2S_CTRL;

/* GCP Config Register  */
typedef union tagHDMITX_GCP_CFG
{
    UINT32 u32;
    struct
    {
        UINT32 GCPEnable         : 1;
        UINT32 AVMute            : 1;
        UINT32 DefaultPhaseBit   : 1;
        UINT32 PPSWVal           : 1;
        UINT32 PPBits            : 4;
        UINT32 ColorDepth        : 4;
        UINT32 PPCtrl            : 1;
        UINT32 Res               : 19;
    };
} HDMITX_GCP_CFG;


typedef union tagHDMITX_VIDEO_CTRL
{
    UINT32 u32;
    struct
    {
        UINT32 InYC              : 2;
        UINT32 FldPol            : 1;
        UINT32 DebugCtrl         : 1;
        UINT32 DetectVidFmtInHW  : 1;
        UINT32 HdmiMode          : 1;
        UINT32 BCHRotate         : 1;
        UINT32 PixelRepeat       : 4;
        UINT32 bit_width_420     : 4;
        UINT32 Reserved          : 17;
    };
}HDMITX_VIDEO_CTRL;

typedef union tagHDMITX_HST_PKT_CTRL
{
    UINT32 u32;
    struct
    {
        UINT32 PktEn           : 6;
        UINT32 PktCfg          : 6;
        UINT32 PktTransSync    : 1;
        UINT32 PktTransVsyncSel: 1;
        UINT32 PktStart        : 1;
        UINT32 Res             : 17;
    };
} HDMITX_HST_PKT_CTRL;


#endif // __VPP_BE_HDMITX_PRV_H__

