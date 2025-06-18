/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef __HDMI_EDID_H__
#define __HDMI_EDID_H__

#include "hdmi_cmn.h"

/*-----------------------------------------------------------------------------
 * EDID Parser - Public Header File
 *-----------------------------------------------------------------------------
 */

/*-----------------------------------------------------------------------------
 * Macros
 *-----------------------------------------------------------------------------
 */
// Check RetCode
#define EDID_CHK_RET_CODE(RetCode) \
    if (RetCode != EDID_ERR_OK) \
    { \
        VPPHDMI_DBG_ERROR(("Error is Parsing: %d\n",RetCode)); \
        return RetCode; \
    }


/*-----------------------------------------------------------------------------
 * Constants
 *-----------------------------------------------------------------------------
 */
#define SIZE_OF_EDID_BLK            128
#define EDID_BLK_TAG_ADDR           0x00
// EDID Block 0 Field Position
#define EDID_BLK0_HDR_ADDR          0x00
#define EDID_BLK0_PRD_ID_ADDR       0x08
#define EDID_BLK0_VERS_ADDR         0x12
#define EDID_BLK0_REV_ADDR          0x13
#define EDID_BLK0_DISP_PARAMS_ADDR  0x14
#define EDID_BLK0_ETD_ADDR          0x23
#define EDID_BLK0_STD_ADDR          0x26
#define EDID_BLK0_DTD_ADDR          0x36
#define EDID_BLK0_MDB_ADDR          0x5A
#define EDID_BLK0_EXTN_FLAG_ADDR    0x7E

#define EDID_DTD_LEN                0x12

// EDID CEA Block Field Position
#define EDID_CEA_EXTN_HDR_ADDR      0x00
#define EDID_CEA_EXTN_REV_ADDR      0x01
#define EDID_CEA_EXTN_DTD_OFS_ADDR  0x02
#define EDID_CEA_EXTN_FMT_ADDR      0x03
#define EDID_CEA_EXTN_DATA_BLK_ADDR 0x04

#define EDID_CEA_RESRVD_TAG         0x00
#define EDID_CEA_AUDIO_BLK_TAG      0x01
#define EDID_CEA_VIDEO_BLK_TAG      0x02
#define EDID_CEA_VNDR_BLK_TAG       0x03
#define EDID_CEA_SPKR_ALLOC_BLK_TAG 0x04
#define EDID_CEA_DTC_BLK_TAG        0x05
#define EDID_CEA_EXTNDED_BLK_TAG    0x07

// CEA Block extension tags
#define EDID_CEA_EXTNDED_VIDEOCAPS_BLK_TAG  0x00
#define EDID_CEA_EXTNDED_VIDEOCAL_BLK_TAG   0x05
#define EDID_CEA_EXTENDED_VIDEO420_BLK_TAG  0x0E
#define EDID_CEA_EXTENDED_420_CAP_MAP_BLK_TAG 0x0F
#define EDID_CEA_EXTENDED_HDR_BLK_TAG       0x06
#define EDID_CEA_EXTENDED_VSVDB_TAG  0x01
#define EDID_CEA_EXTENDED_VSADB_TAG  0x11
#define EDID_CEA_EXTENDED_SCDB_BLK_TAG  0x79

// EDID Extension Tags
#define EDID_BLOCK_MAP_TAG          0xF0
#define EDID_CEA_EXTN_TAG           0x02
#define EDID_DI_EXTN_TAG            0x40
#define EDID_LS_EXTN_TAG            0x50
#define EDID_MI_EXTN_TAG            0x60
#define EDID_MNTR_MNFR_EXTN_TAG     0xFF

// EDID Error Codes
typedef enum tagEDID_RETURNS
{
    EDID_ERR_OK = 0,
    EDID_ERR_INVALID_PARAM,
    EDID_ERR_INVALID_BLK_DATA,
    EDID_ERR_CRC_FAILED,
    EDID_ERR_UNSUPPORTED_BLK,
    EDID_ERR_MAX
}EDID_RETURNS;

