/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#include "com_type.h"
#include "hdmi_cmn.h"
#include "hdmi_edid.h"
#include "hdmi_edid_prv.h"
#include "vpp_module.h"
#include "vpp_dv_api.h"
#define EDID_PRINTF(...) VPP_log(VPP_LOG_HDMI, VPP_LOG_INFO, "VPPEDID", __VA_ARGS__)

static EDID_RETURNS EdidParseCapabilityBlk(
                        unsigned char *pDataBuf,
                        unsigned char  BlkLen,
                        PEDID_CAPABILITY_INFO pCaPInfo)
{
    if (pDataBuf == NULL || pCaPInfo == NULL)
    {
        EDID_PRINTF("Parameters are NULL\n");
        return EDID_ERR_INVALID_PARAM;
    }

    // Skip header fields
    pDataBuf += 2;

    if (BlkLen < 2)
    {
       return (EDID_ERR_INVALID_BLK_DATA);
    }

    pCaPInfo->QS  = IS_BIT_SET(pDataBuf[0],6);
    pCaPInfo->QY  = IS_BIT_SET(pDataBuf[0],7);
    return EDID_ERR_OK;
}

static EDID_RETURNS EdidParse420CapabilityBlk(
                        unsigned char *pDataBuf,
                        unsigned char  BlkLen,
                        PEDID_YCbCr420_CAP_INFO pCapInfo)
{
    UINT8 VicCount=0;

    if (pDataBuf == NULL || pCapInfo == NULL)
    {
        EDID_PRINTF("Parameters are NULL\n");
        return EDID_ERR_INVALID_PARAM;
    }

    // Skip header fields
    pDataBuf += 2;

    if (BlkLen <= 0)
       return (EDID_ERR_INVALID_BLK_DATA);

    while((BlkLen-1)!=0)
    {
        pCapInfo->VideoIdCode[VicCount]=pDataBuf[VicCount];
        VicCount++;
        BlkLen--;
    }

    return EDID_ERR_OK;
}

static EDID_RETURNS EdidParse420CapabilityMapBlk(
                        unsigned char *pDataBuf,
                        unsigned char  BlkLen,
                        PEDID_YCbCr420_CAP_MAP_INFO pCapMapInfo,
                        PEDID_CEA_TIMING_EXTN_BLK pCEAExtnBlk)
{
    UINT8 VicCount=0;
    UINT8 BitMapIndex=0;
    UINT8 SVDIndex=0;
    int i = 0;

    if (pDataBuf == NULL || pCapMapInfo == NULL || pCEAExtnBlk == NULL)
    {
        EDID_PRINTF("Parameters are NULL\n");
        return EDID_ERR_INVALID_PARAM;
    }

    pDataBuf += 2;

    if (BlkLen <= 0)
        return (EDID_ERR_INVALID_BLK_DATA);

    if (BlkLen == 1){
        for(i=0;i<sizeof(PEDID_YCbCr420_CAP_MAP_INFO);i++)
            pCapMapInfo->VideoIdCode[i]=pCEAExtnBlk->VideoResInfo[i].VideoIDCode;
        return EDID_ERR_OK;
    }
    else
    {
        while((BlkLen-1)!= 0)
        {
            if (pDataBuf[BitMapIndex] != 0)
            {
                // check the bitmap of SVD index;
                for (i = 0; i < 8; i++)
                {
                    if( pDataBuf[BitMapIndex] & (1<<i))
                    {
                        SVDIndex = i + (BitMapIndex * 8);
                        pCapMapInfo->VideoIdCode[VicCount]=pCEAExtnBlk->VideoResInfo[SVDIndex].VideoIDCode;
                        VicCount++;
                        if( VicCount >= 31)  /* Currently - max vics in svd is 31,Ref - CEA Spec*/
                            return EDID_ERR_OK;
                    }
                }
            }
            BitMapIndex++;
            BlkLen--;
        }
     }

    return EDID_ERR_OK;
}

static EDID_RETURNS EdidParseHDRBlk(unsigned char* pDataBuf,
                                        unsigned char  BlkLen,
                                        PHDR_METADATA_INFO pHDRMetadataInfo)
{
    if (pDataBuf == NULL || pHDRMetadataInfo == NULL)
    {
        EDID_PRINTF("Parameters are NULL\n");
        return EDID_ERR_INVALID_PARAM;
    }

    // Skip header fields
    pDataBuf += 2;

    if (BlkLen < 3)
    {
        return (EDID_ERR_INVALID_BLK_DATA);
    }

    pHDRMetadataInfo->EOTF_ET_0  = IS_BIT_SET(pDataBuf[0],0);
    pHDRMetadataInfo->EOTF_ET_1  = IS_BIT_SET(pDataBuf[0],1);
    pHDRMetadataInfo->EOTF_ET_2  = IS_BIT_SET(pDataBuf[0],2);
    pHDRMetadataInfo->EOTF_ET_3  = IS_BIT_SET(pDataBuf[0],3);

    pHDRMetadataInfo->EOTF_SM_0  = IS_BIT_SET(pDataBuf[1],0);

    if(BlkLen > 3) {
        switch(BlkLen) {
            case 6:
            {
                pHDRMetadataInfo->DesCntMaxLuminanceData = ((unsigned int)(1<<((pDataBuf[2])>>5))*50)*10000;
                pHDRMetadataInfo->DesCntMaxFrmAvgLuminanceData = ((unsigned int)(1<<((pDataBuf[3])>>5))*50)*10000;
                pHDRMetadataInfo->DesCntMinLuminanceData = (pDataBuf[4]*pDataBuf[2]*pDataBuf[4]*100)/(255*255);
            }
            break;

            case 5:
            {
                pHDRMetadataInfo->DesCntMaxLuminanceData = ((unsigned int)(1<<((pDataBuf[2])>>5))*50)*10000;
                pHDRMetadataInfo->DesCntMaxFrmAvgLuminanceData = ((unsigned int)(1<<((pDataBuf[3])>>5))*50)*10000;
                pHDRMetadataInfo->DesCntMinLuminanceData = 0x0;
            }
            break;

            case 4:
            {
                pHDRMetadataInfo->DesCntMaxLuminanceData = ((unsigned int)(1<<((pDataBuf[2])>>5))*50)*10000;
                pHDRMetadataInfo->DesCntMaxFrmAvgLuminanceData = 0x0;
                pHDRMetadataInfo->DesCntMinLuminanceData = 0x0;
            }
            break;
        }
    }
    return EDID_ERR_OK;
}

#ifdef CONFIG_AMP_VPP_HDR10_PLUS
static EDID_RETURNS EdidParseHDR10PlusBlk(unsigned char* pDataBuf,
                                            unsigned char  BlkLen,
                                            EDID_HDR_CAPABILITY *pHDRCapability)
{
    UINT32 uiHdr10PlusIEEEOUICode;
    UINT8 application_version;

    if (pDataBuf == NULL || pHDRCapability == NULL)
    {
        EDID_PRINTF("Parameters are NULL\n");
        return EDID_ERR_INVALID_PARAM;
    }

    if (BlkLen != 5)
    {
        return EDID_ERR_INVALID_BLK_DATA;
    }

    uiHdr10PlusIEEEOUICode = ((pDataBuf[4] << 16) | (pDataBuf[3] << 8) | (pDataBuf[2]));
    if (uiHdr10PlusIEEEOUICode != HDR10PLUS_IEEE_CODE)
    {
        EDID_PRINTF("Invalid/Unsupported OUI Idenitifier for HDR10+ :%d, Block Skipped\n", uiHdr10PlusIEEEOUICode);
        return EDID_ERR_INVALID_BLK_DATA;
    }

    application_version = (UINT8)(pDataBuf[5] & 0x3);
    if (application_version != 1)
    {
        EDID_PRINTF("Invalid application version for HDR10+ : %d, Block Skipped\n", application_version);
        return EDID_ERR_INVALID_BLK_DATA;
    }

    pHDRCapability->isHDR10PlusSupported = 1;
    return EDID_ERR_OK;
}
#endif

static EDID_RETURNS ParseHFSCDB(unsigned char* pDataBuf,unsigned char  HfScdbLen , PEDID_CEA_HFVSDB pEdidHfscdb) {

    UINT8 pos=0 ;

    if (pDataBuf == NULL || pEdidHfscdb == NULL)
    {
        EDID_PRINTF("Parameters are NULL\n");
        return EDID_ERR_INVALID_PARAM;
    }

    // Check Length - minimum value is 7, max is 31
    if (HfScdbLen < 7 || HfScdbLen > 31)
    {
        return EDID_ERR_INVALID_BLK_DATA;
    }

    // parse the tag code and length
    pEdidHfscdb->VendorSpecificTagCode = (pDataBuf[0] & 0xE0) >> 5;
    pEdidHfscdb->Length = (pDataBuf[0] & 0x1F);

    //copy the version
    pEdidHfscdb->Version = pDataBuf[4];
    if(pEdidHfscdb->Version!=1)
    {
        EDID_PRINTF("Invalid version block, version != 1 \n");
        return EDID_ERR_INVALID_BLK_DATA;
    }

    //Copy the Max TMDS Rate
    pEdidHfscdb->MAX_TMDS_Character_Rate = pDataBuf[5];

    pos = 6;

    //copy all the flags data
    pEdidHfscdb->HfvsdbFeatures.Hdmi_3D_OSD_Disparity = IS_BIT_SET(pDataBuf[pos],0);
    pEdidHfscdb->HfvsdbFeatures.Dual_View             = IS_BIT_SET(pDataBuf[pos],1);
    pEdidHfscdb->HfvsdbFeatures.Independent_view      = IS_BIT_SET(pDataBuf[pos],2);
    pEdidHfscdb->HfvsdbFeatures.LTE_340Mcsc_scramble  = IS_BIT_SET(pDataBuf[pos],3);
    pEdidHfscdb->HfvsdbFeatures.RR_Capable            = IS_BIT_SET(pDataBuf[pos],6);
    pEdidHfscdb->HfvsdbFeatures.SCDC_Present          = IS_BIT_SET(pDataBuf[pos],7);

    pos=7;
    //copy flag 2 data
    pEdidHfscdb->HfVsdbDeepCol.DC_30bit_420          = IS_BIT_SET(pDataBuf[pos],0);
    pEdidHfscdb->HfVsdbDeepCol.DC_36bit_420          = IS_BIT_SET(pDataBuf[pos],1);
    pEdidHfscdb->HfVsdbDeepCol.DC_48bit_420          = IS_BIT_SET(pDataBuf[pos],2);

    return EDID_ERR_OK;
}

static EDID_RETURNS ParseHFVSDB(unsigned char* pDataBuf,unsigned char  HfVsdbLen , PEDID_CEA_HFVSDB pEdidHfvsdb) {

    if (pDataBuf == NULL || pEdidHfvsdb == NULL)
    {
        EDID_PRINTF("Parameters are NULL\n");
        return EDID_ERR_INVALID_PARAM;
    }

    // Check Length - minimum value is 7, max is 31
    if (HfVsdbLen < 7 || HfVsdbLen > 31)
    {
        return EDID_ERR_INVALID_BLK_DATA;
    }

    //The length should be minimum seven , so all the below fiels should be present , no check for length done as all are mandatory
    pEdidHfvsdb->IEEE_OUI_ThirdOctet  =  pDataBuf[1];
    pEdidHfvsdb->IEEE_OUI_SecondOctet =  pDataBuf[2];
    pEdidHfvsdb->IEEE_OUI_FirstOctet  =  pDataBuf[3];

    if(!(pEdidHfvsdb->IEEE_OUI_ThirdOctet == 0xD8 &&  pEdidHfvsdb->IEEE_OUI_SecondOctet == 0x5D &&  pEdidHfvsdb->IEEE_OUI_FirstOctet == 0xC4)){
        EDID_PRINTF("The IEEE OUI Present in the HF VSDB is not the one allocated to HDMI Forum");
        return EDID_ERR_INVALID_PARAM;
    }

    return ParseHFSCDB(pDataBuf, HfVsdbLen, pEdidHfvsdb);
}

