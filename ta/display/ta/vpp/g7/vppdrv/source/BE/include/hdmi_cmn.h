/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef __HDMI_CMN_H__
#define __HDMI_CMN_H__

#ifndef CONFIG_AMP_TEE_ENABLE
#include "OSAL_api.h"
#else
#include "OSAL_api.h"
#include "tee_ta_osal.h"
#include "ErrorCode.h"
#endif

#include "vpp_api_types.h"

/*-----------------------------------------------------------------------------
 * Macros and Constants
 *-----------------------------------------------------------------------------
 */
// Macro to find if the given bit is set
#define IS_BIT_SET(Num,Pos) (((Num) & (1 << Pos)) == (1 << Pos))

#define HDMI_USE_EXISTING   (-1)

/*-----------------------------------------------------------------------------
 * Info frames
 *-----------------------------------------------------------------------------
 */
// CEA-861D (as per HDMI 1.3 spec)
// Infoframe types
typedef enum tagCEA_INFO_FRM_TYPE
{
    CEA_INFO_FRM_RSVD = 0x00,   // Reserved
    CEA_INFO_FRM_VENDOR_SPEC,   // Vendor specific info frame
    CEA_INFO_FRM_AVI,           // Auxiliary video information
    CEA_INFO_FRM_SPD,           // Source product descriptor
    CEA_INFO_FRM_AUDIO,         // Audio info frame
    CEA_INFO_FRM_MPEG_SRC,      // MPEG source info frame
    CEA_INFO_FRM_NTSC_VBI,
    CEA_INFO_FRM_HDR,          // HDR info frame
    CEA_INFO_FRM_MAX,
}CEA_INFO_FRM_TYPE;

// Info frame versions
#define CEA_VENDOR_SPEC_FRM_VERS    0x01
#define CEA_AVI_FRM_VERS            0x02
#define CEA_SPD_FRM_VERS            0x01
#define CEA_AUD_FRM_VERS            0x01
#define CEA_MPEG_SRC_FRM_VERS       0x01
#define CEA_HDR_FRM_VERS            0x01

// Info frame length
#define FRM_HDR_LEN                 0x04
#define CEA_AVI_FRM_LEN             0x0D
#define CEA_SPD_FRM_LEN             0x19
#define CEA_AUD_FRM_LEN             0x0A
#define CEA_MPEG_SRC_FRM_LEN        0x0A
#define CEA_HDR_FRM_LEN             0x1A

/*-----------------------------------------------------------------------------
 * AVI Info frame
 *-----------------------------------------------------------------------------
 */

// Color Space Formats
typedef enum tagCEA_COL_SPACE_FMT
{
    CEA_COLSP_RGB = 0x00,
    CEA_COLSP_YCBCR_422,
    CEA_COLSP_YCBCR_444,
    CEA_COLSP_YCBCR_420,
    CEA_COLSP_MAX
}CEA_COL_SPACE_FMT;

// Bar Information
typedef enum tagCEA_BAR_INFO
{
    CEA_BAR_INFO_INVALID = 0x00,
    CEA_VERT_BAR_INFO_VALID,
    CEA_HORIZ_BAR_INFO_VALID,
    CEA_VERT_HORIZ_BAR_INFO_VALID,
    CEA_BAR_INFO_MAX
}CEA_BAR_INFO;

// Scan Information
typedef enum tagCEA_SCAN_INFO
{
    CEA_SCAN_DATA_NONE = 0x00,
    CEA_OVER_SCAN_DATA,
    CEA_UNDER_SCAN_DATA,
    CEA_SCAN_DATA_MAX,
}CEA_SCAN_INFO;

// Calorimetry information
typedef enum tagCEA_CALORIMETRY_INFO
{
    CEA_CAL_DATA_NONE = 0x00,
    CEA_CAL_ITU601_FMT,
    CEA_CAL_ITU709_FMT,
    // Extended calorimetry information valid
    CEA_CAL_EXT_FMT
}CEA_CALORIMETRY_INFO;

// Picture Aspect Ratio
typedef enum tagCEA_PICT_ASP_RATIO
{
    CEA_PICT_AR_NONE = 0x00,
    CEA_PICT_AR_21_9 = CEA_PICT_AR_NONE,
    CEA_PICT_AR_16_10 = CEA_PICT_AR_NONE,
    CEA_PICT_AR_4_3,
    CEA_PICT_AR_16_9,
    CEA_PICT_AR_MAX
}CEA_PICT_ASP_RATIO;