// EDID Blocks that are supported for parsing
typedef enum tagEDID_BLK_TYPE
{
    EDID_BASIC_BLK = 0x00,
    EDID_CEA_TIMING_EXTN,
    EDID_BLOCK_MAP,
    EDID_UNDEF_BLK,
    EDID_MAX_BLK_TYPE
}EDID_BLK_TYPE, *PEDID_BLK_TYPE;

#define SPRT_ENABLED        1
#define SPRT_DISABLED       0
#define RES_INFO_UNDEF      0xFFFF

/* Edid Basic Block Structures */
typedef struct tagEDID_GEN_INFO
{
    // EDID Version
    unsigned char   Version;
    unsigned char   Revision;

    // Receiver Product Details
    unsigned char   MnfrName[4];
    unsigned char   MnfrWeek;
    unsigned short  MnfrYear;
    char ProductCode[5];
    unsigned int    SerialNum;
}EDID_GEN_INFO, *PEDID_GEN_INFO;

typedef struct tagEDID_DIGITAL_VID_INP
{
    unsigned char    DFP1XEnabled;
}EDID_DIGITAL_VID_INP, *PEDID_DIGITAL_VID_INP;

typedef enum tagEDID_ANALOG_VPP
{
    EDID_VPP_LVL1 = 0, // 0.3 - 0.7
    EDID_VPP_LVL2 = 2, // 0.286 - 0.714
    EDID_VPP_LVL3 = 4, // 0.4 - 1.0
    EDID_VPP_LVL4 = 6  // 0.0 - 0.7
}EDID_ANALOG_VPP, *PEDID_ANALOG_VPP;

typedef struct tagEDID_ANALOG_VID_INP
{
    EDID_ANALOG_VPP VppLevel;
    unsigned char   BlankToBlackSetup;
    unsigned char   SepSyncs;
    unsigned char   CompSync;
    unsigned char   SyncOnGreen;
    unsigned char   SerrationVSync;
}EDID_ANALOG_VID_INP, *PEDID_ANALOG_VID_INP;

typedef union tagEDID_VIDEO_INP
{
    EDID_DIGITAL_VID_INP    DigVidInp;
    EDID_ANALOG_VID_INP     AnalogVidInp;
}EDID_VIDEO_INP, *PEDID_VIDEO_INP;

typedef enum tagEDID_DISP_TYPE
{
    EDID_MONOCHROME_DISP = 0,
    EDID_RGB_DISP,
    EDID_NONRGB_DISP,
    EDID_UNDEF_DISP
}EDID_DISP_TYPE, *PEDID_DISP_TYPE;

typedef struct tagEDID_FEATURE_SPRT
{
    unsigned char    GTFSprt;
    unsigned char    PrefTiming;
    unsigned char    sRGBSprt;
    unsigned char    ActiveOffSprt;
    unsigned char    SuspendSprt;
    unsigned char    StandbySprt;
    EDID_DISP_TYPE   DispType;
}EDID_FEATURE_SPRT, *PEDID_FEATURE_SPRT;


typedef struct tagEDID_HDMI_3D_SPRT
{
    UINT8 Hdmi_Video_Present;            //Additional Video format capabilities are described
    UINT8 Hdmi_3D_Present;                //HDMI Sink Supports mandatory 3D Video Formats plus any additional formats
                                        //indicated by

    UINT8 Hdmi_3D_Multi_Present;
    UINT8 Hdmi_3D_Len;                    //This field is 0 if the sink supports only the mandatory 3D Video formats.
    UINT8 Hdmi_VIC_Len;                    //Indicates the total length from HDMi_VIC_1 to HDMI_VIC_M

    UINT8 Hdmi_VIC[7];                    //This field indicates the video formats which are supported by the sink.
    UINT16 Hdmi_3D_Structure_All;
    UINT16 Hdmi_3D_Mask;                //For each corresponding VIC within the first 16 entries in EDID,sink has 3D support as
                                        //designated by 3D_Structure_All field.
    UINT8 Hdmi_2D_VIC_Num;                    // number of 2D VICs
    UINT8 Hdmi_2D_VIC_Order[16];            //value in this field is the pointer to particular VIC in EDID.
    UINT8 Hdmi_3D_Structure[16];            //This field indicates 3D capability for corresponding VIC Code indicated by 2D_VIC_Order
    UINT8 Hdmi_3D_Detail[16];                //
}EDID_HDMI_3D_SPRT,*PEDID_HDMI_3D_SPRT;