static EDID_RETURNS EdidParseVSADBlk(unsigned char* pDataBuf,
                                        unsigned char  BlkLen,
                                        PEDID_DOLBY_AUDIO_CAP pDolbyAudCap)
{
    if (pDataBuf == NULL || pDolbyAudCap == NULL) {
        EDID_PRINTF("Parameters are NULL\n");
        return EDID_ERR_INVALID_PARAM;
    }

    if (BlkLen < 6) {
        return (EDID_ERR_INVALID_BLK_DATA);
    }

    if((pDataBuf[2] == 0x46) && (pDataBuf[3] == 0xD0) && (pDataBuf[4] == 0x00)) {
        pDolbyAudCap->vsadbPresent = TRUE;
        pDolbyAudCap->dolbyVsadbVersion = pDataBuf[5] & 0x7;
        pDolbyAudCap->f63 = (pDataBuf[5] >> 3) & 0x1;
        pDolbyAudCap->outputConfig = (pDataBuf[5] >> 4) & 0x7;
        pDolbyAudCap->isHeadphonePB = (pDataBuf[5] >> 7) & 0x1;
        pDolbyAudCap->sinkDevCap = pDataBuf[6];
    } else {
        return EDID_ERR_INVALID_PARAM;
    }

    return EDID_ERR_OK;
}

EDID_RETURNS ParseEdid (
    unsigned char *pDataBuf,
    PEDID_BLK_TYPE pEdidBlkType,
    PEDID_DATA     pEdidBlkData,
    BOOL isFirstBlock)
{
    EDID_RETURNS RetCode;

    if (pDataBuf == NULL)
    {
        EDID_PRINTF("ParseEdid pDataBuf is NULL\n");
        return EDID_ERR_INVALID_PARAM;
    }

    // Check CRC of block data
    RetCode = CheckCRC(pDataBuf);
    if (RetCode != EDID_ERR_OK)
    {
        EDID_PRINTF("CheckCRC failed: %d\n", RetCode);
        return RetCode;
    }

    // Identify block type
    if ((pDataBuf[0] == 0x0) &&  (pDataBuf[7] == 0x0) &&
        (pDataBuf[1] == 0xFF)&&  (pDataBuf[2] == 0xFF)&&
        (pDataBuf[3] == 0xFF)&&  (pDataBuf[4] == 0xFF)&&
        (pDataBuf[5] == 0xFF)&&  (pDataBuf[6] == 0xFF))
    {
        EDID_PRINTF("Parsing HDMI TX basic block\n");
        // Header of basic block
        *pEdidBlkType = EDID_BASIC_BLK;
        return ParseBasicBlk (pDataBuf,&pEdidBlkData->BasicBlk);
    }
    else if (pDataBuf[EDID_BLK_TAG_ADDR] == EDID_CEA_EXTN_TAG)
    {
        EDID_PRINTF("Parsing HDMI TX EXT block\n");
        *pEdidBlkType = EDID_CEA_TIMING_EXTN;
        return ParseCEAExtn (pDataBuf,&pEdidBlkData->CEAExtnBlk, isFirstBlock);
    }
    else if (pDataBuf[EDID_BLK_TAG_ADDR] == EDID_BLOCK_MAP_TAG)
    {
        EDID_PRINTF("Parsing HDMI TX BLOCK MAP block\n");
        *pEdidBlkType = EDID_BLOCK_MAP;
        return ParseBlkMap (pDataBuf,&pEdidBlkData->BlkMap);
    }
    else
    {
        EDID_PRINTF("Undefined block\n");
        *pEdidBlkType = EDID_UNDEF_BLK;
    }
    return EDID_ERR_OK;
}

EDID_RETURNS EdidGetBlkType (
    unsigned char *pDataBuf,
    PEDID_BLK_TYPE pEdidBlkType)
{
    EDID_RETURNS RetCode;

    if (pDataBuf == NULL)
    {
        EDID_PRINTF("ParseEdid pDataBuf is NULL\n");
        return EDID_ERR_INVALID_PARAM;
    }

    // Check CRC of block data
    RetCode = CheckCRC(pDataBuf);
    if (RetCode != EDID_ERR_OK)
    {
        EDID_PRINTF("CheckCRC failed: %d\n", RetCode);
        return RetCode;
    }

    // Identify block type
    if ((pDataBuf[0] == 0x0) &&  (pDataBuf[7] == 0x0) &&
        (pDataBuf[1] == 0xFF)&&  (pDataBuf[2] == 0xFF)&&
        (pDataBuf[3] == 0xFF)&&  (pDataBuf[4] == 0xFF)&&
        (pDataBuf[5] == 0xFF)&&  (pDataBuf[6] == 0xFF))
    {
        *pEdidBlkType = EDID_BASIC_BLK;
    }
    else if (pDataBuf[EDID_BLK_TAG_ADDR] == EDID_CEA_EXTN_TAG)
    {
        *pEdidBlkType = EDID_CEA_TIMING_EXTN;
    }
    else if (pDataBuf[EDID_BLK_TAG_ADDR] == EDID_BLOCK_MAP_TAG)
    {
        *pEdidBlkType = EDID_BLOCK_MAP;
    }
    else
    {
        *pEdidBlkType = EDID_UNDEF_BLK;
    }
    return EDID_ERR_OK;
}

/*-----------------------------------------------------------------------------
 * Basic Block Parsing Functions
 *-----------------------------------------------------------------------------
 */
static EDID_RETURNS ParseBasicBlk(
                        unsigned char *pDataBuf,
                        PEDID_BASIC_BLK_DATA pEdidBasicBlk)
{
    EDID_RETURNS     RetCode;
    unsigned char       DataOfs;
    unsigned char       i;

    VPPHDMI_DBG_INFO(("Verify EDID version\n"));
    // Check EDID Version, revisions ranged from v1.0 ~ V1.4 are well supported
    if (!((pDataBuf[EDID_BLK0_VERS_ADDR] == 0x1) &&
          (pDataBuf[EDID_BLK0_REV_ADDR]  <= 0x4)))
    {
        EDID_PRINTF("ALERT:The current EDID version is: %d.%d\n", pDataBuf[EDID_BLK0_VERS_ADDR], pDataBuf[EDID_BLK0_REV_ADDR]);
    }

    pEdidBasicBlk->GenInfo.Version  = pDataBuf[EDID_BLK0_VERS_ADDR];
    pEdidBasicBlk->GenInfo.Revision = pDataBuf[EDID_BLK0_REV_ADDR];

    // Read Manufacturer Details
    VPPHDMI_DBG_INFO(("Parse Manufacturer info\n"));
    RetCode = EdidParseMnfrInfo(&pDataBuf[EDID_BLK0_PRD_ID_ADDR],
                                &pEdidBasicBlk->GenInfo);
    if (RetCode != EDID_ERR_OK)
    {
        EDID_PRINTF("ERROR: Parse Vendor & Product Identification(0x08 ~ 0x11)\n");
        return RetCode;
    }

    // Read Basic Display Features
    VPPHDMI_DBG_INFO(("Parse display features\n"));
    RetCode = EdidParseDispParams(&pDataBuf[EDID_BLK0_DISP_PARAMS_ADDR],
                              &pEdidBasicBlk->DispParams);
    if (RetCode != EDID_ERR_OK)
    {
        EDID_PRINTF("ERROR: Parse Basic Display Parameters & Features(0x14 ~ 0x18), Color Characteristics(0x19 ~ 0x22)\n");
        return RetCode;
    }

    // Initialize number of supported formats
    pEdidBasicBlk->NumSpprtedVidFmts = 0;

    // Read Established timing data
    VPPHDMI_DBG_INFO(("Parse ETD\n"));
    RetCode = EdidParseETD(&pDataBuf[EDID_BLK0_ETD_ADDR],
                           &pEdidBasicBlk->NumSpprtedVidFmts,
                           &pEdidBasicBlk->VideoResInfo[pEdidBasicBlk->NumSpprtedVidFmts]);
    if (RetCode != EDID_ERR_OK)
    {
        EDID_PRINTF("ERROR: Parse Established Timings(0x23 ~ 0x25)\n");
        return RetCode;
    }

    // Read standard timing data
    VPPHDMI_DBG_INFO(("Parse STD\n"));
    RetCode = EdidParseSTD(&pDataBuf[EDID_BLK0_STD_ADDR],
                           &pEdidBasicBlk->NumSpprtedVidFmts,
                           &pEdidBasicBlk->VideoResInfo[pEdidBasicBlk->NumSpprtedVidFmts]);
    if (RetCode != EDID_ERR_OK)
    {
        EDID_PRINTF("ERROR: Parse Standard Timings(0x26 ~ 0x35)\n");
        return RetCode;
    }

    // Parse DTDs and MDBs
    DataOfs = EDID_BLK0_DTD_ADDR;
    pEdidBasicBlk->PrefTimingEntry = 0xFF;
    for (i = 0; i < 4; i++)
    {
        if (pDataBuf[DataOfs]   == 0x00 &&
            pDataBuf[DataOfs+1] == 0x00)
        {
            // If data structure version 1 revision 3 is used then bit 1 of
            // the feature support byte (offset 18h) must be set to one.
            // If this bit is not set to one then the first detailed timing
            // does not necessarily represent the display’s preferred timing
            // Monitor Descriptor Block
            VPPHDMI_DBG_INFO(("Parse MDB\n"));
            RetCode = EdidParseMDB(&pDataBuf[DataOfs],
                                   &pEdidBasicBlk->MonitorInfo);
            if (RetCode != EDID_ERR_OK)
            {
                EDID_PRINTF("ERROR: Parse Monitor Descriptor Block: offset:0x%x\n", DataOfs);
                return RetCode;
            }
        }
        else
        {
            // Detailed Timing Descriptor
            VPPHDMI_DBG_INFO(("Parse DTD\n"));
            RetCode = EdidParseDTD(&pDataBuf[DataOfs],
                                   &pEdidBasicBlk->NumSpprtedVidFmts,
                                   &pEdidBasicBlk->VideoResInfo[pEdidBasicBlk->NumSpprtedVidFmts],
                                   &pEdidBasicBlk->DispParams.MaxHImageSize,
                                   &pEdidBasicBlk->DispParams.MaxVImageSize);
            if (RetCode != EDID_ERR_OK)
            {
                EDID_PRINTF("ERROR: Parse Detailed Timing Descriptor %d, offset:0x%x\n", i, DataOfs);
                return RetCode;
            }
            if (i == 0)
                pEdidBasicBlk->PrefTimingEntry = pEdidBasicBlk->NumSpprtedVidFmts - 1;
        }
        DataOfs += EDID_DTD_LEN;
    }

    // Read Extension
    pEdidBasicBlk->NumExtns = pDataBuf[EDID_BLK0_EXTN_FLAG_ADDR];

    // Not all versions support CEA Timing extension block

#if defined (DEBUG_EDID_PARSE)
    /* Print EDID contents */
    DumpBasicBlk (pEdidBasicBlk);
#endif
    return (EDID_ERR_OK);
}

static EDID_RETURNS EdidParseMnfrInfo(
                        unsigned char *pDataBuf,
                        PEDID_GEN_INFO pGenInfo)
{
    // Manufacturer name is in compressed Ascii format
    pGenInfo->MnfrName[0]   = Chars[(pDataBuf[0x0] & 0x7E)>>2];
    pGenInfo->MnfrName[1]   = Chars[((pDataBuf[0x0] & 0x03)<<3)
                             + ((pDataBuf[0x01] & 0xE0)>>5)];
    pGenInfo->MnfrName[2]   = Chars[(pDataBuf[0x01] & 0x1F)];
    pGenInfo->MnfrName[3]   = '\0';

    sprintf(&pGenInfo->ProductCode[0],"%02X%02X",pDataBuf[0x03], pDataBuf[0x02]);
    pGenInfo->SerialNum = (pDataBuf[0x04]) |
                          (pDataBuf[0x05] << 8) |
                          (pDataBuf[0x06] << 16)|
                          (pDataBuf[0x07] << 24);
    pGenInfo->MnfrWeek  = pDataBuf[0x08];

    /* Year is calculated as Actual year - 1990;so, 1990 is added
     * here to get the actual value
     */
    pGenInfo->MnfrYear      = pDataBuf[0x09] + 1990;
    return EDID_ERR_OK;
}

