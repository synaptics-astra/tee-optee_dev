/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _VPP_AVIOMTR_H_
#define _VPP_AVIOMTR_H_


#ifdef __cplusplus
extern "C" {
#endif

#define VPP_AVIOMTR_DEBUG_EN (1)
//#define VPP_AVIOMTR_DEBUG_PRINT_EN (1)
#define VPP_AVIOMTR_PBM_STRIDE_ALIGN (6)
#define VPP_AVIOMTR_TILEHEIGHT_ALIGN (2)
#define VPP_AVIOMTR_BASE_ADDR_ALIGN (12)
#define VPP_AVIOMTR_DHUB_ADDR (0)

#define VPP_AVIOMTR_MMU_TAG_ADDR_START (RA_AVIOMTR_MMU_TAG)
#define VPP_AVIOMTR_MMU_TAG_ADDR_END (0x17C)

#define VPP_AVIOMTR_CONFIG_VPP (0)
#define VPP_AVIOMTR_CONFIG_OVPS (1)

#ifdef VPP_AVIOMTR_DEBUG_EN
#define VPP_AVIOMTR_CHECK_RETVAL(cmd)\
    do{\
        uiRetVal = cmd;\
        if(MV_VPP_OK != uiRetVal)\
        return uiRetVal;\
    }while(0)
#else
#define VPP_AVIOMTR_CHECK_RETVAL(cmd) cmd
#endif

/* Definitions for AVIO MTR thread IDs */
typedef enum VPP_AVIOMTR_THREAD_ID_E
{
    VPP_AVIOMTR_THREAD_INVALID  = -1,
    VPP_AVIOMTR_THREAD_MIN      = 0,
    VPP_AVIOMTR_THREAD_0        = VPP_AVIOMTR_THREAD_MIN,
    VPP_AVIOMTR_THREAD_1,
    VPP_AVIOMTR_THREAD_2,
    VPP_AVIOMTR_THREAD_3,
    VPP_AVIOMTR_THREAD_4,
    VPP_AVIOMTR_THREAD_5,
    VPP_AVIOMTR_THREAD_6,
    VPP_AVIOMTR_THREAD_7,
    VPP_AVIOMTR_THREAD_8,
    VPP_AVIOMTR_THREAD_9,
	VPP_AVIOMTR_THREAD_10,
	VPP_AVIOMTR_THREAD_11,
    VPP_AVIOMTR_THREAD_MAX
}VPP_AVIOMTR_THREAD_ID;


typedef struct VPP_AVIOMTR_MTRR_CFG_T
{
    UINT8  ucfg_stride;
    UINT8  ucfg_format;
    UINT8  ucfg_enable;
    UINT8  ucfg_mode;
    UINT8  ucfg_weave;
    BOOL   ucfg_hflip;
    BOOL   ucfg_vflip;
    UINT32 ubase_addr;

}VPP_AVIOMTR_MTRR_CFG, *PVPP_AVIOMTR_MTRR_CFG;

typedef struct VPP_AVIOMTR_MMU_CFG_T
{

    UINT8  upbm_shy_bw;
    UINT8  upbm_shy_pos;
    UINT32 upbm_stride_64B;
    UINT8  upbm_tileMode;
    UINT8  upbm_pm_enable;
    UINT8  upbm_shuffle_en;
    UINT8  upbm_bm_enable;
    UINT8  upbm_weave;

    UINT8  uvm_enable;
    UINT8  uvm_mode;
    UINT32 uvm_base;

}VPP_AVIOMTR_MMU_CFG, *PVPP_AVIOMTR_MMU_CFG;

typedef struct VPP_AVIOMTR_MTRR_Prog_T
{
    VPP_AVIOMTR_MTRR_CFG vppAviomtrMtrrCfg;
    VPP_AVIOMTR_MMU_CFG vppAviomtrMmuCfg;
} VPP_AVIOMTR_MTRR_Prog, *PVPP_AVIOMTR_MTRR_Prog;


UINT32 VPP_AVIOMTR_Reset(VPP_OBJ *vpp_obj, INT flag);
UINT32 VPP_AVIOMTR_Config(VPP_OBJ *vpp_obj, INT flag);
UINT32 VPP_AVIOMTR_kickOff(VPP_OBJ *vpp_obj, INT flag);
UINT32 VPP_AVIOMTR_UpdateDesc(VPP_OBJ *vpp_obj, INT flag, VBUF_INFO *pinfo, INT planeID);

#ifdef __cplusplus
}
#endif

#endif