typedef struct tagEDID_LATENCY_FIELDS
{
    UINT8 Latency_Fields_Present;
    UINT8 I_Latency_Fields_Present;
    UINT8 Video_Latency;
    UINT8 Audio_Latency;
    UINT8 Interlaced_Video_Latency;
    UINT8 Interlaced_Audio_Latency;

}EDID_LATENCY_FIELDS, *PEDID_LATENCY_FIELDS;


typedef struct tagEDID_CEA_FEATURE_SPRT
{
    unsigned char    UnderScanSprt      : 1;
    unsigned char    YCbCr444Sprt       : 1;
    unsigned char    YCbCr422Sprt       : 1;
    // Indicates if 30/36/48 bit deep color modes are supported
    unsigned char    DC30bit            : 1;
    unsigned char    DC36bit            : 1;
    unsigned char    DC48bit            : 1;
    // Indicates whether YCbCr444 is supported at Deep color modes
    unsigned char    YCbCr444AtDC       : 1;
    // Indicates whether sink accepts ACP,ISRC packets
    unsigned char    SupportsAI         : 1;
}EDID_CEA_FEATURE_SPRT, *PEDID_CEA_FEATURE_SPRT;

typedef struct tagEDID_COL_CHAR
{
    unsigned short  Rx;
    unsigned short  Ry;
    unsigned short  Gx;
    unsigned short  Gy;
    unsigned short  Bx;
    unsigned short  By;
    unsigned short  Wx;
    unsigned short  Wy;
}EDID_COL_CHAR, *PEDID_COL_CHAR;

typedef struct tagEDID_DISP_PARAMS
{
    unsigned char   DigVideoInput;
    EDID_VIDEO_INP  VideoInpDet;
    // In cm
    unsigned char   MaxHImageSize;
    // In cm
    unsigned char   MaxVImageSize;
    // (Actual Gamma Value * 100) - 100
    unsigned char   Gamma;

    EDID_FEATURE_SPRT   FeatureSprt;
    EDID_COL_CHAR       ColorChar;
}EDID_DISP_PARAMS, *PEDID_DISP_PARAMS;

typedef struct tagEDID_MONITOR_INFO
{
    unsigned char   MonitorName[14];// Monitor Name in ASCII format
    unsigned char   MinVRate;       // Minimum Vertical Rate in Hz
    unsigned char   MaxVRate;       // Maximum Vertical Rate in Hz
    unsigned char   MinHRate;       // Minimum Horizontal Rate in KHz
    unsigned char   MaxHRate;       // Maximum Horizontal Rate in KHz
    unsigned char   MaxPixelClock;  // Maximum supported pixel clock rate in MHz/10
}EDID_MONITOR_INFO, *PEDID_MONITOR_INFO;

// Audio Data Structures
typedef enum tagEDID_AUDIO_FORMAT
{
    EDID_AUD_FMT_RESERVED = 0,
    EDID_AUD_FMT_LPCM,
    EDID_AUD_FMT_AC3,
    EDID_AUD_FMT_MPEG1,
    EDID_AUD_FMT_MP3,
    EDID_AUD_FMT_MPEG2,
    EDID_AUD_FMT_AAC,
    EDID_AUD_FMT_DTS,
    EDID_AUD_FMT_ATRAC,
    EDID_AUD_FMT_ONE_BIT_AUDIO,
    EDID_AUD_FMT_DOLBY_DIGITAL,
    EDID_AUD_FMT_DTS_HD,
    EDID_AUD_FMT_MAT,
    EDID_AUD_FMT_DST,
    EDID_AUD_FMT_WMA_PRO,
    EDID_AUD_FMT_MAX
}EDID_AUDIO_FORMAT;