static EDID_RETURNS EdidParseDispParams(
                        unsigned char *pDataBuf,
                        PEDID_DISP_PARAMS pDispParams)
{
    if (IS_BIT_SET(pDataBuf[0],7))
    {
        // Digital Interface
        pDispParams->DigVideoInput = SPRT_ENABLED;
        pDispParams->VideoInpDet.DigVidInp.DFP1XEnabled = IS_BIT_SET(pDataBuf[0],0);
    }
    else
    {
        // Analog Interface
        pDispParams->DigVideoInput = SPRT_DISABLED;
        pDispParams->VideoInpDet.AnalogVidInp.VppLevel =
                                        (pDataBuf[0] & 0xF0) >> 4;
        pDispParams->VideoInpDet.AnalogVidInp.BlankToBlackSetup=
                                        IS_BIT_SET(pDataBuf[0],4);
        pDispParams->VideoInpDet.AnalogVidInp.SepSyncs         =
                                        IS_BIT_SET(pDataBuf[0],3);
        pDispParams->VideoInpDet.AnalogVidInp.CompSync         =
                                        IS_BIT_SET(pDataBuf[0],2);
        pDispParams->VideoInpDet.AnalogVidInp.SyncOnGreen      =
                                        IS_BIT_SET(pDataBuf[0],1);
        pDispParams->VideoInpDet.AnalogVidInp.SerrationVSync   =
                                        IS_BIT_SET(pDataBuf[0],0);
    }

    pDispParams->MaxHImageSize  = pDataBuf[1];
    pDispParams->MaxVImageSize  = pDataBuf[2];
    pDispParams->Gamma          = pDataBuf[3];

    // Parse feature support fields
    pDispParams->FeatureSprt.GTFSprt        = IS_BIT_SET(pDataBuf[4],0);
    pDispParams->FeatureSprt.PrefTiming     = IS_BIT_SET(pDataBuf[4],1);
    pDispParams->FeatureSprt.sRGBSprt       = IS_BIT_SET(pDataBuf[4],2);
    pDispParams->FeatureSprt.ActiveOffSprt  = IS_BIT_SET(pDataBuf[4],5);
    pDispParams->FeatureSprt.SuspendSprt    = IS_BIT_SET(pDataBuf[4],6);
    pDispParams->FeatureSprt.StandbySprt    = IS_BIT_SET(pDataBuf[4],7);
    pDispParams->FeatureSprt.DispType       = (pDataBuf[4] & 0x18) >> 3;

    pDispParams->ColorChar.Rx = (pDataBuf[7] << 2) | ((pDataBuf[5] & 0xC0) >> 6);
    pDispParams->ColorChar.Ry = (pDataBuf[8] << 2) | ((pDataBuf[5] & 0x30) >> 4);
    pDispParams->ColorChar.Gx = (pDataBuf[9] << 2) | ((pDataBuf[5] & 0x0C) >> 2);
    pDispParams->ColorChar.Gy = (pDataBuf[0x0A] << 2) | (pDataBuf[5]  & 0x03);
    pDispParams->ColorChar.Bx = (pDataBuf[0x0B] << 2) | ((pDataBuf[6] & 0xC0) >> 6);
    pDispParams->ColorChar.By = (pDataBuf[0x0C] << 2) | ((pDataBuf[6] & 0x30) >> 4);
    pDispParams->ColorChar.Wx = (pDataBuf[0x0D] << 2) | ((pDataBuf[6] & 0x0C) >> 2);
    pDispParams->ColorChar.Wy = (pDataBuf[0x0E] << 2) | (pDataBuf[6]  & 0x03);

    // If data structure version 1 revision 3 is used then bit 1 of the feature
    // support byte (offset 18h) must be set to one.  Previous revisions of the
    // EDID structure supported the use of this filed but did not require its use.

    return EDID_ERR_OK;
}

static EDID_RETURNS EdidParseDTD(
                    unsigned char *pDataBuf,
                    unsigned char *pEdidResCnt,
                    PCEA_RES_INFO pEdidResInfo,
                    unsigned char *HImgSize,
                    unsigned char *VImgSize)
{
    unsigned int    PixelClock;
    unsigned int    HBlank, VBlank, HSize, VSize;

    pEdidResInfo->HActive     = pDataBuf[2] | ((pDataBuf[4] & 0xF0) << 4);
    pEdidResInfo->VActive     = pDataBuf[5] | ((pDataBuf[7] & 0xF0) << 4);
    HBlank  = pDataBuf[3] | ((pDataBuf[4] & 0x0F) << 8);
    VBlank  = pDataBuf[6] | ((pDataBuf[7] & 0x0F) << 8);
    pEdidResInfo->FrmFmt = IS_BIT_SET(pDataBuf[0x11],7);

    // Calculate Refresh Rate
    // RR = (Pixel Clock) / (HTotal * VTotal)
    PixelClock = (pDataBuf[0] | (pDataBuf[1] << 8));
    if (!IntDivision((PixelClock * 10000),
                          (pEdidResInfo->HActive + HBlank)*
                          (pEdidResInfo->VActive + VBlank),
                          &pEdidResInfo->RefreshRate))
    {
        return EDID_ERR_INVALID_PARAM;
    }

    pEdidResInfo->AspRatio = CEA_PICT_AR_NONE;

    // H and V Size for calculating aspect ratio
    HSize  = pDataBuf[0x0C] | ((pDataBuf[0x0E] & 0xF0) << 4);
    VSize  = pDataBuf[0x0D] | ((pDataBuf[0x0E] & 0x0F) << 8);
    if((HSize != 0) && (VSize != 0) && (HSize > VSize)){
        if((HSize-VSize) > VSize/2){
            pEdidResInfo->AspRatio = CEA_PICT_AR_16_9;
        }
        else{
            pEdidResInfo->AspRatio = CEA_PICT_AR_4_3;
        }
    }
    if ((*HImgSize == 0) && (*VImgSize == 0)) {
        *HImgSize = HSize/10;
        *VImgSize = VSize/10;
    }

    // For interlaced resolutions, double VActive
    if (pEdidResInfo->FrmFmt == RES_INFO_INTERLACED)
        pEdidResInfo->VActive *= 2;

    // Check if it can be mapped to a Video ID Code
    GetVideoIDForRes (pEdidResInfo);

    // Increment supported display resolution counter
    (*pEdidResCnt)++;
    return EDID_ERR_OK;
}

// Established timing descriptor
static EDID_RETURNS EdidParseETD(
                        unsigned char *pDataBuf,
                        unsigned char *pEdidResCnt,
                        PCEA_RES_INFO pEdidResInfo)
{
    unsigned short  EtdVal;
    unsigned char   i;

    EtdVal = (pDataBuf[0] | (pDataBuf[1] << 8));
    for (i = 0; i < 16; i++)
    {
        if (IS_BIT_SET(EtdVal,i))
        {
            pEdidResInfo->VideoIDCode   = EtdList[i].VideoIDCode; // undefined
            pEdidResInfo->HActive       = EtdList[i].HActive;
            pEdidResInfo->VActive       = EtdList[i].VActive;
            pEdidResInfo->RefreshRate   = EtdList[i].RefreshRate;
            pEdidResInfo->FrmFmt        = EtdList[i].FrmFmt;
            pEdidResInfo->AspRatio      = EtdList[i].AspRatio;
            (*pEdidResCnt)++;
            pEdidResInfo++;
        }
    }
    return EDID_ERR_OK;
}

// Standard timing descriptor
static EDID_RETURNS EdidParseSTD(
                        unsigned char *pDataBuf,
                        unsigned char *pEdidResCnt,
                        PCEA_RES_INFO pEdidResInfo)
{
    unsigned short  HSize, VSize;
    unsigned char   Count;

    /* Byte 0: Horizontal Resolution
     * Byte 1: |7    6  |5 |4  3  2  1  0|
     *       : |H&V Size|  |Refresh Rate |
     */

    // Number of standard timing definitions is 8
    for (Count = 0; Count < 8; Count++)
    {
        if (pDataBuf[0] == 0x01 || pDataBuf[1] == 0x01)
        {
            pDataBuf += 2;
            // standard timing definition not defined
            continue;
        }

        pEdidResInfo->HActive = (pDataBuf[0] + 31) * 8;
        switch (pDataBuf[1] >> 6)
        {
            case 0x00:
                HSize = 16; VSize = 10;
                pEdidResInfo->AspRatio = CEA_PICT_AR_16_9;
                break;
            case 0x01:
                HSize = 4; VSize = 3;
                pEdidResInfo->AspRatio = CEA_PICT_AR_4_3;
                break;
            case 0x02:
                HSize = 5; VSize = 4;
                pEdidResInfo->AspRatio = CEA_PICT_AR_4_3;
                break;
            case 0x03:
                HSize = 16; VSize = 9;
                pEdidResInfo->AspRatio = CEA_PICT_AR_16_9;
                break;
            default:
                HSize = 1; VSize = 1;
                pEdidResInfo->AspRatio = CEA_PICT_AR_NONE;
                break; // Undefined
        }
        pEdidResInfo->VActive = (pEdidResInfo->HActive * VSize)/HSize;
        pEdidResInfo->RefreshRate = 60 + (pDataBuf[1] & 0x1F);

        // Following are undefined in this descriptor
        pEdidResInfo->FrmFmt = RES_INFO_FRM_FMT_UNDEF;

        // Check if it can be mapped to a Video ID Code
        GetVideoIDForRes (pEdidResInfo);

        pDataBuf += 2;
        // Increment supported display resolution counter
        (*pEdidResCnt)++;
        pEdidResInfo++;
    }
    return EDID_ERR_OK;
}

// Monitor Descriptor Block
static EDID_RETURNS EdidParseMDB(
                        unsigned char *pDataBuf,
                        PEDID_MONITOR_INFO pMonitorInfo)
{
    unsigned char i;

    // Verify the flags for monitor descriptor block
    if (!(pDataBuf[0] == 0x00 &&
          pDataBuf[1] == 0x00 &&
          pDataBuf[2] == 0x00))
    {
        return EDID_ERR_INVALID_BLK_DATA;
    }

    // Check the data type tag
    if (pDataBuf[3] == 0xFC)
    {
        // Get Monitor Name (Max Size 13 bytes)
        for (i = 0; i < 13; i++)
            pMonitorInfo->MonitorName[i] = pDataBuf[i+5];
        pMonitorInfo->MonitorName[13] = '\0';
    }
    else if (pDataBuf[3] == 0xFD)
    {
        // Get Monitor Range Limits
        pMonitorInfo->MinVRate = pDataBuf[5];
        pMonitorInfo->MaxVRate = pDataBuf[6];
        pMonitorInfo->MinHRate = pDataBuf[7];
        pMonitorInfo->MaxHRate = pDataBuf[8];
        pMonitorInfo->MaxPixelClock = pDataBuf[9];
    }
    return EDID_ERR_OK;
}

/*-----------------------------------------------------------------------------
 * CEA Block Parsing Functions
 *-----------------------------------------------------------------------------
 */
