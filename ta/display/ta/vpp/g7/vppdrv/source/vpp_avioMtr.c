/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#define _VPP_AVIOMTR_C_

#include "vpp_module.h"
#include "vpp_api.h"
#include "vpp_bcmbuf.h"
#include "vpp_cfg.h"
#include "vpp_avioMtr.h"

#include "mtr.h"

#ifdef VPP_AVIOMTR_DEBUG_PRINT_EN
#define VPPDBG_AVIOMTR_LOG(...)   VPPAPI_DBG_INFO(__VA_ARGS__)
#else
#define VPPDBG_AVIOMTR_LOG(...)
#endif

/***************************************************************************************
 * FUNCTION: Clear cache tat information of AVIO MTR
 * PARAMS: vpp_obj - vpp object
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
static UINT32 VPP_AVIOMTR_ClearCacheTagInfo(VPP_OBJ *vpp_obj, INT flag)
{

    UINT32 uiRetVal = MV_VPP_OK;
    UINT32 uiRegAddr = 0;
    UINT32 uiAddrOffset = 0;
    BCMBUF *bcmbuf = NULL;

    if(flag == VPP_AVIOMTR_CONFIG_OVPS)
    {
        bcmbuf = vpp_obj->ovp_obj.pOvpSclBcmBuf;
    }
    else
    {
        bcmbuf = vpp_obj->pVbiBcmBuf;
    }

    uiRegAddr = vpp_obj->base_addr_avioMtr;

    for(uiAddrOffset = RA_AVIOMTR_MMU_TAG; uiAddrOffset < RA_AVIOMTR_kickoffR0; uiAddrOffset += 4)
    {
        VPP_AVIOMTR_CHECK_RETVAL(VPP_BCMBUF_Write(bcmbuf, uiRegAddr + uiAddrOffset, 0));
    }

    return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: Reset AVIO MTR block and load default values
 * PARAMS: vpp_obj - vpp object
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
UINT32 VPP_AVIOMTR_Reset(VPP_OBJ *vpp_obj, INT flag)
{
    UINT32 uiRetVal = MV_VPP_OK;

    VPP_AVIOMTR_Config(vpp_obj, flag);
    VPP_AVIOMTR_ClearCacheTagInfo(vpp_obj, flag);

    return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: Return MMU cache base and ways details
 * PARAMS: vpp_obj - vpp object
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
static UINT32 VPP_AVIOMTR_MTRR_GetCacheLines(VPP_AVIOMTR_THREAD_ID eThreadId, UINT8 *base, UINT8 *ways)
{
    UINT32 uiRetVal = MV_VPP_OK;

    /*Tune it based on the system requirements for a product */
    switch(eThreadId)
    {
        case VPP_AVIOMTR_THREAD_0:
            *base = 0;
            *ways = 1;
            break;
        case VPP_AVIOMTR_THREAD_1:
            *base = 2;
            *ways = 1;
            break;
        case VPP_AVIOMTR_THREAD_2:
            *base = 4;
            *ways = 1;
            break;
        case VPP_AVIOMTR_THREAD_3:
            *base = 6;
            *ways = 1;
            break;
        case VPP_AVIOMTR_THREAD_4:
            *base = 8;
            *ways = 1;
            break;
        case VPP_AVIOMTR_THREAD_5:
            *base = 10;
            *ways = 1;
            break;
        case VPP_AVIOMTR_THREAD_6:
            *base = 12;
            *ways = 1;
            break;
        case VPP_AVIOMTR_THREAD_7:
            *base = 14;
            *ways = 1;
            break;
        case VPP_AVIOMTR_THREAD_8:
        case VPP_AVIOMTR_THREAD_9:
        case VPP_AVIOMTR_THREAD_10:
        case VPP_AVIOMTR_THREAD_11:
        case VPP_AVIOMTR_THREAD_MAX:
        case VPP_AVIOMTR_THREAD_INVALID:
            return  MV_VPP_EBADPARAM;

    }

    return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: Configure MTR_R block of AVIOMTR
 * PARAMS: vpp_obj - vpp object
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
static UINT32 VPP_AVIOMTR_MTRR_Config(VPP_OBJ *vpp_obj, INT flag, VPP_AVIOMTR_THREAD_ID eThreadId, PVPP_AVIOMTR_MTRR_CFG psVppAviomtrMtrrMtrrCfg)
{
    UINT32 uiRetVal = MV_VPP_OK;
    UINT32 uiRegAddr = 0;
    T32MTRR_CFG_cfg  sMtrrCfgCfg;
    T32MTRR_CFG_base sMtrrCfgBase;
    UINT8 base=0xFF, ways=0xFF;
    BCMBUF *bcmbuf  = NULL;

    if(flag == VPP_AVIOMTR_CONFIG_OVPS)
    {
        bcmbuf = vpp_obj->ovp_obj.pOvpSclBcmBuf;
    }
    else
    {
        bcmbuf = vpp_obj->pVbiBcmBuf;
    }

    VPP_AVIOMTR_MTRR_GetCacheLines(eThreadId, &base, &ways);

    uiRegAddr = vpp_obj->base_addr_avioMtr + RA_AVIOMTR_mtrr + (eThreadId * sizeof(SIE_MTRR)) + RA_MTRR_mtr;

    sMtrrCfgCfg.u32                     = 0;
    sMtrrCfgCfg.ucfg_stride             = psVppAviomtrMtrrMtrrCfg->ucfg_stride;
    sMtrrCfgCfg.ucfg_format             = psVppAviomtrMtrrMtrrCfg->ucfg_format;
    sMtrrCfgCfg.ucfg_enable             = psVppAviomtrMtrrMtrrCfg->ucfg_enable;
    sMtrrCfgCfg.ucfg_mode               = psVppAviomtrMtrrMtrrCfg->ucfg_mode;
    sMtrrCfgCfg.ucfg_prefetch_disable   = 0;
    sMtrrCfgCfg.ucfg_cbase              = base;
    sMtrrCfgCfg.ucfg_cways              = ways;
    sMtrrCfgCfg.ucfg_auto_tag_clr       = 0;
    sMtrrCfgCfg.ucfg_mosaic_xnum        = 0;
    sMtrrCfgCfg.ucfg_mosaic_ynum        = 0;
    sMtrrCfgCfg.ucfg_weave              = psVppAviomtrMtrrMtrrCfg->ucfg_weave;
#if defined(BA_MTRR_CFG_cfg_ctype)
    sMtrrCfgCfg.ucfg_ctype              = 0;
#elif defined(BA_MTRR_CFG_cfg_byte)
    sMtrrCfgCfg.ucfg_byte               = 0;
#endif
    sMtrrCfgCfg.ucfg_hflip              = psVppAviomtrMtrrMtrrCfg->ucfg_hflip;
    sMtrrCfgCfg.ucfg_vflip              = psVppAviomtrMtrrMtrrCfg->ucfg_vflip;

    sMtrrCfgBase.u32         = 0;
    sMtrrCfgBase.ubase_addr  = psVppAviomtrMtrrMtrrCfg->ubase_addr;

    VPP_AVIOMTR_CHECK_RETVAL(VPP_BCMBUF_Write(bcmbuf, uiRegAddr + RA_MTRR_CFG_cfg, sMtrrCfgCfg.u32));
    VPP_AVIOMTR_CHECK_RETVAL(VPP_BCMBUF_Write(bcmbuf, uiRegAddr + RA_MTRR_CFG_base, sMtrrCfgBase.u32));

    VPPDBG_AVIOMTR_LOG("stride=0x%x, format=0x%x, enable = 0x%x mode=0x%x, hflip=0x%x, vflip=0x%x, base_addr=0x%x, \n", psVppAviomtrMtrrMtrrCfg->ucfg_stride, psVppAviomtrMtrrMtrrCfg->ucfg_format,  psVppAviomtrMtrrMtrrCfg->ucfg_enable, psVppAviomtrMtrrMtrrCfg->ucfg_mode, psVppAviomtrMtrrMtrrCfg->ucfg_hflip, psVppAviomtrMtrrMtrrCfg->ucfg_vflip, psVppAviomtrMtrrMtrrCfg->ubase_addr);
    VPPDBG_AVIOMTR_LOG("uiRegAddr = %x sMtrrCfgCfg.u32 = %x, sMtrrCfgBase.u32 = %x \n", uiRegAddr, sMtrrCfgCfg.u32, sMtrrCfgBase.u32);

    return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: Return MMU cache base and ways details
 * PARAMS: vpp_obj - vpp object
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
static UINT32 VPP_AVIOMTR_MMU_GetCacheLines(VPP_AVIOMTR_THREAD_ID eThreadId, UINT8 *base, UINT8 *ways)
{
    UINT32 uiRetVal = MV_VPP_OK;

    *ways = 1; /* Fixed values as per BG5CTpZ1 MTR programming guide */
    *base = (eThreadId * 2); /* Fixed programing as ways is constant value */

    return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: Clear cache tat information of AVIO MTR_MMU
 * PARAMS: vpp_obj - vpp object
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
static UINT32 VPP_AVIOMTR_MMU_ClearCacheTagInfo(VPP_OBJ *vpp_obj, VPP_AVIOMTR_THREAD_ID eThreadId, INT flag)
{

    UINT32 uiRetVal = MV_VPP_OK;
    UINT32 uiRegAddr = 0;
    UINT32 uiAddrOffset = 0;
    BCMBUF *bcmbuf = NULL;
    UINT8 base=0xFF, ways=0xFF;

    uiRegAddr = vpp_obj->base_addr_avioMtr;

    if(flag == VPP_AVIOMTR_CONFIG_OVPS)
    {
        bcmbuf = vpp_obj->ovp_obj.pOvpSclBcmBuf;
    }
    else
    {
        bcmbuf = vpp_obj->pVbiBcmBuf;
    }

    VPP_AVIOMTR_MMU_GetCacheLines(eThreadId, &base, &ways);

    for(uiAddrOffset = (RA_AVIOMTR_MMU_TAG + (base*4)); uiAddrOffset <= (RA_AVIOMTR_MMU_TAG + (base*4) + (ways*4)); uiAddrOffset += 4)
    {
        VPP_AVIOMTR_CHECK_RETVAL(VPP_BCMBUF_Write(bcmbuf, uiRegAddr + uiAddrOffset, 0));
    }

    return uiRetVal;
}
/***************************************************************************************
 * FUNCTION: Configure AVIOMTR MMU with frame descriptor
 * PARAMS: vpp_obj - vpp object
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
static UINT32 VPP_AVIOMTR_MMU_Config(VPP_OBJ *vpp_obj, INT flag, VPP_AVIOMTR_THREAD_ID eThreadId, PVPP_AVIOMTR_MMU_CFG psVppAviomtrMtrrMmuCfg)
{
    UINT32 uiRetVal = MV_VPP_OK;
    UINT32 uiRegAddr = 0;
    T32MMU_CFG_pbm   sMmuCfgPbm;
    T32MMU_CFG_vm    sMmuCfgVm;
    T32MMU_CFG_cdesc sMmuCfgCdesc;
    UINT8 base=0xFF, ways=0xFF;
    BCMBUF *bcmbuf  = NULL;

    if(flag == VPP_AVIOMTR_CONFIG_OVPS)
    {
        bcmbuf = vpp_obj->ovp_obj.pOvpSclBcmBuf;
    }
    else
    {
        bcmbuf = vpp_obj->pVbiBcmBuf;
    }

    VPP_AVIOMTR_MMU_GetCacheLines(eThreadId, &base, &ways);

    uiRegAddr = vpp_obj->base_addr_avioMtr + RA_AVIOMTR_mtrr + (eThreadId * sizeof(SIE_MTRR)) + RA_MTRR_mmu;

    sMmuCfgPbm.u32              = 0;
    sMmuCfgPbm.upbm_shy_bw      = psVppAviomtrMtrrMmuCfg->upbm_shy_bw;
    sMmuCfgPbm.upbm_shy_pos     = psVppAviomtrMtrrMmuCfg->upbm_shy_pos;
    sMmuCfgPbm.upbm_stride_64B  = psVppAviomtrMtrrMmuCfg->upbm_stride_64B;
    sMmuCfgPbm.upbm_tileMode    = psVppAviomtrMtrrMmuCfg->upbm_tileMode;
    sMmuCfgPbm.upbm_pm_enable   = psVppAviomtrMtrrMmuCfg->upbm_pm_enable;
    sMmuCfgPbm.upbm_shuffle_en  = psVppAviomtrMtrrMmuCfg->upbm_shuffle_en;
    sMmuCfgPbm.upbm_bm_enable   = psVppAviomtrMtrrMmuCfg->upbm_bm_enable;
    sMmuCfgPbm.upbm_weave       = psVppAviomtrMtrrMmuCfg->upbm_weave;

    sMmuCfgVm.u32                  = 0;
    sMmuCfgVm.uvm_enable           = psVppAviomtrMtrrMmuCfg->uvm_enable;
    sMmuCfgVm.uvm_prefecth_disable = 0;
    sMmuCfgVm.uvm_mode             = psVppAviomtrMtrrMmuCfg->uvm_mode;
    /*
     * The mTyp and delta are valid when uvm_mode = 1.
     * If mTyp = 0: The prefetch can be happened at any point of current section. ways >= 2 for this case.
     * If mTyp = 1:  enable prefetch at the bottom line of each section to avoid fault replacement when there are only two cache lines.
     * So we have only two cache lines , choosing value mTyp = 1.
    */
    if(sMmuCfgVm.uvm_mode == 1)
    {
        sMmuCfgVm.uvm_mTyp         = 1;
        sMmuCfgVm.uvm_delta        = 1;
    }
    else
    {
        sMmuCfgVm.uvm_mTyp         = 0;
        sMmuCfgVm.uvm_delta        = 0;
    }
    sMmuCfgVm.uvm_base             = psVppAviomtrMtrrMmuCfg->uvm_base;
    sMmuCfgVm.uvm_auto_tag_clr     = 0;

    sMmuCfgCdesc.u32          = 0;
    sMmuCfgCdesc.ucdesc_base  = base;
    sMmuCfgCdesc.ucdesc_ways  = ways;
    sMmuCfgCdesc.ucdesc_rpTyp = 1;

    VPP_AVIOMTR_CHECK_RETVAL(VPP_BCMBUF_Write(bcmbuf, uiRegAddr + RA_MMU_CFG_pbm, sMmuCfgPbm.u32));
    VPP_AVIOMTR_CHECK_RETVAL(VPP_BCMBUF_Write(bcmbuf, uiRegAddr + RA_MMU_CFG_vm, sMmuCfgVm.u32));
    VPP_AVIOMTR_CHECK_RETVAL(VPP_BCMBUF_Write(bcmbuf, uiRegAddr + RA_MMU_CFG_cdesc, sMmuCfgCdesc.u32));

    VPPDBG_AVIOMTR_LOG("shy_bw=0x%x, shy_pos=0x%x, stride_64B=0x%x, tilemode = 0x%x pm_en=0x%x, shuffle_en=0x%x, bm_en=0x%x, vm_en=0x%x, vbase=0x%x\n", psVppAviomtrMtrrMmuCfg->upbm_shy_bw, psVppAviomtrMtrrMmuCfg->upbm_shy_pos, psVppAviomtrMtrrMmuCfg->upbm_stride_64B, psVppAviomtrMtrrMmuCfg->upbm_tileMode, psVppAviomtrMtrrMmuCfg->upbm_pm_enable, psVppAviomtrMtrrMmuCfg->upbm_shuffle_en, psVppAviomtrMtrrMmuCfg->upbm_bm_enable, psVppAviomtrMtrrMmuCfg->uvm_enable, psVppAviomtrMtrrMmuCfg->uvm_base);
    VPPDBG_AVIOMTR_LOG("%d %s >> uiRegAddr = %x sMmuCfgPbm.u32  = %x, sMmuCfgVm.u32  = %x  sMmuCfgCdesc.u32 = %x \n", __LINE__, __func__, uiRegAddr, sMmuCfgPbm.u32, sMmuCfgVm.u32, sMmuCfgCdesc.u32);

    return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: Set the input window for MTR_R IP
 * PARAMS: vpp_obj - vpp object
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
static UINT32 VPP_AVIOMTR_MTRR_SetROI(VPP_OBJ *vpp_obj, INT flag, VBUF_INFO *pinfo, VPP_AVIOMTR_THREAD_ID eThreadId, VPP_WIN *mtrRWin)
{
    UINT32 uiRetVal = MV_VPP_OK;
    UINT32 uiRegAddr = 0;
    T32MTRR_roix sMtrrRoix;
    T32MTRR_roiy sMtrrRoiy;
    UINT32 mtrR_X_factor = 0;
    UINT32 mtrR_Y_factor = 0;
    BCMBUF *bcmbuf  = NULL;

    if(flag == VPP_AVIOMTR_CONFIG_OVPS)
    {
        bcmbuf = vpp_obj->ovp_obj.pOvpSclBcmBuf;
    }
    else
    {
        bcmbuf = vpp_obj->pVbiBcmBuf;
    }

    uiRegAddr = vpp_obj->base_addr_avioMtr + RA_AVIOMTR_mtrr + (eThreadId * sizeof(SIE_MTRR));

    if(SRCFMT_YUV420SP_TILED_MODE0 == pinfo->m_srcfmt || SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)
    {
        /* Tile mode */
        mtrR_Y_factor = 4;
    }
    else
    {
        /*420SP case */
        mtrR_Y_factor = 1;
    }
    if(SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)
    {
        mtrR_X_factor = 64;
    }
    else if(10 == pinfo->m_bits_per_pixel && SRCFMT_YUV420SP_TILED_MODE0 == pinfo->m_srcfmt)
    {
        mtrR_X_factor = 48;
    }
    else if(SRCFMT_YUV420SP == pinfo->m_srcfmt)
    {
        /* 420SP case. 8Bit/10Bit needs to be handled seperatly */
        mtrR_X_factor = 64;
    }
    else if(SRCFMT_YUV420SP_DWA == pinfo->m_srcfmt)
    {
        mtrR_X_factor = 48;
    }

    sMtrrRoix.u32       = 0;
    if(SRCFMT_YUV420SP == pinfo->m_srcfmt && 10 == pinfo->m_bits_per_pixel)
    {
        /* Special case for 420SP 10Bit */
        sMtrrRoix.uroix_xs  = mtrRWin->x * 10 / 8 / mtrR_X_factor;
        sMtrrRoix.uroix_xm  = (mtrRWin->width - 1) * 10 / 8 / mtrR_X_factor;
    }
    else
    {
        sMtrrRoix.uroix_xs  = mtrRWin->x / mtrR_X_factor;
        sMtrrRoix.uroix_xm  = (mtrRWin->width - 1)  / mtrR_X_factor;
    }

    sMtrrRoiy.u32       = 0;
    sMtrrRoiy.uroiy_ys  = mtrRWin->y / mtrR_Y_factor;
    sMtrrRoiy.uroiy_ym  = (mtrRWin->height - 1) / mtrR_Y_factor;

    // Offset of 32 shows the improvement in the issue of top artifacts (garbage) for compressed input. So adding by default 32 offset.
    if(pinfo->m_luma_top_ofst > 0 || pinfo->m_chroma_top_ofst > 0)
    {
        sMtrrRoiy.uroiy_ys =  sMtrrRoiy.uroiy_ys + 32;
        sMtrrRoiy.uroiy_ym = sMtrrRoiy.uroiy_ym + 32;
    }

    VPP_AVIOMTR_CHECK_RETVAL(VPP_BCMBUF_Write(bcmbuf, uiRegAddr + RA_MTRR_roix, sMtrrRoix.u32));
    VPP_AVIOMTR_CHECK_RETVAL(VPP_BCMBUF_Write(bcmbuf, uiRegAddr + RA_MTRR_roiy, sMtrrRoiy.u32));

    VPPDBG_AVIOMTR_LOG("uiRegAddr = %x  sMtrrRoix.u32 = %x , sMtrrRoiy.u32 = %x \n", uiRegAddr,  sMtrrRoix.u32, sMtrrRoiy.u32);

    return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: Prgrame MTRR block
 * PARAMS: vpp_obj - vpp object
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
static UINT32 VPP_AVIOMTR_MTRR_Program(VPP_OBJ *vpp_obj, INT planeID, INT flag, VPP_AVIOMTR_THREAD_ID eThreadId, BOOL kickOffEn, PVPP_AVIOMTR_MTRR_Prog psVppAviomtrMtrrProg)
{
    UINT32 uiRetVal = MV_VPP_OK;
    UINT32 *aviomtrKickoff = NULL;

    if(flag == VPP_AVIOMTR_CONFIG_OVPS)
    {
        aviomtrKickoff = &vpp_obj->ovp_obj.uiAviomtrKickoffOvps;
    }
    else
    {
        aviomtrKickoff = &vpp_obj->uiAviomtrKickoff;
    }

    if(FALSE == kickOffEn)
    {
        *aviomtrKickoff = ((*aviomtrKickoff) & ~(0x1 << eThreadId));
        return uiRetVal; /* Disable this thread kickoff. So no needs to prgram MTR IP */
    }
    else
    {
        *aviomtrKickoff = ((*aviomtrKickoff) | (0x1 << eThreadId));
    }

    VPP_AVIOMTR_CHECK_RETVAL(VPP_AVIOMTR_MTRR_Config(vpp_obj, flag, eThreadId, &psVppAviomtrMtrrProg->vppAviomtrMtrrCfg));
    VPP_AVIOMTR_CHECK_RETVAL(VPP_AVIOMTR_MMU_Config(vpp_obj, flag, eThreadId, &psVppAviomtrMtrrProg->vppAviomtrMmuCfg));

    return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: AVIOMTR system level configuration
 * PARAMS: vpp_obj - vpp object
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 * PARAMS:
 * RETURN:
 ***************************************************************************************/
UINT32 VPP_AVIOMTR_Config(VPP_OBJ *vpp_obj, INT flag)
{
    UINT32 uiRetVal = MV_VPP_OK;
    UINT32 uiRegAddr = 0;
    T32AVIOMTR_cfg sAviomtrCfg;
    BCMBUF *bcmbuf  = NULL;

    if(flag == VPP_AVIOMTR_CONFIG_OVPS)
    {
        bcmbuf = vpp_obj->ovp_obj.pOvpSclBcmBuf;
    }
    else
    {
        bcmbuf = vpp_obj->pVbiBcmBuf;
    }

    uiRegAddr = vpp_obj->base_addr_avioMtr;

    sAviomtrCfg.u32 = 0;
    sAviomtrCfg.ucfg_mtrr_flow_ctrl = 0;
    sAviomtrCfg.ucfg_mtrr_qos_en    = AVIO_DHUB_MTR_CFG_QOS_EN;
    sAviomtrCfg.ucfg_mtrr_qos       = AVIO_DHUB_MTR_CFG_QOS;
    sAviomtrCfg.ucfg_mtrr_dis_mtid  = 1;
    sAviomtrCfg.ucfg_mmu_pageSz     = 0;
    sAviomtrCfg.ucfg_mmu_qos_en     = AVIO_DHUB_MTR_CFG_QOS_EN;
    sAviomtrCfg.ucfg_mmu_qos        = AVIO_DHUB_MTR_CFG_QOS;
    sAviomtrCfg.ucfg_mmu_banks      = 1;
    sAviomtrCfg.ucfg_mmu_sysSz      = 0;
    sAviomtrCfg.ucfg_mmu_tarSz      = 0;
    VPP_AVIOMTR_CHECK_RETVAL(VPP_BCMBUF_Write(bcmbuf, uiRegAddr + RA_AVIOMTR_cfg, sAviomtrCfg.u32));

    return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: Kickoff the the active threads for next interrupt
 * PARAMS: vpp_obj - vpp object
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
UINT32 VPP_AVIOMTR_kickOff(VPP_OBJ *vpp_obj, INT flag)
{
    UINT32 uiRetVal = MV_VPP_OK;
    UINT32 uiRegAddr = 0;
    INT32 i  = 0;
    uiRegAddr = vpp_obj->base_addr_avioMtr + RA_AVIOMTR_kickoffR0;
    BCMBUF *bcmbuf  = NULL;
    UINT32 aviomtrKickoff;

    if(flag == VPP_AVIOMTR_CONFIG_OVPS)
    {
        bcmbuf = vpp_obj->ovp_obj.pOvpSclBcmBuf;
        aviomtrKickoff = vpp_obj->ovp_obj.uiAviomtrKickoffOvps;
    }
    else
    {
        bcmbuf = vpp_obj->pVbiBcmBuf;
        aviomtrKickoff = vpp_obj->uiAviomtrKickoff;
    }

    /*Kickoff shold be done after all the configs. And all thread kickoff should to together */
    for (i = 0;i < VPP_AVIOMTR_THREAD_MAX;i++)
    {
        if(aviomtrKickoff & (1<<i))
        {
            VPP_AVIOMTR_CHECK_RETVAL(VPP_BCMBUF_Write(bcmbuf, uiRegAddr, 1));
        }
        uiRegAddr+=4;
    }

    return uiRetVal;
}

/***************************************************************************************
 * FUNCTION:Retrun thread IDs for Luma and croma
 * PARAMS: vpp_obj - vpp object
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
UINT32 VPP_AVIOMTR_GetThreadIDs(INT planeID, UINT8 *threadID_Y, UINT8 *threadID_UV)
{
    UINT32 uiRetVal = MV_VPP_OK;

    switch(planeID)
    {
#ifdef ENABLE_EL_PIPE_PATH
        case PLANE_OVP_EL:
            *threadID_Y  = VPP_AVIOMTR_THREAD_0;
            *threadID_UV = VPP_AVIOMTR_THREAD_1;
            break;
#endif
        case PLANE_MAIN:
            *threadID_Y  = VPP_AVIOMTR_THREAD_2;
            *threadID_UV = VPP_AVIOMTR_THREAD_3;
            break;
        case PLANE_GFX1:
        case PLANE_MAIN_EL:
            *threadID_Y  = VPP_AVIOMTR_THREAD_4;
            *threadID_UV = VPP_AVIOMTR_THREAD_5;
            break;
#ifdef ENABLE_PIP_GFX0_PATH
        case PLANE_PIP:
            *threadID_Y  = VPP_AVIOMTR_THREAD_6;
            *threadID_UV = VPP_AVIOMTR_THREAD_7;
            break;
#endif
        default:
            uiRetVal = MV_VPP_EBADPARAM;
            break;
    }

    return uiRetVal;
}
/***************************************************************************************
 * FUNCTION: convert Pinfo MTR param to MTRR CFG
 * PARAMS: MTR_BUFF_DESC - MTR buffer descriptor which gives MTR INFO of Y and UV
 *                       - buffer Start Address, stride & MTRR field
 * RETURN:
 ***************************************************************************************/
static void VPP_AVIOMTR_Convert2MtrCfg(VPP_AVIOMTR_MTRR_Prog *pMtrCfg,
                                       MTR_BUFF_DESC *pinfoMtr,
                                       UINT32 startAddr,
                                       UINT32 mBufstride,
                                       UINT32 m_is_compressed)
{
    pMtrCfg->vppAviomtrMtrrCfg.ucfg_stride    = pinfoMtr->m_MtrrCfgCfg_stride;
    pMtrCfg->vppAviomtrMtrrCfg.ucfg_format    = pinfoMtr->m_MtrrCfgCfg_format;
    pMtrCfg->vppAviomtrMtrrCfg.ucfg_enable    = IS_MTR_ENABLED(m_is_compressed);
    pMtrCfg->vppAviomtrMtrrCfg.ucfg_mode      = pinfoMtr->m_MtrrCfgCfg_mode;
    pMtrCfg->vppAviomtrMtrrCfg.ucfg_weave     = pinfoMtr->m_MtrrCfgCfg_weave;
    pMtrCfg->vppAviomtrMtrrCfg.ucfg_hflip     = 0;
    pMtrCfg->vppAviomtrMtrrCfg.ucfg_vflip     = 0;
    pMtrCfg->vppAviomtrMtrrCfg.ubase_addr     = pinfoMtr->m_MtrrCfgBase_addr;
    pMtrCfg->vppAviomtrMmuCfg.upbm_shy_bw     = pinfoMtr->m_MmuCfgPbm_shy_bw;
    pMtrCfg->vppAviomtrMmuCfg.upbm_shy_pos    = pinfoMtr->m_MmuCfgPbm_shy_pos;
    pMtrCfg->vppAviomtrMmuCfg.upbm_stride_64B = mBufstride >> (VPP_AVIOMTR_PBM_STRIDE_ALIGN+
                                                                VPP_AVIOMTR_TILEHEIGHT_ALIGN);
    pMtrCfg->vppAviomtrMmuCfg.upbm_pm_enable  = pinfoMtr->m_MmuCfgPbm_pm_enable;
    pMtrCfg->vppAviomtrMmuCfg.upbm_shuffle_en = pinfoMtr->m_MmuCfgPbm_shuffle_en;
    pMtrCfg->vppAviomtrMmuCfg.upbm_bm_enable  = pinfoMtr->m_MmuCfgPbm_bm_enable;
    pMtrCfg->vppAviomtrMmuCfg.upbm_weave      = pinfoMtr->m_MmuCfgPbm_weave;
    pMtrCfg->vppAviomtrMmuCfg.uvm_enable      = pinfoMtr->m_MmuCfgVm_enable;
    pMtrCfg->vppAviomtrMmuCfg.uvm_mode        = pinfoMtr->m_MmuCfgVm_mode;
    pMtrCfg->vppAviomtrMmuCfg.uvm_base        = startAddr >> VPP_AVIOMTR_BASE_ADDR_ALIGN;
}
/***************************************************************************************
 * FUNCTION: Update frame descriptor to AVIO MTR registers
 * PARAMS: vpp_obj - vpp object
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
UINT32 VPP_AVIOMTR_UpdateDesc(VPP_OBJ *vpp_obj, INT flag, VBUF_INFO *pinfo, INT planeID)
{
    UINT32 uiRetVal = MV_VPP_OK;
    VPP_AVIOMTR_MTRR_Prog sVppAviomtrMtrProg;
    PLANE *plane = NULL;
    UINT8 threadID_Y = 0xFF, threadID_UV = 0xFF;
    VPP_WIN mtrRWin;
    BOOL kickOffEn = 0;

    VPP_AVIOMTR_GetThreadIDs(planeID, &threadID_Y, &threadID_UV);
    if(PLANE_MAIN_EL == planeID)
    {
        plane = &vpp_obj->plane[PLANE_MAIN];
        plane->m_mtrR_chkSemId_Y_El  = threadID_Y;
        plane->m_mtrR_chkSemId_UV_El = threadID_UV;
    }
    else if(PLANE_OVP_EL == planeID)
    {
        plane = &vpp_obj->ovp_obj.plane;
        plane->m_mtrR_chkSemId_Y  = threadID_Y;
        plane->m_mtrR_chkSemId_UV = threadID_UV;
    }
    else
    {
        plane = &vpp_obj->plane[planeID];
        plane->m_mtrR_chkSemId_Y  = threadID_Y;
        plane->m_mtrR_chkSemId_UV = threadID_UV;
    }

    if(SRCFMT_YUV420SP == pinfo->m_srcfmt || SRCFMT_YUV420SP_DWA == pinfo->m_srcfmt || \
        IS_GFX_SRC_FORMAT(pinfo->m_srcfmt) || SRCFMT_YUV422 == pinfo->m_srcfmt)
    {
        sVppAviomtrMtrProg.vppAviomtrMmuCfg.upbm_tileMode = 0;
    }
    else
    {
        sVppAviomtrMtrProg.vppAviomtrMmuCfg.upbm_tileMode = 1;
    }
    if(pinfo->m_is_compressed)
    {
        kickOffEn = 1;
        VPP_AVIOMTR_MMU_ClearCacheTagInfo(vpp_obj, threadID_Y, flag);
        if (!VPP_IS_SINGLE_CLIENT_SRC_FMT(pinfo->m_srcfmt)) {
            VPP_AVIOMTR_MMU_ClearCacheTagInfo(vpp_obj, threadID_UV, flag);
        }
    }
    else
    {
        kickOffEn = 0;
    }
    VPP_AVIOMTR_Convert2MtrCfg(&sVppAviomtrMtrProg, &pinfo->m_mtr_buf_desc_Y,
                        (UINT32)pinfo->m_pbuf_start, pinfo->m_buf_stride, pinfo->m_is_compressed);

    VPP_AVIOMTR_MTRR_Program(vpp_obj, planeID, flag, threadID_Y, kickOffEn, &sVppAviomtrMtrProg);

    if (VPP_IS_SINGLE_CLIENT_SRC_FMT(pinfo->m_srcfmt)) {
        kickOffEn = FALSE;
    } else {
        VPP_AVIOMTR_Convert2MtrCfg(&sVppAviomtrMtrProg, &pinfo->m_mtr_buf_desc_UV,
                (UINT32)pinfo->m_buf_pbuf_start_UV, pinfo->m_buf_stride_UV, pinfo->m_is_compressed);
    }

    VPP_AVIOMTR_MTRR_Program(vpp_obj, planeID, flag, threadID_UV, kickOffEn, &sVppAviomtrMtrProg);
    if (pinfo->m_is_compressed)
    {
        mtrRWin.x       = pinfo->m_active_left;
        mtrRWin.y       = pinfo->m_active_top;
        mtrRWin.width   = pinfo->m_active_width;
        mtrRWin.height  = pinfo->m_active_height;
        VPP_AVIOMTR_MTRR_SetROI(vpp_obj, flag, pinfo, threadID_Y, &mtrRWin);
        if (!VPP_IS_SINGLE_CLIENT_SRC_FMT(pinfo->m_srcfmt)) {
            mtrRWin.height  = pinfo->m_active_height/2;
            VPP_AVIOMTR_MTRR_SetROI(vpp_obj, flag, pinfo, threadID_UV, &mtrRWin);
        }
    }

    VPPDBG_AVIOMTR_LOG("planeID = %d  Y Base_addr = %x, stride_64B = %x, vbase = %x, \n", planeID, pinfo->m_mtr_buf_desc_Y.m_MtrrCfgBase_addr, pinfo->m_buf_stride, pinfo->m_pbuf_start);
    VPPDBG_AVIOMTR_LOG("planeID = %d UV Base_addr = %x, stride_64B = %x, vbase = %x, \n", planeID, pinfo->m_mtr_buf_desc_UV.m_MtrrCfgBase_addr, pinfo->m_buf_stride_UV, pinfo->m_buf_pbuf_start_UV);

    return uiRetVal;
}