typedef struct tagEDID_AUD_FREQ
{
    unsigned char   Freq32KHz   : 1;
    unsigned char   Freq44_1KHz : 1;
    unsigned char   Freq48KHz   : 1;
    unsigned char   Freq88_2KHz : 1;
    unsigned char   Freq96KHz   : 1;
    unsigned char   Freq176_4KHz: 1;
    unsigned char   Freq192KHz  : 1;
    unsigned char   Reserved    : 1;
}EDID_AUD_FREQ;

typedef struct tagEDID_UNCOMPR_AUDRATE
{
    unsigned char    Sprt16Bps;
    unsigned char    Sprt20Bps;
    unsigned char    Sprt24Bps;
}EDID_UNCOMPR_AUDRATE, *PEDID_UNCOMPR_AUDRATE;

// Should be considered along with the audio format
typedef union tagEDID_AUDRATE
{
    unsigned int            CompAudRate;
    EDID_UNCOMPR_AUDRATE    UnComprAudRate;
}EDID_AUDRATE, *PEDID_AUDRATE;

typedef struct tagEDID_AUDIO_INFO
{
    EDID_AUDIO_FORMAT   AudioFmt;
    EDID_AUDRATE        AudioRate;
    unsigned char       MaxNumChnls;
    unsigned char       Frequency;
}EDID_AUDIO_INFO, *PEDID_AUDIO_INFO;

typedef struct tagEDID_SPKR_ALLOC_FLD
{
    unsigned char   FlFr  : 1; // FrontLeft/Front Rear
    unsigned char   Lfe   : 1; // Low Frequency Effect
    unsigned char   Fc    : 1; // Front Center
    unsigned char   RlRr  : 1; // Rear Left/Rear Right
    unsigned char   Rc    : 1; // Rear Center
    unsigned char   FlcFrc: 1; // Front Left Center/Front Right Center
    unsigned char   RlcRrc: 1; // Rear Left Center /Rear Right Center
    unsigned char   Res   : 1;
}EDID_SPKR_ALLOC_FLD, *PEDID_SPKR_ALLOC_FLD;

// Calorimetry support
typedef struct tagEDID_CALORIMETRY_INFO
{
    unsigned short  xvYCC601    : 1;
    unsigned short  xvYCC709    : 1;
    unsigned short  sYCC601     : 1;
    unsigned short  AdobeYCC601 : 1;
    unsigned short  AdobeRGB    : 1;
    unsigned short  BT2020RGB   : 1;
    unsigned short  BT2020YCC   : 1;
    unsigned short  BT2020cYCC  : 1;
    unsigned short  MD0         : 1;
    unsigned short  MD1         : 1;
    unsigned short  MD2         : 1;
    unsigned short  MD3         : 1;
    unsigned short  res         : 4;
}EDID_CALORIMETRY_INFO, *PEDID_CALORIMETRY_INFO;

typedef struct tagEDID_CAPABILITY_INFO
{
    unsigned char   QS        : 1;
    unsigned char   QY        : 1;
    unsigned char   res       : 6;
}EDID_CAPABILITY_INFO, *PEDID_CAPABILITY_INFO;

typedef struct tagEDID_YCbCr420_CAP_INFO
{
    UINT8    VideoIdCode[16]; //Check max VICs at that support only 420
}EDID_YCbCr420_CAP_INFO, *PEDID_YCbCr420_CAP_INFO;

typedef struct tagEDID_YCbCr420_CAP_MAP_INFO
{
    UINT8    VideoIdCode[31]; //Max 31 VICs in SVD that support 420 along with other color formats
}EDID_YCbCr420_CAP_MAP_INFO, *PEDID_YCbCr420_CAP_MAP_INFO;

// Edid Data structures
typedef struct tagEDID_BASIC_BLK_DATA
{
    // Edid Product, Manufacturer Code
    EDID_GEN_INFO           GenInfo;

    // Basic Display Parameters
    EDID_DISP_PARAMS        DispParams;

    // Monitor Information
    EDID_MONITOR_INFO       MonitorInfo;

    // Supported Video Resolution Information
    unsigned char           NumSpprtedVidFmts;
    CEA_RES_INFO            VideoResInfo[64];
    unsigned char           PrefTimingEntry;

    // Number of extension blocks
    unsigned char           NumExtns;
}EDID_BASIC_BLK_DATA, *PEDID_BASIC_BLK_DATA;


