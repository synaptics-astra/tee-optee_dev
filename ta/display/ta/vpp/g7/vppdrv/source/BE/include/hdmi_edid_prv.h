/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef __HDMI_EDID_PRV_H__
#define __HDMI_EDID_PRV_H__

#include "hdmi_cmn.h"
#include "vpp_module.h"

/*-----------------------------------------------------------------------------
 * Static Variables
 *-----------------------------------------------------------------------------
 */
static const char Chars[28]="-ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// To be filled after checking the vesa spec
static CEA_RES_INFO EtdList[16] =
{
    {0, 800, 600, 60, RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},// 800*600   @60Hz
    {0, 800, 600, 56, RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},// 800*600   @56Hz
    {0, 640, 480, 75, RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},// 640*480   @75Hz
    {0, 640, 480, 72, RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},// 640*480   @72Hz
    {0, 640, 480, 67, RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},// 640*480   @67Hz
    {0, 640, 480, 60, RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},// 640*480   @60Hz
    {0, 720, 400, 88, RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},// 720*400   @88Hz
    {0, 720, 400, 70, RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},// 720*400   @70Hz
    {0, 1280,1024,75, RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},// 1280*1024 @75Hz
    {0, 1024,768, 75, RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},// 1024*768  @75Hz
    {0, 1024,768, 70, RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},// 1024*768  @70Hz
    {0, 1024,768, 60, RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},// 1024*768  @60Hz
    {0, 1024,768, 87, RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},// 1024*768  @87Hz
    {0, 832, 624, 75, RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},// 832*624   @75Hz
    {0, 800, 600, 75, RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},// 800*600   @75Hz
    {0, 800, 600, 72, RES_INFO_FRM_FMT_UNDEF, CEA_PICT_AR_NONE},// 800*600   @72Hz
};

/*-----------------------------------------------------------------------------
 * Function Prototypes
 *-----------------------------------------------------------------------------
 */
static EDID_RETURNS CheckCRC (
                        unsigned char *pDataBuf);

static EDID_RETURNS ParseBasicBlk(
                        unsigned char *pDataBuf,
                        PEDID_BASIC_BLK_DATA pEdidBasicBlk);

static EDID_RETURNS EdidParseMnfrInfo(
                        unsigned char *pDataBuf,
                        PEDID_GEN_INFO pGenInfo);

static EDID_RETURNS EdidParseDispParams(
                        unsigned char *pDataBuf,
                        PEDID_DISP_PARAMS pDispParams);

static EDID_RETURNS EdidParseETD(
                        unsigned char *pDataBuf,
                        unsigned char *pEdidResCnt,
                        PCEA_RES_INFO pEdidResInfo);

static EDID_RETURNS EdidParseDTD(
                        unsigned char *pDataBuf,
                        unsigned char *pEdidResCnt,
                        PCEA_RES_INFO pEdidResInfo,
                        unsigned char *HImgSize,
                        unsigned char *VImgSize);

static EDID_RETURNS EdidParseSTD(
                        unsigned char *pDataBuf,
                        unsigned char *pEdidResCnt,
                        PCEA_RES_INFO pEdidResInfo);

static EDID_RETURNS EdidParseMDB(
                        unsigned char *pDataBuf,
                        PEDID_MONITOR_INFO pMonitorInfo);

/*---------------------------------------------------------------------------*/
static EDID_RETURNS ParseCEAExtn(
                        unsigned char *pDataBuf,
                        PEDID_CEA_TIMING_EXTN_BLK pCEAExtnBlk,
                        BOOL isFirstBlock);

static EDID_RETURNS EdidParseSVD(
                        unsigned char *pDataBuf,
                        unsigned char SvdLen,
                        unsigned char *pEdidResCnt,
                        PCEA_RES_INFO pEdidResInfo,
                        unsigned char *pPrefTimingEntry);

static EDID_RETURNS EdidParseSAD(
                        unsigned char *pDataBuf,
                        unsigned char  SadLen,
                        PEDID_CEA_TIMING_EXTN_BLK pCEAExtnBlk);

static EDID_RETURNS EdidParseSpkrAllocBlk(
                        unsigned char *pDataBuf,
                        unsigned char  BlkLen,
                        PEDID_SPKR_ALLOC_FLD pSpkrAllocFld);

static EDID_RETURNS EdidParseVSDB(
                        unsigned char *pDataBuf,
                        unsigned char  VsdbLen,
                        PEDID_CEA_TIMING_EXTN_BLK pCEAExtnBlk);

static EDID_RETURNS EdidParseCalorimetryBlk(
                        unsigned char *pDataBuf,
                        unsigned char  BlkLen,
                        PEDID_CALORIMETRY_INFO pCalInfo);
/*---------------------------------------------------------------------------*/
static EDID_RETURNS ParseBlkMap(
                        unsigned char *pDataBuf,
                        PEDID_BLOCK_MAP_DATA pBlkMap);
/*---------------------------------------------------------------------------*/
#if defined (DEBUG_EDID_PARSE)
static void DumpBasicBlk(PEDID_BASIC_BLK_DATA pBasicBlk);
static void DumpCEAExtnBlk(PEDID_CEA_TIMING_EXTN_BLK pCeaBlk);
#endif
#endif // __HDMI_EDID_PRV_H__