static EDID_RETURNS ParseCEAExtn(
                        unsigned char *pDataBuf,
                        PEDID_CEA_TIMING_EXTN_BLK pCEAExtnBlk,
                        BOOL isFirstBlock)
{
    unsigned char   DtdOfs;
    unsigned char   DataBlkOfs;
    unsigned char   DataBlkTag;
    unsigned char   DataBlkLen;
    unsigned char   EndOfDataBlk;
    unsigned char   SvdFound;
    unsigned char   DupHfscdb = 0;
    EDID_RETURNS    RetCode = EDID_ERR_OK;

    if (isFirstBlock) {
        // Until a VSDB with Hdmi registration ID is found, it would be
        // assumed that the receiver supports only DVI
        pCEAExtnBlk->HdmiMode = FALSE;
        *(unsigned char*)(&pCEAExtnBlk->SpkrAlloc) = 0xFF;
        *(unsigned char*)(&pCEAExtnBlk->CapabilityInfo) = 0xFF;
        *(unsigned char*)(&pCEAExtnBlk->YCbCr420CapInfo) = 0xFF;
        *(unsigned char*)(&pCEAExtnBlk->CeaHdmiVSDB) = 0xFF;
        *(unsigned char*)(&pCEAExtnBlk->DolbyVisionVSVDB) = 0xFF;
        *(unsigned char*)(&pCEAExtnBlk->DolbyAudioVSADB) = 0xFF;
        pCEAExtnBlk->PrefTimingEntry = 0xFF;
    }


    if (pDataBuf[EDID_CEA_EXTN_FMT_ADDR] != 0x00)
    {
        // Parse for native format details
        pCEAExtnBlk->FeatureSprt.UnderScanSprt  =
                    IS_BIT_SET(pDataBuf[EDID_CEA_EXTN_FMT_ADDR],7);
        pCEAExtnBlk->FeatureSprt.YCbCr444Sprt   =
                    IS_BIT_SET(pDataBuf[EDID_CEA_EXTN_FMT_ADDR],5);
        pCEAExtnBlk->FeatureSprt.YCbCr422Sprt   =
                    IS_BIT_SET(pDataBuf[EDID_CEA_EXTN_FMT_ADDR],4);
        pCEAExtnBlk->BasicAudioSprt =
                    IS_BIT_SET(pDataBuf[EDID_CEA_EXTN_FMT_ADDR],6);
    }

    SvdFound = 0;
    DtdOfs = pDataBuf[EDID_CEA_EXTN_DTD_OFS_ADDR];

    if (DtdOfs != EDID_CEA_EXTN_DATA_BLK_ADDR)
    {
        // Check for short descriptors
        // Parse CEA Data Blocks
        DataBlkOfs = EDID_CEA_EXTN_DATA_BLK_ADDR;
        EndOfDataBlk = 0;
        if (isFirstBlock &&
            (((pDataBuf[DataBlkOfs] & 0xE0) >> 5) == EDID_CEA_EXTNDED_BLK_TAG) &&
            (pDataBuf[DataBlkOfs+1] == 0x78)) {
            /* EEODB present at offset 4 of fist CEA Ext block */
            DataBlkLen = (pDataBuf[DataBlkOfs] & 0x1F);
            pCEAExtnBlk->ExtBlkCnt = pDataBuf[DataBlkOfs+2];
            DataBlkOfs += (DataBlkLen+1);
        }
        while (!EndOfDataBlk)
        {
            // Parse data block header
            DataBlkTag = (pDataBuf[DataBlkOfs] & 0xE0) >> 5;
            DataBlkLen = (pDataBuf[DataBlkOfs] & 0x1F);

            switch (DataBlkTag)
            {
                case EDID_CEA_AUDIO_BLK_TAG:
                {
                    RetCode = EdidParseSAD(
                                    &pDataBuf[DataBlkOfs],DataBlkLen,pCEAExtnBlk);
                    if (RetCode != EDID_ERR_OK)
                    {
                        EDID_PRINTF("ERROR: Parse EDID_CEA_AUDIO_BLK_TAG, offset:0x%x, len:0x%x\n", DataBlkOfs, DataBlkLen);
                        break;
                    }
                }
                break;

                case EDID_CEA_VIDEO_BLK_TAG:
                {
                    pCEAExtnBlk->FirstSVDEntry = pCEAExtnBlk->NumSpprtedVidFmts;
                    RetCode = EdidParseSVD(
                                   &pDataBuf[DataBlkOfs],DataBlkLen,
                                   &pCEAExtnBlk->NumSpprtedVidFmts,
                                   &pCEAExtnBlk->VideoResInfo[pCEAExtnBlk->NumSpprtedVidFmts],
                                   &pCEAExtnBlk->PrefTimingEntry);
                    if (RetCode != EDID_ERR_OK)
                    {
                        EDID_PRINTF("ERROR: Parse EDID_CEA_VIDEO_BLK_TAG, offset:0x%x, len:0x%x\n", DataBlkOfs, DataBlkLen);
                        break;
                    }
                    pCEAExtnBlk->LastSVDEntry = pCEAExtnBlk->NumSpprtedVidFmts;
                    SvdFound = 1;
                }
                break;

                case EDID_CEA_SPKR_ALLOC_BLK_TAG:
                {
                    RetCode = EdidParseSpkrAllocBlk(
                                    &pDataBuf[DataBlkOfs],DataBlkLen,&pCEAExtnBlk->SpkrAlloc);
                    if (RetCode != EDID_ERR_OK)
                    {
                        EDID_PRINTF("ERROR: Parse EDID_CEA_SPKR_ALLOC_BLK_TAG, offset:0x%x, len:0x%x\n", DataBlkOfs, DataBlkLen);
                        break;
                    }
                }
                break;

                case EDID_CEA_VNDR_BLK_TAG:
                {
                    //Decide which vendor block by parsing the IEEE OUI
                    if((pDataBuf[DataBlkOfs + 0x1] == 0x03) && (pDataBuf[DataBlkOfs + 0x2] == 0x0C) && (pDataBuf[DataBlkOfs + 0x3] == 0x00))
                    {
                        RetCode = EdidParseVSDB(
                                  &pDataBuf[DataBlkOfs],DataBlkLen,pCEAExtnBlk);
                        if (!((RetCode == EDID_ERR_OK) ||
                            (RetCode == EDID_ERR_UNSUPPORTED_BLK)))
                        {
                            EDID_PRINTF("ERROR: Parse EDID_CEA_VNDR_BLK_TAG, offset:0x%x, len:0x%x\n", DataBlkOfs, DataBlkLen);
                            break;
                        }
                        pCEAExtnBlk->VndrBlkFound = TRUE;
                    }
                    else if((pDataBuf[DataBlkOfs + 0x1] == 0xD8) &&  (pDataBuf[DataBlkOfs + 0x2]  == 0x5D) &&  (pDataBuf[DataBlkOfs + 0x3] == 0xC4))
                    {
                        if (DupHfscdb == 1)
                            EDID_PRINTF("ERROR: HFSCDB block already parsed\n");
                        RetCode = ParseHFVSDB(
                                      &pDataBuf[DataBlkOfs],DataBlkLen,&pCEAExtnBlk->CeaHdmiVSDB);
                        if (!((RetCode == EDID_ERR_OK) ||
                            (RetCode == EDID_ERR_UNSUPPORTED_BLK)))
                        {
                            EDID_PRINTF("ERROR: Parse EDID_CEA_HDMI_FORUM_VNDR_BLK_TAG, offset:0x%x, len:0x%x\n", DataBlkOfs, DataBlkLen);
                            break;
                        }
                        DupHfscdb = 1;
                    }
                    else
                    {
                        RetCode = EDID_ERR_INVALID_PARAM;
                        EDID_PRINTF("ERROR : Not a Valid CEA VNDR BLOCK DATA");
                    }
                }
                break;

                case EDID_CEA_EXTNDED_BLK_TAG:
                {
                    switch (pDataBuf[DataBlkOfs+1])
                    {
                        case EDID_CEA_EXTNDED_VIDEOCAL_BLK_TAG:
                        {
                            RetCode = EdidParseCalorimetryBlk(
                                            &pDataBuf[DataBlkOfs],DataBlkLen,&pCEAExtnBlk->CalorimetryInfo);
                            if (RetCode != EDID_ERR_OK)
                            {
                                EDID_PRINTF("ERROR: Parse EDID_CEA_EXTNDED_VIDEOCAL_BLK_TAG, offset:0x%x, len:0x%x\n", DataBlkOfs, DataBlkLen);
                                break;
                            }
                        }
                        break;
                        case EDID_CEA_EXTENDED_VSVDB_TAG:
                        {
                            RetCode = VPP_DV_ParseEdid(&pDataBuf[DataBlkOfs], \
                                          DataBlkLen, &pCEAExtnBlk->DolbyVisionVSVDB);
                            if (RetCode != EDID_ERR_OK)
                            {
                                EDID_PRINTF("ERROR: Parse DV EDID_CEA_EXTENDED_VSVDB_TAG, offset:0x%x, len:0x%x\n", \
                                        DataBlkOfs, DataBlkLen);
                                break;
                            }
#ifdef CONFIG_AMP_VPP_HDR10_PLUS
                            RetCode = EdidParseHDR10PlusBlk(&pDataBuf[DataBlkOfs], \
                                        DataBlkLen, &pCEAExtnBlk->HdrCapInfo);
                             if (RetCode != EDID_ERR_OK)
                            {
                                EDID_PRINTF("ERROR: Parse HDR10+ EDID_CEA_EXTENDED_VSVDB_TAG, offset:0x%x, len:0x%x\n", \
                                        DataBlkOfs, DataBlkLen);
                                break;
                            }
#endif
                        }
                        break;

                        case EDID_CEA_EXTNDED_VIDEOCAPS_BLK_TAG:
                        {
                            RetCode = EdidParseCapabilityBlk(
                                          &pDataBuf[DataBlkOfs],DataBlkLen,&pCEAExtnBlk->CapabilityInfo);
                            if (RetCode != EDID_ERR_OK)
                            {
                                EDID_PRINTF("ERROR: Parse EDID_CEA_EXTNDED_VIDEOCAPS_BLK_TAG, offset:0x%x, len:0x%x\n", DataBlkOfs, DataBlkLen);
                                break;
                            }
                        }
                        break;

                        case EDID_CEA_EXTENDED_VIDEO420_BLK_TAG:
                        {
                            RetCode = EdidParse420CapabilityBlk(
                                           &pDataBuf[DataBlkOfs],DataBlkLen,&pCEAExtnBlk->YCbCr420CapInfo);
                            if (RetCode != EDID_ERR_OK)
                            {
                                EDID_PRINTF("ERROR: Parse EDID_CEA_EXTNDED_YCBCR420CAPS_BLK_TAG, offset:0x%x, len:0x%x\n", DataBlkOfs, DataBlkLen);
                                break;
                            }
                        }
                        break;

                        case EDID_CEA_EXTENDED_420_CAP_MAP_BLK_TAG:
                        {
                            RetCode = EdidParse420CapabilityMapBlk(
                                           &pDataBuf[DataBlkOfs],DataBlkLen,
                                           &pCEAExtnBlk->YCbCrCapMapInfo,pCEAExtnBlk);
                            if (RetCode != EDID_ERR_OK)
                            {
                                EDID_PRINTF("ERROR: Parse EDID_CEA_EXTNDED_YCBCR420CAPS_BLK_TAG, offset:0x%x, len:0x%x\n", DataBlkOfs, DataBlkLen);
                                break;
                            }
                        }
                        break;

                        case EDID_CEA_EXTENDED_HDR_BLK_TAG:
                        {

                            RetCode = EdidParseHDRBlk(&pDataBuf[DataBlkOfs],DataBlkLen,
                                                              &pCEAExtnBlk->HDRMetadataInfo);
                            if(RetCode != EDID_ERR_OK)
                            {
                                EDID_PRINTF("ERROR: Parse EDID_CEA_EXTENDED_HDR_BLK_TAG, offset:0x%x, len:0x%x\n", DataBlkOfs, DataBlkLen);
                                break;
                            }
                        }
                        break;

                        case EDID_CEA_EXTENDED_VSADB_TAG:
                        {
                            RetCode = EdidParseVSADBlk(&pDataBuf[DataBlkOfs],DataBlkLen,
                                                              &pCEAExtnBlk->DolbyAudioVSADB);
                            if (RetCode != EDID_ERR_OK)
                            {
                                EDID_PRINTF("ERROR: Parse EDID_CEA_EXTENDED_VSADB_TAG, offset:0x%x, len:0x%x\n", DataBlkOfs, DataBlkLen);
                                break;
                            }
                        }
                        break;

                        case EDID_CEA_EXTENDED_SCDB_BLK_TAG:
                        {
                            if (DupHfscdb == 1)
                                EDID_PRINTF("ERROR: HFVSDB block already parsed\n");
                            RetCode = ParseHFSCDB(&pDataBuf[DataBlkOfs],DataBlkLen,
                                                    &pCEAExtnBlk->CeaHdmiVSDB);
                            if(RetCode != EDID_ERR_OK)
                            {
                                EDID_PRINTF("ERROR: Parse EDID_CEA_EXTENDED_SCDB_BLK_TAG, offset:0x%x, len:0x%x\n", DataBlkOfs, DataBlkLen);
                                break;
                            }
                            DupHfscdb = 1;
                        }
                        break;

                        default:
                            break;
                    }
                }
                break;

                default:
                    break;
            }

            // Update offset
            DataBlkOfs += (DataBlkLen+1);
            if ((DtdOfs != 0) && (DataBlkOfs >= DtdOfs))
            {
                EndOfDataBlk = 1;
            }
        }
    }

    // Parse DTDs if any
    if (SvdFound == 0 /* FALSE */ && DtdOfs != 0)
    {
        // Read DTDs
        while (pDataBuf[DtdOfs] != 0x00)
        {
            EdidParseDTD(&pDataBuf[DtdOfs],
                                &pCEAExtnBlk->NumSpprtedVidFmts,
                                &pCEAExtnBlk->VideoResInfo[pCEAExtnBlk->NumSpprtedVidFmts],
                                &pCEAExtnBlk->HImgSize, &pCEAExtnBlk->VImgSize);
            DtdOfs += EDID_DTD_LEN;
        }
    }
    if(*(unsigned char*)(&pCEAExtnBlk->HDRMetadataInfo) != 0x0)
    {
       pCEAExtnBlk->HdrCapInfo.isHDR10Supported = (pCEAExtnBlk->HDRMetadataInfo.EOTF_ET_2)?1:0;
       pCEAExtnBlk->HdrCapInfo.isHLGSupported   = (pCEAExtnBlk->HDRMetadataInfo.EOTF_ET_3)?1:0;
    }
    if(*(unsigned char*)(&pCEAExtnBlk->DolbyVisionVSVDB)!=0xFF)
    {
        pCEAExtnBlk->HdrCapInfo.isDVSupported = 1;
    }

    EDID_PRINTF("HDR[%d] HDR10+[%d] HLG[%d] DV[%d]\n",
                        pCEAExtnBlk->HdrCapInfo.isHDR10Supported,
                        pCEAExtnBlk->HdrCapInfo.isHDR10PlusSupported,
                        pCEAExtnBlk->HdrCapInfo.isHLGSupported,
                        pCEAExtnBlk->HdrCapInfo.isDVSupported);
#if defined (DEBUG_EDID_PARSE)
    /* Print EDID contents */
    DumpCEAExtnBlk (pCEAExtnBlk);
#endif
    return EDID_ERR_OK;
}