//Hdmi Forum VSDB Structures
typedef struct tagEDID_CEA_HFVSDB_FEATURE_SUPPORT {

     UINT8 Hdmi_3D_OSD_Disparity :1;
     UINT8 Dual_View             :1;
     UINT8 Independent_view      :1;
     UINT8 LTE_340Mcsc_scramble  :1;
     UINT8 Reserved              :2;
     UINT8 RR_Capable            :1;
     UINT8 SCDC_Present          :1;
}EDID_CEA_HFVSDB_FEATURE_SUPPORT,*PEDID_CEA_HFVSDB_FEATURE_SUPPORT;


typedef struct tagEDID_CEA_HFVSDB_DEEPCOL_SUPPORT{
    UINT8 DC_30bit_420          :1;
    UINT8 DC_36bit_420          :1;
    UINT8 DC_48bit_420          :1;
    UINT8 Reserved              :5;

}EDID_CEA_HFVSDB_DEEPCOL_SUPPORT , *PEDID_CEA_HFVSDB_DEEPCOL_SUPPORT;


typedef struct tagEDID_CEA_HFVSDB {

     UINT8 VendorSpecificTagCode;
     UINT8 Length;
     UINT8 IEEE_OUI_ThirdOctet;
     UINT8 IEEE_OUI_SecondOctet;
     UINT8 IEEE_OUI_FirstOctet;
     UINT8 Version;
     UINT8 MAX_TMDS_Character_Rate;
     EDID_CEA_HFVSDB_FEATURE_SUPPORT HfvsdbFeatures;
     EDID_CEA_HFVSDB_DEEPCOL_SUPPORT HfVsdbDeepCol;

}EDID_CEA_HFVSDB , *PEDID_CEA_HFVSDB;

//HDR Statci Metadata block
typedef struct tagEDID_HDR_STATIC_METADATA
{
    UINT8  EOTF_ET_0    : 1;
    UINT8  EOTF_ET_1    : 1;
    UINT8  EOTF_ET_2    : 1;
    UINT8  EOTF_ET_3    : 1;

    /*ET4 to ET5 are reserved for future use*/
    UINT8  EOTF_ET_4    : 1;
    UINT8  EOTF_ET_5    : 1;
    UINT8  rsvd1        : 1;
    UINT8  rsvd2        : 1;

    UINT8  EOTF_SM_0    : 1;
    /*SM1 to SM7 are reserved for future use*/
    UINT8  EOTF_SM_1    : 1;
    UINT8  EOTF_SM_2    : 1;
    UINT8  EOTF_SM_3    : 1;
    UINT8  EOTF_SM_4    : 1;
    UINT8  EOTF_SM_5    : 1;
    UINT8  EOTF_SM_6    : 1;
    UINT8  EOTF_SM_7    : 1;

    UINT32 DesCntMaxLuminanceData;
    UINT32  DesCntMaxFrmAvgLuminanceData;
    UINT32  DesCntMinLuminanceData;

}HDR_METADATA_INFO, *PHDR_METADATA_INFO;

typedef struct tagEDID_DOLBYVISION_CAP
{
    UINT16 isDolbyVisionSupported  :1;
    UINT16 is4Kp60Supported        :1;
    UINT16 is422_12bitSupported    :1;
    UINT16 dv_version              :2;
    UINT16 latency                 :2;
    UINT16 parity                  :1;
    UINT16 dm_version              :3;
    UINT16 is444_10_12bitSupported :2;
    UINT16 Reserved                :3;
    UINT32 MaxLum;
    UINT32 MinLum;
}EDID_DOLBYVISION_CAP,*PEDID_DOLBYVISION_CAP;

typedef struct tagEDID_HDR_CAPABILITY
{
    UINT8 isHDR10Supported      :1;
    UINT8 isDVSupported         :1;
    UINT8 isHLGSupported        :1;
    UINT8 isHDR10PlusSupported  :1;
    UINT8 Reserved              :4;
}EDID_HDR_CAPABILITY,*PEDID_HDR_CAPABILITY;

