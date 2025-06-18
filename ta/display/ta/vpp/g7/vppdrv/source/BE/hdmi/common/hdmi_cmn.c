/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "com_type.h"
#include "hdmi_cmn.h"
#include "vpp_osal.h"
#include "vpp_api.h"
#include "vpp_debug.h"
#ifdef CONFIG_AMP_TEE_ENABLE
#endif



#define MAX_VIC_CODE 255    //Maximum possible VIC Code
static const CEA_RES_INFO VideoCodeMap[] =
{
    {0 , 0,    0,   0,  RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},//0  - Undefined
    {1 , 640,  480, 60, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_4_3 },//1  - 640*480p   @59.94/60Hz 4:3
    {2 , 720,  480, 60, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_4_3 },//2  - 720*480p   @59.94/60Hz 4:3
    {3 , 720,  480, 60, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//3  - 720*480p   @59.94/60Hz 16:9
    {4 , 1280, 720, 60, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//4  - 1280*720p  @59.94/60Hz 16:9
    {5 , 1920, 1080,60, RES_INFO_INTERLACED   , CEA_PICT_AR_16_9},//5  - 1920*1080i @59.94/60Hz 16:9
    {6 , 1440, 480, 60, RES_INFO_INTERLACED   , CEA_PICT_AR_4_3 },//6  - 1440*480i  @59.94/60Hz 4:3
    {7 , 1440, 480, 60, RES_INFO_INTERLACED   , CEA_PICT_AR_16_9},//7  - 1440*480i  @59.94/60Hz 16:9
    {8 , 1440, 240, 60, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_4_3 },//8  - 1440*240p  @59.94/60Hz 4:3
    {9 , 1440, 240, 60, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//9  - 1440*240p  @59.94/60Hz 16:9
    {10, 2880, 480, 60, RES_INFO_INTERLACED   , CEA_PICT_AR_4_3 },//10 - 2880*480i  @59.94/60Hz 4:3
    {11, 2880, 480, 60, RES_INFO_INTERLACED   , CEA_PICT_AR_16_9},//11 - 2880*480i  @59.94/60Hz 16:9
    {12, 2880, 240, 60, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_4_3 },//12 - 2880*240p  @59.94/60Hz 4:3
    {13, 2880, 240, 60, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//13 - 2880*240p  @59.94/60Hz 16:9
    {14, 1440, 480, 60, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_4_3 },//14 - 1440*480p  @59.94/60Hz 4:3
    {15, 1440, 480, 60, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//15 - 1440*480p  @59.94/60Hz 16:9
    {16, 1920, 1080,60, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//16 - 1920*1080p @59.94/60Hz 16:9
    {17, 720,  576, 50, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_4_3 },//17 - 720*576p   @50Hz       4:3
    {18, 720,  576, 50, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//18 - 720*576p   @50Hz       16:9
    {19, 1280, 720, 50, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//19 - 1280*720p  @50Hz       16:9
    {20, 1920, 1080,50, RES_INFO_INTERLACED   , CEA_PICT_AR_16_9},//20 - 1920*1080i @50Hz       16:9
    {21, 1440, 576, 50, RES_INFO_INTERLACED   , CEA_PICT_AR_4_3 },//21 - 1440*576i  @50Hz       4:3
    {22, 1440, 576, 50, RES_INFO_INTERLACED   , CEA_PICT_AR_16_9},//22 - 1440*576i  @50Hz       16:9
    {23, 1440, 288, 50, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_4_3 },//23 - 1440*288p  @50Hz       4:3
    {24, 1440, 288, 50, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//24 - 1440*288p  @50Hz       16:9
    {25, 2880, 576, 50, RES_INFO_INTERLACED   , CEA_PICT_AR_4_3 },//25 - 2880*576i  @50Hz       4:3
    {26, 2880, 576, 50, RES_INFO_INTERLACED   , CEA_PICT_AR_16_9},//26 - 2880*576i  @50Hz       16:9
    {27, 2880, 288, 50, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_4_3 },//27 - 2880*288p  @50Hz       4:3
    {28, 2880, 288, 50, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//28 - 2880*288p  @50Hz       16:9
    {29, 1440, 576, 50, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_4_3 },//29 - 1440*576p  @50Hz       4:3
    {30, 1440, 576, 50, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//30 - 1440*576p  @50Hz       16:9
    {31, 1920,1080, 50, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//31 - 1920*1080p @50Hz       16:9
    {32, 1920,1080, 24, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//32 - 1920*1080p @23.97/24Hz 16:9
    {33, 1920,1080, 25, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//33 - 1920*1080p @25Hz       16:9
    {34, 1920,1080, 30, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//34 - 1920*1080p @29.97/30Hz 16:9
    {35, 2880, 480, 60, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_4_3 },//35 - 2880*480p  @59.94/60Hz 4:3
    {36, 2880, 480, 60, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//36 - 2880*480p  @59.94/60Hz 16:9
    {37, 2880, 576, 50, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_4_3 },//37 - 2880*576p  @59.94/60Hz 4:3
    {38, 2880, 576, 50, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//38 - 2880*576p  @59.94/60Hz 16:9
    {47, 1280,720, 120, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//47 - 1280*720p  @120 Hz     16:9
    {63, 1920,1080,120, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//63 - 1920*1080p @120 Hz     16:9
    {93, 3840,2160, 24, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//93 - 4k @23.98/24Hz 16:9
    {94, 3840,2160, 25, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//94 - 4k @25Hz 16:9
    {95, 3840,2160, 30, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//95 - 4k @29.97/30Hz 16:9
    {96, 3840,2160, 50, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//96 - 4k @50Hz 16:9
    {97, 3840,2160, 60, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//97 - 4k @59.94/60Hz 16:9
    {98, 4096,2160, 24, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//98 - 4k smpte  @23.98/24Hz 16:9
    {99, 4096,2160, 25, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//99 - 4k smpte @25Hz 16:9
    {100, 4096,2160, 30,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//100 - 4k smpte @29.97/30Hz 16:9
    {101, 4096,2160, 50,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//101 - 4k smpte @50Hz 16:9
    {102, 4096,2160, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//102 - 4k smpte @59.94/60Hz 16:9
    {220, 3840,1600, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_21_9},//220 - 3840x1600 @60Hz 21:9
    {221, 1920,1600, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_21_9},//221 - 1920x1600 @60HZ 21:9
    {222, 3440,1440, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_21_9},//222 - 3440x1440 @60Hz 21:9
    {223, 1720,1440, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_21_9},//223 - 3440x1440 @60Hz 21:9
    {224, 1920,2160, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_21_9},//224 - 1920x2160 @60Hz 21:9
    {225, 1920,1200, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_10},//225- 1920x1200 @60Hz 16:10
    {226, 2880,1440, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_21_9},//226 - 2880x1440 @60Hz 21:9
    {227,  800, 600, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_4_3}, //227 - 800x600P60   @60Hz 4:3
    {228, 1280, 768, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//228 - 1280x768P60  @60Hz 16:9
    {229, 1360, 768, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//229 - 1360x768P60  @60Hz 16:9
    {230, 1024, 768, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_4_3}, //230 - 1024x768P60  @60Hz 4:3
    {231, 1280, 800, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//231 - 1280x800P60  @60Hz 16:9
    {232, 1280,1024, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//232 - 1280x1024P60 @60Hz 16:9
    {233, 1440, 900, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//233 - 1440X900P60  @60Hz 16:9
    {234, 1400,1050, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//234 - 1400x1050P60 @60Hz 16:9
    {235, 1680,1050, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//235 - 1680x1050P60 @60Hz 16:9
    {236, 1600,1200, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//236 - 1600x1200P60 @60Hz 16:9
    {237, 1366, 768, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//237 - 1366X768P60  @60Hz 16:9
    {238, 2560,1440, 60,RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//238 - 2560X1440P60 @60Hz 16:9

    {255, 0  , 0  , 0 , RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},//255 - max VIC Code in the current CEA spec
};

static const CEA_RES_INFO VideoCodeMap_Ext[] = {
    {0 , 0,    0,   0,  RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},//0  - Undefined
    {93, 3840,2160, 24, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//93 - 4k @24Hz 16:9
    {94, 3840,2160, 25, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//94 - 4k @25Hz 16:9
    {95, 3840,2160, 30, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//95 - 4k @30Hz 16:9
    {98, 4096,2160, 24, RES_INFO_PROGRESSIVE  , CEA_PICT_AR_16_9},//98 - 4k smpte  @23.98/24Hz 16:9
    {255, 0  , 0  , 0 , RES_INFO_FRM_FMT_UNDEF , CEA_PICT_AR_NONE},//255 - max VIC Code in the current CEA spec
};
// FIFO wrap around condition
#define FIFO_WRAP_AROUND(curElem,maxElem)    (((curElem)>=(maxElem))?0:(curElem))

/*-----------------------------------------------------------------------------
 * Description  : This function returns if the given video id code is
 *              : defined in the table
 * Arguments    : VideoIDCode - Video ID code
 * Returns      : TRUE, if it is defined
 *              : FALSE, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
BOOL IsVideoIDCodeDefined (UINT16 VideoIDCode)
{
    UINT16 maxIndex = 0;
    maxIndex = sizeof(VideoCodeMap)/sizeof(CEA_RES_INFO)-1;

    // Check given VideoID
    if (VideoIDCode == 0 || VideoIDCode >= VideoCodeMap[maxIndex].VideoIDCode)
    {
        return FALSE;
    }

    return TRUE;
}

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
BOOL GetResForVideoID (CEA_RES_INFO *pResInfo)
{
    UINT16 maxIndex = 0;
    UINT16 VideoIDCode = 0;
    UINT32 VICindex = 0;
    BOOL   vicfound = 0;
    UINT32 i = 0;

    if (pResInfo == NULL)
    {
        return FALSE;
    }

    maxIndex = sizeof(VideoCodeMap)/sizeof(CEA_RES_INFO)-1;
    VideoIDCode    = pResInfo->VideoIDCode;

    // Check given VideoID
    if ((VideoIDCode == 0)  ||
        (VideoIDCode >= VideoCodeMap[maxIndex].VideoIDCode))
    {
        return FALSE;
    }

    for( i = 0; i < maxIndex; i++)
    {
        if(VideoIDCode == VideoCodeMap[i].VideoIDCode) {
            VICindex = i;
            vicfound = TRUE;
            break;
        }
    }

    /*VICs 93 to 95 can't be in the above table due to HDMI CTS limitation
      So use this as a work round. If changing this part check with HDMI team */
    if(VideoIDCode>=93 && VideoIDCode<=95)
    {
        pResInfo->HActive       = 3840;
        pResInfo->VActive       = 2160;
        pResInfo->FrmFmt        = RES_INFO_PROGRESSIVE;
        pResInfo->AspRatio      = CEA_PICT_AR_16_9;
        switch(VideoIDCode)
        {
            case 93:   pResInfo->RefreshRate = 24;
                                  break;
            case 94:   pResInfo->RefreshRate = 25;
                                  break;
            case 95:   pResInfo->RefreshRate = 30;
                                  break;
        }
        return TRUE;
    }

    if(!vicfound || (i == maxIndex)) {
        return FALSE;
    }

    pResInfo->HActive       = VideoCodeMap[VICindex].HActive;
    pResInfo->VActive       = VideoCodeMap[VICindex].VActive;
    pResInfo->RefreshRate   = VideoCodeMap[VICindex].RefreshRate;
    pResInfo->FrmFmt        = VideoCodeMap[VICindex].FrmFmt;
    pResInfo->AspRatio      = VideoCodeMap[VICindex].AspRatio;
    return TRUE;
}

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
BOOL GetVideoIDForRes (CEA_RES_INFO *pResInfo)
{
    UINT16   maxIndex = 0;
    UINT16   Count;

    maxIndex = sizeof(VideoCodeMap)/sizeof(CEA_RES_INFO);

    if (pResInfo == NULL)
    {
        return FALSE;
    }

    pResInfo->VideoIDCode = 0;
    for (Count = 0; Count < maxIndex; Count++)
    {
        if ((pResInfo->HActive      == VideoCodeMap[Count].HActive) &&
            (pResInfo->VActive      == VideoCodeMap[Count].VActive) &&
            (pResInfo->RefreshRate  == VideoCodeMap[Count].RefreshRate))
        {
            if (pResInfo->FrmFmt != RES_INFO_FRM_FMT_UNDEF)
            {
                if (pResInfo->FrmFmt != VideoCodeMap[Count].FrmFmt)
                    continue;
            }
            if (pResInfo->AspRatio != CEA_PICT_AR_NONE)
            {
                if (pResInfo->AspRatio != VideoCodeMap[Count].AspRatio)
                    continue;
            }
            break;
        }
    }

    if (Count == maxIndex)
    {
        // Reached the end of loop without finding a match
        VPPHDMI_DBG_ERROR("Reached end of loop without finding match Res[H=%d V=%d R=%d AR=%d] \n",\
                    pResInfo->HActive,pResInfo->VActive, pResInfo->RefreshRate,pResInfo->AspRatio);
        return FALSE;
    }

    pResInfo->VideoIDCode = VideoCodeMap[Count].VideoIDCode;
    return TRUE;
}

BOOL GetVideoIDForResExt (CEA_RES_INFO *pResInfo)
{
    UINT16   maxIndex = 0;
    UINT16   Count;

    maxIndex = sizeof(VideoCodeMap_Ext)/sizeof(CEA_RES_INFO);

    if (pResInfo == NULL)
    {
        return FALSE;
    }

    pResInfo->VideoIDCode = 0;
    for (Count = 0; Count < maxIndex; Count++)
    {
        if ((pResInfo->HActive      == VideoCodeMap_Ext[Count].HActive) &&
            (pResInfo->VActive      == VideoCodeMap_Ext[Count].VActive) &&
            (pResInfo->RefreshRate  == VideoCodeMap_Ext[Count].RefreshRate))
        {
            if (pResInfo->FrmFmt != RES_INFO_FRM_FMT_UNDEF)
            {
                if (pResInfo->FrmFmt != VideoCodeMap_Ext[Count].FrmFmt)
                    continue;
            }
            if (pResInfo->AspRatio != CEA_PICT_AR_NONE)
            {
                if (pResInfo->AspRatio != VideoCodeMap_Ext[Count].AspRatio)
                    continue;
            }
            break;
        }
    }

    if (Count == maxIndex)
    {
        // Reached the end of loop without finding a match
        return FALSE;
    }

    pResInfo->VideoIDCode = VideoCodeMap_Ext[Count].VideoIDCode;
    return TRUE;
}
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
BOOL IntDivision (unsigned int Num, unsigned int Den, unsigned int *Res)
{
    unsigned int Rem;

    if (Num == 0 || Den == 0)
    {
        return FALSE;
    }

    *Res = Num / Den;
    Rem = Num % Den;
    if ((Rem<<1) >= Den) (*Res) += 1;

    return TRUE;
}

/*-----------------------------------------------------------------------------
 * FIFO APIs
 *-----------------------------------------------------------------------------
 */
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
BOOL HdmiFifoCreate (int numElem, int elemSize, PHDMI_FIFO pFifoPtr)
{
    if (numElem == 0 || elemSize == 0)
        return (FALSE);

    if (pFifoPtr == NULL)
        return (FALSE);

    VPP_OSAL_SHM_Dynamic_Malloc(VPP_SHM_SECURE, ((numElem+1) * elemSize), 32, (VPP_SHM_HANDLE*)&(pFifoPtr->pElemAddr));

    if (pFifoPtr->pElemAddr == NULL)
    {
        HdmiFifoDelete (pFifoPtr);
        return (FALSE);
    }

    if (MV_OSAL_Sem_Create (&pFifoPtr->fifoSem, 0) != MV_VPP_OK)
    {
        HdmiFifoDelete (pFifoPtr);
        return (FALSE);
    }

    pFifoPtr->numElem = numElem+1;
    pFifoPtr->elemSize= elemSize;
    HdmiFifoReset (pFifoPtr);
    return (TRUE);
}

/*-----------------------------------------------------------------------------
 * Description  : This function deletes the FIFO specified.
 * Arguments    : pFifoPtr - Pointer to FIFO
 * Returns      : TRUE
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
BOOL HdmiFifoDelete (PHDMI_FIFO pFifoPtr)
{
    if (pFifoPtr != NULL)
    {
        if (pFifoPtr->pElemAddr != NULL)
            GaloisFree (pFifoPtr->pElemAddr);

        if (pFifoPtr->fifoSem != NULL)
            MV_OSAL_Sem_Destroy (&pFifoPtr->fifoSem);
    }
    return (TRUE);
}

/*-----------------------------------------------------------------------------
 * Description  : This function resets the given FIFO, all elements
 *              : are removed.
 * Arguments    : pFifoPtr - Pointer to FIFO
 * Returns      : TRUE, if FIFO is reset
 *              : FALSE, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
BOOL HdmiFifoReset (PHDMI_FIFO pFifoPtr)
{
    if (pFifoPtr == NULL)
        return (FALSE);

    pFifoPtr->head = 0;
    pFifoPtr->tail = 0;
    pFifoPtr->fifoSignal = 0;
    GaloisMemClear (pFifoPtr->pElemAddr, pFifoPtr->numElem * pFifoPtr->elemSize);
    return (TRUE);
}

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
BOOL HdmiFifoPush (PHDMI_FIFO pFifoPtr, UINT8* pElem, BOOL enSignal)
{
    int i;
    UINT8 *baseAddr;

    if (pFifoPtr == NULL || pElem == NULL)
        return (FALSE);

    if (FIFO_WRAP_AROUND(pFifoPtr->tail+1,pFifoPtr->numElem) == pFifoPtr->head)
        return (FALSE);

    baseAddr = &pFifoPtr->pElemAddr[pFifoPtr->tail * pFifoPtr->elemSize];
    for (i = 0; i < pFifoPtr->elemSize; i++) {
        baseAddr[i] = pElem[i];
    }

    pFifoPtr->tail = FIFO_WRAP_AROUND(pFifoPtr->tail+1,pFifoPtr->numElem);
    if (enSignal) {
        HdmiFifoSignal (pFifoPtr);
    }
    return (TRUE);
}

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
BOOL HdmiFifoPop (PHDMI_FIFO pFifoPtr, UINT8 *pElem)
{
    int     i;
    UINT8   *baseAddr;

    if (pFifoPtr == NULL || pElem == NULL)
        return (FALSE);

    // Update signal status
    pFifoPtr->fifoSignal = pFifoPtr->tail;
    if (pFifoPtr->head == pFifoPtr->tail) {
        // FIFO empty
        return (FALSE);
    }

    baseAddr = &pFifoPtr->pElemAddr[pFifoPtr->head * pFifoPtr->elemSize];
    for (i = 0; i < pFifoPtr->elemSize; i++) {
        pElem[i] = baseAddr[i];
    }
    pFifoPtr->head = FIFO_WRAP_AROUND(pFifoPtr->head+1,pFifoPtr->numElem);
    return (TRUE);
}

/*-----------------------------------------------------------------------------
 * Description  : This function returns the number of elements in FIFO
 * Arguments    : pFifoPtr - Pointer to FIFO
 * Returns      : Number of elements; -1, in case of error
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
int HdmiFifoGetNumElems (PHDMI_FIFO pFifoPtr)
{
    if ((pFifoPtr == NULL) || (pFifoPtr->tail < pFifoPtr->head)) {
        return (-1);
    }
    return (pFifoPtr->tail - pFifoPtr->head);
}

/*-----------------------------------------------------------------------------
 * Description  : This function posts the semaphore associated with the FIFO
 *              : if there are elements in the FIFO
 * Arguments    : pFifoPtr - Pointer to FIFO
 * Returns      : TRUE, if the semaphore is posted
 *              : FALSE, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
BOOL HdmiFifoSignal (PHDMI_FIFO pFifoPtr)
{
    if (pFifoPtr == NULL) {
        return FALSE;
    }

    if (pFifoPtr->tail != pFifoPtr->fifoSignal) {
        pFifoPtr->fifoSignal = pFifoPtr->tail;
        // Post Semaphore
        MV_OSAL_Sem_Post(pFifoPtr->fifoSem);
    }
    return (TRUE);
}

/*-----------------------------------------------------------------------------
 * Description  : This function pends on the semaphore associated with the FIFO
 * Arguments    : pFifoPtr - Pointer to FIFO
 * Returns      : TRUE, when the semaphore is released
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
BOOL HdmiFifoWaitForSignal (PHDMI_FIFO pFifoPtr)
{
    if (pFifoPtr == NULL) {
        return FALSE;
    }

    // Pend on semaphore
    MV_OSAL_Sem_Wait (pFifoPtr->fifoSem);
    return (TRUE);
}