// Short Video Descriptor
static EDID_RETURNS EdidParseSVD(
                        unsigned char *pDataBuf,
                        unsigned char SvdLen,
                        unsigned char *pEdidResCnt,
                        PCEA_RES_INFO pEdidResInfo,
                        unsigned char *pPrefTimingEntry)
{
    unsigned char   DataOfs = 0;
    unsigned char   VideoIDCode;
    unsigned char   SVD_code = 0;
    BOOL            NativeCode = FALSE;
    // Skip the header
    DataOfs++;
    while (DataOfs <= SvdLen)
    {
        NativeCode = FALSE;

        // Parse SVD
        SVD_code = pDataBuf[DataOfs];
        DataOfs++;

        // Interpret SVD according to CEA-861-F
        if (SVD_code == 0)
        {
            continue;
        }
        else if ((SVD_code >= 1) && (SVD_code <= 64))
        {
            VideoIDCode = SVD_code & 0x7F;  // not native code
        }
        else if ((SVD_code >= 65) && (SVD_code <= 127))
        {
            VideoIDCode = SVD_code; // 8bit vic
        }
        else if (SVD_code == 128)
        {
            continue; //reserved;
        }
        else if ((SVD_code >= 129) && (SVD_code <= 192))
        {
            VideoIDCode = SVD_code & 0x7F;  // native code
            NativeCode = TRUE;
        }
        else if ((SVD_code >= 193) && (SVD_code <= 253))
        {
            VideoIDCode = SVD_code; // 8bit vic;
        }
        else if (SVD_code == 254)
        {
            continue; //reserved;
        }
        else if (SVD_code == 255)
        {
            continue; //reserved;
        }
        else
            continue;

        if (!IsVideoIDCodeDefined(VideoIDCode))
        {
            // Don't add it
            continue;
        }

        pEdidResInfo->VideoIDCode = VideoIDCode;
        GetResForVideoID(pEdidResInfo);

        if (NativeCode) {
            *pPrefTimingEntry = *pEdidResCnt;
        }
        (*pEdidResCnt)++;
        pEdidResInfo++;
    }
    return EDID_ERR_OK;
}

static EDID_RETURNS EdidParseSAD(
                        unsigned char *pDataBuf,
                        unsigned char  SadLen,
                        PEDID_CEA_TIMING_EXTN_BLK pCEAExtnBlk)
{
    unsigned char       DataOfs = 0;
    EDID_RETURNS        RetCode;
    PEDID_AUDIO_INFO    pAudioInfoElem;

    // Skip the header
    DataOfs++;
    RetCode = EDID_ERR_OK;

    while (DataOfs <= SadLen)
    {
        if (pCEAExtnBlk->NumSprtedAudFmts > 14)
        {
            RetCode = EDID_ERR_INVALID_BLK_DATA;
            break;
        }
        pAudioInfoElem = &pCEAExtnBlk->AudioInfo[pCEAExtnBlk->NumSprtedAudFmts];

        // Get Audio format
        pAudioInfoElem->AudioFmt    = (pDataBuf[DataOfs] & 0x78) >> 3;
        if ((pAudioInfoElem->AudioFmt >= EDID_AUD_FMT_MAX) ||
            (pAudioInfoElem->AudioFmt == EDID_AUD_FMT_RESERVED))
        {
            // Skip this audio descriptor - not a valid format
            DataOfs += 3; // Length of SAD is 3
            continue;
        }

        // Get max number of channels supported for the format
        pAudioInfoElem->MaxNumChnls = (pDataBuf[DataOfs] & 0x07)+1;
        DataOfs++;

        // Get supported frequencies
        pAudioInfoElem->Frequency = pDataBuf[DataOfs] << 1;
        DataOfs++;

        // Get supported bit rates
        if (pAudioInfoElem->AudioFmt == EDID_AUD_FMT_LPCM)
        {
            // Uncompressed audio format
            pAudioInfoElem->AudioRate.UnComprAudRate.Sprt16Bps =
                    IS_BIT_SET(pDataBuf[DataOfs],0);
            pAudioInfoElem->AudioRate.UnComprAudRate.Sprt20Bps =
                    IS_BIT_SET(pDataBuf[DataOfs],1);
            pAudioInfoElem->AudioRate.UnComprAudRate.Sprt24Bps =
                    IS_BIT_SET(pDataBuf[DataOfs],2);
        }
        else if (pAudioInfoElem->AudioFmt >= EDID_AUD_FMT_AC3 &&
                 pAudioInfoElem->AudioFmt <= EDID_AUD_FMT_ATRAC)
        {
            pAudioInfoElem->AudioRate.CompAudRate = pDataBuf[DataOfs]*8;
        }
        else
        {
            // Compressed audio format
            pAudioInfoElem->AudioRate.CompAudRate = pDataBuf[DataOfs];
        }
        DataOfs++;
        pCEAExtnBlk->NumSprtedAudFmts++;
    }
    return RetCode;
}

static EDID_RETURNS EdidParseSpkrAllocBlk(
                        unsigned char *pDataBuf,
                        unsigned char  BlkLen,
                        PEDID_SPKR_ALLOC_FLD pSpkrAllocFld)
{
    EDID_SPKR_ALLOC     SpkrAlloc;
    if (BlkLen != 3)
    {
        return EDID_ERR_INVALID_BLK_DATA;
    }

    // Skip header
    pDataBuf++;
    SpkrAlloc.Value = *pDataBuf;
    *pSpkrAllocFld = SpkrAlloc.Fld;

    return EDID_ERR_OK;
}

static EDID_RETURNS EdidParseVSDB(
                        unsigned char *pDataBuf,
                        unsigned char  VsdbLen,
                        PEDID_CEA_TIMING_EXTN_BLK pCEAExtnBlk)
{
    UINT8 pos=0, i=0, hdmi_3d_len=0;

    // Check for HDMI registration identifier
    if (!((pDataBuf[0x1] == 0x03) && (pDataBuf[0x2] == 0x0C) && (pDataBuf[0x3] == 0x00)))
    {
        // Parsing of this vendor specific block is not supported
        return EDID_ERR_UNSUPPORTED_BLK;
    }

    // Check Length
    if (VsdbLen < 5)
    {
        return EDID_ERR_INVALID_BLK_DATA;
    }

    pCEAExtnBlk->HdmiMode = TRUE;

    // Get source physical address (CEC)
    pCEAExtnBlk->CecPhyAddr = (pDataBuf[0x4] << 8) | pDataBuf[0x5];

    // Parse extension fields until given VsdbLen
    /* Note:
     * Extension fields not required now are commented - to be used if and
     * when required
     */
    if(VsdbLen > 5)
    {
        pos=6;
        if(VsdbLen >= pos)
        {
            pCEAExtnBlk->FeatureSprt.SupportsAI = IS_BIT_SET(pDataBuf[pos],7);
            pCEAExtnBlk->FeatureSprt.DC48bit = IS_BIT_SET(pDataBuf[pos],6);
            pCEAExtnBlk->FeatureSprt.DC36bit = IS_BIT_SET(pDataBuf[pos],5);
            pCEAExtnBlk->FeatureSprt.DC30bit = IS_BIT_SET(pDataBuf[pos],4);
            pCEAExtnBlk->FeatureSprt.YCbCr444AtDC = IS_BIT_SET(pDataBuf[pos],3);
        }
        // DVIDualSprt = IS_BIT_SET(pDataBuf[6],0);
        pos=7;
        pCEAExtnBlk->MaxTMDSClock = pDataBuf[7] * 5 /* in MHz */;

        pos=8;
        if(VsdbLen >= pos)
        {
            pCEAExtnBlk->HdmiLatencyFields.Latency_Fields_Present = IS_BIT_SET(pDataBuf[pos],7);
            pCEAExtnBlk->HdmiLatencyFields.I_Latency_Fields_Present = IS_BIT_SET(pDataBuf[pos],6);
            pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_Video_Present = IS_BIT_SET(pDataBuf[pos],5);

            pos ++;
            if(pCEAExtnBlk->HdmiLatencyFields.Latency_Fields_Present) //Latency_Present
            {
                pCEAExtnBlk->HdmiLatencyFields.Video_Latency = pDataBuf[pos];
                pos ++;
                pCEAExtnBlk->HdmiLatencyFields.Audio_Latency = pDataBuf[pos];
                pos ++;

                if(pCEAExtnBlk->HdmiLatencyFields.I_Latency_Fields_Present) //Interlaced_Latency_Present
                {
                    pCEAExtnBlk->HdmiLatencyFields.Interlaced_Video_Latency = pDataBuf[pos];
                    pos ++;
                    pCEAExtnBlk->HdmiLatencyFields.Interlaced_Audio_Latency = pDataBuf[pos];
                    pos ++;
                }
            }
        }

        if(VsdbLen >= pos)
        {
            if(pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_Video_Present) //Video_Present
            {
                pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_3D_Present = IS_BIT_SET(pDataBuf[pos],7);
                pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_3D_Multi_Present = (pDataBuf[pos] & 0x60)>>5;
                        pos++;
                    pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_VIC_Len = (pDataBuf[pos] >> 5) & 0x07;
                    pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_3D_Len = (pDataBuf[pos] & 0x1F);
                        pos++;
            }
        }

        if(VsdbLen >= pos)
        {
            for (i=0; i<pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_VIC_Len; i++) {
                pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_VIC[i] = pDataBuf[pos];
                pos++;
                        }
            hdmi_3d_len = pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_3D_Len;
            if(hdmi_3d_len > 0)
            {
                if(pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_3D_Multi_Present == 0x01) //assigns 3D formats to all of the VICs listed
                {                                                                //first 16 entries in the EDID
                    pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All = (pDataBuf[pos] << 8);
                    pos++;
                    hdmi_3d_len--;
                    pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All |= (unsigned short)pDataBuf[pos];
                    pos++;
                    hdmi_3d_len--;
                }
                else if(pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_3D_Multi_Present == 0x02)
                {
                    pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All = (pDataBuf[pos] << 8);
                    pos++;
                    hdmi_3d_len--;
                    pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All |= (unsigned short)pDataBuf[pos];
                    pos++;
                    hdmi_3d_len--;
                    pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_3D_Mask = (pDataBuf[pos] << 8);
                    pos++;
                    hdmi_3d_len--;
                    pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_3D_Mask |= (unsigned short)pDataBuf[pos];
                    pos++;
                    hdmi_3d_len--;
                }

                i=0;
                while(hdmi_3d_len > 0)
                {
                    pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_2D_VIC_Order[i]= (((pDataBuf[pos] & 0xF0) >> 4)) /*-1*/; // probably the 1st VIC in EDID is 640x480 (mandatory) in ETD
                    pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure[i]= (pDataBuf[pos] & 0x0F);
                    pos++;
                    hdmi_3d_len--;
                    if(pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure[i] >= VPP_HDMI_3D_STRUCT_SIDE_BY_SIDE_HALF)
                    {
                        pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_3D_Detail[i]= ((pDataBuf[pos] & 0xF0) >> 4);
                        pos++;
                            hdmi_3d_len--;
                    }
                    i++;
                }
                pCEAExtnBlk->Hdmi3DFeatureSprt.Hdmi_2D_VIC_Num = i;
            }
        }
    }

    return EDID_ERR_OK;
}