// Active format aspect ratio
typedef enum tagCEA_ACT_FMT_ASP_RATIO
{
    CEA_ACT_FMT_AR_SAME_AS_PICT = 0x08,
    CEA_ACT_FMT_AR_4_3_CENTER,
    CEA_ACT_FMT_AR_16_9_CENTER,
    CEA_ACT_FMT_AR_14_9_CENTER
}CEA_ACT_FMT_ASP_RATIO;

// Extended Calorimetry information
typedef enum tagCEA_EXT_CAL_INFO
{
    CEA_EXT_CAL_XVYCC_601_FMT = 0x00,
    CEA_EXT_CAL_XVYCC_709_FMT,
    CEA_EXT_CAL_sYCC_601_FMT,
    CEA_EXT_CAL_AdobeYCC_601_FMT,
    CEA_EXT_CAL_AdobeRGB_FMT,
    CEA_EXT_CAL_BT2020cYCC_FMT, //const luminance
    CEA_EXT_CAL_BT2020YCC_FMT, //YCbCr or RGB
    CEA_EXT_CAL_MAX
}CEA_EXT_CAL_INFO;

// RGB Quantization Range
typedef enum tagCEA_RGB_QUANT_RANGE
{
    CEA_RGB_QUANT_DEF_RANGE = 0x00, // Depends on video format
    CEA_RGB_QUANT_LTD_RANGE,
    CEA_RGB_QUANT_FULL_RANGE,
    CEA_RGB_QUANT_RANGE_MAX
}CEA_RGB_QUANT_RANGE;

// YCC Quantization Range
typedef enum tagCEA_YCC_QUANT_RANGE
{
    CEA_YCC_QUANT_LTD_RANGE= 0x00,
    CEA_YCC_QUANT_FULL_RANGE,
    CEA_YCC_QUANT_RANGE_MAX
}CEA_YCC_QUANT_RANGE;

// Picture Scaling Information
typedef enum tagCEA_SCALING_INFO
{
    CEA_SCL_INFO_NON_UNIF_NONE = 0x00,
    CEA_SCL_INFO_HORIZ,
    CEA_SCL_INFO_VERT,
    CEA_SCL_INFO_HORIZ_VERT,
    CEA_SCL_INFO_MAX
}CEA_SCALING_INFO;

// Pixel Repetition Factor
typedef enum tagCEA_PIX_RPT_FACT
{
    CEA_PIX_RPT_FACT_NONE = 0x00,
    CEA_PIX_RPT_FACT_2,
    CEA_PIX_RPT_FACT_3,
    CEA_PIX_RPT_FACT_4,
    CEA_PIX_RPT_FACT_5,
    CEA_PIX_RPT_FACT_6,
    CEA_PIX_RPT_FACT_7,
    CEA_PIX_RPT_FACT_8,
    CEA_PIX_RPT_FACT_9,
    CEA_PIX_RPT_FACT_10,
    CEA_PIX_RPT_FACT_MAX
}CEA_PIX_RPT_FACT;

/*-----------------------------------------------------------------------------
 * Audio Info frame
 *-----------------------------------------------------------------------------
 */
// Audio coding types
typedef enum tagCEA_AUDIO_FMT
{
    CEA_AUDIO_FMT_IN_STRM_HDR = 0x00,
    CEA_AUDIO_FMT_PCM,
    CEA_AUDIO_FMT_AC3,
    CEA_AUDIO_FMT_MPEG1, // MPEG 1 Layer 1 & 2
    CEA_AUDIO_FMT_MP3,   // MPEG 1 Layer 3
    CEA_AUDIO_FMT_MPEG2,
    CEA_AUDIO_FMT_AAC,
    CEA_AUDIO_FMT_DTS,
    CEA_AUDIO_FMT_ATRAC,
    CEA_AUDIO_FMT_1BIT_AUD,
    CEA_AUDIO_FMT_DOLBY_DIG,
    CEA_AUDIO_FMT_DTS_HD,
    CEA_AUDIO_FMT_MAT,
    CEA_AUDIO_FMT_DST,
    CEA_AUDIO_FMT_WMA_PRO,
    CEA_AUDIO_FMT_MAX
}CEA_AUDIO_FMT;

// Number of audio channels
typedef enum tagCEA_AUDIO_CHN_CNT
{
    CEA_AUDIO_CHNLS_IN_STRM_HDR = 0x00,
    CEA_AUDIO_CHNLS_2 = 0x01,
    CEA_AUDIO_CHNLS_3,
    CEA_AUDIO_CHNLS_4,
    CEA_AUDIO_CHNLS_5,
    CEA_AUDIO_CHNLS_6,
    CEA_AUDIO_CHNLS_7,
    CEA_AUDIO_CHNLS_8,
    CEA_AUDIO_CHNLS_MAX
}CEA_AUDIO_CHN_CNT;