typedef struct tagEDID_DOLBY_AUDIO_CAP
{
    UINT8 vsadbPresent;
    UINT8 dolbyVsadbVersion     :3;
    UINT8 f63                   :1;
    UINT8 outputConfig          :3;
    UINT8 isHeadphonePB         :1;
    UINT8 sinkDevCap;
}EDID_DOLBY_AUDIO_CAP,*PEDID_DOLBY_AUDIO_CAP;

typedef struct tagEDID_CEA_TIMING_EXTN_BLK
{
    BOOL                    VndrBlkFound;
    BOOL                    HdmiMode;

    // CEA Block specified feature support
    EDID_CEA_FEATURE_SPRT   FeatureSprt;
    EDID_HDMI_3D_SPRT       Hdmi3DFeatureSprt;
    EDID_LATENCY_FIELDS     HdmiLatencyFields;
    unsigned char           FirstSVDEntry;
    unsigned char           LastSVDEntry;

    // Supported Video Resolutions
    unsigned char           NumSpprtedVidFmts;
    CEA_RES_INFO            VideoResInfo[64];
    unsigned char           PrefTimingEntry;

    // Audio Information
    unsigned char           BasicAudioSprt;
    unsigned char           NumSprtedAudFmts;
    EDID_AUDIO_INFO         AudioInfo[15]; //check max number

    // Speaker Allocation Table
    EDID_SPKR_ALLOC_FLD     SpkrAlloc;

    // Calorimtery information
    EDID_CALORIMETRY_INFO   CalorimetryInfo;
    EDID_CAPABILITY_INFO   CapabilityInfo;
    EDID_YCbCr420_CAP_INFO YCbCr420CapInfo;

    unsigned short          CecPhyAddr;
    unsigned short          MaxTMDSClock;
    EDID_CEA_HFVSDB         CeaHdmiVSDB;
    EDID_YCbCr420_CAP_MAP_INFO YCbCrCapMapInfo;
    HDR_METADATA_INFO       HDRMetadataInfo;
    EDID_DOLBYVISION_CAP  DolbyVisionVSVDB;
    EDID_HDR_CAPABILITY     HdrCapInfo;
    EDID_DOLBY_AUDIO_CAP    DolbyAudioVSADB;
    UINT8                   ExtBlkCnt;
    UINT8                   HImgSize;
    UINT8                   VImgSize;
}EDID_CEA_TIMING_EXTN_BLK,  *PEDID_CEA_TIMING_EXTN_BLK;

typedef struct tagEDID_BLOCK_MAP_DATA
{
   // Defines the block number where CEA extension is present
    unsigned char           CurBlkNum;
    unsigned char           CeaExtnBlkNum;

   // If specific block is not present, then, it would be set at 0
}EDID_BLOCK_MAP_DATA, *PEDID_BLOCK_MAP_DATA;

typedef union tagEDID_DATA
{
   EDID_BASIC_BLK_DATA          BasicBlk;
   EDID_CEA_TIMING_EXTN_BLK     CEAExtnBlk;
   EDID_BLOCK_MAP_DATA          BlkMap;
}EDID_DATA, *PEDID_DATA;

typedef union tagAUD_FREQ
{
    EDID_AUD_FREQ FreqBitFld;
    unsigned char FreqValue;
}AUD_FREQ;

typedef union tagEDID_SPKR_ALLOC
{
    EDID_SPKR_ALLOC_FLD  Fld;
    unsigned char        Value;
}EDID_SPKR_ALLOC;

/*-----------------------------------------------------------------------------
 * Function Prototypes
 *-----------------------------------------------------------------------------
 */
EDID_RETURNS ParseEdid (
    unsigned char *pDataBuf,
    PEDID_BLK_TYPE pEdidBlkType,
    PEDID_DATA     pEdidBlkData,
    BOOL isFirstBlock);
EDID_RETURNS EdidGetBlkType (
    unsigned char *pDataBuf,
    PEDID_BLK_TYPE pEdidBlkType);
#endif // __HDMI_EDID_H__