static EDID_RETURNS EdidParseCalorimetryBlk(
                        unsigned char *pDataBuf,
                        unsigned char  BlkLen,
                        PEDID_CALORIMETRY_INFO pCalInfo)
{
    // Skip header fields
    pDataBuf += 2;

    if (BlkLen < 3)
    {
        return (EDID_ERR_INVALID_BLK_DATA);
    }

    pCalInfo->xvYCC601  = IS_BIT_SET(pDataBuf[0],0);
    pCalInfo->xvYCC709  = IS_BIT_SET(pDataBuf[0],1);
    pCalInfo->sYCC601   = IS_BIT_SET(pDataBuf[0],2);
    pCalInfo->AdobeYCC601 = IS_BIT_SET(pDataBuf[0],3);
    pCalInfo->AdobeRGB    = IS_BIT_SET(pDataBuf[0],4);
    pCalInfo->BT2020RGB   = IS_BIT_SET(pDataBuf[0],5);
    pCalInfo->BT2020YCC   = IS_BIT_SET(pDataBuf[0],6);
    pCalInfo->BT2020cYCC  = IS_BIT_SET(pDataBuf[0],7);
    pCalInfo->MD0       = IS_BIT_SET(pDataBuf[1],0);
    pCalInfo->MD1       = IS_BIT_SET(pDataBuf[1],1);
    pCalInfo->MD2       = IS_BIT_SET(pDataBuf[1],2);
    pCalInfo->MD3       = IS_BIT_SET(pDataBuf[1],3);

    return EDID_ERR_OK;
}

/*-----------------------------------------------------------------------------
 * Block Map Parsing Functions
 *-----------------------------------------------------------------------------
 */
static EDID_RETURNS ParseBlkMap(
                        unsigned char *pDataBuf,
                        PEDID_BLOCK_MAP_DATA pBlkMap)
{
    unsigned char i;

    pBlkMap->CeaExtnBlkNum = 0;
    for (i = pBlkMap->CurBlkNum; i < 126; i++)
    {
        if (pDataBuf[i] == EDID_CEA_EXTN_TAG)
        {
            pBlkMap->CeaExtnBlkNum = i+1;
            break;
        }
    }
    return EDID_ERR_OK;
}

/*-----------------------------------------------------------------------------
 * Common Functions
 *-----------------------------------------------------------------------------
 */
static EDID_RETURNS CheckCRC (
                        unsigned char *pDataBuf)
{
    unsigned char       Count;
    unsigned char       CheckSum;
    EDID_RETURNS        RetCode;

    CheckSum = 0;
    for (Count = 0; Count < SIZE_OF_EDID_BLK; Count++)
    {
        // One byte checksum
        CheckSum += pDataBuf[Count];
    }

    RetCode = (CheckSum == 0)?EDID_ERR_OK:EDID_ERR_CRC_FAILED;
    return RetCode;
}

#if defined (DEBUG_EDID_PARSE)
static char *SepLine = "------------------------------------------------------------\n";

#define PRINT_HEADING(Str) \
    EDID_PRINTF (SepLine); \
    EDID_PRINTF ("%s\n", Str); \
    EDID_PRINTF (SepLine);

#define PRINT_TITLE(Str) \
    EDID_PRINTF ("\n************************************************************\n"); \
    EDID_PRINTF ("%s\n", Str); \
    EDID_PRINTF ("************************************************************\n\n");

static void DumpBasicBlk(PEDID_BASIC_BLK_DATA pBasicBlk)
{
    char str1[15] = "Enabled";
    char str2[15] = "Not Enabled";
    char str3[40];
    unsigned char i;
    float gamma;

    if (pBasicBlk == NULL)
    {
        return;
    }

    PRINT_TITLE("START OF BASIC BLOCK");

    // Print Product Information
    PRINT_HEADING("Version Info");
    EDID_PRINTF ("Edid Version: %d\n" ,  pBasicBlk->GenInfo.Version);
    EDID_PRINTF ("Edid Revision: %d\n" , pBasicBlk->GenInfo.Revision);

    PRINT_HEADING("Product Information");
    EDID_PRINTF ("Manufacturer Name: %s\n" , pBasicBlk->GenInfo.MnfrName);

    EDID_PRINTF ("Product Code: %s\n",pBasicBlk->GenInfo.ProductCode);
    EDID_PRINTF ("Serial Number: %d\n" , pBasicBlk->GenInfo.SerialNum);
    EDID_PRINTF ("Week of Manufacture: %d\n" , pBasicBlk->GenInfo.MnfrWeek);
    EDID_PRINTF ("Year of Manufacture: %d\n" , pBasicBlk->GenInfo.MnfrYear);

    // Print Basic Display Parameters
    PRINT_HEADING("Basic Display Parameters");
    EDID_PRINTF ("Video Input: %s\n" ,pBasicBlk->DispParams.DigVideoInput?"Digital":"Analog");
    if (pBasicBlk->DispParams.DigVideoInput)
    {
        EDID_PRINTF ("DFP 1.X %s\n" ,pBasicBlk->DispParams.VideoInpDet.DigVidInp.DFP1XEnabled?
                        str1:str2);
    }
    EDID_PRINTF ("Max Horizontal Image Size: %d cm\n" ,pBasicBlk->DispParams.MaxHImageSize*10);
    EDID_PRINTF ("Maximum Vertical Image Size: %d cm\n" ,pBasicBlk->DispParams.MaxVImageSize*10);
    gamma = (float)(pBasicBlk->DispParams.Gamma + 100)/100;
    EDID_PRINTF ("Gamma: %3f\n\n" ,gamma);

    // Feature support
    EDID_PRINTF ("GTF support %s\n",
        pBasicBlk->DispParams.FeatureSprt.GTFSprt?str1:str2);
    EDID_PRINTF ("Preferred timing support %s\n",
        pBasicBlk->DispParams.FeatureSprt.PrefTiming?str1:str2);
    EDID_PRINTF ("sRGB support %s\n",
        pBasicBlk->DispParams.FeatureSprt.sRGBSprt?str1:str2);
    EDID_PRINTF ("Suspend support %s\n",
        pBasicBlk->DispParams.FeatureSprt.ActiveOffSprt?str1:str2);
    EDID_PRINTF ("Standby support %s\n",
        pBasicBlk->DispParams.FeatureSprt.StandbySprt?str1:str2);

    // Display Type
    switch (pBasicBlk->DispParams.FeatureSprt.DispType)
    {
        case EDID_MONOCHROME_DISP:
            sprintf (str3,"Monochrome display");
            break;
        case EDID_RGB_DISP:
            sprintf (str3,"RGB display");
            break;
        case EDID_NONRGB_DISP:
            sprintf (str3,"Non RGB display");
            break;
        default:
            sprintf (str3,"Undefined display");
            break;
    }
    EDID_PRINTF ("Display Type: %s\n", str3);

    // Color Characteristics
    EDID_PRINTF("Rx: %d\n",pBasicBlk->DispParams.ColorChar.Rx);
    EDID_PRINTF("Ry: %d\n",pBasicBlk->DispParams.ColorChar.Ry);
    EDID_PRINTF("Gx: %d\n",pBasicBlk->DispParams.ColorChar.Gx);
    EDID_PRINTF("Gy: %d\n",pBasicBlk->DispParams.ColorChar.Gy);
    EDID_PRINTF("Bx: %d\n",pBasicBlk->DispParams.ColorChar.Bx);
    EDID_PRINTF("By: %d\n",pBasicBlk->DispParams.ColorChar.By);
    EDID_PRINTF("Wx: %d\n",pBasicBlk->DispParams.ColorChar.Wx);
    EDID_PRINTF("Wy: %d\n",pBasicBlk->DispParams.ColorChar.Wy);

    // Print Monitor Information
    PRINT_HEADING("Monitor Info");
    EDID_PRINTF("Monitor Name: %s\n",pBasicBlk->MonitorInfo.MonitorName);
    EDID_PRINTF("Vertical Rate Range: %d - %d Hz\n",
        pBasicBlk->MonitorInfo.MinVRate, pBasicBlk->MonitorInfo.MaxVRate);
    EDID_PRINTF("Horizontal Rate Range: %d - %d Hz\n",
        pBasicBlk->MonitorInfo.MinHRate, pBasicBlk->MonitorInfo.MaxHRate);
    EDID_PRINTF("Pixel Clock: %d MHz\n",pBasicBlk->MonitorInfo.MaxPixelClock*10);

    // Print supported video resolution
    PRINT_HEADING("Supported Video Resolution");
    for (i = 0; i < pBasicBlk->NumSpprtedVidFmts; i++)
    {
        EDID_PRINTF ("Resolution %d\n", i+1);
        EDID_PRINTF ("--------------------\n");

        if (pBasicBlk->VideoResInfo[i].VideoIDCode != 0)
            EDID_PRINTF("VideoIDCode: %d\n",pBasicBlk->VideoResInfo[i].VideoIDCode);

        if (pBasicBlk->VideoResInfo[i].HActive != RES_INFO_UNDEF)
            EDID_PRINTF("HActive: %d\n",pBasicBlk->VideoResInfo[i].HActive);

        if (pBasicBlk->VideoResInfo[i].VActive != RES_INFO_UNDEF)
            EDID_PRINTF("VActive: %d\n",pBasicBlk->VideoResInfo[i].VActive);

        if (pBasicBlk->VideoResInfo[i].RefreshRate != RES_INFO_UNDEF)
            EDID_PRINTF("Refresh Rate: %d Hz\n",pBasicBlk->VideoResInfo[i].RefreshRate);

        if (pBasicBlk->VideoResInfo[i].FrmFmt != RES_INFO_FRM_FMT_UNDEF)
            EDID_PRINTF("Format: %s\n",(pBasicBlk->VideoResInfo[i].FrmFmt?"Interlaced":"Progressive"));

        EDID_PRINTF ("--------------------\n");
    }

    // Print number of extension blocks
    PRINT_HEADING("Extension Info");
    EDID_PRINTF ("Number of extension blocks: %d\n", pBasicBlk->NumExtns);
    PRINT_TITLE("END OF BASIC BLOCK");
}