// Audio sampling frequency
typedef enum tagCEA_AUDIO_SAMP_FREQ
{
    CEA_AUDIO_SAMP_FREQ_IN_STRM_HDR = 0x00,
    CEA_AUDIO_SAMP_FREQ_32_KHZ,
    CEA_AUDIO_SAMP_FREQ_44_1_KHZ,
    CEA_AUDIO_SAMP_FREQ_48_KHZ,
    CEA_AUDIO_SAMP_FREQ_88_2_KHZ,
    CEA_AUDIO_SAMP_FREQ_96_KHZ,
    CEA_AUDIO_SAMP_FREQ_176_4_KHZ,
    CEA_AUDIO_SAMP_FREQ_192_KHZ,
    CEA_AUDIO_SAMP_FREQ_MAX
}CEA_AUDIO_SAMP_FREQ;

typedef enum tagCEA_AUDIO_SAMP_SIZE
{
    CEA_AUDIO_SAMP_SIZE_IN_STRM_HDR = 0x00,
    CEA_AUDIO_SAMP_SIZE_16_BITS,
    CEA_AUDIO_SAMP_SIZE_20_BITS,
    CEA_AUDIO_SAMP_SIZE_24_BITS,
    CEA_AUDIO_SAMP_SIZE_MAX
}CEA_AUDIO_SAMP_SIZE;

typedef enum tagCEA_RES_FRM_FMT
{
   RES_INFO_PROGRESSIVE = 0,
   RES_INFO_INTERLACED,
   RES_INFO_FRM_FMT_UNDEF
}CEA_RES_FRM_FMT;

typedef struct tagCEA_RES_INFO
{
    UINT16              VideoIDCode;
    UINT16              HActive;
    UINT16              VActive;
    UINT32              RefreshRate;
    CEA_RES_FRM_FMT     FrmFmt;
    CEA_PICT_ASP_RATIO  AspRatio;
}CEA_RES_INFO, *PCEA_RES_INFO;

typedef struct tagAudioInfoFrame {
    CEA_AUDIO_FMT           CodingType;
    CEA_AUDIO_CHN_CNT       ChannelCnt;
    CEA_AUDIO_SAMP_FREQ     SamplingFreq;
    CEA_AUDIO_SAMP_SIZE     SampleSize;
    // Applicable only for multi-channel uncompressed audio
    UINT8                   SpkrAlloc;          // Speaker locations w.r.t channels
    UINT8                   LvlShiftVal;        // Defines the attenuation associated with
                                                // level shift value
    BOOL                    DownMixInhFlag;     // Down-Mix inhibit flag
}AUDIO_INFO_FRAME, *PAUDIO_INFO_FRAME;

/*-----------------------------------------------------------------------------
 * FIFO Data Type
 *-----------------------------------------------------------------------------
 */
typedef struct tagHDMI_FIFO
{
    UINT8                   *pElemAddr;
    int                     numElem;
    int                     elemSize;
    int                     head;
    int                     tail;
    int                     fifoSignal;
    MV_OSAL_HANDLE_SEM_t    fifoSem;
}HDMI_FIFO, *PHDMI_FIFO;

// Data access types
typedef enum tagREG_DATA_ACCESS
{
    REG_DATA_ACCESS_VBI_BCMBUF = 0,
    REG_DATA_ACCESS_VDE_BCMBUF,
    REG_DATA_ACCESS_DIRECT
}REG_DATA_ACCESS;

/* HDMI CTS Ver 1.4b:
   Test ID 7-23: To output RGB 640x480 video format signal, RGB quantization range be either default or full range.
   To output RGB except 640x480 video format signal, RGB quantization range be either default or limited range.
   Test ID 7-24: To output YCbCr, RGB quantization range be either default or limited range. */
#define RGB_RANGE_RESCHECK(pResinfo) ((pResinfo->active_width == 640) && (pResinfo->active_height == 480))

#define VPP_HDMI_GET_RGB_RANGE(pHdmiTx, pResinfo)  (pHdmiTx->edidInfo.capInfo.QS ? \
    (((pHdmiTx->aviParams.ColorSpace == CEA_COLSP_RGB) && RGB_RANGE_RESCHECK(pResinfo)) \
    ? CEA_RGB_QUANT_FULL_RANGE : CEA_RGB_QUANT_LTD_RANGE) : CEA_RGB_QUANT_DEF_RANGE)

/*-----------------------------------------------------------------------------
 * Function Prototypes
 *-----------------------------------------------------------------------------
 */