static void DumpCEAExtnBlk(PEDID_CEA_TIMING_EXTN_BLK pCeaBlk)
{
    char str1[15] = "Enabled";
    char str2[15] = "Not Enabled";
    char str3[40];
    unsigned char i;

    if (pCeaBlk == NULL)
    {
        return;
    }

    PRINT_TITLE("START OF CEA EXTENSION BLOCK");

    EDID_PRINTF ( "HDMI / DVI mode: %d\n", pCeaBlk->HdmiMode);
    EDID_PRINTF ( "Extention block count: %d\n", pCeaBlk->ExtBlkCnt);
    // Dump CEA Block specified feature support
    PRINT_HEADING("CEA Block specified feature support");
    EDID_PRINTF ( "UnderScan support %s\n",
        pCeaBlk->FeatureSprt.UnderScanSprt?str1:str2);
    EDID_PRINTF ( "YCbCr 4:2:2 support %s\n",
        pCeaBlk->FeatureSprt.YCbCr422Sprt?str1:str2);
    EDID_PRINTF ( "YCbCr 4:4:4 support %s\n",
        pCeaBlk->FeatureSprt.YCbCr444Sprt?str1:str2);
    EDID_PRINTF ( "YCbCr 4:4:4 at Deep-color support %s\n",
        pCeaBlk->FeatureSprt.YCbCr444AtDC?str1:str2);
    EDID_PRINTF ( "30-bit Deep-color support %s\n",
        pCeaBlk->FeatureSprt.DC30bit?str1:str2);
    EDID_PRINTF ( "36-bit Deep-color support %s\n",
        pCeaBlk->FeatureSprt.DC36bit?str1:str2);
    EDID_PRINTF ( "48-bit Deep-color support %s\n",
        pCeaBlk->FeatureSprt.DC48bit?str1:str2);
    EDID_PRINTF ( "AI support %s\n",
        pCeaBlk->FeatureSprt.SupportsAI?str1:str2);
    EDID_PRINTF ( "Max TMDS Clock %d\n",
        pCeaBlk->MaxTMDSClock);

    if (pCeaBlk->CalorimetryInfo.xvYCC601 || pCeaBlk->CalorimetryInfo.xvYCC709 || pCeaBlk->CalorimetryInfo.sYCC601 || pCeaBlk->CalorimetryInfo.AdobeYCC601 || \
          pCeaBlk->CalorimetryInfo.AdobeRGB || pCeaBlk->CalorimetryInfo.BT2020RGB || pCeaBlk->CalorimetryInfo.BT2020YCC || pCeaBlk->CalorimetryInfo.BT2020cYCC)
    {
        EDID_PRINTF ("x.v.YCC 601 %s\n", (pCeaBlk->CalorimetryInfo.xvYCC601?"supported":"not support"));
        EDID_PRINTF ("x.v.YCC 709 %s\n", (pCeaBlk->CalorimetryInfo.xvYCC709?"supported":"not support"));
        EDID_PRINTF ("s.YCC 601 %s\n", (pCeaBlk->CalorimetryInfo.sYCC601?"supported":"not support"));
        EDID_PRINTF ("Adobe YCC 601 %s\n", (pCeaBlk->CalorimetryInfo.AdobeYCC601?"supported":"not support"));
        EDID_PRINTF ("Adobe RGB %s\n", (pCeaBlk->CalorimetryInfo.AdobeRGB?"supported":"not support"));
        EDID_PRINTF ("BT.2020 RGB %s\n", (pCeaBlk->CalorimetryInfo.BT2020RGB?"supported":"not support"));
        EDID_PRINTF ("BT.2020 YCC %s\n", (pCeaBlk->CalorimetryInfo.BT2020YCC?"supported":"not support"));
        EDID_PRINTF ("BT.2020 cYCC %s\n", (pCeaBlk->CalorimetryInfo.BT2020cYCC?"supported":"not support"));
    }
    else
    {
        EDID_PRINTF ("Extended Calorimetry no support\n");
    }

    if(pCeaBlk->CalorimetryInfo.MD0 || pCeaBlk->CalorimetryInfo.MD1 || pCeaBlk->CalorimetryInfo.MD2 || pCeaBlk->CalorimetryInfo.MD3)
    {
        EDID_PRINTF ("MD0 %s\n", (pCeaBlk->CalorimetryInfo.MD0?"supported":"not support"));
        EDID_PRINTF ("MD1 %s\n", (pCeaBlk->CalorimetryInfo.MD1?"supported":"not support"));
        EDID_PRINTF ("MD2 %s\n", (pCeaBlk->CalorimetryInfo.MD2?"supported":"not support"));
        EDID_PRINTF ("MD3 %s\n", (pCeaBlk->CalorimetryInfo.MD3?"supported":"not support"));
    }
    else
    {
        EDID_PRINTF ("Meta Data Profile no support\n");
    }

    EDID_PRINTF ("----RGB Quantization QS %s\n", (pCeaBlk->CapabilityInfo.QS?"supported":"not support"));
    EDID_PRINTF ("----RGB Quantization QY %s\n", (pCeaBlk->CapabilityInfo.QY?"supported":"not support"));
    EDID_PRINTF("HDMI 1.4 video capability support %s\n", pCeaBlk->Hdmi3DFeatureSprt.Hdmi_Video_Present?str1:str2);
    if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_Video_Present) {
        EDID_PRINTF("    3D video support %s \n", pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Present?str1:str2);
        if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Len) {
            EDID_PRINTF("    Additional 3D formats supported:\n");
            if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Multi_Present == 0x01) {
                EDID_PRINTF("    Assign 3D_Structure_ALL to all first 16 VICs\n");
                for (i=pCeaBlk->FirstSVDEntry; i<pCeaBlk->LastSVDEntry; i++) {
                    EDID_PRINTF("        VideoIDCode: %d\n",pCeaBlk->VideoResInfo[i].VideoIDCode);
                    EDID_PRINTF("        HActive: %d\n",pCeaBlk->VideoResInfo[i].HActive);
                    EDID_PRINTF("        VActive: %d\n",pCeaBlk->VideoResInfo[i].VActive);
                    EDID_PRINTF("        Refresh Rate: %d Hz\n",pCeaBlk->VideoResInfo[i].RefreshRate);
                    EDID_PRINTF("        Format: %s\n",(pCeaBlk->VideoResInfo[i].FrmFmt?"Interlaced":"Progressive"));
                    EDID_PRINTF("        3D Structures: \n");
                    if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All & 0x01)
                        EDID_PRINTF("            Frame packing\n");
                    if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All & 0x02)
                        EDID_PRINTF("            Field  alternative\n");
                    if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All & 0x04)
                        EDID_PRINTF("            Line alternative\n");
                    if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All & 0x08)
                        EDID_PRINTF("            Side-by-Side Full\n");
                    if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All & 0x10)
                        EDID_PRINTF("            L + depth\n");
                    if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All & 0x20)
                        EDID_PRINTF("            L + depth + graphics + graphics-depth\n");
                    if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All & 0x40)
                        EDID_PRINTF("            Top-and-Bottom\n");
                    if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All & 0x0100)
                        EDID_PRINTF("            Side-by-Side Half with all sub-sampling methods\n");
                }
            } else if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Multi_Present == 0x02) {
                EDID_PRINTF("    Assign 3D_Structure_ALL to some of first 16 VICs\n");
                for (i=pCeaBlk->FirstSVDEntry; i<pCeaBlk->LastSVDEntry; i++) {
                    if (!(pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Mask & (1<<(i-pCeaBlk->FirstSVDEntry))))
                        continue;
                    EDID_PRINTF("        VideoIDCode: %d\n",pCeaBlk->VideoResInfo[i].VideoIDCode);
                    EDID_PRINTF("        HActive: %d\n",pCeaBlk->VideoResInfo[i].HActive);
                    EDID_PRINTF("        VActive: %d\n",pCeaBlk->VideoResInfo[i].VActive);
                    EDID_PRINTF("        Refresh Rate: %d Hz\n",pCeaBlk->VideoResInfo[i].RefreshRate);
                    EDID_PRINTF("        Format: %s\n",(pCeaBlk->VideoResInfo[i].FrmFmt?"Interlaced":"Progressive"));
                    EDID_PRINTF("        3D Structures: \n");
                    if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All & 0x01)
                        EDID_PRINTF("            Frame packing\n");
                    if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All & 0x02)
                        EDID_PRINTF("            Field  alternative\n");
                    if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All & 0x04)
                        EDID_PRINTF("            Line alternative\n");
                    if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All & 0x08)
                        EDID_PRINTF("            Side-by-Side Full\n");
                    if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All & 0x10)
                        EDID_PRINTF("            L + depth\n");
                    if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All & 0x20)
                        EDID_PRINTF("            L + depth + graphics + graphics-depth\n");
                    if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All & 0x40)
                        EDID_PRINTF("            Top-and-Bottom\n");
                    if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure_All & 0x0100)
                        EDID_PRINTF("            Side-by-Side Half with all sub-sampling methods\n");
                }
            }

            for (i=0; i<pCeaBlk->Hdmi3DFeatureSprt.Hdmi_2D_VIC_Num; i++) {
                EDID_PRINTF("    Assign special 3D structure to special VICs\n");
                EDID_PRINTF("        VideoIDCode: %d\n",pCeaBlk->VideoResInfo[pCeaBlk->FirstSVDEntry+pCeaBlk->Hdmi3DFeatureSprt.Hdmi_2D_VIC_Order[i]].VideoIDCode);
                EDID_PRINTF("        HActive: %d\n",pCeaBlk->VideoResInfo[pCeaBlk->FirstSVDEntry+pCeaBlk->Hdmi3DFeatureSprt.Hdmi_2D_VIC_Order[i]].HActive);
                EDID_PRINTF("        VActive: %d\n",pCeaBlk->VideoResInfo[pCeaBlk->FirstSVDEntry+pCeaBlk->Hdmi3DFeatureSprt.Hdmi_2D_VIC_Order[i]].VActive);
                EDID_PRINTF("        Refresh Rate: %d Hz\n",pCeaBlk->VideoResInfo[pCeaBlk->FirstSVDEntry+pCeaBlk->Hdmi3DFeatureSprt.Hdmi_2D_VIC_Order[i]].RefreshRate);
                EDID_PRINTF("        Format: %s\n",(pCeaBlk->VideoResInfo[pCeaBlk->FirstSVDEntry+pCeaBlk->Hdmi3DFeatureSprt.Hdmi_2D_VIC_Order[i]].FrmFmt?"Interlaced":"Progressive"));
                EDID_PRINTF("        3D Structures: \n");
                switch (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Structure[i]) {
                    case  VPP_HDMI_3D_STRUCT_FRAME_PACKING:
                        EDID_PRINTF("            Frame packing\n"); break;
                    case  VPP_HDMI_3D_STRUCT_FIELD_ALTERNATIVE:
                        EDID_PRINTF("            Field  alternative\n"); break;
                    case  VPP_HDMI_3D_STRUCT_LINE_ALTERNATIVE:
                        EDID_PRINTF("            Line alternative\n"); break;
                    case  VPP_HDMI_3D_STRUCT_SIDE_BY_SIDE_FULL:
                        EDID_PRINTF("            Side-by-Side Full\n"); break;
                    case  VPP_HDMI_3D_STRUCT_L_DEPTH:
                        EDID_PRINTF("            L + depth\n"); break;
                    case  VPP_HDMI_3D_STRUCT_L_DEPTH_GFX_GDEPTH:
                        EDID_PRINTF("            L + depth + graphics + graphics-depth\n"); break;
                    case  VPP_HDMI_3D_STRUCT_TOP_AND_BOTTOM:
                        EDID_PRINTF("            Top-and-Bottom\n"); break;
                    case  VPP_HDMI_3D_STRUCT_SIDE_BY_SIDE_HALF:
                        EDID_PRINTF("            Side-by-Side Half: %d\n", pCeaBlk->Hdmi3DFeatureSprt.Hdmi_3D_Detail[i]); break;
                    default:
                        EDID_PRINTF("            Undefined\n"); break;
                }
            }
        }
        EDID_PRINTF("    4kx2k video format support %s \n", pCeaBlk->Hdmi3DFeatureSprt.Hdmi_VIC_Len?str1:str2);
        if (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_VIC_Len) {
            for (i=0; i<pCeaBlk->Hdmi3DFeatureSprt.Hdmi_VIC_Len; i++) {
                switch (pCeaBlk->Hdmi3DFeatureSprt.Hdmi_VIC[i]) {
                    case  VPP_HDMI_VIC_4K_2K_30:
                        EDID_PRINTF("        4k x 2k @ 30Hz\n"); break;
                    case  VPP_HDMI_VIC_4K_2K_25:
                        EDID_PRINTF("        4k x 2k @ 25Hz\n"); break;
                    case  VPP_HDMI_VIC_4K_2K_24:
                        EDID_PRINTF("        4k x 2k @ 24Hz\n"); break;
                    case  VPP_HDMI_VIC_4K_2K_24_SMPTE:
                        EDID_PRINTF("        4k x 2k @ 24Hz SMPTE\n"); break;
                    default:
                        EDID_PRINTF("        unknown\n"); break;
                }
            }
        }
    }

    // Print supported video resolution
    PRINT_HEADING("Supported Video Resolutions");
    for (i = 0; i < pCeaBlk->NumSpprtedVidFmts; i++)
    {
        EDID_PRINTF ("Resolution %d\n", i+1);
        EDID_PRINTF ("--------------------\n");

        if (pCeaBlk->VideoResInfo[i].VideoIDCode != 0)
            EDID_PRINTF("VideoIDCode: %d\n",pCeaBlk->VideoResInfo[i].VideoIDCode);

        if (pCeaBlk->VideoResInfo[i].HActive != RES_INFO_UNDEF)
            EDID_PRINTF("HActive: %d\n",pCeaBlk->VideoResInfo[i].HActive);

        if (pCeaBlk->VideoResInfo[i].VActive != RES_INFO_UNDEF)
            EDID_PRINTF("VActive: %d\n",pCeaBlk->VideoResInfo[i].VActive);

        if (pCeaBlk->VideoResInfo[i].RefreshRate != RES_INFO_UNDEF)
            EDID_PRINTF("Refresh Rate: %d Hz\n",pCeaBlk->VideoResInfo[i].RefreshRate);

        if (pCeaBlk->VideoResInfo[i].FrmFmt != RES_INFO_FRM_FMT_UNDEF)
            EDID_PRINTF("Format: %s\n",(pCeaBlk->VideoResInfo[i].FrmFmt?"Interlaced":"Progressive"));

        EDID_PRINTF ("--------------------\n");
    }

    // Print audio information
    PRINT_HEADING("Audio Support Info");
    EDID_PRINTF ( "Basic Audio support %s\n",pCeaBlk->BasicAudioSprt?str1:str2);
    for (i = 0; i < pCeaBlk->NumSprtedAudFmts; i++)
    {
        switch (pCeaBlk->AudioInfo[i].AudioFmt)
        {
            case EDID_AUD_FMT_LPCM:
                sprintf (str3,"LPCM");
                break;
            case EDID_AUD_FMT_AC3:
                sprintf (str3,"AC3");
                break;
            case EDID_AUD_FMT_MPEG1:
                sprintf (str3,"MPEG1");
                break;
            case EDID_AUD_FMT_MP3:
                sprintf (str3,"MP3");
                break;
            case EDID_AUD_FMT_MPEG2:
                sprintf (str3,"MPEG2");
                break;
            case EDID_AUD_FMT_AAC:
                sprintf (str3,"AAC");
                break;
            case EDID_AUD_FMT_DTS:
                sprintf (str3,"DTS");
                break;
            case EDID_AUD_FMT_ATRAC:
                sprintf (str3,"ATRAC");
                break;
            case EDID_AUD_FMT_ONE_BIT_AUDIO:
                sprintf (str3,"ONE BIT AUDIO");
                break;
            case EDID_AUD_FMT_DOLBY_DIGITAL:
                sprintf (str3,"DOLBY DIGITAL");
                break;
            case EDID_AUD_FMT_DTS_HD:
                sprintf (str3,"DTS HD");
                break;
            case EDID_AUD_FMT_MAT:
                sprintf (str3,"MAT (MLP)");
                break;
            case EDID_AUD_FMT_DST:
                sprintf (str3,"DST");
                break;
            case EDID_AUD_FMT_WMA_PRO:
                sprintf (str3,"WMA PRO");
                break;
            default:
                sprintf (str3,"Undefined");
                break;
        }
        EDID_PRINTF("Audio Format: %s\n", str3);
        EDID_PRINTF("Max number of channels: %d\n", pCeaBlk->AudioInfo[i].MaxNumChnls);
        EDID_PRINTF("Supported Audio Frequencies: ");
        if (IS_BIT_SET(pCeaBlk->AudioInfo[i].Frequency, CEA_AUDIO_SAMP_FREQ_32_KHZ))
            EDID_PRINTF("32 KHz ");
        if (IS_BIT_SET(pCeaBlk->AudioInfo[i].Frequency,CEA_AUDIO_SAMP_FREQ_44_1_KHZ))
            EDID_PRINTF("44.1 KHz ");
        if (IS_BIT_SET(pCeaBlk->AudioInfo[i].Frequency,CEA_AUDIO_SAMP_FREQ_48_KHZ))
            EDID_PRINTF("48 KHz ");
        if (IS_BIT_SET(pCeaBlk->AudioInfo[i].Frequency,CEA_AUDIO_SAMP_FREQ_88_2_KHZ))
            EDID_PRINTF("88.2 KHz ");
        if (IS_BIT_SET(pCeaBlk->AudioInfo[i].Frequency,CEA_AUDIO_SAMP_FREQ_96_KHZ))
            EDID_PRINTF("96 KHz ");
        if (IS_BIT_SET(pCeaBlk->AudioInfo[i].Frequency,CEA_AUDIO_SAMP_FREQ_176_4_KHZ))
            EDID_PRINTF("176.4 KHz ");
        if (IS_BIT_SET(pCeaBlk->AudioInfo[i].Frequency,CEA_AUDIO_SAMP_FREQ_192_KHZ))
            EDID_PRINTF("192 KHz ");
        EDID_PRINTF ( "\n");

        // Audio rates
        if (pCeaBlk->AudioInfo[i].AudioFmt == EDID_AUD_FMT_LPCM)
        {
            EDID_PRINTF ("Bit rates supported: ");
            if (pCeaBlk->AudioInfo[i].AudioRate.UnComprAudRate.Sprt16Bps)
                EDID_PRINTF ( "16 Bps ");
            if (pCeaBlk->AudioInfo[i].AudioRate.UnComprAudRate.Sprt20Bps)
                EDID_PRINTF ( "20 Bps ");
            if (pCeaBlk->AudioInfo[i].AudioRate.UnComprAudRate.Sprt24Bps)
                EDID_PRINTF ( "24 Bps ");
            EDID_PRINTF ( "\n");
        }
        else if (pCeaBlk->AudioInfo[i].AudioFmt >= EDID_AUD_FMT_AC3 &&
                 pCeaBlk->AudioInfo[i].AudioFmt <= EDID_AUD_FMT_WMA_PRO)
        {
            if (pCeaBlk->AudioInfo[i].AudioRate.CompAudRate != 0)
                EDID_PRINTF ("Bit rate (Vendor Specified): %d KHz\n",
                            pCeaBlk->AudioInfo[i].AudioRate.CompAudRate);
        }
    }

    // Print speaker allocation table
    if (*(unsigned char*)(&pCeaBlk->SpkrAlloc) != 0xFF)
    {
        EDID_PRINTF ("Front Left/Front Rear Speaker %s\n", (pCeaBlk->SpkrAlloc.FlFr?"Present":"Absent"));
        EDID_PRINTF ("Low Frequency Effect %s\n", (pCeaBlk->SpkrAlloc.Lfe?"Present":"Absent"));
        EDID_PRINTF ("Front Center Speaker %s\n", (pCeaBlk->SpkrAlloc.Fc?"Present":"Absent"));
        EDID_PRINTF ("Rear Left/Rear Right Speaker %s\n", (pCeaBlk->SpkrAlloc.RlRr?"Present":"Absent"));
        EDID_PRINTF ("Rear Center Speaker %s\n", (pCeaBlk->SpkrAlloc.Rc?"Present":"Absent"));
        EDID_PRINTF ("Front Left Center/Front Right Center Speaker %s\n", (pCeaBlk->SpkrAlloc.FlcFrc?"Present":"Absent"));
        EDID_PRINTF ("Rear Left Center /Rear Right Center Speaker %s\n", (pCeaBlk->SpkrAlloc.RlcRrc?"Present":"Absent"));
    }

    if(*(unsigned char*)(&pCeaBlk->HDRMetadataInfo) != 0x0)
    {
       EDID_PRINTF("  ............... HDR DATA BLOCK ...............\n");
       EDID_PRINTF(" HDR SMPTE ST 2084/SMPTE ST 428(DCI P3)  %s\n",(pCeaBlk->HDRMetadataInfo.EOTF_ET_2?"Supported":"Not Supported"));
       EDID_PRINTF(" Traditional Gamma - HDR Luminance Range %s\n",(pCeaBlk->HDRMetadataInfo.EOTF_ET_1?"Supported":"Not Supported"));
       EDID_PRINTF(" Traditional Gamma - SDR Luminance Range %s\n",(pCeaBlk->HDRMetadataInfo.EOTF_ET_0?"Supported":"Not Supported"));
       EDID_PRINTF(" Static Metadata Type 1  %s\n",(pCeaBlk->HDRMetadataInfo.EOTF_SM_0?"Supported":"Not Supported"));
       EDID_PRINTF(" Desired Content Max Luminance Data =  0x%x\n",(pCeaBlk->HDRMetadataInfo.DesCntMaxLuminanceData));
       EDID_PRINTF(" Desired Content Max Frame Average Luminance Data =  0x%x\n",(pCeaBlk->HDRMetadataInfo.DesCntMaxFrmAvgLuminanceData));
       EDID_PRINTF(" Desired Content Min Luminance Data =  0x%x\n",(pCeaBlk->HDRMetadataInfo.DesCntMinLuminanceData));

    }
    if(*(unsigned char*)(&pCeaBlk->DolbyVisionVSVDB)!=0xFF)
    {
        EDID_PRINTF("............DOLBY VSVDB.................\n");
            EDID_PRINTF("Supports Dolby           : 0x%x\n",
                    pCeaBlk->DolbyVisionVSVDB.isDolbyVisionSupported);
            EDID_PRINTF("Supports_YUV422_12Bit    : 0x%x\n",
                    pCeaBlk->DolbyVisionVSVDB.is422_12bitSupported);
            EDID_PRINTF("Supports_2160p_60        : 0x%x\n",
                    pCeaBlk->DolbyVisionVSVDB.is4Kp60Supported);
            EDID_PRINTF("Dolbyvision version      :0x%x\n",
                    pCeaBlk->DolbyVisionVSVDB.dv_version);
            EDID_PRINTF("DM version               :0x%x\n",
                    pCeaBlk->DolbyVisionVSVDB.dm_version);
            EDID_PRINTF("Parity                   :0x%x\n",
                    pCeaBlk->DolbyVisionVSVDB.parity);
            EDID_PRINTF("MaxLuminance             :0x%x\n",
                    pCeaBlk->DolbyVisionVSVDB.MaxLum);
            EDID_PRINTF("MinLuminance             :0x%x\n",
                    pCeaBlk->DolbyVisionVSVDB.MinLum);
    }
    else
        EDID_PRINTF("Null Dolby block encountered in dumping \n");
    EDID_PRINTF("HDR capability info    :0x%x\n",
            pCeaBlk->HdrCapInfo);

    // Dump HFVSDB block information
    if (*(unsigned char*)(&pCeaBlk->CeaHdmiVSDB) != 0xFF)
    {
        PRINT_TITLE("HFVSDB information");
        EDID_PRINTF ("Version                   =   %d\n",
                        pCeaBlk->CeaHdmiVSDB.Version);
        EDID_PRINTF ("Max TMDS Character Rate   =   %d Mhz\n",
                        pCeaBlk->CeaHdmiVSDB.MAX_TMDS_Character_Rate * 5);
        EDID_PRINTF ("Hdmi_3D_OSD_Disparitye    =   %d\n",
                        pCeaBlk->CeaHdmiVSDB.HfvsdbFeatures.Hdmi_3D_OSD_Disparity);
        EDID_PRINTF ("Dual_View                 =   %d\n",
                        pCeaBlk->CeaHdmiVSDB.HfvsdbFeatures.Dual_View);
        EDID_PRINTF ("Independent_view          =   %d\n",
                        pCeaBlk->CeaHdmiVSDB.HfvsdbFeatures.Independent_view);
        EDID_PRINTF ("LTE_340Mcsc_scramble      =   %d\n",
                        pCeaBlk->CeaHdmiVSDB.HfvsdbFeatures.LTE_340Mcsc_scramble);
        EDID_PRINTF ("RR_Capable                =   %d\n",
                        pCeaBlk->CeaHdmiVSDB.HfvsdbFeatures.RR_Capable);
        EDID_PRINTF ("SCDC_Present              =   %d\n",
                        pCeaBlk->CeaHdmiVSDB.HfvsdbFeatures.SCDC_Present);

        EDID_PRINTF ("DC_30bit_420              =   %d\n",
                        pCeaBlk->CeaHdmiVSDB.HfVsdbDeepCol.DC_30bit_420);
        EDID_PRINTF ("DC_36bit_420              =   %d\n",
                        pCeaBlk->CeaHdmiVSDB.HfVsdbDeepCol.DC_36bit_420);
        EDID_PRINTF ("DC_48bit_420              =   %d\n",
                        pCeaBlk->CeaHdmiVSDB.HfVsdbDeepCol.DC_48bit_420);
    }

    if(*(unsigned char*)(&pCeaBlk->DolbyAudioVSADB)!=0xFF)
    {
        EDID_PRINTF("............DOLBY VSADB.................\n");
            EDID_PRINTF("version                  : %s\n",
                    pCeaBlk->DolbyAudioVSADB.dolbyVsadbVersion?"Reserved":"1.0");
            EDID_PRINTF("f63                      : %d\n",
                    pCeaBlk->DolbyAudioVSADB.f63);
            EDID_PRINTF("Output config            : 0x%x\n",
                    pCeaBlk->DolbyAudioVSADB.outputConfig);
            EDID_PRINTF("Supports headphone       : %d\n",
                    pCeaBlk->DolbyAudioVSADB.isHeadphonePB);
            EDID_PRINTF("Sink Device Capability   : 0x%x\n",
                    pCeaBlk->DolbyAudioVSADB.sinkDevCap);
    } else {
        EDID_PRINTF("Null Dolby Audio cap block encountered in dumping \n");
    }
    PRINT_TITLE("END OF CEA EXTENSION BLOCK");
}
#endif