/*-----------------------------------------------------------------------------
 * Description  : This function returns if the given video id code is
 *              : defined in the table
 * Arguments    : VideoIDCode - Video ID code
 * Returns      : TRUE, if it is defined
 *              : FALSE, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
BOOL IsVideoIDCodeDefined (UINT16 VideoIDCode);

/*-----------------------------------------------------------------------------
 * Description  : This function returns resolution information from the
 *              : given Video ID code
 * Arguments    : VideoIDCode - Video ID code
 *              : pResInfo - Pointer to resolution information (Output)
 * Returns      : TRUE, on success
 *              : FALSE, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
BOOL GetResForVideoID (CEA_RES_INFO *pResInfo);

/*-----------------------------------------------------------------------------
 * Description  : This function returns Video ID code for the given resolution
 *              : information
 * Arguments    : pResInfo - Pointer to resolution information (Input)
 *              : VideoIDCode - Pointer to return Video ID code (Output)
 * Returns      : TRUE, on success
 *              : FALSE, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
BOOL GetVideoIDForRes (CEA_RES_INFO *pResInfo);

/*-----------------------------------------------------------------------------
 * Description  : This function performs integer division rounding up
 * Arguments    : Num  - Numerator
 *              : Den  - Denominator
 *              : *Res - Pointer to return result
 * Returns      : TRUE, on success
 *              : FALSE, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
BOOL IntDivision (unsigned int Num, unsigned int Den, unsigned int *Res);

/*-----------------------------------------------------------------------------
 * Description  : This function creates a FIFO for given number of elements
 *              : each with a given size.
 * Arguments    : numElem  - Number of elements in FIFO
 *              : elemSize - Size of each element
 *              : pFifoPtr - Pointer to FIFO
 * Returns      : TRUE, if FIFO is created
 *              : FALSE, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
BOOL HdmiFifoCreate (int numElem, int elemSize, PHDMI_FIFO pFifoPtr);

/*-----------------------------------------------------------------------------
 * Description  : This function deletes the FIFO specified.
 * Arguments    : pFifoPtr - Pointer to FIFO
 * Returns      : TRUE
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
BOOL HdmiFifoDelete (PHDMI_FIFO pFifoPtr);

/*-----------------------------------------------------------------------------
 * Description  : This function resets the given FIFO, all elements
 *              : are removed.
 * Arguments    : pFifoPtr - Pointer to FIFO
 * Returns      : TRUE, if FIFO is reset
 *              : FALSE, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
BOOL HdmiFifoReset  (PHDMI_FIFO pFifoPtr);

/*-----------------------------------------------------------------------------
 * Description  : This function adds the given element to FIFO
 * Arguments    : pFifoPtr - Pointer to FIFO
 *              : pElem    - Pointer to element to be added (Input)
 *              : enSignal - Indicates if the associated semaphore needs to
 *              :          - be posted if the element is added
 * Returns      : TRUE, if the element can be added
 *              : FALSE, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
BOOL HdmiFifoPush   (PHDMI_FIFO pFifoPtr, UINT8* pElem, BOOL enSignal);

/*-----------------------------------------------------------------------------
 * Description  : This function removes the first element from the FIFO and
 *              : returns it
 * Arguments    : pFifoPtr - Pointer to FIFO
 *              : pElem    - Pointer to element to be removed (Output)
 * Returns      : TRUE, if the element is removed
 *              : FALSE, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
BOOL HdmiFifoPop    (PHDMI_FIFO pFifoPtr, UINT8 *pElem);

/*-----------------------------------------------------------------------------
 * Description  : This function returns the number of elements in FIFO
 * Arguments    : pFifoPtr - Pointer to FIFO
 * Returns      : Number of elements; -1, in case of error
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
int HdmiFifoGetNumElems (PHDMI_FIFO pFifoPtr);

/*-----------------------------------------------------------------------------
 * Description  : This function posts the semaphore associated with the FIFO
 *              : if there are elements in the FIFO
 * Arguments    : pFifoPtr - Pointer to FIFO
 * Returns      : TRUE, if the semaphore is posted
 *              : FALSE, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
BOOL HdmiFifoSignal (PHDMI_FIFO pFifoPtr);

/*-----------------------------------------------------------------------------
 * Description  : This function pends on the semaphore associated with the FIFO
 * Arguments    : pFifoPtr - Pointer to FIFO
 * Returns      : TRUE, when the semaphore is released
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
BOOL HdmiFifoWaitForSignal (PHDMI_FIFO pFifoPtr);

BOOL GetVideoIDForResExt (CEA_RES_INFO *pResInfo);

#endif // __HDMI_CMN_H__
