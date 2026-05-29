/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#define _VPP_API_C_
#define VPP_OSAL_SHM_ALLOC_ONLY
#include "Galois_memmap.h"
#include "galois_io.h"
#include "global.h"
#include "vbuf.h"
#include "vpp_module.h"
#include "vpp_apifuncs.h"
#include "vpp_api_types.h"
#include "vpp_cfg.h"
#include "vpp_fe.h"
#include "vpp_scl.h"
#include "vpp_csc.h"
#include "vpp_cpcb.h"
#include "vpp_cpcb_ovl.h"
#include "vpp_be.h"
#include "vpp_be_hdmitx.h"
#include "vpp_framecapture.h"
#include "vpp_gamma.h"
#include "vpp_avioMtr.h"
#include "vpp_be_hdcp.h"
#include "vpp_dv_api.h"
#include "vpp_intr_util.h"
#include "vpp_clk.h"

#ifdef CONFIG_AMP_DCM
#include "vpp_mp.h"
#endif

#ifndef CONFIG_AMP_TEE_ENABLE
#include "OSAL_api.h"
#else
#include "tee_ta_osal.h"
#endif

#if defined(VPP_DIAGS)
#include "pllDiag.h"
#else
#include "vpp_apifuncs_wrapper.h"
#endif //VPP_DIAGS
#include "vpp.h"

#include "vpp_isr.h"
#include "api_avio_dhub.h"

#ifdef TZ_3_0
#include "bm_heap.h"
#ifndef CONFIG_OPTEE
#include "tee_internal_core_mem_api.h"
#include "tee_internal_api.h"
#endif
#endif
#include "vpp_clkGate.h"
#include "vpp_debug_util.h"
#include "vpp_debug_dump.h"
#include "vpp_intr_monitor.h"
#include "vpp_dv_frame_dump.h"
#include "vpp_frc_scl.h"
#ifndef CONFIG_AMP_TEE_ENABLE
#include <sys/ioctl.h>
#endif
#include "vpp_env_ur.h"
#ifdef CONFIG_AMP_DCM
#endif

#include "vpp_ngptv_api.h"

#include "vpp_vmx_apifuncs.h"

#include "SNPS_HDMI_tx.h"
#include "vpp_apifuncs_fastlogo.h"
#include "vpp_gdetee.h"

#include "vpp_obj.h"

#define VPP_FIXED_COLOR_FMT /* Undefine this once MV_VPPOBJ_SetCPCBOutputResolution is enabled to take color_fmt as param */

/*FIXME: VPP_CT changes - need correct mapping*/
#define avioDhubSemMap_vpp_vppCPCB0_intr avioDhubSemMap_vpp128b_vpp_inr0
#define vpp_vop2_DTG_frst_intr avioDhubSemMap_vpp128b_vpp_inr14
#define avioDhubSemMap_vpp_vppOUT4_intr avioDhubSemMap_vpp128b_vpp_inr6
#define avioDhubSemMap_vpp_vppOUT3_intr avioDhubSemMap_vpp128b_vpp_inr9
#define avioDhubSemMap_vpp_CH10_intr avioDhubSemMap_vpp128b_CH8_intr
#define avioDhubSemMap_vpp_vppOUT0_intr avioDhubSemMap_vpp128b_vpp_inr6


#ifdef NEW_MEM_MAP
#undef GALOIS_1K_MALLOC
#undef GALOIS_1K_FREE

#define GALOIS_1K_MALLOC GALOIS_SHM_1K_MALLOC
#define GALOIS_1K_FREE GALOIS_SHM_1K_FREE
#endif

#define bTST(x, b) (((x) >> (b)) & 1)

#if defined (VPP_DIAGS)
int berlin_pe_dev = -1;
#else
extern int berlin_pe_dev;
#endif

#define VPP_RESET_CPCB_TIMEOUT_IN_10MS 100
/*Refer vpp_cpcb_tg.c: CPCB_TG_SetHVTotal*/
#define VPP_CPCBTG_RESET_HTOT 0
#define VPP_CPCBTG_RESET_VTOT 0
#define VPP_CPCB_RESET_COND(HTOT,VTOT) ((HTOT==VPP_CPCBTG_RESET_HTOT) && (VTOT==VPP_CPCBTG_RESET_VTOT))

typedef enum {
    VPP_WIN_TYPE_REF,
    VPP_WIN_TYPE_DISP,
}VPP_WIN_TYPE;

/* End -- Added for suspend and resume */

#define VPP_BASE_ADDR           (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_VPP_REG_BASE)
#define VPP_HDMITX_BASE_ADDR    (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMITX_REG_BASE)
#define VPP_MP_BASE_ADDR        (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_VPP_MP_REG_BASE)
#define VPP_GFX_VOP_BASE_ADDR   (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_VPP_GFX_VOP_REG_BASE)
#define VPP_AVIOMTR_REG_BASE    (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_GBL_BASE + RA_avioGbl_MEMMAP_MTR)
#define VPP_VDCM_BASE_ADDR      (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_VPP_MP_REG_BASE)
#define VPP_GDCM_BASE_ADDR      (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_VPP_GFX_VOP_REG_BASE)

#ifdef CONFIG_AMP_DCM
#define VPP_DCM_BASE_ADDR       (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_VPP_DCM_REG_BASE)
#endif

// change the PLL speed in MHZ

#define IS_CLRFMT_420(fmt)      \
        ((fmt == OUTPUT_COLOR_FMT_YCBCR420) ||     \
        (fmt == OUTPUT_COLOR_FMT_YCBCR420_P3D65) ||   \
        (fmt == OUTPUT_COLOR_FMT_YCBCR420_R2020))

#define HDMI_COLORFMT420_CHANGE(hdmiTx, clrfmt)     \
        ((hdmiTx != NULL) &&        \
        (IS_CLRFMT_420(((BE_HDMITX_DATA*)hdmiTx)->videoParams.VideoFmt) || IS_CLRFMT_420(clrfmt)) &&      \
        (((BE_HDMITX_DATA*)hdmiTx)->videoParams.VideoFmt != clrfmt))

static int IS_VALID_VPP_WINDOW(VPP_OBJ *vpp_obj, VPP_WIN_TYPE type, int width, int height) {
    int retVal = 1;

    if(VPP_WIN_TYPE_REF == type) {
        if( (width<VPP_WIN_MIN_INPUT_WIDTH) || (height<VPP_WIN_MIN_INPUT_HEIGHT) ) {
            retVal = 0;
        }
    }
    if(VPP_WIN_TYPE_DISP == type) {
        if( (width<VPP_WIN_MIN_OUTPUT_WIDTH) || (height<VPP_WIN_MIN_OUTPUT_HEIGHT) ) {
            retVal = 0;
        }
    }

    return retVal;
}

static int IS_CPCB_ID_INVALID(int cpcbID) {
    int retVal;
    retVal = ((cpcbID < CPCB_1) || (cpcbID >= MAX_NUM_CPCBS));
    return retVal;
}


int ParseCCData(VPP_OBJ *vpp_obj, void *frame)
{
#ifndef CONFIG_AMP_TEE_ENABLE
    VDEC_VID_BUF_DESC *frm_descr;
#endif
    VBUF_INFO *pinfo;
    USER_DATA_BLOCK_HEADER *pUserDataH;
    UINT8 *pCCRawData;
    UINT8 cc_count;
    unsigned int cc_ctrl; // bit 23: cc enable/disable for the field
    unsigned char field_id; // 0: top field  1: bottom field
    int i;

#ifndef CONFIG_AMP_TEE_ENABLE
    frm_descr = (VDEC_VID_BUF_DESC *)frame;
    pinfo = (VBUF_INFO *)frm_descr->user_data;
#else
    pinfo = (VBUF_INFO *)frame;
#endif
    if (pinfo != NULL){
        pUserDataH = (USER_DATA_BLOCK_HEADER*) (pinfo->m_user_data_block[VIDEO_USER_DATA_TYPE_CC]);

        if (pUserDataH != NULL){

            /* get cc_data start address */
            pCCRawData = pUserDataH->m_pStart + pUserDataH->m_body_offset;

            /* DVD CC parsing */
            if (pUserDataH->m_sub_type == 1){

                /* get CC data count */
                cc_count = *pCCRawData & 0x3F;

                /* check CC data length */
                if ((pCCRawData + 3 * cc_count) > (pUserDataH->m_pStart + pUserDataH->m_length))
                    return MV_VPP_EBADPARAM;

                /* init field indicator, first CC data is line 21 field 1 data, if top_field_flag_of_gop is true */
                field_id = !(*pCCRawData >> 7);

                pCCRawData ++;

                /* set CC control byte and data, packet (cc_ctrl, cc_data1, cc_data2) */
                for (i=0; i<cc_count; i++ ){

                    /* following CC data is valid */
                    if (*pCCRawData == 0xFF){

                        pCCRawData ++;

                        /* construct CC control byte and data, keep them atomic*/
                        cc_ctrl = 0x01 | (*pCCRawData << 8) | (*(pCCRawData + 1) << 16);

                        /* write CC control byte and CC data to buffer */
                        if (!vbi_buf_write(&(vpp_obj->vout[VOUT_SD].cc_buf[field_id]), (unsigned char *)(&cc_ctrl), 3)){
                            /* VBI buffer is full, CC data writing fail. */
                            vbi_buf_uncommit(&(vpp_obj->vout[VOUT_SD].cc_buf[field_id]));
                            return (MV_VPP_EVBIBUFFULL);
                        }
                        /* commit write */
                        vbi_buf_commit(&(vpp_obj->vout[VOUT_SD].cc_buf[field_id]));
                    }else if (*pCCRawData == 0xFE){
                        pCCRawData ++;

                        /* construct CC control byte */
                        cc_ctrl = 0x00;

                        /* write CC control byte to buffer */
                        if (!vbi_buf_write(&(vpp_obj->vout[VOUT_SD].cc_buf[field_id]), (unsigned char *)(&cc_ctrl), 1)){
                            /* VBI buffer is full, CC data writing fail. */
                            vbi_buf_uncommit(&(vpp_obj->vout[VOUT_SD].cc_buf[field_id]));
                            return (MV_VPP_EVBIBUFFULL);
                        }
                        /* commit write */
                        vbi_buf_commit(&(vpp_obj->vout[VOUT_SD].cc_buf[field_id]));

                    }else{
                        return MV_VPP_EBADPARAM;
                    }

                    /* updata field ID */
                    field_id = (field_id + 1) % 2;

                    pCCRawData += 2 ;
                }
            }else if (pUserDataH->m_sub_type == 0){
                /* ATSC CC parsing */

                /* check process_cc_data_flag */
                if ((*pCCRawData & 0x40) >> 6 == 0)
                    return (MV_VPP_OK);

                /* get CC data count */
                cc_count = *pCCRawData & 0x1F;

                /* check CC data length */
                if ((pCCRawData + 3 * cc_count + 1) > (pUserDataH->m_pStart + pUserDataH->m_length))
                    return MV_VPP_EBADPARAM;

                pCCRawData += 2;

                /* set CC control byte and data, packet (cc_ctrl, cc_data1, cc_data2) */
                for (i=0; i<cc_count; i++ ){

                    /* get field id */
                    field_id = *pCCRawData & 0x01;

                    /* check cc_type for NTSC */
                    if ((*pCCRawData & 0x02) == 0x00){

                        /* check cc_valid */
                        if ((*pCCRawData & 0x04) >> 2 == 0x01){

                            /* construct CC control byte and data, keep them atomic*/
                            cc_ctrl = 0x01 | ((*(pCCRawData + 1) & 0x7F) << 8) | ((*(pCCRawData + 2) & 0x7F) << 16);

                            /* write CC control byte and CC data to buffer */
                            if (!vbi_buf_write(&(vpp_obj->vout[VOUT_SD].cc_buf[field_id]), (unsigned char *)(&cc_ctrl), 3)){
                                /* VBI buffer is full, CC data writing fail. */
                                vbi_buf_uncommit(&(vpp_obj->vout[VOUT_SD].cc_buf[field_id]));
                                return (MV_VPP_EVBIBUFFULL);
                            }
                            /* commit write */
                            vbi_buf_commit(&(vpp_obj->vout[VOUT_SD].cc_buf[field_id]));

                        }

                        pCCRawData += 3 ;
                    }
                }
            }
        }
    }
    return (MV_VPP_OK);
}

static int MV_VPP_GetCPCBTGStatus(VPP_OBJ *vpp_obj, int CpcbId, int *status)
{
    T32TG_PRG_Total  stTgTotal;
    UINT32 uiTgRegAddr;

    if(CpcbId == CPCB_1)
        uiTgRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_DTG;
#ifdef ENABLE_GFX2_VOP2_PATH
    else if(CpcbId == CPCB_2)
        uiTgRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP2 + RA_VOP2_DTG;
#endif
    else
        return MV_VPP_EBADPARAM;

    uiTgRegAddr = uiTgRegAddr + RA_DTG_DISPLAY_TG + RA_BETG_TG_PRG + RA_TG_PRG_Total;
    GA_REG_WORD32_READ(uiTgRegAddr, &stTgTotal.u32);
    if (VPP_CPCB_RESET_COND(stTgTotal.uTotal_horizontal, stTgTotal.uTotal_vertical)){
        *status = STATUS_INACTIVE;
    }

    return MV_VPP_OK;
}

static int MV_VPP_GetCpcbStatus(VPP_OBJ *vpp_obj, int CpcbId, int *status)
{
    if (MV_VPP_GetCPCBTGStatus(vpp_obj, CpcbId, status)) {
        return MV_VPP_EBADPARAM;
    } else {
        if ((STATUS_INACTIVE == *status) &&
            (STATUS_INACTIVE == vpp_obj->dv[CpcbId].status)) {
            *status = STATUS_INACTIVE;
        } else if (!vpp_obj->dv[CpcbId].status) {
            *status = STATUS_INACTIVE_PENDING;
        } else {
            *status = vpp_obj->dv[CpcbId].status;
        }
    }

    return MV_VPP_OK;
}


static int VPP_WaitUntilResetCPCB(VPP_OBJ *vpp_obj, int cpcbID)
{
    UINT32 cnt=0;
    int status;

    while(1) {
       MV_VPP_GetCpcbStatus(vpp_obj, cpcbID, &status);
       if((status == STATUS_INACTIVE) || (cnt > VPP_RESET_CPCB_TIMEOUT_IN_10MS)){
               break;
       }
        MV_OSAL_Task_Sleep(10);
        cnt++;
    }

    MV_VPP_GetCPCBTGStatus(vpp_obj, cpcbID, &status);
    if (status == STATUS_INACTIVE) {
       VPPAPI_DBG_ERROR("Valid Force setting status to STATUS_INACTIVE, cnt = %d\n",cnt);
    }else {
       VPPAPI_DBG_ERROR("Invalid Force setting status to STATUS_INACTIVE, cnt = %d\n",cnt);
    }

    return (MV_VPP_OK);
}

static int ResetCPCBTG(VPP_OBJ *vpp_obj, int cpcbID, int mode)
{
    int params[8];
    CMD cmd;
    int cnt = 0;
    int cpcb = cpcbID;

    /* reset CPCB TG first */
    params[0] = cpcbID;
    params[1] = RES_RESET;
    params[2] = OUTPUT_COLOR_FMT_INVALID;
    params[3] = OUTPUT_BIT_DEPTH_INVALID;
    params[4] = 1; //pixel repeat
    params[5] = 0; //quiescent mode
    params[6] = mode; /*display mode*/
    params[7] = VOUT_INVALID;
    make_cmd(&cmd, (FUNCPTR)VPP_SetCPCBOutputResolution, 7, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcb].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcb].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcb].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcb].cmdq_lock);

#ifndef CONFIG_VPP_FASTLOGO_SUPPORT
    VPP_DEBUG_UTIL_monitorResetCPCBStart(vpp_obj, "ResetCPCBTG", VPP_DEBUG_UTIL_BCMBUFF_DUMP_DISABLE);

    /* wait for CPCB TG reset done */
    while((vpp_obj->dv[cpcbID].status != STATUS_INACTIVE) && (cnt++ <= VPP_RESET_CPCB_TIMEOUT_IN_10MS)) {
        VPP_DEBUG_UTIL_monitorResetCPCB(vpp_obj, "Status", VPP_DEBUG_UTIL_BCMBUFF_DUMP_ENABLE);
        MV_OSAL_Task_Sleep(10);
    }

    while(vpp_obj->dv[cpcbID].vbi_num < 1) {
        VPP_DEBUG_UTIL_monitorResetCPCB(vpp_obj, "VBI", VPP_DEBUG_UTIL_BCMBUFF_DUMP_ENABLE);
        MV_OSAL_Task_Sleep(100);
    }

#ifdef ENABLE_GFX2_VOP2_PATH
    if(IS_CPCB_VOUT_DISP_MODE(cpcbID, SINGLE_MODE_SEC)) {
        cpcb = CPCB_2;
    }
#endif
    VPP_WaitUntilResetCPCB(vpp_obj, cpcb);
    vpp_obj->dv[cpcbID].status = STATUS_INACTIVE;
#endif

    return (MV_VPP_OK);
}

#ifndef CONFIG_VPP_ENABLE_INTERRUPT_ON_PUSHFRAME
static void VPP_dhub_sem_clear(VPP_OBJ *vpp_obj)
{
    int instat;
    HDL_semaphore *pSemHandle = vpp_obj->pSemHandle;

    instat = semaphore_chk_full(pSemHandle, -1);

    if (bTST(instat, avioDhubSemMap_vpp_vppCPCB0_intr)){
        semaphore_pop(pSemHandle, avioDhubSemMap_vpp_vppCPCB0_intr, 1);
        semaphore_clr_full(pSemHandle, avioDhubSemMap_vpp_vppCPCB0_intr);
    }else if (bTST(instat, avioDhubSemMap_vpp_vppOUT4_intr)){
        semaphore_pop(pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr, 1);
        semaphore_clr_full(pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr);
    }
    if (bTST(instat, vpp_vop2_DTG_frst_intr)){
        semaphore_pop(pSemHandle, vpp_vop2_DTG_frst_intr, 1);
        semaphore_clr_full(pSemHandle, vpp_vop2_DTG_frst_intr);
    }
    return;
}
#endif // CONFIG_VPP_ENABLE_INTERRUPT_ON_PUSHFRAMEi

/******************************************************
  * FUNCTION: AVIO Reset
  * INPUT: NONE
  * RETURN: NONE
  * NOTE : This function resets complete AVIO block. Need
  *         to config complete AVIO including DHUB block
  *******************************************************/
int MV_VPPOBJ_AVIO_Reset(void)
{
    TGbl_avioReset reg;

    reg.u32[0] = 0;
    reg.uavioReset_SyncReset = 1;
    GA_REG_WORD32_WRITE((MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_avioReset), reg.u32[0]);
    reg.uavioReset_SyncReset = 0;
    GA_REG_WORD32_WRITE((MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_avioReset), reg.u32[0]);

    return (MV_VPP_OK);
}

int MV_VPP_INITVPPS(int instance, UINT32 dhub_init_flag, UINT32 intr_init_flag)
{
    unsigned int channel_init_mask_vpp = 0;
    unsigned int channel_init_mask_ag = 0;

    if (dhub_init_flag) {
        //Initialize DHUB & BCM Schedular, We need VPP handle inside TA for DHUB operation
        channel_init_mask_vpp = (1 << VPP_NUM_OF_CHANNELS) - 1;
        channel_init_mask_ag = (1 << AG_NUM_OF_CHANNELS) - 1;
    }

    //Initialize DHUB & BCM Schedular, We need VPP handle inside TA for DHUB operation
    DhubInitialization(0, VPP_DHUB_BASE, VPP_HBO_SRAM_BASE, &VPP_dhubHandle, VPP_config, VPP_NUM_OF_CHANNELS, channel_init_mask_vpp);
    DhubInitialization(0, AG_DHUB_BASE, AG_HBO_SRAM_BASE, &AG_dhubHandle, AG_config, AG_NUM_OF_CHANNELS, channel_init_mask_ag);

    if (intr_init_flag) {
        MV_VPPOBJ_EnableInterrupt(0, 1);
    }

    return 0;
}

#if 1//VPP_SW_BRINGUP: To debug interrupt infrastructure with Linux
#define BCM_CHANNEL_ID  avioDhubChMap_aio64b_BCM_R
#define IRQ_AUDIO_ID    IRQ_dHubIntrAvio1


HDL_dhub *g_aio64bDhubHandle;
void ag_intr_handler(void)
{
    HDL_semaphore *pSemHandle;
    unsigned int instant=0;

    pSemHandle = dhub_semaphore(g_aio64bDhubHandle);
    instant = semaphore_chk_full(pSemHandle, -1);

    VPPAPI_DBG_INFO("%s:%d - instant = %x, BCM = %x\n", __func__, __LINE__, instant, 1<<BCM_CHANNEL_ID);

    // Clear BCM Interrupt
    if (bTST(instant, BCM_CHANNEL_ID))
    {
        semaphore_pop(pSemHandle, BCM_CHANNEL_ID, 1);
        semaphore_clr_full(pSemHandle, BCM_CHANNEL_ID);
    }
}

void enable_bcm_intr(void) {
    HDL_semaphore *pSemHandle;
    pSemHandle = dhub_semaphore(&AG_dhubHandle.dhub);

    /* configure and enable BCM interrupt */
    semaphore_cfg(pSemHandle, BCM_CHANNEL_ID, 1, 0);
    semaphore_clr_full(pSemHandle, BCM_CHANNEL_ID);
    semaphore_intr_enable(pSemHandle, BCM_CHANNEL_ID, 0/*empty*/, 1/*full*/, 0/*almost empty*/, 0/*almost full*/, 0);//CPU_ID/*cpu id*/);

}
#endif

/***********************************************************
 * FUNCTION: initialize VPP module
 * PARAMS: none
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EBADCALL - function called previously
 **********************************************************/
int MV_VPP_Init(VPP_INIT_PARM *vpp_init_parm)
{
    int i;
    TEE_Result res;
    unsigned int channel_init_mask_vpp = (1 << VPP_NUM_OF_CHANNELS) - 1;
    unsigned int channel_init_mask_ag = (1 << AG_NUM_OF_CHANNELS) - 1;

    res = MV_VPP_InitVppObj(vpp_init_parm);
    if(res != TEE_SUCCESS) {
        return res;
    }


    VPP_log_init(VPP_LOG_INFO, VPP_LOG_CONSOLE);

//    VPPAPI_DBG_INFO("enter\n");
    if (g_vpp->magic == VPP_MAGIC){
        return (MV_VPP_EBADCALL);
    }

    for (i=0; i<MAX_NUM_OBJECTS; i++)
        g_vpp->vpp_objs[i] = NULL;

#if defined(VPP_SUS_RES)
    for (i=0; i<MAX_NUM_OBJECTS; i++)
    {
        g_vpp->vpp_suss[i] = NULL;
        g_vpp->suspend_status[i] = 0;
    }
#endif

#ifdef CONFIG_VPP_FASTLOGO_SUPPORT
    {
        g_vpp->VPP_fl.enableFlushCache = vpp_init_parm->iHDMIEnable;
        g_vpp->VPP_fl.enableInterrupt[CPCB_1] = 0;
#ifdef ENABLE_GFX2_VOP2_PATH
        g_vpp->VPP_fl.enableInterrupt[CPCB_2] = 0;
#endif
        VPPAPI_DBG_INFO("%s:%d: enableFlushCache=%d, iHDMIEnable=%d\n", __func__, __LINE__, g_vpp->VPP_fl.enableFlushCache, vpp_init_parm->iHDMIEnable);
#ifdef CONFIG_VPP_AUTOPUSH_SUPPORT
        VPPAPI_DBG_INFO("%s:%d: AutoPush_Flag is enabled\n", __func__, __LINE__);
#endif //CONFIG_VPP_AUTOPUSH_SUPPORT
    }
#endif //CONFIG_VPP_FASTLOGO_SUPPORT
#ifdef CONFIG_AMP_TEE_ENABLE
    //Initialize DHUB & BCM Schedular, We need VPP handle inside TA for DHUB operation
    DhubInitialization(0, VPP_DHUB_BASE, VPP_HBO_SRAM_BASE, &VPP_dhubHandle, VPP_config, VPP_NUM_OF_CHANNELS, channel_init_mask_vpp);
    DhubInitialization(0, AG_DHUB_BASE, AG_HBO_SRAM_BASE, &AG_dhubHandle, AG_config, AG_NUM_OF_CHANNELS, channel_init_mask_ag);
    BCM_SCHED_Open();
#endif

    g_vpp->magic = VPP_MAGIC;

    MV_VPP_Clk_Ctrl();

    return (MV_VPP_OK);
}

#ifndef CONFIG_AMP_TEE_ENABLE
static void VPP_ISR_Task(VPP_OBJ *vpp_obj){
//    VPPAPI_DBG_INFO("VPP ISR task is runing\n");
    prctl(PR_SET_NAME, "VPP_ISR_Task", 0, 0, 0);
    while(!vpp_obj->vpp_isr_need_exit) {
         VPP_ISR_TaskHandler(vpp_obj);
    }
    return;
}

static void CEC_ISR_Task(VPP_OBJ *vpp_obj){
//    VPPAPI_DBG_INFO("VPP ISR task is runing\n");
    prctl(PR_SET_NAME, "CEC_ISR_Task", 0, 0, 0);
    while(!vpp_obj->vpp_isr_need_exit) {
         CEC_ISR_TaskHandler(vpp_obj);
    }
    return;
}
#endif

/***********************************************
 * FUNCTION: create a VPP object
 * PARAMS: base_addr - VPP object base address
 *         *handle - pointer to object handle
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EUNCONFIG - not initialized
 *         MV_VPP_ENODEV - no device
 *         MV_VPP_ENOMEM - no memory
 ***********************************************/
int MV_VPPOBJ_Create(int base_addr, int *handle)
{
    int i = 0;
    int cnt;
#ifdef TZ_3_0
    uintptr_t AllocAddr=0;
#endif

#if !defined(CONFIG_AMP_TEE_ENABLE) || defined(VPP_ENABLE_FRAME_CAPTURE)
    HRESULT Ret = S_OK;
#endif

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (!handle){
        return (MV_VPP_EBADPARAM);
    }

    for (i=0; i<MAX_NUM_OBJECTS; i++){
        /* find a free handle */
        if (g_vpp->vpp_objs[i] == NULL)
            break;
    }

    if (i >= MAX_NUM_OBJECTS) {
        *handle = 0;
        return (MV_VPP_OK);
    }

#ifdef TZ_3_0
    VPP_SHM32_MALLOC(VPP_SHM_SECURE,sizeof(VPP_OBJ),&AllocAddr);
    if (!(g_vpp->vpp_objs[i] = (VPP_OBJ *)AllocAddr)){
        return (MV_VPP_ENOMEM);
    }
#else
    if (!(g_vpp->vpp_objs[i] = (VPP_OBJ *)VPP_Malloc(sizeof(VPP_OBJ)))){
        return (MV_VPP_ENOMEM);
    }
#endif

    VPP_Memset(g_vpp->vpp_objs[i], 0, sizeof(VPP_OBJ));
    //VPP_DEBUG_Printf("%x=%x, %x=%x\n",VPP_BASE_ADDR,base_addr, VPP_HDMITX_BASE_ADDR, VPP_HDMITX_BASE_ADDR /*hdmi_base_addr*/);
    g_vpp->vpp_objs[i]->base_addr = VPP_BASE_ADDR;
    g_vpp->vpp_objs[i]->base_addr_mp = VPP_MP_BASE_ADDR;
    g_vpp->vpp_objs[i]->base_addr_gfx_vop = VPP_GFX_VOP_BASE_ADDR;
    g_vpp->vpp_objs[i]->base_addr_avioMtr = VPP_AVIOMTR_REG_BASE;
    g_vpp->vpp_objs[i]->dcm_base_addr_mp = VPP_VDCM_BASE_ADDR;
    g_vpp->vpp_objs[i]->dcm_base_addr_gfx = VPP_GDCM_BASE_ADDR;
#ifdef TZ_3_0
    VPP_Memcpy((void *)&(g_vpp->vpp_objs[i]->VPP_dhubHandle),(const void *) &VPP_dhubHandle, sizeof(VPP_dhubHandle));
#endif

#if defined(VPP_SUS_RES)
#ifdef TZ_3_0
    VPP_SHM32_MALLOC(VPP_SHM_SECURE,sizeof(VPP_OBJ),&AllocAddr);
    if (!(g_vpp->vpp_suss[i] = (VPP_OBJ *)AllocAddr)){
        return (MV_VPP_ENOMEM);
    }
#else
    if (!(g_vpp->vpp_suss[i] = (VPP_OBJ *)VPP_Malloc(sizeof(VPP_OBJ)))){
        return (MV_VPP_ENOMEM);
    }
#endif

    VPP_Memset(g_vpp->vpp_suss[i], 0, sizeof(VPP_OBJ));


    // Allocate memory for data store
#ifdef TZ_3_0
    VPP_SHM32_MALLOC(VPP_SHM_SECURE, sizeof(BE_HDMITX_DATA), &AllocAddr);
    if (!(g_vpp->vpp_suss[i]->be.hdmiTx = (BE_HDMITX_DATA*)AllocAddr)) {
        return (MV_VPP_ENOMEM);
    }
    VPP_SHM32_MALLOC(VPP_SHM_SECURE, sizeof(CEC_SRV_DATA), &AllocAddr);
    if (!(g_vpp->vpp_suss[i]->be.hdmiCec = (CEC_SRV_DATA*)AllocAddr)) {
        return (MV_VPP_ENOMEM);
    }
#else
    g_vpp->vpp_suss[i]->be.hdmiTx = (BE_HDMITX_DATA*)VPP_Malloc (sizeof(BE_HDMITX_DATA));
    if (!g_vpp->vpp_suss[i]->be.hdmiTx)
        return (MV_VPP_ENOMEM);
    g_vpp->vpp_suss[i]->be.hdmiCec = (CEC_SRV_DATA*)VPP_Malloc (sizeof(CEC_SRV_DATA));
    if (!g_vpp->vpp_suss[i]->be.hdmiCec)
        return (MV_VPP_ENOMEM);
#endif

    GaloisMemClear ((char *)(g_vpp->vpp_suss[i]->be.hdmiTx), sizeof(BE_HDMITX_DATA));

#endif

    /* create VBI BCM buffer */
    for (cnt=FIRST_CPCB; cnt<MAX_NUM_CPCBS; cnt++) {
        if (VPP_BCMBUF_Create(&(g_vpp->vpp_objs[i]->vbi_bcm_buf[cnt][0]), BCM_BUFFER_SIZE) != MV_VPP_OK){
            VPP_Free(g_vpp->vpp_objs[i]);
            return (MV_VPP_ENOMEM);
        }

        if (VPP_BCMBUF_Create(&(g_vpp->vpp_objs[i]->vbi_bcm_buf[cnt][1]), BCM_BUFFER_SIZE) != MV_VPP_OK){
            VPP_Free(g_vpp->vpp_objs[i]);
            return (MV_VPP_ENOMEM);
        }

        if (VPP_BCMBUF_Create(&(g_vpp->vpp_objs[i]->vbi_clear_bcm_buf[cnt][0]), BCM_BUFFER_SIZE) != MV_VPP_OK){
            VPP_Free(g_vpp->vpp_objs[i]);
            return (MV_VPP_ENOMEM);
        }

        if (VPP_BCMBUF_Create(&(g_vpp->vpp_objs[i]->vbi_clear_bcm_buf[cnt][1]), BCM_BUFFER_SIZE) != MV_VPP_OK){
            VPP_Free(g_vpp->vpp_objs[i]);
            return (MV_VPP_ENOMEM);
        }
    }
    MV_OSAL_Mutex_Create(&(g_vpp->vpp_objs[i]->bcmbuf_lock));
    for (cnt=FIRST_CPCB; cnt<MAX_NUM_CPCBS; cnt++) {
        if (VPP_BCMBUF_Create(&(g_vpp->vpp_objs[i]->vde_bcm_buf[cnt][0]), VDE_BCM_BUFFER_SIZE) != MV_VPP_OK){
            VPP_Free(g_vpp->vpp_objs[i]);
            return (MV_VPP_ENOMEM);
        }

        if (VPP_BCMBUF_Create(&(g_vpp->vpp_objs[i]->vde_bcm_buf[cnt][1]), VDE_BCM_BUFFER_SIZE) != MV_VPP_OK){
            VPP_Free(g_vpp->vpp_objs[i]);
            return (MV_VPP_ENOMEM);
        }
    }

#ifndef CONFIG_AMP_TEE_ENABLE
    Ret = MV_OSAL_Sem_Create (&(g_vpp->vpp_objs[i]->vpp_dsr_sem), 0);
    if (Ret != S_OK){
        return (Ret);
    }
#endif //CONFIG_AMP_TEE_ENABLE

    g_vpp->vpp_objs[i]->vpp_isr_need_exit = 0;

#ifndef CONFIG_AMP_TEE_ENABLE
    Ret = MV_OSAL_Task_Create_Detailed(&(g_vpp->vpp_objs[i]->vpp_isr_task),
            VPP_ISR_Task,
            g_vpp->vpp_objs[i],
            MV_OSAL_TASK_PRIORITY_HIGHEST,
            "VPP ISR task",
            NULL,
            0,
            32);


    if (Ret != S_OK){
        return (Ret);
    }

    Ret = MV_OSAL_Task_Create_Detailed(&(g_vpp->vpp_objs[i]->cec_isr_task),
            CEC_ISR_Task,
            g_vpp->vpp_objs[i],
            MV_OSAL_TASK_PRIORITY_HIGHEST,
            "CEC ISR task",
            NULL,
            0,
            32);


    if (Ret != S_OK){
        return (Ret);
    }

    Ret = MV_OSAL_Task_Create_Detailed(&(g_vpp->vpp_objs[i]->vpp_dsr_task),
            VPP_DSR_Task,
            g_vpp->vpp_objs[i],
            MV_OSAL_TASK_PRIORITY_HIGHEST - 2,
            "VPP DSR task",
            NULL,
            0,
            32);


    if (Ret != S_OK ){
        return (Ret);
    }
#endif //CONFIG_AMP_TEE_ENABLE

    g_vpp->vpp_objs[i]->hdmi_cpcb0_dsr_cb = NULL;
    for (cnt=FIRST_PLANE; cnt<MAX_NUM_PLANES; cnt++){
        /* reset frame descriptor queues */
        frmq_reset(&(g_vpp->vpp_objs[i]->plane[cnt].inputq));
        frmq_reset(&(g_vpp->vpp_objs[i]->plane[cnt].outputq));

    } /* Plane memory allocation end */

    if (VPP_NGPTV_Create(g_vpp->vpp_objs[i]) != SUCCESS) {
        return MV_VPP_ENOMEM;
    }

    /* For DVs */
    for (cnt=FIRST_CPCB; cnt<MAX_NUM_CPCBS; cnt++) {
        // initialize cmd queue mutex
        MV_OSAL_Mutex_Create(&(g_vpp->vpp_objs[i]->dv[cnt].cmdq_lock));
        /* allocate DMA dhub config queue*/
        if(VPP_SHM32_MALLOC(VPP_SHM_NONSECURE, DMA_CMD_BUFFER_SIZE, &g_vpp->vpp_objs[i]->dv[cnt].vbi_dma_cfgQ[0].handle) != SUCCESS)
            return (MV_VPP_ENOMEM);
        g_vpp->vpp_objs[i]->dv[cnt].vbi_dma_cfgQ[0].len = 0;
        g_vpp->vpp_objs[i]->dv[cnt].vbi_dma_cfgQ[0].addr = VPP_SHM_GetVirtualAddress(g_vpp->vpp_objs[i]->dv[cnt].vbi_dma_cfgQ[0].handle, 0);
        g_vpp->vpp_objs[i]->dv[cnt].vbi_dma_cfgQ[0].phy_addr = VPP_SHM_GetPhysicalAddress(g_vpp->vpp_objs[i]->dv[cnt].vbi_dma_cfgQ[0].handle, 0);

        if(VPP_SHM32_MALLOC(VPP_SHM_NONSECURE,DMA_CMD_BUFFER_SIZE, &g_vpp->vpp_objs[i]->dv[cnt].vbi_dma_cfgQ[1].handle) != SUCCESS)
            return (MV_VPP_ENOMEM);
        g_vpp->vpp_objs[i]->dv[cnt].vbi_dma_cfgQ[1].len = 0;
        g_vpp->vpp_objs[i]->dv[cnt].vbi_dma_cfgQ[1].addr = VPP_SHM_GetVirtualAddress(g_vpp->vpp_objs[i]->dv[cnt].vbi_dma_cfgQ[1].handle, 0);
        g_vpp->vpp_objs[i]->dv[cnt].vbi_dma_cfgQ[1].phy_addr = VPP_SHM_GetPhysicalAddress(g_vpp->vpp_objs[i]->dv[cnt].vbi_dma_cfgQ[1].handle, 0);

        g_vpp->vpp_objs[i]->dv[cnt].curr_cpcb_vbi_dma_cfgQ = &(g_vpp->vpp_objs[i]->dv[cnt].vbi_dma_cfgQ[0]);

        /* allocate VBI dhub config queue*/
        if(VPP_SHM32_MALLOC(VPP_SHM_NONSECURE,DMA_CMD_BUFFER_SIZE, &g_vpp->vpp_objs[i]->dv[cnt].vbi_bcm_cfgQ[0].handle) != SUCCESS)
            return (MV_VPP_ENOMEM);
        g_vpp->vpp_objs[i]->dv[cnt].vbi_bcm_cfgQ[0].len = 0;
        g_vpp->vpp_objs[i]->dv[cnt].vbi_bcm_cfgQ[0].addr = VPP_SHM_GetVirtualAddress(g_vpp->vpp_objs[i]->dv[cnt].vbi_bcm_cfgQ[0].handle, 0);
        g_vpp->vpp_objs[i]->dv[cnt].vbi_bcm_cfgQ[0].phy_addr = VPP_SHM_GetPhysicalAddress(g_vpp->vpp_objs[i]->dv[cnt].vbi_bcm_cfgQ[0].handle, 0);

        if(VPP_SHM32_MALLOC(VPP_SHM_NONSECURE,DMA_CMD_BUFFER_SIZE, &g_vpp->vpp_objs[i]->dv[cnt].vbi_bcm_cfgQ[1].handle) != SUCCESS)
            return (MV_VPP_ENOMEM);
        g_vpp->vpp_objs[i]->dv[cnt].vbi_bcm_cfgQ[1].len = 0;
        g_vpp->vpp_objs[i]->dv[cnt].vbi_bcm_cfgQ[1].addr = VPP_SHM_GetVirtualAddress(g_vpp->vpp_objs[i]->dv[cnt].vbi_bcm_cfgQ[1].handle, 0);
        g_vpp->vpp_objs[i]->dv[cnt].vbi_bcm_cfgQ[1].phy_addr = VPP_SHM_GetPhysicalAddress(g_vpp->vpp_objs[i]->dv[cnt].vbi_bcm_cfgQ[1].handle, 0);

        g_vpp->vpp_objs[i]->dv[cnt].curr_cpcb_vbi_bcm_cfgQ = &(g_vpp->vpp_objs[i]->dv[cnt].vbi_bcm_cfgQ[0]);

        /* allocate VDE dhub config queue*/
        if(VPP_SHM32_MALLOC(VPP_SHM_NONSECURE,DMA_CMD_BUFFER_SIZE, &g_vpp->vpp_objs[i]->dv[cnt].vde_bcm_cfgQ[0].handle) != SUCCESS)
            return (MV_VPP_ENOMEM);
        g_vpp->vpp_objs[i]->dv[cnt].vde_bcm_cfgQ[0].len = 0;
        g_vpp->vpp_objs[i]->dv[cnt].vde_bcm_cfgQ[0].addr = VPP_SHM_GetVirtualAddress(g_vpp->vpp_objs[i]->dv[cnt].vde_bcm_cfgQ[0].handle, 0);
        g_vpp->vpp_objs[i]->dv[cnt].vde_bcm_cfgQ[0].phy_addr = VPP_SHM_GetPhysicalAddress(g_vpp->vpp_objs[i]->dv[cnt].vde_bcm_cfgQ[0].handle, 0);

        if(VPP_SHM32_MALLOC(VPP_SHM_NONSECURE,DMA_CMD_BUFFER_SIZE, &g_vpp->vpp_objs[i]->dv[cnt].vde_bcm_cfgQ[1].handle) != SUCCESS)
            return (MV_VPP_ENOMEM);
        g_vpp->vpp_objs[i]->dv[cnt].vde_bcm_cfgQ[1].len = 0;
        g_vpp->vpp_objs[i]->dv[cnt].vde_bcm_cfgQ[1].addr = VPP_SHM_GetVirtualAddress(g_vpp->vpp_objs[i]->dv[cnt].vde_bcm_cfgQ[1].handle, 0);
        g_vpp->vpp_objs[i]->dv[cnt].vde_bcm_cfgQ[1].phy_addr = VPP_SHM_GetPhysicalAddress(g_vpp->vpp_objs[i]->dv[cnt].vde_bcm_cfgQ[1].handle, 0);

        g_vpp->vpp_objs[i]->dv[cnt].curr_cpcb_vde_bcm_cfgQ = &(g_vpp->vpp_objs[i]->dv[cnt].vde_bcm_cfgQ[0]);

        g_vpp->vpp_objs[i]->dv[cnt].skip_vde_int = 0;
    }

    /* config VOUTs */
    for (cnt=FIRST_VOUT; cnt<MAX_NUM_VOUTS; cnt++) {
        if (cnt == VOUT_SD){
            VPP_SHM32_MALLOC(VPP_SHM_NONSECURE, CC_DATA_QUEUE_SIZE * sizeof(UINT16), &g_vpp->vpp_objs[i]->vout[cnt].cc_buf[0].handle);
             g_vpp->vpp_objs[i]->vout[cnt].cc_buf[0].addr = (UINT8*)VPP_SHM_GetVirtualAddress(g_vpp->vpp_objs[i]->vout[cnt].cc_buf[0].handle, 0);
            g_vpp->vpp_objs[i]->vout[cnt].cc_buf[0].length = CC_DATA_QUEUE_SIZE * sizeof(UINT16);
            vbi_buf_reset(&g_vpp->vpp_objs[i]->vout[cnt].cc_buf[0]);

            VPP_SHM32_MALLOC(VPP_SHM_NONSECURE, CC_DATA_QUEUE_SIZE * sizeof(UINT16), &g_vpp->vpp_objs[i]->vout[cnt].cc_buf[1].handle);
             g_vpp->vpp_objs[i]->vout[cnt].cc_buf[1].addr = (UINT8*)VPP_SHM_GetVirtualAddress(g_vpp->vpp_objs[i]->vout[cnt].cc_buf[1].handle, 0);
            g_vpp->vpp_objs[i]->vout[cnt].cc_buf[1].length = CC_DATA_QUEUE_SIZE * sizeof(UINT16);
            vbi_buf_reset(&g_vpp->vpp_objs[i]->vout[cnt].cc_buf[1]);

            VPP_SHM32_MALLOC(VPP_SHM_NONSECURE, (WSS_DATA_QUEUE_SIZE * sizeof(unsigned int)), &g_vpp->vpp_objs[i]->vout[cnt].wss_buf.handle);
            g_vpp->vpp_objs[i]->vout[cnt].wss_buf.addr = (UINT8*)VPP_SHM_GetVirtualAddress(g_vpp->vpp_objs[i]->vout[cnt].wss_buf.handle, 0);
            g_vpp->vpp_objs[i]->vout[cnt].wss_buf.length = WSS_DATA_QUEUE_SIZE * sizeof(unsigned int);
            vbi_buf_reset(&g_vpp->vpp_objs[i]->vout[cnt].wss_buf);

            VPP_SHM32_MALLOC(VPP_SHM_NONSECURE, (CGMS_DATA_QUEUE_SIZE * sizeof(unsigned int)), &g_vpp->vpp_objs[i]->vout[cnt].cgms_buf.handle);
            g_vpp->vpp_objs[i]->vout[cnt].cgms_buf.addr = (UINT8*)VPP_SHM_GetVirtualAddress(g_vpp->vpp_objs[i]->vout[cnt].cgms_buf.handle, 0);
            g_vpp->vpp_objs[i]->vout[cnt].cgms_buf.length = CGMS_DATA_QUEUE_SIZE * sizeof(unsigned int);
            vbi_buf_reset(&g_vpp->vpp_objs[i]->vout[cnt].cgms_buf);

            VPP_SHM32_MALLOC(VPP_SHM_NONSECURE, (TT_DATA_QUEUE_SIZE * sizeof(unsigned char)), &g_vpp->vpp_objs[i]->vout[cnt].tt_buf.handle);
            g_vpp->vpp_objs[i]->vout[cnt].tt_buf.addr = (UINT8*)VPP_SHM_GetVirtualAddress(g_vpp->vpp_objs[i]->vout[cnt].tt_buf.handle, 0);
            g_vpp->vpp_objs[i]->vout[cnt].tt_buf.length = TT_DATA_QUEUE_SIZE * sizeof(unsigned char);
            vbi_buf_reset(&g_vpp->vpp_objs[i]->vout[cnt].tt_buf);

            VPP_SHM32_MALLOC(VPP_SHM_NONSECURE, TT_DATA_LEN_PER_FRAME * sizeof(unsigned char), &g_vpp->vpp_objs[i]->vout[cnt].tt_dma_handle);
            g_vpp->vpp_objs[i]->vout[cnt].tt_dma_buf = (UINT8*)VPP_SHM_GetVirtualAddress(g_vpp->vpp_objs[i]->vout[cnt].tt_dma_handle, 0);
            MV_OSAL_Mutex_Create(&(g_vpp->vpp_objs[i]->vout[cnt].cc_buf_lock));
            MV_OSAL_Mutex_Create(&(g_vpp->vpp_objs[i]->vout[cnt].wss_buf_lock));
            MV_OSAL_Mutex_Create(&(g_vpp->vpp_objs[i]->vout[cnt].cgms_buf_lock));
            MV_OSAL_Mutex_Create(&(g_vpp->vpp_objs[i]->vout[cnt].tt_buf_lock));
        }else if (cnt == VOUT_HD){
            VPP_SHM32_MALLOC(VPP_SHM_NONSECURE, (WSS_DATA_QUEUE_SIZE * sizeof(unsigned int)), &g_vpp->vpp_objs[i]->vout[cnt].wss_buf.handle);
            g_vpp->vpp_objs[i]->vout[cnt].wss_buf.addr =(unsigned char*)VPP_SHM_GetVirtualAddress(g_vpp->vpp_objs[i]->vout[cnt].wss_buf.handle, 0);
            g_vpp->vpp_objs[i]->vout[cnt].wss_buf.length = WSS_DATA_QUEUE_SIZE * sizeof(unsigned int);
            vbi_buf_reset(&g_vpp->vpp_objs[i]->vout[cnt].wss_buf);

            VPP_SHM32_MALLOC(VPP_SHM_NONSECURE, (CGMS_DATA_QUEUE_SIZE * sizeof(unsigned int)), &g_vpp->vpp_objs[i]->vout[cnt].cgms_buf.handle);
            g_vpp->vpp_objs[i]->vout[cnt].cgms_buf.addr = (unsigned char*)VPP_SHM_GetVirtualAddress(g_vpp->vpp_objs[i]->vout[cnt].cgms_buf.handle, 0);
            g_vpp->vpp_objs[i]->vout[cnt].cgms_buf.length = CGMS_DATA_QUEUE_SIZE * sizeof(unsigned int);
            vbi_buf_reset(&g_vpp->vpp_objs[i]->vout[cnt].cgms_buf);

            MV_OSAL_Mutex_Create(&(g_vpp->vpp_objs[i]->vout[cnt].wss_buf_lock));
            MV_OSAL_Mutex_Create(&(g_vpp->vpp_objs[i]->vout[cnt].cgms_buf_lock));
        }
    } // <- config VOUTs

    /* End to allocate memory for be/LVDS */

    int ch;
    for(ch = 0;ch < VPP_MAX_VOP;ch++){
        /* Begin to allocate memory for cpcb */
        /* Command BCMBuf for updating Gamma LUT in CMU */
        if (!g_vpp->vpp_objs[i]->gamma.GmBcmBuf[ch][0]) {
#ifdef VPP_SHM_API
            VPP_SHM32_MALLOC(VPP_SHM_NONSECURE, GM_BCM_BUFFER_SIZE, &g_vpp->vpp_objs[i]->gamma.GmBcmBuf_handle[ch][0] );
            g_vpp->vpp_objs[i]->gamma.GmBcmBuf[ch][0] = (UINT32*)VPP_SHM_GetVirtualAddress(g_vpp->vpp_objs[i]->gamma.GmBcmBuf_handle[ch][0], 0);
#else
            g_vpp->vpp_objs[i]->gamma.GmBcmBuf[ch][0] = (UINT32*)GALOIS_32_MALLOC(GM_BCM_BUFFER_SIZE);
#endif
            if(NULL == g_vpp->vpp_objs[i]->gamma.GmBcmBuf[ch][0]){
                return MV_VPP_ENOMEM;
            }
        }

        if (!g_vpp->vpp_objs[i]->gamma.GmBcmBuf[ch][1]) {
#ifdef VPP_SHM_API
            VPP_SHM32_MALLOC(VPP_SHM_NONSECURE, GM_BCM_BUFFER_SIZE, &g_vpp->vpp_objs[i]->gamma.GmBcmBuf_handle[ch][1] );
            g_vpp->vpp_objs[i]->gamma.GmBcmBuf[ch][1] = (UINT32*)VPP_SHM_GetVirtualAddress(g_vpp->vpp_objs[i]->gamma.GmBcmBuf_handle[ch][1], 0);
#else
            g_vpp->vpp_objs[i]->gamma.GmBcmBuf[ch][1] = (UINT32*)GALOIS_32_MALLOC(GM_BCM_BUFFER_SIZE);
#endif
            if(NULL == g_vpp->vpp_objs[i]->gamma.GmBcmBuf[ch][1]){
                return MV_VPP_ENOMEM;
            }
        }
    }


    /* Begin to allocate memory for SCL Coeff */

    for (cnt=FIRST_SCALAR_COEFF; cnt<MAX_NUM_PREDEFINED_COEFFS; cnt++)
    {
        int sclLayer;
        int numOfSclLayers = 0;
        if(SCALAR_COEFF_MAIN_MIN<=cnt && SCALAR_COEFF_MAIN_MAX>=cnt)
        {
            numOfSclLayers = VPP_FRC_SCL_MAIN_LAY_MAX;
        }
        else
        {
            numOfSclLayers = VPP_FRC_SCL_GFX_LAY_MAX;
        }

        if(!g_vpp->vpp_objs[i]->scl_coeffs[cnt])
        {
            VPP_SHM32_MALLOC(VPP_SHM_NONSECURE, VPP_FRC_COEFFTAB_BCMBUF_SIZE * numOfSclLayers, &g_vpp->vpp_objs[i]->scl_coeffs_handle[cnt]);
            g_vpp->vpp_objs[i]->scl_coeffs[cnt] = (UINT32*)VPP_SHM_GetVirtualAddress(g_vpp->vpp_objs[i]->scl_coeffs_handle[cnt], 0);

            if(NULL == g_vpp->vpp_objs[i]->scl_coeffs[cnt])
            {
                return MV_VPP_ENOMEM;
            }
        }
        for(sclLayer = 0; sclLayer<numOfSclLayers; sclLayer++)
        {
            if(!g_vpp->vpp_objs[i]->cust_scl_coeffs[sclLayer][cnt])
            {
                VPP_SHM32_MALLOC(VPP_SHM_NONSECURE, VPP_FRC_SCL_COEFF_TAB_SIZE, &g_vpp->vpp_objs[i]->cust_scl_coeffs_handle[sclLayer][cnt]);
                g_vpp->vpp_objs[i]->cust_scl_coeffs[sclLayer][cnt] = (UINT16*) VPP_SHM_GetVirtualAddress(g_vpp->vpp_objs[i]->cust_scl_coeffs_handle[sclLayer][cnt], 0);
                if(NULL == g_vpp->vpp_objs[i]->cust_scl_coeffs[sclLayer][cnt])
                {
                    return MV_VPP_ENOMEM;
                }
            }
        }
    }
    /* End to allocate memory for SCL Coeff */

    /*Initialize watermark feature for VPP*/
    VPP_VMXAPI_Init(g_vpp->vpp_objs[i]);
    /* Begin to allocate memory for be/HDMI */
    if(VPP_BE_HdmiExt_Create(g_vpp->vpp_objs[i]) == MV_VPP_ENOMEM)
        return MV_VPP_ENOMEM;

    /* End to allocate memory for be/HDMI */

    /* End to allocate memory for be */

#ifdef VPP_ENABLE_FRAME_CAPTURE
    Ret = MV_OSAL_Sem_Create (&(g_vpp->vpp_objs[i]->vpp_FrameCapture_sem), 0);
    if (Ret != S_OK){
        return (Ret);
    }

    Ret = VPP_CreateFrameCaptureTask(g_vpp->vpp_objs[i]);
    if (Ret != S_OK){
        return (Ret);
    }
#endif

    if(VPP_DVOBJ_Init(g_vpp->vpp_objs[i]) != MV_VPP_OK) {
        return (MV_VPP_ENOMEM);
    }

    VPP_BE_MIPIOBJ_Create(g_vpp->vpp_objs[i]);

    *handle = i;

    g_vpp->vpp_objs[i]->povps_obj_params = &g_vpp->vpp_objs[i]->ovp_obj.ovps_obj_params;

    return (MV_VPP_OK);
}

/***********************************************
 * FUNCTION: destroy a VPP object
 * PARAMS: handle - VPP object handle
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EUNCONFIG - not initialized
 *         MV_VPP_ENODEV - no device
 *         MV_VPP_ENOMEM - no memory
 ***********************************************/
int MV_VPPOBJ_Destroy(int handle)
{
    int i;

    VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    if (g_vpp->vpp_objs[handle] == NULL)
        return (MV_VPP_ENODEV);

    MV_VPP_Stop(g_vpp->vpp_objs[handle]);

#ifndef CONFIG_VPP_ENABLE_INTERRUPT_ON_PUSHFRAME
    VPP_Disable_VDE_Interrupt(g_vpp->vpp_objs[handle], CPCB_1);
    VPP_Disable_ISR_Interrupt(g_vpp->vpp_objs[handle], CPCB_1);

#ifdef ENABLE_GFX2_VOP2_PATH
    VPP_Disable_ISR_Interrupt(g_vpp->vpp_objs[handle], CPCB_2);
#endif
#endif

    g_vpp->vpp_objs[handle]->vpp_isr_need_exit = 1;

#ifndef CONFIG_AMP_TEE_ENABLE
    MV_OSAL_Sem_Post(g_vpp->vpp_objs[handle]->vpp_dsr_sem);

    if (g_vpp->vpp_objs[handle]->vpp_dsr_task != NULL)
    {
        MV_OSAL_Task_Join(&g_vpp->vpp_objs[handle]->vpp_dsr_task);
        g_vpp->vpp_objs[handle]->vpp_dsr_task = NULL;
    }
    /* join task first, then clear the structure*/
    if (g_vpp->vpp_objs[handle]->vpp_dsr_sem != NULL)
    {
        MV_OSAL_Sem_Destroy (&(g_vpp->vpp_objs[handle]->vpp_dsr_sem));
        g_vpp->vpp_objs[handle]->vpp_dsr_sem = NULL;
    }

#endif //CONFIG_AMP_TEE_ENABLE

    /* unregister VPP interrupt service routines */
    VPP_ISR_UnRegistration(g_vpp->vpp_objs[handle]);
    VPP_CEC_ISR_UnRegistration(g_vpp->vpp_objs[handle]);

    /* free main video plane de-interlacer field buffers */
    if (g_vpp->vpp_objs[handle]->plane[PLANE_MAIN].vp_buff_base)
        VPP_SHM_Free(g_vpp->vpp_objs[handle]->plane[PLANE_MAIN].vp_buff_handle);

    /* free dhub config queue */
    for (i=FIRST_CPCB; i<MAX_NUM_CPCBS; i++) {
        /* free DMA dhub config queue */
        if (g_vpp->vpp_objs[handle]->dv[i].vbi_dma_cfgQ[0].addr)
            VPP_SHM_Free(g_vpp->vpp_objs[handle]->dv[i].vbi_dma_cfgQ[0].handle);
        if (g_vpp->vpp_objs[handle]->dv[i].vbi_dma_cfgQ[1].addr)
            VPP_SHM_Free(g_vpp->vpp_objs[handle]->dv[i].vbi_dma_cfgQ[1].handle);

        /* free VBI dhub config queue */
        if (g_vpp->vpp_objs[handle]->dv[i].vbi_bcm_cfgQ[0].addr)
            VPP_SHM_Free(g_vpp->vpp_objs[handle]->dv[i].vbi_bcm_cfgQ[0].handle);
        if (g_vpp->vpp_objs[handle]->dv[i].vbi_bcm_cfgQ[1].addr)
            VPP_SHM_Free(g_vpp->vpp_objs[handle]->dv[i].vbi_bcm_cfgQ[1].handle);

        /* free VDE dhub config queue */
        if (g_vpp->vpp_objs[handle]->dv[i].vde_bcm_cfgQ[0].addr)
            VPP_SHM_Free(g_vpp->vpp_objs[handle]->dv[i].vde_bcm_cfgQ[0].handle);
        if (g_vpp->vpp_objs[handle]->dv[i].vde_bcm_cfgQ[1].addr)
            VPP_SHM_Free(g_vpp->vpp_objs[handle]->dv[i].vde_bcm_cfgQ[1].handle);

        if (g_vpp->vpp_objs[handle]->dv[i].gammaLutinfo.pstgammaLut) {
            VPP_Free(g_vpp->vpp_objs[handle]->dv[i].gammaLutinfo.pstgammaLut);
        }
    }
#ifdef VPP_ENABLE_FRAME_CAPTURE
    MV_OSAL_Sem_Post(g_vpp->vpp_objs[handle]->vpp_FrameCapture_sem);
    VPP_DestroyFrameCaptureTask(g_vpp->vpp_objs[handle]);

    MV_OSAL_Sem_Destroy (&(g_vpp->vpp_objs[handle]->vpp_FrameCapture_sem));
    g_vpp->vpp_objs[handle]->vpp_FrameCapture_sem = NULL;
#endif

    /*Destroy watermark feature*/
    VPP_VMXAPI_Destroy(g_vpp->vpp_objs[handle]);

    VPP_DVOBJ_Destroy(g_vpp->vpp_objs[handle]);

    /* free vpp object memory */
    VPP_Free(g_vpp->vpp_objs[handle]);

    /* free handle */
    g_vpp->vpp_objs[handle] = NULL;

#if defined(VPP_SUS_RES)
    VPP_Free(g_vpp->vpp_suss[handle]->be.hdmiTx);
    /* free vpp suspend memory */
    VPP_Free(g_vpp->vpp_suss[handle]);

    /* free suspend handle */
    g_vpp->vpp_suss[handle] = NULL;
#endif
    g_vpp->magic = 0;

    //g_vpp is no more required, So un-map/un-register global memory
    MV_VPP_DeInitVppObj();

    return (MV_VPP_OK);
}

#if defined(VPP_SUS_RES)
#ifdef CONFIG_VPP_FASTLOGO_SUPPORT
#define VPP_LINUXSDK_FORCE_SETHDMIFMT_AFTER_DELAY
#endif
#define VPP_ENABLE_OPTIMIZED_METHOD_FOR_SUS_RES
#ifdef VPP_ENABLE_OPTIMIZED_METHOD_FOR_SUS_RES
int MV_VPPOBJ_Suspend_Optimized(int handle)
{
    int i;
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if(vpp_obj->suspend != POWER_MODE_NORMAL) {
       return MV_VPP_OK;
    }

#ifdef CONFIG_OVP_WITH_VPPLIB
    MV_OVPS_PreSuspend( );
#endif

    for(i=FIRST_CPCB; i<MAX_NUM_CPCBS; i++)
    {
        if (vpp_obj->dv[i].status != STATUS_INACTIVE)
        {
            /* reset CPCB TG */
            ResetCPCBTG(vpp_obj, i, vpp_obj->m_VoutMode);
#ifdef CONFIG_VPP_FASTLOGO_SUPPORT
            VPP_WaitUntilResetCPCB(vpp_obj, i);
#endif
            /* No need to set the 'status' to INACTIVE and output res is RESET
             * Instead go for force setting resolution */
        }
    }

    VPP_ISR_UnRegistration(vpp_obj);
    /* ISR is still coming During reset CPCB TG, So reset the vbi num here*/
    vpp_obj->dv[CPCB_1].vbi_num = 0;

    /* set the suspend flag */
    vpp_obj->suspend = POWER_MODE_SUSPEND_OPTIMIZED;

    return (MV_VPP_OK);

}

int MV_VPPOBJ_Resume_Optimized(int handle)
{
    VPP_OBJ *vpp_obj;
    int cpcbID = FIRST_CPCB;
    BE_HDMITX_DATA* pHdmiTx;
    VPP_DISP_OUT_PARAMS dispParams;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    pHdmiTx = vpp_obj->be.hdmiTx;

    if(vpp_obj->suspend != POWER_MODE_SUSPEND_OPTIMIZED) {
       return 0;
    }

    VPP_ISR_Registration(vpp_obj);

    /*Force set to previous CPCB resolution*/
    dispParams.iPixelRepeat = 1;

    if (IS_VOUT_DISP_MODE(SINGLE_MODE_PRI) || IS_VOUT_DISP_MODE_DUAL())
        dispParams.uiDispId = VOUT_HDMI;
    else
        dispParams.uiDispId = VOUT_DSI;

    dispParams.uiResId = VPP_GetResId(cpcbID)  | 0x80000000;
    dispParams.uiBitDepth = pHdmiTx->videoParams.ColorDepth;
    dispParams.uiColorFmt = pHdmiTx->videoParams.VideoFmt;
    dispParams.uiDisplayMode = vpp_obj->m_VoutMode;

    MV_VPPOBJ_SetFormat(handle, cpcbID, &dispParams);

#ifdef ENABLE_GFX2_VOP2_PATH
    if (IS_VOUT_DISP_MODE_DUAL())
    {
        dispParams.iPixelRepeat = 1;
        dispParams.uiDispId = VOUT_DSI;
        dispParams.uiResId = VPP_GetResId(CPCB_2)  | 0x80000000;
        dispParams.uiBitDepth = OUTPUT_BIT_DEPTH_8BIT;
        dispParams.uiColorFmt = OUTPUT_COLOR_FMT_RGB888;
        dispParams.uiDisplayMode = vpp_obj->m_VoutMode;
        MV_VPPOBJ_SetFormat(handle, 1, &dispParams);
    }
#endif

    /* reset the suspend flag */
    vpp_obj->suspend = POWER_MODE_NORMAL;

    return (MV_VPP_OK);
}
#endif //VPP_ENABLE_OPTIMIZED_METHOD_FOR_SUS_RES

/***************************************
 * FUNCTION: VPP Suspend
 * INPUT: NONE
 * RETURN: NONE
 **************************************/
int MV_VPPOBJ_Suspend(int handle, BOOL optimize)
{
    int retVal = MV_VPP_OK;
    VPP_OBJ *vpp_obj;
    VPP_OBJ *vpp_sus;
    void *ptr = NULL;
    void *ptr1 = NULL;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle < 0 || handle >= MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    vpp_sus = g_vpp->vpp_suss[handle];

    if (vpp_obj->suspend == POWER_MODE_SUSPEND)
        return MV_VPP_OK;

    if (vpp_obj->suspend == POWER_MODE_NORMAL) {
        // Backup current status to vpp_sus before freeze/suspend
        VPP_Memcpy(vpp_sus->be.hdmiTx, vpp_obj->be.hdmiTx, sizeof(BE_HDMITX_DATA));
        ptr = vpp_sus->be.hdmiTx;
#ifdef CONFIG_HDMI_CEC_SUPPORT
        VPP_Memcpy(vpp_sus->be.hdmiCec, vpp_obj->be.hdmiCec, sizeof(CEC_SRV_DATA));
        ptr1 = vpp_sus->be.hdmiCec;
#endif
        VPP_Memcpy(vpp_sus, vpp_obj, sizeof(VPP_OBJ));
        vpp_sus->be.hdmiTx = ptr;
#ifdef CONFIG_HDMI_CEC_SUPPORT
        vpp_sus->be.hdmiCec = ptr1;
#endif

        retVal = MV_VPPOBJ_Suspend_Optimized(handle);
#ifdef CONFIG_OVP_WITH_VPPLIB
        /* OVPS has clock dependency with VPP, So OVPS is suspend after VPP */
        MV_OVPS_Suspend();
#endif
        /*Gate Power/CLK here */
        MV_VPP_EnableClkGate();

    }
    // Only optimized suspend for Freeze call
    if (!optimize) {
        // Save the pll clocks
        MV_VPP_AVPLL_Save(vpp_obj->avpll_regs);
        vpp_obj->suspend = POWER_MODE_SUSPEND;
        g_vpp->suspend_status[handle] = 1;
#ifdef VPP_ENABLE_FRAME_CAPTURE
        MV_OSAL_Sem_Post(vpp_obj->vpp_FrameCapture_sem);
#endif
        retVal = VPP_BE_Suspend(vpp_obj);
    }
    return retVal;
}

/***************************************
 * FUNCTION: VPP Resume
 * INPUT: NONE
 * RETURN: NONE
 **************************************/
int MV_VPPOBJ_Resume(int handle, BOOL optimize)
{
    int i, step;
    VPP_OBJ *vpp_obj;
    VPP_OBJ *vpp_sus;
    int vinport_cfg[MAX_NUM_PLANES];
    int dv_cfg[MAX_NUM_CHANS];
    int zorder_cfg[MAX_NUM_CHANS];
    int voutport_cfg[MAX_NUM_VOUTS];
    int cpcbID, planeID, chanID;
    BE_HDMITX_DATA* pHdmiTx;
    CEC_SRV_DATA* pCecSrv;
    MV_VPP_EVENT_CALLBACK HdmiTxCbFn, CecCbFn;
    UINT32 HdmiTxEvtCode, CecEvtCode;
    void* HdmiTxCtxPara;
    void* CecCtxPara;
    unsigned int channel_init_mask_vpp = (1 << VPP_NUM_OF_CHANNELS) - 1;
    unsigned int channel_init_mask_ag = (1 << AG_NUM_OF_CHANNELS) - 1;

    /*Resume power/clock here*/
    MV_VPP_DisableClkGate();

    if (optimize)
    {
#ifdef CONFIG_OVP_WITH_VPPLIB
       MV_OVPS_Resume();
#endif
        return MV_VPPOBJ_Resume_Optimized(handle);
    }

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    if (g_vpp->suspend_status[handle] !=1)
        return (MV_VPP_EUNCONFIG);

    vpp_obj = g_vpp->vpp_objs[handle];
    vpp_sus = g_vpp->vpp_suss[handle];

    //AVIO Reset upon resume - Fix BCM hang issue
    VPP_AVIO_Reset();

    // Restore the pll clocks
    MV_VPP_AVPLL_Restore(vpp_obj->avpll_regs);
#ifdef VPP_ENABLE_AVIO_RESET_DURING_SUSPEND_RESUME
#ifdef VPP_DIAGS
    DhubInitialization(getMPid(), VPP_DHUB_BASE, VPP_HBO_SRAM_BASE, &VPP_dhubHandle, VPP_config, VPP_NUM_OF_CHANNELS, DHUB_TYPE_128BIT, channel_init_mask_vpp);
    DhubInitialization(getMPid(), AG_DHUB_BASE, AG_HBO_SRAM_BASE, &AG_dhubHandle, AG_config, AG_NUM_OF_CHANNELS, DHUB_TYPE_64BIT, channel_init_mask_ag);
#else
    //Initialize DHUB & BCM Schedular, We need VPP handle inside TA for DHUB operation
    DhubInitialization(0, VPP_DHUB_BASE, VPP_HBO_SRAM_BASE, &VPP_dhubHandle, VPP_config, VPP_NUM_OF_CHANNELS, channel_init_mask_vpp);
    DhubInitialization(0, AG_DHUB_BASE, AG_HBO_SRAM_BASE, &AG_dhubHandle, AG_config, AG_NUM_OF_CHANNELS, channel_init_mask_ag);
#endif
#endif //VPP_ENABLE_AVIO_RESET_DURING_SUSPEND_RESUME

    //Enable A/V-PLL clk enable
    MV_VPP_Clk_Ctrl();

   /* Initializing VPP module */
    pHdmiTx =(BE_HDMITX_DATA*) vpp_sus->be.hdmiTx;
    HdmiTxCbFn = pHdmiTx->appCbFn;
    HdmiTxEvtCode = pHdmiTx->eventCode;
    HdmiTxCtxPara = pHdmiTx->contextParam;

    pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);
    CecCbFn = pCecSrv->appCbFn;
    CecEvtCode = pCecSrv->eventCode;
    CecCtxPara = pCecSrv->contextParam;

    MV_VPPOBJ_UnRegisterHdmiCallback(handle);
    MV_VPPOBJ_UnRegisterHdcpCallback(handle);

#if CONFIG_HDMI_CEC_SUPPORT
    MV_VPPOBJ_CecUnregisterCallback(handle);
#endif

    MV_VPPOBJ_UnRegisterHdmiCpcb0Callback(handle);

    MV_VPPOBJ_Reset(handle);

    MV_VPPOBJ_EnableBypassMode(handle, vpp_sus->bypass_mode);

    for(i=FIRST_PLANE; i<MAX_NUM_PLANES;i++)
        vinport_cfg[i] = vpp_sus->plane[i].chanID;

    for(i=FIRST_CHAN; i<MAX_NUM_CHANS; i++)
    {
        dv_cfg[i] = vpp_sus->chan[i].dvID;
        zorder_cfg[i] = vpp_sus->chan[i].zorder;
    }

    for(i=FIRST_CPCB; i<MAX_NUM_VOUTS; i++)
        voutport_cfg[i] = vpp_sus->vout[i].dvID;

    MV_VPPOBJ_Config(handle, vinport_cfg, dv_cfg, zorder_cfg, voutport_cfg, vpp_sus->iVppFeatureCfg);
    VPP_BE_Resume(vpp_obj);

    if (vpp_obj->iVppFeatureCfg[0] & VPP_FEATURE_MIPI_DSI)
        VPP_BE_MIPI_DsiInit(vpp_obj);

    /* 1. Resuming CPCBs */
    for (cpcbID=FIRST_CPCB; cpcbID<MAX_NUM_CPCBS; cpcbID++)
    {
        if(vpp_sus->dv[cpcbID].status != STATUS_INACTIVE)
        {
            VPP_DISP_OUT_PARAMS dispParams;

            VPP_Memcpy(&dispParams, &vpp_sus->dv[cpcbID].dispOutParams,  sizeof(VPP_DISP_OUT_PARAMS));

            MV_VPPOBJ_SetFormat(handle, cpcbID, &dispParams);

            if(vpp_sus->dv[cpcbID].bgColor.ColorFmt != 0x7F7F7F7F)
                MV_VPPOBJ_SetCPCBBgColor(handle, cpcbID, &(vpp_sus->dv[cpcbID].bgColor));
        }
    }

    /* 2. Resuming planes */
    for(planeID=FIRST_PLANE; planeID<MAX_NUM_PLANES; planeID++)
    {
        if(vpp_sus->plane[planeID].status != STATUS_INACTIVE)
        {
            int dispMode;

            chanID = vpp_sus->plane[planeID].chanID;

            MV_VPPOBJ_OpenDispWindow(handle, planeID, &(vpp_sus->chan[chanID].disp_win), &(vpp_sus->chan[chanID].disp_win_attr));

            if(vpp_sus->plane[planeID].still_picture)
                MV_VPPOBJ_SetStillPicture(handle, planeID, vpp_sus->plane[planeID].still_picture, NULL);

            switch(vpp_sus->plane[planeID].status)
            {
            case STATUS_DISP_LOGO:
                dispMode = DISP_STILL_PIC;
                break;
            case STATUS_DISP_VIDEO:
                dispMode = DISP_FRAME;
                break;
            case STATUS_DISP_PATGEN:
                dispMode = DISP_PATGEN;
                break;
            default:
                dispMode = DISP_NON_DEFINED;
            }
            if(dispMode != DISP_NON_DEFINED)
                MV_VPPOBJ_SetDisplayMode(handle, planeID, dispMode);

            if(dispMode == DISP_FRAME)
            {
                // Restore the frames which were being handled while doing to suspend.
                vpp_obj->plane[planeID].curr_frame = vpp_sus->plane[planeID].curr_frame;
                vpp_obj->plane[planeID].prev_curr_frame = vpp_sus->plane[planeID].prev_curr_frame;
            }

            if(vpp_sus->plane[planeID].crop_win_valid)
                MV_VPPOBJ_SetCropWindow(handle, planeID, &(vpp_sus->plane[planeID].crop_win));

            MV_VPPOBJ_SetRefWindow(handle, planeID, &(vpp_sus->plane[planeID].ref_win));

            MV_VPPOBJ_SetScalerNonLinearControl(handle, planeID, vpp_sus->chan[chanID].scl_nl_enable, vpp_sus->chan[chanID].scl_nl_center, vpp_sus->chan[chanID].scl_nl_cratio);

            if(vpp_sus->plane[planeID].borderAlpha != 0x7F7F7F7F)
                MV_VPPOBJ_SetPlaneBorderAlpha(handle, planeID, vpp_sus->plane[planeID].borderAlpha);

            MV_VPPOBJ_SetPlaneMute(handle, planeID, vpp_sus->plane[planeID].mute);
            switch(planeID)
            {
            case PLANE_GFX1:
                if(vpp_sus->scl.gfxInputMode0 != 0x7F7F7F7F)
                    MV_VPPOBJ_SetGfxInputMode(handle, planeID, vpp_sus->scl.gfxInputMode0);
                break;
            //TBD - Check if we need to handle when PIP is in GFX mode
            default:
                ;
            }
        }
    }

#ifndef CONFIG_VPP_FASTLOGO_SUPPORT
    /* Wait for VBI interrupts to come through userspace */
    MV_VPPOBJ_GetCPCBInterruptCounter(handle, CPCB_1, &step);
    do {
	    /* Also check if interrupts started in kernel */
	    if (MV_VPPOBJ_isVBIIntrGenerated(vpp_obj))
		    break;
	    MV_VPPOBJ_GetCPCBInterruptCounter(handle, CPCB_1, &i);
	    MV_OSAL_Task_Sleep(100);
    } while ((i < step+1));
#endif

#ifdef ENABLE_PIP_GFX0_PATH
    /* 4. Resuming DVs, scl coeff */
    if((cpcbID = CPCB_OF_CHAN(vpp_obj, CHAN_PIP)) != CPCB_INVALID && vpp_sus->lumaKeyLow_key != 0x7F7F7F7F && vpp_sus->lumaKeyHigh_key != 0x7F7F7F7F)
        MV_VPPOBJ_SetLumaKey(handle, vpp_sus->dv[cpcbID].LumaKey, vpp_sus->lumaKeyLow_key, vpp_sus->lumaKeyHigh_key);
#endif

    /* 5. Resuming vout ports */
    MV_VPPOBJ_SetVoutMute(handle, VOUT_HD, vpp_sus->vout[VOUT_HD].mute);
    MV_VPPOBJ_SetVoutMute(handle, VOUT_SD, vpp_sus->vout[VOUT_SD].mute);

    /* 6. Resuming PQ settings */
    MV_VPPOBJ_SetDeintMode(handle, vpp_sus->plane[PLANE_MAIN].deint_mode);

    /* 7. Resuming Hdmi  */
    /* TODO: confirm if MV_VPPOBJ_Resume or MV_VPPOBJ_Resume_Optimized is the final version used. Add vpp_sus support depending on that */
    VPP_BE_SetHdmiPhyPower(vpp_obj, TRUE);
    MV_VPPOBJ_SetHdmiVideoFmt(handle, pHdmiTx->videoParams.VideoFmt, pHdmiTx->videoParams.ColorDepth, pHdmiTx->videoParams.PixelRpt + 1);
    //MV_VPPOBJ_SetHdmi3DStructure(handle, vpp_sus->hdmi3DStruct, vpp_sus->hdmi3DExt_data);
    //MV_VPPOBJ_EnableHdmiAudioFmt(handle, ((HDMITX_AUD_CTRL *)(&vpp_sus->be.hdmiTxDrv.audCtrl))->AudEnable);
    //MV_VPPOBJ_EnableHdmiAudioFmt(handle, ((HDMITX_AUD_CTRL *)(&vpp_sus->be.hdmiTxDrv.audCtrl))->AudEnable);

#ifdef XVYCC_SUPPORT
    MV_VPPOBJ_xvYCCEnable(handle, vpp_sus->xvYCC_enable);
#endif

#ifdef CONFIG_HDMI_CEC_SUPPORT
    // resuming CEC
    MV_VPPOBJ_CecConfigFeatureSupport(handle, vpp_sus->be.cecFeatureSupportCfg);
    MV_VPPOBJ_CecEnable(handle, vpp_sus->be.enableCec);

    if(vpp_sus->be.cecPhyAddr != VPP_CEC_INVALID_PHY_ADDR)
       MV_VPPOBJ_CecSetPhysicalAddr(handle, vpp_sus->be.cecPhyAddr);

    MV_VPPOBJ_CecRegisterCallback(handle, CecEvtCode, CecCbFn, CecCtxPara);
#endif

    /* 9. Resuming LVDS  */
    /* 10. Resuming Misc  */
    MV_VPPOBJ_RegisterHdmiCallback(handle, HdmiTxEvtCode, HdmiTxCbFn, HdmiTxCtxPara);
    MV_VPPOBJ_RegisterHdcpCallback(handle, HdmiTxEvtCode, HdmiTxCbFn, HdmiTxCtxPara);
    MV_VPPOBJ_RegisterHdmiCpcb0Callback(handle, vpp_sus->hdmi_cpcb0_dsr_cb);

    g_vpp->suspend_status[handle] = 0;
    vpp_obj->suspend = POWER_MODE_NORMAL;

#ifdef VPP_LINUXSDK_FORCE_SETHDMIFMT_AFTER_DELAY
    //LinuxSDK/VPP_Test_APP does not have application to force set resolution again, So try again here
    MV_OSAL_Task_Sleep(1600);
    MV_VPPOBJ_SetHdmiVideoFmt(handle, pHdmiTx->videoParams.VideoFmt, pHdmiTx->videoParams.ColorDepth, pHdmiTx->videoParams.PixelRpt + 1);
#endif
    return (MV_VPP_OK);
}
#else
int MV_VPPOBJ_Suspend(int handle, BOOL optimize)
{
    return (MV_VPP_OK);
}

int MV_VPPOBJ_Resume(int handle, BOOL optimize)
{
    return (MV_VPP_OK);
}
#endif //defined(VPP_SUS_RES)

/***************************************
 * FUNCTION: VPP reset
 * INPUT: NONE
 * RETURN: NONE
 **************************************/
int MV_VPPOBJ_Reset(int handle)
{
    int i;
    VPP_OBJ *vpp_obj;

#ifdef BERLIN_BG2_BCM_SCHE_IN_KERNEL
    unsigned int bcm_sched_cmd[3] = {0}, cnt;
#endif
    VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    //Initialize debug utility framework
    VPP_DEBUG_UTIL_init(vpp_obj);

#ifndef CONFIG_AMP_TEE_ENABLE
    /* clean up bcm command in kernel */
#ifdef BERLIN_BG2_BCM_SCHE_IN_KERNEL
    for (cnt=0; cnt<=5; cnt++) {
        bcm_sched_cmd[2] = cnt;
        ioctl(berlin_pe_dev, VPP_IOCTL_BCM_SCHE_CMD, bcm_sched_cmd);
    }
#endif
#else //CONFIG_AMP_TEE_ENABLE
    //if required we may have to invoke VPP_CB_CLEANUPBCM
#endif //CONFIG_AMP_TEE_ENABLE

    // ASSERT a VPP glocal reset signal
    // reset VPP object variable
    vpp_obj->status = STATUS_INACTIVE;
    vpp_obj->suspend = 0;

#ifdef XVYCC_SUPPORT
    vpp_obj->xvYCC_enable = TRUE;
    vpp_obj->xvYCC_mode = 0; //file mode
#endif

    /*Disable BT2020 by default*/
    vpp_obj->BT2020_capability = FALSE;
    /*Disable HDR by default*/
    vpp_obj->HDR_capable = FALSE;
    vpp_obj->isHDRContent = FALSE;
    vpp_obj->HLG_capable = FALSE;
    vpp_obj->HDR10PLUS_capable = FALSE;
    vpp_obj->HDR_Status = FALSE;
    //vpp_obj->MV_enable = FALSE;
    vpp_obj->MV_enable = TRUE;

    vpp_obj->m_VoutMode = VOUT_DISP_SINGLE_MODE_PRI;
#ifdef PE_3D_FMT_CONVERT
    vpp_obj->src_3D_cvt_mode = SRC_3D_FMT_AUTO;
#endif

    vpp_obj->need_dcm_update = 0;

    VPP_DVOBJ_Reset(vpp_obj);

    for (i = VPP_FIRST_CHANNEL; i  <VPP_MAX_CHANNELS; i++){
        vpp_obj->gSaturation[i]= VPP_DEFAULT_SATURATION;
        vpp_obj->gHue[i] = VPP_DEFAULT_HUE;
        vpp_obj->gContrast[i] = VPP_DEFAULT_CONTRAST;
        vpp_obj->gBrightness[i] = VPP_DEFAULT_BRIGHTNESS;
    }

    /* reset planes */
    for (i = FIRST_PLANE; i < MAX_NUM_PLANES; i++){
        vpp_obj->plane[i].status = STATUS_INACTIVE;
        vpp_obj->plane[i].chanID = -1; // invalid
        vpp_obj->plane[i].curr_frame = NULL;
        vpp_obj->plane[i].prev_curr_frame = NULL;
        vpp_obj->plane[i].curr_still_picture = NULL;
        vpp_obj->plane[i].still_picture = NULL;
        vpp_obj->plane[i].srcfmt = -1; // invalid
        vpp_obj->plane[i].order = -1; // invalid
        vpp_obj->plane[i].bits_per_pixel = -1; // invalid


        vpp_obj->plane[i].frm_count = 0;
        vpp_obj->plane[i].pts.hi_word = 0;
        vpp_obj->plane[i].pts.lo_word = 0;
        vpp_obj->plane[i].delta_pts.hi_word = 0;
        vpp_obj->plane[i].delta_pts.lo_word = 0;

        vpp_obj->plane[i].actv_win.x = 0;
        vpp_obj->plane[i].actv_win.y = 0;
        vpp_obj->plane[i].actv_win.width = 0;
        vpp_obj->plane[i].actv_win.height = 0;

        vpp_obj->plane[i].frame_type = -1; // invalid
        vpp_obj->plane[i].field = 0; // top field
        vpp_obj->plane[i].vp_buff_w_index = -1;
        vpp_obj->plane[i].vp_buff_r0_index = -1;
        vpp_obj->plane[i].vp_buff_r1_index = -1;
        vpp_obj->plane[i].vp_buff_num = 0;

        vpp_obj->plane[i].deint_mode = -1; // invalid mode
        vpp_obj->plane[i].DeintPreset.ViMode = VPP_DEINTMODE_SAFE;
        vpp_obj->plane[i].DeintPreset.ChrMotVidMode = VPP_DEINTCHROMA_MOT_MAX_OF_CHROMA_AND_LUMA_MOT;
        vpp_obj->plane[i].DeintPreset.ChrMotFlmMode = TRUE;
        vpp_obj->plane[i].vp_updated = 0;

        vpp_obj->plane[i].ref_win.x = 0;
        vpp_obj->plane[i].ref_win.y = 0;
        vpp_obj->plane[i].ref_win.width = 0;
        vpp_obj->plane[i].ref_win.height = 0;
        vpp_obj->plane[i].ref_win_updated = 0;

        vpp_obj->plane[i].crop_win_valid = 0;
        vpp_obj->plane[i].source_win_valid = -1;   //-1 enable initial logging of invlid window
        vpp_obj->plane[i].crop_win.x = 0;
        vpp_obj->plane[i].crop_win.y = 0;
        vpp_obj->plane[i].crop_win.width = 0;
        vpp_obj->plane[i].crop_win.height = 0;

        vpp_obj->plane[i].skip_frame = 0;

        vpp_obj->plane[i].mute = 0; // un-mute

        vpp_obj->plane[i].m_leftpadadjust_y = 0;
        vpp_obj->plane[i].m_leftpad_y = 0;
        vpp_obj->plane[i].m_rightpad_y = 0;
        vpp_obj->plane[i].m_toppad_y = 0;
        vpp_obj->plane[i].m_toppadadjust_y = 0;
        vpp_obj->plane[i].m_bottompad_y = 0;

        vpp_obj->plane[i].m_leftpad_c = 0;
        vpp_obj->plane[i].m_leftpadadjust_c = 0;
        vpp_obj->plane[i].m_rightpad_c = 0;
        vpp_obj->plane[i].m_toppad_c = 0;
        vpp_obj->plane[i].m_toppadadjust_c = 0;
        vpp_obj->plane[i].m_bottompad_c = 0;
        vpp_obj->plane[i].m_PicHeightAdjusted = 0;
        vpp_obj->plane[i].offline_scl_bpp = 24;
        vpp_obj->plane[i].offline_scl_srcfmt = SRCFMT_YUV422;
        vpp_obj->plane[i].uiDolbyPresentMode = 0;
        vpp_obj->plane[i].m_SampleCropEnable = 0;
        vpp_obj->plane[i].m_SampleCropEnable_el = 0;
#ifdef CONFIG_HARDCODE_BUILT_IN_FRAME_SCL_TO_1_1
        vpp_obj->plane[i].ref_win_user.x        = 0;
        vpp_obj->plane[i].ref_win_user.y        = 0;
        vpp_obj->plane[i].ref_win_user.width    = 0;
        vpp_obj->plane[i].ref_win_user.height   = 0;
        vpp_obj->plane[i].ref_win_user_init     = 0;
#endif
    }

    /* reset channels */
    for (i=FIRST_CHAN; i<MAX_NUM_CHANS; i++) {
        vpp_obj->chan[i].status = STATUS_INACTIVE;
        vpp_obj->chan[i].dvID = -1; // invalid
        vpp_obj->chan[i].dvlayerID = -1; // invalid
        vpp_obj->chan[i].zorder = -1; // invalid

        vpp_obj->chan[i].num_of_planes = 0;

        vpp_obj->chan[i].scl_nl_enable = 0;
        vpp_obj->chan[i].scl_nl_center = 0x80;
        vpp_obj->chan[i].scl_nl_cratio = 0x40;

        vpp_obj->chan[i].disp_win.x = 0;
        vpp_obj->chan[i].disp_win.y = 0;
        vpp_obj->chan[i].disp_win.width = 0;
        vpp_obj->chan[i].disp_win.height = 0;
        vpp_obj->chan[i].disp_win_updated = 0;
        vpp_obj->chan[i].disp_win_attr.bgcolor = 0;
        vpp_obj->chan[i].disp_win_attr.alpha = 0;
        vpp_obj->chan[i].disp_win_attr.globalAlphaFlag = 0;

#ifdef CONFIG_HARDCODE_BUILT_IN_FRAME_SCL_TO_1_1
        vpp_obj->chan[i].disp_win_user.x = 0;
        vpp_obj->chan[i].disp_win_user.y = 0;
        vpp_obj->chan[i].disp_win_user.width = 0;
        vpp_obj->chan[i].disp_win_user.height = 0;
        vpp_obj->chan[i].disp_win_user_updated = 0;
        vpp_obj->chan[i].built_in_frame_pushed = 0;
        vpp_obj->chan[i].ref_win_user.x = 0;
        vpp_obj->chan[i].ref_win_user.y = 0;
        vpp_obj->chan[i].ref_win_user.width = 0;
        vpp_obj->chan[i].ref_win_user.height = 0;
#endif
    }

    /* reset DVs */
    for (i=FIRST_CPCB; i<MAX_NUM_CPCBS; i++) {
        vpp_obj->dv[i].status = STATUS_INACTIVE;
        VPP_SetResId(i, RES_INVALID);
        vpp_obj->dv[i].output_res_updated = 0;
        vpp_obj->dv[i].num_of_chans = 0;
        vpp_obj->dv[i].num_of_vouts = 0;
        vpp_obj->dv[i].LumaKey = 0;

        vpp_obj->dv[i].vbi_num = 0;
        cmdq_reset(&(vpp_obj->dv[i].cmdq));

        vpp_obj->dv[i].vco_freq = -1;
        vpp_obj->vde_int_enable[i] = 0;

        vpp_obj->enable_vbibcm_autopush[i] = FALSE;
        vpp_obj->uiUfCount[i] = 0;
    }

    vpp_obj->dsiInfo.CurrentResID = RES_INVALID;

    /* reset VOUTs */
    for (i=FIRST_VOUT; i<MAX_NUM_VOUTS; i++) {
        vpp_obj->vout[i].status = STATUS_INACTIVE;
        vpp_obj->vout[i].dvID = -1; // invalid

        vpp_obj->vout[i].cc_buf[0].addr = NULL;
        vpp_obj->vout[i].cc_buf[0].length = 0;
        vbi_buf_reset(&vpp_obj->vout[i].cc_buf[0]);

        vpp_obj->vout[i].cc_buf[1].addr = NULL;
        vpp_obj->vout[i].cc_buf[1].length = 0;
        vbi_buf_reset(&vpp_obj->vout[i].cc_buf[1]);

        vpp_obj->vout[i].wss_buf.addr = NULL;
        vpp_obj->vout[i].wss_buf.length = 0;
        vbi_buf_reset(&vpp_obj->vout[i].wss_buf);

        vpp_obj->vout[i].cgms_buf.addr = NULL;
        vpp_obj->vout[i].cgms_buf.length = 0;
        vbi_buf_reset(&vpp_obj->vout[i].cgms_buf);

        vpp_obj->vout[i].tt_buf.addr = NULL;
        vpp_obj->vout[i].tt_buf.length = 0;
        vbi_buf_reset(&vpp_obj->vout[i].tt_buf);

        vpp_obj->vout[i].tt_ctrl = -1; // invalid
        vpp_obj->vout[i].tt_dma_buf = NULL;

        vpp_obj->vout[i].cc_top_field_enable = 0;
        vpp_obj->vout[i].cc_bottom_field_enable = 0;

    }

    vpp_obj->main_last_frame = NULL;
    vpp_obj->main_frame_repeat = 0;
    vpp_obj->main_last_takeout_field = 0;
    vpp_obj->main_frame_first_field_cnt =0;

    /* reset event callbacks */
//    VPP_EventCallback_Reset(vpp_obj);

    vpp_obj->mv_mode = 0;
    vpp_obj->HDR_enable = FALSE;
    vpp_obj->HDR_Status = FALSE;
    vpp_obj->m_HdmiOutputMode = VPP_HDMI_OUTPUT_MODE_SDR;
    vpp_obj->m_HdrMode = VPP_HDR_MODE_DEFAULT;
    vpp_obj->clrtemp.Rgain = 1;
    vpp_obj->clrtemp.Ggain = 1;
    vpp_obj->clrtemp.Bgain = 1;

    vpp_obj->be.enableCec = FALSE;
    vpp_obj->bypass_mode = 0;



    for (i = FIRST_CPCB; i < MAX_NUM_CPCBS; i++) {
        /* reset VBI BCM buffer */
        VPP_BCMBUF_Reset(&vpp_obj->vbi_bcm_buf[i][0]);
        VPP_BCMBUF_Reset(&vpp_obj->vbi_bcm_buf[i][1]);
        vpp_obj->pVbiBcmBufCpcb[i] = &vpp_obj->vbi_bcm_buf[i][0];

        /* reset VBI CLEAR BCM buffer */
        VPP_BCMBUF_Reset(&vpp_obj->vbi_clear_bcm_buf[i][0]);
        VPP_BCMBUF_Reset(&vpp_obj->vbi_clear_bcm_buf[i][1]);
        vpp_obj->pVbiClearBcmBufCpcb[i] = &vpp_obj->vbi_clear_bcm_buf[i][0];
    }
    vpp_obj->pVbiBcmBuf = &vpp_obj->vbi_bcm_buf[0][0];
    vpp_obj->pVbiClearBcmBuf = &vpp_obj->vbi_clear_bcm_buf[0][0];

    /* reset dHub cmdQ */
    for (i = 0; i < MAX_DHUB_CHANNEL_NUMBER; i++)
        vpp_obj->dhub_cmdQ[i] = 0;

    MV_OSAL_Mutex_Lock(vpp_obj->bcmbuf_lock);

    for (i=FIRST_CPCB; i<MAX_NUM_CPCBS; i++) {
        VPP_BCMBUF_Reset(&vpp_obj->vde_bcm_buf[i][0]);
        VPP_BCMBUF_Reset(&vpp_obj->vde_bcm_buf[i][1]);
        vpp_obj->pVdeBcmBuf[i] = &vpp_obj->vde_bcm_buf[i][0];
        //Enable the canvas change flag, so that inline line dropping is forced
        //for 1st frame, when offline scaling is required
        vpp_obj->resIdUpdated[i] = 1;
    }

    /* select BCM sub-buffer to dump register settings */
    VPP_BCMBUF_Select(vpp_obj->pVbiBcmBuf, -1);

    GA_REG_WORD32_READ(vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL7, &vpp_obj->stMainPlCtrl7.u32);
    vpp_obj->stMainPlCtrl7.uCTRL7_hdrlut_bypass = 0;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL7, vpp_obj->stMainPlCtrl7.u32);

    GA_REG_WORD32_READ(vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_SCL_CLKEN_CTRL, &vpp_obj->stVidplSclClkenCtrl.u32);
    /** For GITM    **/
    GA_REG_WORD32_READ(vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_CTRL10, &vpp_obj->m_stCpcb0_ctrl10.u32);
        /** For VDCM **/
#ifdef CONFIG_AMP_DCM
    GA_REG_WORD32_READ(vpp_obj->dcm_base_addr_mp + RA_VPP_MP_VDCM + RA_VDCM_YAOI, &(vpp_obj->plane[PLANE_MAIN].m_uiDcmAoiReg));
#endif

    VPP_FE_Reset(vpp_obj);
    VPP_SCL_Reset(vpp_obj);
    VPP_CSC_Reset(vpp_obj);
    VPP_CPCB_Reset(vpp_obj);
    VPP_BE_Reset(vpp_obj);
#ifndef CONFIG_VPP_DISABLE_CMU
    VPP_CMU_Reset(vpp_obj);
#endif //CONFIG_VPP_DISABLE_CMU
#ifndef CONFIG_VPP_DISABLE_GDETEE
    VPP_DETEE_reset(vpp_obj);
#endif //CONFIG_VPP_DISABLE_GDETEE
    VPP_AVIOMTR_Reset(vpp_obj, VPP_AVIOMTR_CONFIG_VPP);

    VPP_NGPTV_Reset(vpp_obj);

    VPP_REG_WORD32_READ(vpp_obj->base_addr + RA_Vpp_rst, &(vpp_obj->vpp_rst.u32));
    VPP_REG_WORD32_READ(vpp_obj->base_addr + RA_Vpp_rst1, &(vpp_obj->vpp_rst1.u32));

    vpp_obj->vpp_rst1.urst1_ngptv = 0;
    vpp_obj->vpp_rst1.urst1_ovpScl_ngptv = 0;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_rst1, vpp_obj->vpp_rst1.u32);
    vpp_obj->vpp_rst1.urst1_ngptv = 1;
    vpp_obj->vpp_rst1.urst1_ovpScl_ngptv = 1;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_rst1, vpp_obj->vpp_rst1.u32);


    /* start BCM engine to program VPP registers */
    VPP_BCMBUF_HardwareTrans(vpp_obj->pVbiBcmBuf, 1/*block*/);

    MV_OSAL_Mutex_Unlock(vpp_obj->bcmbuf_lock);

    vpp_obj->enClkGate = 0; /* Disable dynamic clock gate by default */
    vpp_obj->m_uchDvEnableDump = 0;  /* Disable dolby frame dump by default */

    return (MV_VPP_OK);
}

/***********************************************
 * FUNCTION: Unregiste VPP interrupts
 * PARAMS: handle - VPP object handle
 * RETURN: MV_VPP_OK - succeed
 ***********************************************/
#ifndef CONFIG_AMP_TEE_ENABLE
void VPP_ISR_Unregistration(int handle)
{
    VPP_OBJ *vpp_obj = g_vpp->vpp_objs[handle];
    int cpu_id = 0;
    VPP_INTR_MSG intr_info = {0, 0};

    vpp_obj->pSemHandle = dhub_semaphore(&VPP_dhubHandle.dhub);
    /* configure and enable CPCB0 interrupt */
    semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppCPCB0_intr, 1, 0);
    semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppCPCB0_intr);
    semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppCPCB0_intr, 0/*empty*/, 0/*full*/, 0/*almost empty*/, 0/*almost full*/, cpu_id/*cpu id*/);
    intr_info.DhubSemMap = avioDhubSemMap_vpp_vppCPCB0_intr;
    ioctl(berlin_pe_dev, VPP_IOCTL_INTR_MSG, &intr_info);

    semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr, 1, 0);
    semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr);
    semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr, 0/*empty*/, 0/*full*/, 0/*almost empty*/, 0/*almost full*/, cpu_id/*cpu id*/);
    intr_info.DhubSemMap = avioDhubSemMap_vpp_vppOUT4_intr;
    ioctl(berlin_pe_dev, VPP_IOCTL_INTR_MSG, &intr_info);

    semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT3_intr, 1, 0);
    semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT3_intr);
    semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT3_intr, 0/*empty*/, 0/*full*/, 0/*almost empty*/, 0/*almost full*/, cpu_id/*cpu id*/);
    intr_info.DhubSemMap = avioDhubSemMap_vpp_vppOUT3_intr;
    ioctl(berlin_pe_dev, VPP_IOCTL_INTR_MSG, &intr_info);

    /* configure and enable Dhub channel #10 interrupt - HDMI audio Dhub channel Interrupt */
    semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp_CH10_intr, 1, 0);
    semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp_CH10_intr);
    semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp_CH10_intr, 0/*empty*/, 0/*full*/, 0/*almost empty*/, 0/*almost full*/, cpu_id/*cpu id*/);
    intr_info.DhubSemMap = avioDhubSemMap_vpp_CH10_intr;
    ioctl(berlin_pe_dev, VPP_IOCTL_INTR_MSG, &intr_info);

    /* configure and enable VOUT0 interrupt - HDCP Interrupt */
    semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT0_intr, 1, 0);
    semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT0_intr);
    semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT0_intr, 0/*empty*/, 0/*full*/, 0/*almost empty*/, 0/*almost full*/, cpu_id/*cpu id*/);
    intr_info.DhubSemMap = avioDhubSemMap_vpp_vppOUT0_intr;
    ioctl(berlin_pe_dev, VPP_IOCTL_INTR_MSG, &intr_info);

    return;
}
#endif //CONFIG_AMP_TEE_ENABLE

/********************************************************************
 * FUNCTION: Enable/Disable VPP video bypass mode
 * INPUT: flag: 1: enable; 0: disable
 * RETURN: NONE
 * NOTE: In video bypass mode, video format keeps in 4:2:2 format
 *       while going through VPP pipeline.
 *       This API needs to be called before MV_VPP_Config().
 ********************************************************************/
int MV_VPPOBJ_EnableBypassMode(int handle, int flag)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    vpp_obj->bypass_mode = flag;

    return (MV_VPP_OK);
}

/***************************************
 * FUNCTION: VPP profile configuration
 * INPUT: NONE
 * RETURN: NONE
 **************************************/
int MV_VPPOBJ_Config(int handle, const int *pvinport_cfg, const int *pdv_cfg, const int *pzorder_cfg, const int *pvoutport_cfg, const int *pfeature_cfg)
{
    int i, j;
    VPP_OBJ *vpp_obj;
    unsigned int uiRegAddr = 0;

    VPPAPI_DBG_INFO("enter\n");
    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (!pvinport_cfg || !pdv_cfg || !pvoutport_cfg)
        return (MV_VPP_EBADPARAM);

    /* VPP module has been configured */
    if (vpp_obj->status == STATUS_ACTIVE)
        return (MV_VPP_OK);

#ifdef TZ_3_0
    vpp_obj->pSemHandle = dhub_semaphore(&VPP_dhubHandle.dhub);
#endif

    /* unregister VPP interrupt service routines */
    VPP_ISR_UnRegistration(vpp_obj);
    VPP_CEC_ISR_UnRegistration(g_vpp->vpp_objs[handle]);

    /* config planes */
    for (i=FIRST_PLANE; i<MAX_NUM_PLANES; i++){

        /* set plane's DMA channel ID */
        if (i == PLANE_MAIN){
            /*FIXME: VPP_CT changes*/
            vpp_obj->plane[i].dmaRID[0] = avioDhubChMap_vpp128b_MV_R0; // inline read DMA
            vpp_obj->plane[i].dmaRID[1] = avioDhubChMap_vpp128b_MV_R1; // inline read DMA
#ifdef ENABLE_OVP
            vpp_obj->plane[i].dmaRID[2] = avioDhubChMap_vpp128b_MV_R2; // inline read DMA
            vpp_obj->plane[i].dmaRID[3] = avioDhubChMap_vpp128b_MV_R3; // inline read DMA
#endif
#ifdef TZ_3_0
            vpp_obj->plane[i].dmaRdhubID = &(vpp_obj->VPP_dhubHandle);
#else
            vpp_obj->plane[i].dmaRdhubID = &VPP_dhubHandle;
#endif

#ifdef CONFIG_DEINT_ENABLE
            vpp_obj->plane[i].deint_dmaWID = avioDhubChMap_vpp128b_DINT_W; // de-interlacer write-back DMA
            vpp_obj->plane[i].deint_dmaR0ID = avioDhubChMap_vpp128b_DINT0_R; // de-interlacer read-back 0 DMA
            vpp_obj->plane[i].deint_dmaR1ID = avioDhubChMap_vpp128b_DINT1_R; // de-interlacer read-back 1 DMA

#ifdef TZ_3_0
            vpp_obj->plane[i].deint_dmaR0dhubID = &(vpp_obj->VPP_dhubHandle);
            vpp_obj->plane[i].deint_dmaWdhubID = &(vpp_obj->VPP_dhubHandle);
            vpp_obj->plane[i].deint_dmaR1dhubID = &(vpp_obj->VPP_dhubHandle);
#else
            vpp_obj->plane[i].deint_dmaR0dhubID = &VPP_dhubHandle;
            vpp_obj->plane[i].deint_dmaWdhubID = &VPP_dhubHandle;
            vpp_obj->plane[i].deint_dmaR1dhubID = &VPP_dhubHandle;
#endif
#endif
            vpp_obj->plane[i].edr_dmaRID = avioDhubChMap_vpp128b_DBEDR_RD; // EDR read DMA
            vpp_obj->plane[i].mdrd_dmaRID = avioDhubChMap_vpp128b_MetaData_RD; // MDS read DMA

#ifdef TZ_3_0
            vpp_obj->plane[i].edr_dmaRdhubID = (int)(&(vpp_obj->VPP_dhubHandle));
            vpp_obj->plane[i].mdrd_dmaRdhubID = (int)(&(vpp_obj->VPP_dhubHandle));
#else
            vpp_obj->plane[i].edr_dmaRdhubID = (int)(&VPP_dhubHandle);
            vpp_obj->plane[i].mdrd_dmaRdhubID = (int)(&VPP_dhubHandle);
#endif

            vpp_obj->plane[i].m_mtrR_chkSemId_Y  = VPP_MTRR_CHKSEM_ID_0;
            vpp_obj->plane[i].m_mtrR_chkSemId_UV = VPP_MTRR_CHKSEM_ID_1;
            vpp_obj->plane[i].m_mtrR_chkSemId_Y_El  = VPP_MTRR_CHKSEM_ID_2;
            vpp_obj->plane[i].m_mtrR_chkSemId_UV_El = VPP_MTRR_CHKSEM_ID_3;

#ifndef CONFIG_VPP_DISABLE_OFFLINE_SCALER_BUFFER
            VPPAPI_DBG_INFO("Offline buffer address for plane %d are ", i);
            for(j = 0; j < OFFLINE_NO_OF_BUFFERS; j++)
            {
                if(!vpp_obj->plane[i].offline_buff_descrs[j].addr)
                {
                    VPP_SHM1K_MALLOC(VPP_SHM_NONSECURE, OFFLINE_BUFFER_SIZE, &vpp_obj->plane[i].offline_buff_descrs[j].handle);
#if defined(BERLIN_SINGLE_CPU)
                    if (!(vpp_obj->plane[i].offline_buff_descrs[j].addr = (void *)VPP_SHM_GetPhysicalAddress(vpp_obj->plane[i].offline_buff_descrs[j].handle, 0)))
#else
                        if (!(vpp_obj->plane[i].offline_buff_descrs[j].addr = (void *)VPP_SHM_GetVirtualAddress(vpp_obj->plane[i].offline_buff_descrs[j].handle, 0)))
#endif
                            return (MV_VPP_ENOMEM);
                }
                VPPAPI_DBG_INFO("%p, ", vpp_obj->plane[i].offline_buff_descrs[j].addr);
            }
            VPPAPI_DBG_INFO("\n");
#endif //CONFIG_VPP_DISABLE_OFFLINE_SCALER_BUFFER

            uiRegAddr = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE;
            GA_REG_WORD32_READ(uiRegAddr + RA_HDMIRX_PIPE_CTRL, &vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.u32);
            GA_REG_WORD32_READ(uiRegAddr + RA_HDMIRX_PIPE_CTRL0, &vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl0.u32);
            GA_REG_WORD32_READ(uiRegAddr + RA_HDMIRX_PIPE_BYPASS_CTRL, &vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.u32);
            GA_REG_WORD32_READ(uiRegAddr + RA_HDMIRX_PIPE_CFG0, &vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg0.u32);
            GA_REG_WORD32_READ(uiRegAddr + RA_HDMIRX_PIPE_CFG01, &vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg01.u32);
            GA_REG_WORD32_READ(uiRegAddr + RA_HDMIRX_PIPE_CFG4, &vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg4.u32);
        }
#ifdef ENABLE_PIP_GFX0_PATH //TODO: Use static table to disable the plane, if(vpp_plane_map[PlaneId].is_valid)
        else if (i == PLANE_PIP){
            vpp_obj->plane[i].dmaRID[0] = avioDhubChMap_vpp128b_GFX_PIP_R0; // inline read DMA
            vpp_obj->plane[i].dmaRID[1] = avioDhubChMap_vpp128b_PIP_R1; // inline read DMA

#ifdef TZ_3_0
            vpp_obj->plane[i].dmaRdhubID = &(vpp_obj->VPP_dhubHandle);
#else
            vpp_obj->plane[i].dmaRdhubID = &VPP_dhubHandle;
#endif
            vpp_obj->plane[i].m_mtrR_chkSemId_Y  = VPP_MTRR_CHKSEM_ID_2;
            vpp_obj->plane[i].m_mtrR_chkSemId_UV = VPP_MTRR_CHKSEM_ID_3;
        }
#endif
        else if (i == PLANE_GFX1)
        {
            vpp_obj->plane[i].dmaRID[0] = avioDhubChMap_vpp128b_GFX_R1; // inline read DMA
#ifdef TZ_3_0
            vpp_obj->plane[i].dmaRdhubID = &(vpp_obj->VPP_dhubHandle);
#else
            vpp_obj->plane[i].dmaRdhubID = &VPP_dhubHandle;
#endif
        }
#ifdef ENABLE_GFX2_VOP2_PATH
        else if (i == PLANE_GFX2){
            vpp_obj->plane[i].dmaRID[0] = avioDhubChMap_aio64b_GFX2_R; // inline read DMA
            vpp_obj->plane[i].dmaRdhubID = &AG_dhubHandle;
        }
#endif
        else if (PLANE_VMX == i)
        {
            vpp_obj->plane[i].dmaRID[0] = avioDhubChMap_vpp128b_VMM_VX_R; // inline read DMA

#ifdef TZ_3_0
            vpp_obj->plane[i].dmaRdhubID = &(vpp_obj->VPP_dhubHandle);
#else
            vpp_obj->plane[i].dmaRdhubID = &VPP_dhubHandle;
#endif
        }
#ifdef ENABLE_EL_PIPE_PATH
        else if (i == PLANE_OVP_EL){ //TODO: VPP_SW_BRINGUP: Review any merge issues
            vpp_obj->plane[i].dmaRID[0] = avioDhubChMap_vpp128b_MV_R2; // inline read DMA
            vpp_obj->plane[i].dmaRID[1] = avioDhubChMap_vpp128b_MV_R3; // inline read DMA
            vpp_obj->plane[i].dmaRdhubID = (int)(&VPP_dhubHandle);
        }
#endif
        /* set channel ID this plane belongs to */
        vpp_obj->plane[i].chanID = pvinport_cfg[i];

#if defined(PE_3D_BD)
        vpp_obj->plane[i].ss_type = 1;//SS BB mode
        vpp_obj->plane[i].ss_offset_mode = SS_OFFSET_NONE;
        vpp_obj->plane[i].ss_offset_default = 0;
        vpp_obj->plane[i].ss_offset_seq_id = 0;
        vpp_obj->plane[i].ss_offset_current = 0;
#endif
        /* Added for suspend and resume */
        vpp_obj->plane[i].borderAlpha = 0x7F7F7F7F;
        vpp_obj->plane[i].src_pol = 0x7F7F7F7F;
        vpp_obj->plane[i].bg_pol = 0x7F7F7F7F;

        /*Line dropping style for down scaling > 2x*/
        //LINEDROPPING_SEL_LINE_FIRST, LINEDROPPING_SEL_LINE_MID, LINEDROPPING_SEL_LINE_LAST,
        vpp_obj->plane[i].lineDroppingStyle = LINEDROPPING_SEL_LINE_MID;
    } // <- config FE planes

    /* config channels */
    for (i=FIRST_CHAN; i<MAX_NUM_CHANS; i++) {
        /* set DV ID and z-order this channel belongs to */
        vpp_obj->chan[i].dvID = pdv_cfg[i];
        vpp_obj->chan[i].zorder = pzorder_cfg[i];

        /* set plane IDs which belong to this channel */
        vpp_obj->chan[i].num_of_planes = 0;
        for (j=FIRST_PLANE; j<MAX_NUM_PLANES; j++){
            if (pvinport_cfg[j] == i)
                vpp_obj->chan[i].planeID[vpp_obj->chan[i].num_of_planes++] = j;
        }
    } // <- config channels

    // for B0, channel connection with CPCB is fixed
    vpp_obj->chan[CHAN_MAIN].dvlayerID = CPCB1_PLANE_1;
    vpp_obj->chan[CHAN_PIP].dvlayerID = CPCB1_PLANE_2;
    vpp_obj->chan[CHAN_GFX1].dvlayerID = CPCB1_PLANE_3;
    vpp_obj->chan[CHAN_GFX2].dvlayerID = CPCB2_PLANE_1; // Invalid
    vpp_obj->chan[CHAN_OVP_EL].dvlayerID = CPCB1_PLANE_5;
    vpp_obj->chan[CHAN_AUX].dvlayerID = CPCB1_PLANE_1;
    vpp_obj->chan[CHAN_VMX].dvlayerID = CPCB1_PLANE_5;

    /* config DVs */
    for (i=FIRST_CPCB; i<MAX_NUM_CPCBS; i++) {
        /* set channel IDs which belong to this DV */
        vpp_obj->dv[i].num_of_chans = 0;
        for (j=FIRST_CHAN; j<MAX_NUM_CHANS; j++){
            if (vpp_obj->chan[j].dvID == i)
                vpp_obj->dv[i].chanID[vpp_obj->dv[i].num_of_chans++] = j;
        }

        /* set VOUT IDs which belong to this DV */
        vpp_obj->dv[i].num_of_vouts = 0;
        for (j=FIRST_VOUT; j<MAX_NUM_VOUTS; j++){
            if (pvoutport_cfg[j] == i)
                vpp_obj->dv[i].voutID[vpp_obj->dv[i].num_of_vouts++] = j;
        }


#if defined(PE_3D_BD)
        // initialize view ID
        vpp_obj->dv[i].viewID = 0;
#endif
        /* Added for suspend and resume */
        vpp_obj->dv[i].bgColor.ColorFmt = 0x7F7F7F7F;
    } // <- config DVs

    /* config VOUTs */
    for (i=FIRST_VOUT; i<MAX_NUM_VOUTS; i++) {

        /* set DV ID this VOUT belongs to */
        vpp_obj->vout[i].dvID = pvoutport_cfg[i];

        /* Added for suspend and resume */
        vpp_obj->vout[i].mute = 0;
    } // <- config VOUTs

    vpp_obj->mv_mode = 0;

#if defined(PE_3D_BD)
    for (i=0; i<32; i++)
        vpp_obj->ss_offset_meta_data[i] = 0;
#endif


    /* Added for suspend and resume */
    vpp_obj->lumaKeyLow_key = 0x7F7F7F7F;
    vpp_obj->lumaKeyHigh_key = 0x7F7F7F7F;

    vpp_obj->be.cecPhyAddr = VPP_CEC_INVALID_PHY_ADDR;

    vpp_obj->scl.gfxInputMode0 = 0x7F7F7F7F;
    vpp_obj->scl.gfxInputMode1 = 0x7F7F7F7F;

#ifdef RA_avioGbl_BLANK_CG
    GA_REG_WORD32_READ(MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_GBL_BASE + RA_avioGbl_BLANK_CG, &vpp_obj->avioGbl_blank_cg.u32);
#ifdef BA_avioGbl_BLANK_CG_cg_en
    vpp_obj->avioGbl_blank_cg.uBLANK_CG_cg_en = 0;
#endif
    GA_REG_WORD32_WRITE(MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_GBL_BASE + RA_avioGbl_BLANK_CG, vpp_obj->avioGbl_blank_cg.u32);
#endif
    GA_REG_WORD32_READ(MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_GBL_BASE + RA_avioGbl_CTRL, &vpp_obj->stAvioGblCtrl.u32);
    vpp_obj->stAvioGblCtrl.uCTRL_VPPDHUB_dyCG_en = 0;
#ifdef BA_avioGbl_CTRL_mainpl_hdrlut_clk_en
    vpp_obj->stAvioGblCtrl.uCTRL_mainpl_hdrlut_clk_en = 1;
#endif
    GA_REG_WORD32_WRITE(MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_GBL_BASE + RA_avioGbl_CTRL, vpp_obj->stAvioGblCtrl.u32);

    // Initialize VPP registers according to configuration
    MV_OSAL_Mutex_Lock(vpp_obj->bcmbuf_lock);

    /* select BCM sub-buffer to dump register settings */
    VPP_BCMBUF_Select(vpp_obj->pVbiBcmBuf, -1);

     //Fix me: Temporarily disable clock gating for now.

    VPP_FE_Config(vpp_obj);
    VPP_SCL_Config(vpp_obj);
    VPP_CPCB_Config(vpp_obj);
    VPP_BE_Config(vpp_obj);
    VPP_VMXAPI_Config(vpp_obj);
    VPP_NGPTV_Config(vpp_obj);

     //Fix me: Temporarily disable clock gating for now.

    /* start BCM engine to program VPP registers */
    VPP_BCMBUF_HardwareTrans(vpp_obj->pVbiBcmBuf, 1/*block*/);

    MV_OSAL_Mutex_Unlock(vpp_obj->bcmbuf_lock);

    /* register VPP interrupt service routines */
    VPP_ISR_Registration(vpp_obj);
    VPP_CEC_ISR_Registration(vpp_obj);

    /* setup video queues in BCM scheduler */
    // BCM scheduler mux has to be set dynamically because of H/W bug: BCM_SCHED_BUG
    BCM_SCHED_SetMux(BCM_SCHED_Q0, 0);  /* CPCB0 VBI -> Q0 */
    BCM_SCHED_SetMux(BCM_SCHED_Q1, 11); /* CPCB0 VDE -> Q1 */
    BCM_SCHED_SetMux(BCM_SCHED_Q3, 27); /* VOP2  FRST-> Q3 */

    if (!(pfeature_cfg[0] & VPP_FEATURE_HDMITX))
        VPP_BE_HdmiPowerDown(vpp_obj);

    VPP_BE_MIPIOBJ_Config(vpp_obj);

    if (!(pfeature_cfg[0] & VPP_FEATURE_MIPI_DSI))
        VPP_BE_MIPI_PanelDeInit(vpp_obj);

    VPP_Memcpy(vpp_obj->iVppFeatureCfg, pfeature_cfg,
               sizeof(vpp_obj->iVppFeatureCfg));
    /* set VPP module to be configured */
    vpp_obj->status = STATUS_ACTIVE;

    return (MV_VPP_OK);
}

// to be removed when VPP suspend/resume is ready
int MV_VPP_GetPlaneInfo(int handle, int planeID, uintptr_t *curr_frame, uintptr_t *curr_still_picture, int *mute)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    *curr_frame = (unsigned int)vpp_obj->plane[planeID].curr_frame;
    *curr_still_picture = (unsigned int)vpp_obj->plane[planeID].curr_still_picture;
    *mute = vpp_obj->plane[planeID].mute;

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Get VPP STS Counter Value
 * INPUT: handle  - VPP object handle
 *      : pSTSCounter  - pointer to STS counter
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_GetSTSCounter(int handle,VPP_STS_COUNTER *pSTSCounter)
{
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Get VPP STS Counter
 * INPUT: handle  - VPP object handle
 *      : pSTSCounter  - pointer to VPP sts counter
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_GetSTSRunningCounter(int handle,  VPP_STS_COUNTER *pSTSCounter)
{
    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION: open a window of a video/graphics plane for display.
 *           the window is defined in end display resolution
 * INPUT: planeID - id of a video/grahpics plane
 *        *win - pointer to a vpp window struct
 *        *attr - pointer to a vpp window attribute struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_OpenDispWindow(int handle, int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr)
{
    CMD cmd;
    int params[8];
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;
    PLANE *plane;

    if (!win)
        return (MV_VPP_EBADPARAM);

    VPPAPI_DBG_INFO("plane [%d],  x[%d], y[%d], width[%d], height[%d]\n", planeID, win->x, win->y, win->width, win->height);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if ((win->width<=0) || (win->height<=0))
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if (VPP_GetResId(cpcbID) == RES_INVALID) {
        /* CPCB resolution need to be set first */
        return (MV_VPP_EBADCALL);
    }

    //Avoid re-opening of window
    plane = &(vpp_obj->plane[planeID]);
    if (plane->status != STATUS_INACTIVE) {
        VPPAPI_DBG_ERROR("%s:%d - Plane already opened - %d!!!\n", __func__, __LINE__, planeID);
        return (MV_VPP_ESWSTATEWRONG);
    }

    /* set video plane background window */
    params[0] = planeID;
    if (attr){
        params[1] = attr->bgcolor;
        params[2] = attr->alpha;
        params[7] = attr->globalAlphaFlag;
    } else {
        params[1] = DEFAULT_BGCOLOR;
        params[2] = DEFAULT_ALPHA;
        params[7] = DEFAULT_GLOBAL_ALPHA_FLAG;
    }
    params[3] = win->x;
    params[4] = win->y;
    params[5] = win->width;
    params[6] = win->height;
    make_cmd(&cmd, (FUNCPTR)VPP_OpenDispWindow, 8, params);
#ifdef TEE_MUTEX_SUPPORT_MULTIPLE_PROCESS
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);
#ifdef TEE_MUTEX_SUPPORT_MULTIPLE_PROCESS
    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION: change a window of a video/graphics plane.
 *           the window is defined in end display resolution
 * INPUT: planeID - id of a video/grahpics plane
 *        *win - pointer to a vpp window struct
 *        *attr - pointer to a vpp window attribute struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_ChangeDispWindow(int handle, int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr)
{
    CMD cmd;
    int params[8];
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;

    if(win){
        VPPAPI_DBG_INFO(" plane [%d], x[%d], y[%d], width[%d], height[%d] \n", planeID, win->x, win->y, win->width, win->height);
    }
    VPP_DEBUG_UTIL_printWindowChange(g_vpp->vpp_objs[handle], win, planeID,  VPP_DEBUG_WINDOWS_DISPWIN, __func__, __LINE__);
    if(!VPP_DEBUG_UTIL_IS_FF_ENABLED_DISPWIN(g_vpp->vpp_objs[handle])) {
        return (MV_VPP_OK);
    }


    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if (!win && !attr)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if (VPP_GetResId(cpcbID) == RES_INVALID) {
        /* CPCB resolution need to be set first */
        return (MV_VPP_EBADCALL);
    }

    /* set video plane background window */
    params[0] = planeID;
    if (attr) {
        params[1] = attr->bgcolor;
        params[2] = attr->alpha;
        params[7] = attr->globalAlphaFlag;
    } else {
        params[1] = -1; /* no change */
        params[2] = -1;
        params[7] = -1;
    }
    if (win) {
    //Increased the values to 2 as window of size 1x1 was causing crash
        if ((win->width<=2) || (win->height<=2))
            return (MV_VPP_EBADPARAM);

        params[3] = win->x;
        params[4] = win->y;
        params[5] = win->width;
        params[6] = win->height;
    } else {
        params[3] = -1; /* no change */
        params[4] = -1;
        params[5] = -1;
        params[6] = -1;
    }

    make_cmd(&cmd, (FUNCPTR)VPP_ChangeDispWindow, 8, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}


/******************************************************************************
 * FUNCTION: change a window of a video/graphics plane.
 *           must be called from ISR because there is no race condition
 *           protection
 ******************************************************************************/
int MV_VPPOBJ_ChangeDispWindowFromISR(int handle, int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr)
{
    int params[8];
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;

    VPP_DEBUG_UTIL_printWindowChange(g_vpp->vpp_objs[handle], win, planeID,  VPP_DEBUG_WINDOWS_DISPWIN_FROMISR, __func__, __LINE__);
    if(!VPP_DEBUG_UTIL_IS_FF_ENABLED_DISPWIN(g_vpp->vpp_objs[handle])) {
        return (MV_VPP_OK);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if (!win && !attr)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if (VPP_GetResId(cpcbID) == RES_INVALID) {
        /* CPCB resolution need to be set first */
        return (MV_VPP_EBADCALL);
    }

    if(win && !IS_VALID_VPP_WINDOW(vpp_obj, VPP_WIN_TYPE_DISP, win->width, win->height) ) {
        //Mask none Supportted window co-ordinates
        return (MV_VPP_EUNSUPPORT);
    }

    /* set video plane background window */
    params[0] = planeID;
    if (attr) {
        params[1] = attr->bgcolor;
        params[2] = attr->alpha;
        params[7] = attr->globalAlphaFlag;
    } else {
        params[1] = -1; /* no change */
        params[2] = -1;
        params[7] = -1;
    }
    if (win) {
        if ((win->width<=0) || (win->height<=0))
            return (MV_VPP_EBADPARAM);

        params[3] = win->x;
        params[4] = win->y;
        params[5] = win->width;
        params[6] = win->height;
    } else {
        params[3] = -1; /* no change */
        params[4] = -1;
        params[5] = -1;
        params[6] = -1;
    }

    VPP_ChangeDispWindow(vpp_obj, params);

    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION: get window information of a video/graphics plane.
 *           the window is defined in end display resolution
 * INPUT: planeID - id of a video/grahpics plane
 *        *win - pointer to a vpp window struct
 *        *attr - pointer to a vpp window attribute struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_GetDispWindow(int handle, int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr)
{
    VPP_OBJ *vpp_obj;
    int chanID;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if (!win && !attr)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if (win) {
        win->x = vpp_obj->chan[chanID].disp_win.x;
        win->y = vpp_obj->chan[chanID].disp_win.y;
        win->width = vpp_obj->chan[chanID].disp_win.width;
        win->height = vpp_obj->chan[chanID].disp_win.height;
    }
    if (attr) {
        attr->bgcolor = vpp_obj->chan[chanID].disp_win_attr.bgcolor;
        attr->alpha = vpp_obj->chan[chanID].disp_win_attr.alpha;
        attr->globalAlphaFlag = vpp_obj->chan[chanID].disp_win_attr.globalAlphaFlag;
    }

    return (MV_VPP_OK);
}

/***************************************************************
 * FUNCTION: set the crop window for a plane
 * INPUT: planeID - id of the plane
 *        *win - pointer to the crop window struct
 * RETURN: MV_VPP_OK - SUCCEE
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_SetCropWindow(int handle, int planeID, VPP_WIN *win)
{
    CMD cmd;
    int params[5];
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;

    VPPAPI_DBG_INFO(" plane [%d], x[%d], y[%d], width[%d], height[%d] \n",  planeID, win->x, win->y, win->width, win->height);
    VPP_DEBUG_UTIL_printWindowChange(g_vpp->vpp_objs[handle], win, planeID,  VPP_DEBUG_WINDOWS_CROPWIN, __func__, __LINE__);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

#ifndef ENABLE_CROP_FOR_FPGA_TESTING
    /* only GFX0 and PG support crop window */
    if ((planeID != PLANE_GFX1))
        return (MV_VPP_EUNSUPPORT);
#endif //ENABLE_CROP_FOR_FPGA_TESTING

    if (!win)
        return (MV_VPP_EBADPARAM);

    if ((win->x<0) || (win->y<0) || (win->width<=0) || (win->height<=0))
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    /* set video plane background window */
    params[0] = planeID;
    params[1] = win->x;
    params[2] = win->y;
    params[3] = win->width;
    params[4] = win->height;
    make_cmd(&cmd, (FUNCPTR)VPP_SetCropWindow, 5, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}


/***************************************************************
 * FUNCTION: set the reference window for a plane
 * INPUT: planeID - id of the plane
 *        *win - pointer to the reference window struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_SetRefWindow(int handle, int planeID, VPP_WIN *win)
{
    CMD cmd;
    int params[5];
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;

    VPPAPI_DBG_INFO(" plane [%d], x[%d], y[%d], width[%d], height[%d] \n", planeID, win->x, win->y, win->width, win->height);
    VPP_DEBUG_UTIL_printWindowChange(g_vpp->vpp_objs[handle], win, planeID, VPP_DEBUG_WINDOWS_REFWIN, __func__, __LINE__);
    if(!VPP_DEBUG_UTIL_IS_FF_ENABLED_REFWIN(g_vpp->vpp_objs[handle])) {
        return (MV_VPP_OK);
    }

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if (!win)
        return (MV_VPP_EBADPARAM);

    if ((win->width<=0) || (win->height<=0))
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    /* set video plane background window */
    params[0] = planeID;
#ifdef ENABLE_REFWIN_FOR_FPGA_TESTING
    params[1] = win->x;
    params[2] = win->y;
    params[3] = win->width;
    params[4] = win->height;
    make_cmd(&cmd, (FUNCPTR)VPP_SetRefWindow, 5, params);
#else //ENABLE_REFWIN_FOR_FPGA_TESTING
    if(planeID == PLANE_GFX1)
    {
        params[1] = win->x;
        params[2] = win->y;
    }
    else
    {
        params[1] = 0;
        params[2] = 0;
    }
    params[3] = win->width;
    params[4] = win->height;
    make_cmd(&cmd, (FUNCPTR)VPP_SetRefWindow, 5, params);
#endif //ENABLE_REFWIN_FOR_FPGA_TESTING

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);
    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
    return (MV_VPP_OK);
}

/***************************************************************
 * FUNCTION: Turn on/off HDMI HPD control
 * INPUT: state
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 **************************************************************/
int MV_VPPOBJ_HdmiHpdControlOff(int handle, int state)
{
   VPP_OBJ *vpp_obj;
   BE_HDMITX_DATA* pHdmiTx;

   if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
   }

   if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

   vpp_obj = g_vpp->vpp_objs[handle];
   pHdmiTx = vpp_obj->be.hdmiTx;

   pHdmiTx->hdmiHpdControlOff = state?1:0;
   VPPAPI_DBG_INFO("Hdmi Hpd interupt control is:%s\n", pHdmiTx->hdmiHpdControlOff?"off":"on");
   return MV_VPP_OK;
}

/***************************************************************
 * FUNCTION: set the reference window for a plane
 *           must be called from ISR because there is no race condition
 *           protection
 **************************************************************/
int MV_VPPOBJ_SetRefWindowFromISR(int handle, int planeID, VPP_WIN *win)
{
    int params[5];
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;

    VPP_DEBUG_UTIL_printWindowChange(g_vpp->vpp_objs[handle], win, planeID,  VPP_DEBUG_WINDOWS_REFWIN_FROMISR, __func__, __LINE__);
    if(!VPP_DEBUG_UTIL_IS_FF_ENABLED_REFWIN(g_vpp->vpp_objs[handle])) {
        return (MV_VPP_OK);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if (!win)
        return (MV_VPP_EBADPARAM);

    if ((win->width<=0) || (win->height<=0))
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if( !IS_VALID_VPP_WINDOW(vpp_obj, VPP_WIN_TYPE_REF, win->width, win->height) ) {
        //Mask none Supportted window co-ordinates
        return (MV_VPP_EUNSUPPORT);
    }

    /* set video plane background window */
    params[0] = planeID;
#ifdef ENABLE_REFWIN_FOR_FPGA_TESTING
    params[1] = win->x;
    params[2] = win->y;
    params[3] = win->width;
    params[4] = win->height;
#else //ENABLE_REFWIN_FOR_FPGA_TESTING
    if(planeID == PLANE_GFX1)
    {
        params[1] = win->x;
        params[2] = win->y;
    }
    else
    {
        params[1] = 0;
        params[2] = 0;
    }
    params[3] = win->width;
    params[4] = win->height;
#endif //ENABLE_REFWIN_FOR_FPGA_TESTING

    return VPP_SetRefWindow(vpp_obj, params);
}

/******************************************************************************
 * FUNCTION: get reference window information of a video/graphics plane.
 *           the window is defined in end display resolution
 * INPUT: planeID - id of a video/grahpics plane
 *        *win - pointer to a vpp window struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_GetRefWindow(int handle, int planeID, VPP_WIN *win)
{
    VPP_OBJ *vpp_obj;
    PLANE *plane;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if (!win)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }
   plane = &(vpp_obj->plane[planeID]);
   if (win)
   {
        win->x = plane->ref_win.x;
        win->y = plane->ref_win.y;
        win->width = plane->ref_win.width;
        win->height = plane->ref_win.height;
    }
    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION: get content window information of a video/graphics plane.
 *           the window is defined in end display resolution
 * INPUT: planeID - id of a video/grahpics plane
 *        *win - pointer to a vpp window struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_GetContentWindow(int handle, int planeID, VPP_WIN *win)
{
    VPP_OBJ *vpp_obj;
    PLANE *plane;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if (!win)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }
   plane = &(vpp_obj->plane[planeID]);
   if (win)
   {
        win->x = plane->frame_win.x;
        win->y = plane->frame_win.y;
        win->width = plane->frame_win.width;
        win->height = plane->frame_win.height;
    }
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: enable/disable non linear horizontal scaling
 * INPUT: planeID - id of the plane
 *        enable - 1: enable; 0: disable
 *        center - center value [0, 255]
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_SetScalerNonLinearControl(int handle, int planeID, int enable, int center_frac, int center_ratio)
{
    VPP_OBJ *vpp_obj;
    CMD cmd;
    int params[4];
    int chanID, cpcbID;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle < 0 || handle >= MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID < FIRST_PLANE || planeID >= MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if (0) //BG4CT have non linear scalar support for all planes??
        return (MV_VPP_EBADPARAM);

    if (enable != 0 &&(center_frac < 0 || center_frac >= 256 || center_ratio <32 || center_ratio >64))
        return (MV_VPP_EBADPARAM);

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID)
        return (MV_VPP_EUNSUPPORT);

    params[0] = planeID;
    params[1] = enable ? 1 : 0;
    params[2] = center_frac;
    params[3] = center_ratio;
    make_cmd(&cmd, (FUNCPTR)VPP_SetScalerNonLinearControl, 4, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: set scaler horizontal and vertial phase DDA count ( sub pixel shifting)
 * INPUT: scalerId - scaler index
 *            hv - horizontal or vertical
 *            counter - DDA counter
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_SetScalerPhaseDda(int handle, VPP_FRC_SCL_NUM scalerId, int hv, int count)
{
    VPP_OBJ *vpp_obj;
    CMD cmd;
    int params[3];
    int chanID, cpcbID, planeID;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle < 0 || handle >= MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (scalerId < VPP_FRC_SCL_MAIN || scalerId >= VPP_FRC_SCL_MAX)
        return (MV_VPP_EBADPARAM);

    if ( (count < 0) || (count > 255) )
        return (MV_VPP_EBADPARAM);

    switch (scalerId) {
        case VPP_FRC_SCL_MAIN:
            planeID = PLANE_MAIN;
            break;

#ifdef ENABLE_PIP_GFX0_PATH
        case VPP_FRC_SCL_PIP:
            planeID = PLANE_PIP;
            break;
#endif

        case VPP_FRC_SCL_OSD:
            planeID = PLANE_GFX1;
            break;

        default:
            return (MV_VPP_EUNSUPPORT);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    params[0] = scalerId;
    params[1] = hv;
    params[2] = count;
    make_cmd(&cmd, (FUNCPTR)VPP_SetScalerPhaseDda, 3, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION: set plane border alpha.
 * INPUT: planeID - id of a video/grahpics plane
 *        alpha - plane border alpha value[0, 255]
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_SetPlaneBorderAlpha(int handle, int planeID, int alpha)
{
    CMD cmd;
    int params[2];
    int chanID;
    int cpcbID;
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if (alpha<0 || alpha>255)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if (VPP_GetResId(cpcbID) == RES_INVALID) {
        /* CPCB resolution need to be set first */
        return (MV_VPP_EBADCALL);
    }

    /* set video plane background window */
    params[0] = planeID;
    params[1] = alpha;

    make_cmd(&cmd, (FUNCPTR)VPP_SetPlaneBorderAlpha, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/**************************************************************************
 * FUNCTION: close display window of a video/graphics plane in displaying
 * INPUT: planeID - id of the plane
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 *************************************************************************/
int MV_VPPOBJ_CloseDispWindow(int handle, int planeID)
{
    CMD cmd;
    int params[2];
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;

//    VPPAPI_DBG_INFO(" plane [%d] \n",  planeID);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if (VPP_GetResId(cpcbID) == RES_INVALID) {
        /* CPCB resolution need to be set first */
        return (MV_VPP_EBADCALL);
    }
    /* set plane background window to 0 to close video plane window */
    params[0] = planeID;
    make_cmd(&cmd, (FUNCPTR)VPP_CloseDispWindow, 1, params);
#ifdef TEE_MUTEX_SUPPORT_MULTIPLE_PROCESS
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);
#ifdef TEE_MUTEX_SUPPORT_MULTIPLE_PROCESS
    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    return (MV_VPP_OK);
}

/***************************************************************
 * FUNCTION: get plane status
 * INPUT: planeID - id of the plane
 *        *status - pointer to status variable
 * RETURN: STATUS_INACTIVE - inactive status
 *         STATUS_INACTIVE_PENDING - inactivate command issued
 *         STATUS_ACTIVE - active status, ready to accept frame
 *         STATUS_ACTIVE_PENDING - activate command issued
 *         STATUS_DISP_VIDEO - displaying video frames
 *         STATUS_DISP_LOGO - displaying logo frame
 **************************************************************/
int MV_VPPOBJ_GetPlaneStatus(int handle, int planeID, int *status)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    if (!status)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    *status = vpp_obj->plane[planeID].status;
    return (MV_VPP_OK);
}

/************************************************************************
 * FUNCTION: recycle plane frame descriptors
 * INPUT: planeID - id of the plane
 * RETURN: NULL - no frame descriptor for recycle
 *         other - pointer to frame descriptor being recycled
 ************************************************************************/
uintptr_t MV_VPPOBJ_RecycleFrames(int handle, int planeID)
{
    VPP_OBJ *vpp_obj;
    void *frame;

    if (g_vpp->magic != VPP_MAGIC){
        return (0);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (0);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (0);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (0);

    /* pop plane's frame queue from output queue */
    frame = (void *)0;

    if (frmq_pop(&(vpp_obj->plane[planeID].outputq), (void **)&frame)){
        frmq_pop_commit(&(vpp_obj->plane[planeID].outputq));
    }

    return ((int)frame);
}

#ifdef CONFIG_VPP_ENABLE_INTERRUPT_ON_PUSHFRAME
static void MV_VPP_EnableInterruptFromPushFrame(VPP_OBJ *vpp_obj, int chanId)
{
    int cpcbID;

    cpcbID = CPCB_OF_CHAN(vpp_obj, chanId);
    MV_VPP_EnableInterrupt(vpp_obj,cpcbID);
}
#endif

/*****************************************************************************
 * FUNCTION: set a new still picture for a plane
 *           old still picture will be returned if available and pold
 *           is non-NULL.
 * INPUT: planeID - id of the plane
 *        *pnew - new still picture frame descriptor
 *        **pold - old still picture frame descriptor
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *****************************************************************************/
int MV_VPPOBJ_SetStillPicture(int handle, int planeID, void *pnew, void **pold)
{
    VPP_OBJ *vpp_obj;
    int chanID;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);


//    if (!pnew)
//        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if (pold) {
#ifdef CONFIG_VPP_FASTLOGO_SUPPORT
        VBUF_INFO *pVbufInfo = (VBUF_INFO*)vpp_obj->plane[planeID].curr_still_picture;
        if (pVbufInfo) {
            *pold = (void*)pVbufInfo->m_hDesc;
        } else {
            *pold = NULL;
        }
#else
        *pold = vpp_obj->plane[planeID].curr_still_picture;
#endif
    }

    vpp_obj->plane[planeID].still_picture = pnew;
#ifdef CONFIG_VPP_ENABLE_INTERRUPT_ON_PUSHFRAME
    MV_VPP_EnableInterruptFromPushFrame(vpp_obj, chanID);
#endif //CONFIG_VPP_ENABLE_INTERRUPT_ON_PUSHFRAME

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: set display mode for a plane
 * INPUT: planeID - id of the plane
 *        mode - DISP_STILL_PIC: still picture, DISP_FRAME: frame
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
int MV_VPPOBJ_SetDisplayMode(int handle, int planeID, int mode)
{
    VPP_OBJ *vpp_obj;
    CMD cmd;
    int params[3];
    int chanID;
    int cpcbID;

    //VPPAPI_DBG_INFO(" plane [%d], mode [%d] \n",  planeID, mode);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    /* set logo display mode */
    params[0] = planeID;
    params[1] = mode; /* logo display mode */
    make_cmd(&cmd, (FUNCPTR)VPP_SetDisplayMode, 2, params);
#ifdef TEE_MUTEX_SUPPORT_MULTIPLE_PROCESS
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);
#ifdef TEE_MUTEX_SUPPORT_MULTIPLE_PROCESS
    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: set display mode for a plane
 *           must be called from ISR because there is no race condition
 *           protection.
 *******************************************************************/
int MV_VPPOBJ_SetDisplayModeFromISR(int handle, int planeID, int mode)
{
    VPP_OBJ *vpp_obj;
    CMD cmd;
    int params[2];
    int chanID;
    int cpcbID;

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    /* set logo display mode */
    params[0] = planeID;
    params[1] = mode; /* logo display mode */

    make_cmd(&cmd, (FUNCPTR)VPP_SetDisplayMode, 2, params);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    return (MV_VPP_OK);
}
int MV_VPPOBJ_EarlyDisplayFrame(int handle, int planeID, void *frame)
{
    VPP_OBJ *vpp_obj;
    int chanID;
    VBUF_INFO *pinfo,*pinfo_last;
    PLANE *plane;
    VDEC_VID_BUF_DESC *inline_frame,*curr_frame;

    if (vpp.magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = vpp.vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    /* only for main plane */
    if (planeID != PLANE_MAIN)
        return (MV_VPP_EBADPARAM);

    if (frame == NULL)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }
    plane = &vpp_obj->plane[planeID];
    curr_frame =   plane->curr_frame;
    inline_frame = frame;

#if !defined(VPP_IN_TRUST_ZONE)
    /* new frame descriptor found */
    pinfo_last = (VBUF_INFO *)curr_frame->user_data;
    pinfo = (VBUF_INFO *)inline_frame->user_data;
#else
    pinfo_last = (VBUF_INFO *)curr_frame;
    pinfo = (VBUF_INFO *)inline_frame;
#endif
    if ( plane->curr_frame != NULL)
    {
        if ( pinfo->m_srcfmt  !=  pinfo_last->m_srcfmt )
              goto Exit;
        if ( pinfo->m_bytes_per_pixel != pinfo_last->m_bytes_per_pixel )
              goto Exit;
        if ( pinfo->m_order != pinfo_last->m_order )
              goto Exit;
        /* Avoid early push incase of compressed and DV */
        if ( IS_MTR_ENABLED(pinfo->m_is_compressed) || IS_MMU_ENABLED(pinfo->m_is_compressed)
             || pinfo_last->m_dv_present_mode)
        {
              goto Exit;
        }
        if ( pinfo->m_content_width != pinfo_last->m_content_width  ||
                     pinfo->m_content_width != pinfo_last->m_content_width   )
        {
              goto Exit;
        }
        if (pinfo->m_disp_offset != pinfo_last->m_disp_offset)
              goto Exit;
         VPP_Early_Update_CurrentFrame(vpp_obj, chanID, frame);
         return MV_VPP_OK;

    }
Exit:
          return MV_VPP_EBADCALL;
}

/*******************************************************************
 * FUNCTION: display a frame for a plane
 * INPUT: planeID - id of the plane
 *        *frame - frame descriptor
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
int MV_VPPOBJ_DisplayFrame(int handle, int planeID, void *frame)
{
    VPP_OBJ *vpp_obj;
    int chanID;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if (frame == NULL)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if (PLANE_VMX != planeID)
        ParseCCData(vpp_obj, frame);

    /* push frame into plane frame queue. */
    if (frmq_push(&(vpp_obj->plane[planeID].inputq), frame) == 0){
        return (MV_VPP_EFRAMEQFULL);
    }

#ifdef CONFIG_VPP_ENABLE_INTERRUPT_ON_PUSHFRAME
    MV_VPP_EnableInterruptFromPushFrame(vpp_obj, chanID);
#endif //CONFIG_VPP_ENABLE_INTERRUPT_ON_PUSHFRAME

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: push frame for recycling
 * INPUT: planeID - id of the plane
 *        *frame - frame descriptor
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
int MV_VPPOBJ_FillOutQueue(int handle, int planeID, void *frame)
{
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if (frame == NULL)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }
    //VPP_DEBUG_Printf("\n chanID = %d at %s\n", chanID, __FUNCTION__);

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    //VPP_DEBUG_Printf("\n cpcbID = %d at %s\n", cpcbID, __FUNCTION__);

    /* push frame into plane frame queue. */
    if (frmq_push(&(vpp_obj->plane[planeID].outputq), frame) == 0){
        return (MV_VPP_EFRAMEQFULL);
    }

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: set CPCB background color
 * INPUT: cpcbID - if of the CPCB
 *        pBgColor - pointer to the structure of background color
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
int MV_VPPOBJ_SetCPCBBgColor(int handle, int cpcbID, VPP_BG_COLOR *pBgColor)
{
    VPP_OBJ *vpp_obj;
    int params[2];
    CMD cmd;
    int red, green, blue;
    int luma, chroma_b, chroma_r;
    unsigned int uyv;

//    VPPAPI_DBG_INFO(" cpcb [%d], bg color [%d:0x%x] \n",  cpcbID, pBgColor->ColorFmt, pBgColor->Color);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (pBgColor->ColorFmt<FIRST_OUTPUT_COLOR_FMT || pBgColor->ColorFmt>=MAX_NUM_OUTPUT_COLOR_FMTS)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if (cpcbID<FIRST_CPCB || cpcbID>=MAX_NUM_CPCBS)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->dv[cpcbID].status == STATUS_INACTIVE){
        /* The CPCB is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if(OUTPUT_COLOR_FMT_RGB888 == pBgColor->ColorFmt){
        /*get R, G and B values*/
        red = (pBgColor->Color & 0xFF);
        green = (pBgColor->Color & 0xFF00) >> 8;
        blue = (pBgColor->Color & 0xFF0000) >> 16;
        /*contert to yuv space*/
        luma = ((66 * red + 129 * green +  25 * blue + 128) >> 8) + 16;
        chroma_b = (( -38 * red - 74 * green + 112 * blue + 128) >> 8) + 128;
        chroma_r = (( 112 * red - 94 * green -18 * blue + 128) >> 8) + 128;
        /*clip luma and chroma*/
        if(luma < 16)
            luma = 16;
        else if(luma > 235)
            luma = 235;

        if(chroma_b < 16)
            chroma_b = 16;
        else if(chroma_b > 240)
            chroma_b = 240;

        if(chroma_r < 16)
            chroma_r = 16;
        else if(chroma_r > 240)
            chroma_r = 240;
        /*BG color order is U Y V*/
        uyv = chroma_r + (luma << 8) + (chroma_b << 16);

    }else if(OUTPUT_COLOR_FMT_YCBCR444 == pBgColor->ColorFmt){
        uyv = pBgColor->Color;

    }else{
        /*YUYV format*/
        uyv = pBgColor->Color & 0xFFFFFF;
    }

    /*set CPCB bg color */
    params[0] = cpcbID;
    params[1] = uyv;

    make_cmd(&cmd, (FUNCPTR)VPP_SetCPCBBgColor, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: get CPCB VCO frequency
 * INPUT: cpcbID - CPCB(for Berlin) or DV(for Galois) id
 *        *pvcofreq - pointer to vco frequency ID
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
int MV_VPPOBJ_GetCPCBVCOFreq(int handle, int cpcbID, int *pvcofreq)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if (cpcbID<FIRST_CPCB || cpcbID>=MAX_NUM_CPCBS)
        return (MV_VPP_EBADPARAM);

    *pvcofreq = vpp_obj->dv[cpcbID].vco_freq;
   return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: set CPCB or DV output resolution
 * INPUT: cpcbID - CPCB(for Berlin) or DV(for Galois) id
 *        resID - id of output resolution
 *        bit_depth - HDMI deep color bit depth
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 * Note: this function has to be called before enabling a plane
 *       which belongs to that CPCB or DV.
 *******************************************************************/
int MV_VPPOBJ_SetCPCBOutputResolution(int handle, int cpcbID, int resID, int bit_depth)
{
    VPP_OBJ *vpp_obj;
    int params[8], i;
    int vco_freq;
    int isForceResChange = resID & 0x80000000;
    UINT32 uiDisplayMode;
#ifdef VPPDRV_VPP_INTR_FREQ_MONITOR
    int vppIntrMonStatus = VPPDRV_vppIntrMonGetStatus(g_vpp->vpp_objs[handle]);
#endif //VPPDRV_VPP_INTR_FREQ_MONITOR
#ifdef VPP_FIXED_COLOR_FMT
    //TODO: VPP_SW_BRINGUP: Instead of passing color_fmt as argument hardcoded here. Fix this appropritely.
    int color_fmt = OUTPUT_COLOR_FMT_YCBCR422;
#endif

    resID &= 0x7FFFFFFF;

    VPPAPI_DBG_INFO(" handle [%x], cpcb [%d], resID [%d], bit_depth [%d] \n", handle, cpcbID, resID, bit_depth);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (resID<FIRST_RES || resID>=MAX_NUM_RESS)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if (cpcbID<FIRST_CPCB || cpcbID>=MAX_NUM_CPCBS)
        return (MV_VPP_EBADPARAM);

    vco_freq = GetVCOFreq(resID, bit_depth);

    /*check the input resolution*/
    if (((VPP_GetResId(cpcbID) == resID) && (vpp_obj->dv[cpcbID].vco_freq == vco_freq)) && !isForceResChange)
        return (MV_VPP_OK);

#ifdef VPPDRV_VPP_INTR_FREQ_MONITOR
    //Disable monitoring during resolution change
    VPPDRV_vppIntrMonSetStatus(vpp_obj, 0);
#endif //VPPDRV_VPP_INTR_FREQ_MONITOR

    if (cpcbID == FIRST_CPCB) {
        if (vpp_obj->ResCallback.ResNotifier) {
            vpp_obj->ResCallback.ResNotifier(VPP_RES_CHANGE_START);
        }
    }
    VPP_BE_SetHdmiPhyPower(vpp_obj, FALSE);

#ifdef CONFIG_VPP_FASTLOGO_SUPPORT
    //This is bug fix for LGU - recovery mode shows no signal sometime, solution ensure recieve resync with transmitter
    /*Disable Scrambling*/
    VPP_BE_HDMI_ScramblingEnable(vpp_obj, FALSE);
    /* clear SCDC reserved bit for LGE UHD TV */
    if (MV_VPP_OK == VPP_BE_HDMI_ClearSCDCReservedBit(vpp_obj))
    {
        MV_OSAL_Task_Sleep(50); //50ms
    }
#endif //CONFIG_VPP_FASTLOGO_SUPPORT

    /* update CPCB vco frequency */
    vpp_obj->dv[cpcbID].vco_freq = vco_freq;

    /*MV_VPPOBJ_SetCPCBOutputResolution support only switching between overlay modes*/
    uiDisplayMode = IS_DSI_PANEL_RES(resID)?VOUT_DISP_SINGLE_MODE_SEC:VOUT_DISP_SINGLE_MODE_PRI;
    for(i=FIRST_CPCB; i<MAX_NUM_CPCBS; i++)
    {
        if (vpp_obj->dv[i].status != STATUS_INACTIVE)
        {
            /*Reset both CPCB in case of mode change*/
            if((vpp_obj->m_VoutMode != uiDisplayMode) || (cpcbID == i)){
                /* reset CPCB TG */
                ResetCPCBTG(vpp_obj, i, uiDisplayMode);
#ifdef CONFIG_VPP_FASTLOGO_SUPPORT
                VPP_WaitUntilResetCPCB(vpp_obj, i);
#endif
            }
        }
    }
    VPP_ResetPlane(vpp_obj, uiDisplayMode);
    vpp_obj->m_VoutMode = uiDisplayMode;

    if (vpp_obj->iVppFeatureCfg[0] & VPP_FEATURE_MIPI_DSI) {
        /*Handle dynamic switching between DSI & HDMI*/
        if(IS_DSI_PANEL_RES(resID)) {
            VPP_BE_MIPI_PanelInit(vpp_obj, (resID - RES_MIN_DSI));
        } else {
            if (!(vpp_obj->iVppFeatureCfg[0] & VPP_FEATURE_HDMITX)) {
                VPPAPI_DBG_ERROR("Invalid MIPI-DSI Res ID = %d\n", resID);
                return (MV_VPP_EBADPARAM);
            }
        }
    }

    if (resID != RES_RESET) {

        VPP_SetClk(vpp_obj, cpcbID, resID);

        MV_OSAL_Mutex_Lock(vpp_obj->bcmbuf_lock);
        /*Enable the canvas change flag,
        so that inline line dropping is forced for 1st frame, when ofline scaling is required*/
        vpp_obj->resIdUpdated[cpcbID] = 1;

        /* select BCM sub-buffer to dump register settings */
        vpp_obj->pVbiBcmBuf = vpp_obj->pVbiBcmBufCpcb[cpcbID];
        VPP_BCMBUF_Select(vpp_obj->pVbiBcmBuf, cpcbID);
        /* set CPCB resolution */
        params[0] = cpcbID;
        params[1] = resID;
        params[2] = color_fmt;
        params[3] = bit_depth;
        params[4] = 1; //pixel repeat
        params[5] = 0; //quiescent mode
        params[6] = uiDisplayMode;
        params[7] = VOUT_HDMI;
        VPP_SetCPCBOutputResolution(vpp_obj, params);

        /* start BCM engine to program VPP registers */
        VPP_BCMBUF_HardwareTrans(vpp_obj->pVbiBcmBuf, 1/*block*/);

        if (cpcbID == FIRST_CPCB) {
            if (vpp_obj->ResCallback.ResNotifier) {
                vpp_obj->ResCallback.ResNotifier(VPP_RES_CHANGE_DONE);
            }
        }
#ifndef CONFIG_VPP_ENABLE_INTERRUPT_ON_PUSHFRAME
        /*DP1->DP2 case, VOP2 DTG interupt used for all programming*/
        if(IS_CPCB_VOUT_DISP_MODE(cpcbID, SINGLE_MODE_SEC)) {
            VPP_Enable_ISR_Interrupt(vpp_obj, CPCB_2);
        } else {
            VPP_Enable_VDE_Interrupt(vpp_obj, cpcbID);
            VPP_Enable_ISR_Interrupt(vpp_obj, cpcbID);
        }
        VPP_GlobalIntr_Config(vpp_obj, cpcbID);
        VPP_dhub_sem_clear(vpp_obj);
#else //CONFIG_VPP_ENABLE_INTERRUPT_ON_PUSHFRAME
        MV_VPP_DisableInterrupt(vpp_obj, cpcbID);
#endif //CONFIG_VPP_ENABLE_INTERRUPT_ON_PUSHFRAME

        /* switch BCM buffer for the CPCB */
        if (vpp_obj->pVbiBcmBufCpcb[cpcbID] == &vpp_obj->vbi_bcm_buf[cpcbID][0])
            vpp_obj->pVbiBcmBufCpcb[cpcbID] = &vpp_obj->vbi_bcm_buf[cpcbID][1];
        else
            vpp_obj->pVbiBcmBufCpcb[cpcbID] = &vpp_obj->vbi_bcm_buf[cpcbID][0];

        MV_OSAL_Mutex_Unlock(vpp_obj->bcmbuf_lock);

        /* set DV status to active */
        vpp_obj->dv[cpcbID].status = STATUS_ACTIVE;
    }
#ifdef BERLIN_DTV_MERGE
    AudioDAC_ReSetSampRate();
#endif

    #ifdef VPPDRV_VPP_INTR_FREQ_MONITOR
    //Restore monitoring status after resolution change
    VPPDRV_vppIntrMonSetStatus(vpp_obj, vppIntrMonStatus);
    #endif //VPPDRV_VPP_INTR_FREQ_MONITOR

    return (MV_VPP_OK);

}

/*******************************************************************
 * FUNCTION: get CPCB or DV output resolution
 * INPUT: cpcbID - CPCB(for Berlin) or DV(for Galois) id
 *        *resID - pointer to resolution ID variable
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
int MV_VPPOBJ_GetCPCBOutputResolution(int handle, int cpcbID, int *resID)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    if (!resID)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (cpcbID<FIRST_CPCB || cpcbID>=MAX_NUM_CPCBS)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if (vpp_obj->dv[cpcbID].status == STATUS_INACTIVE)
        *resID = -1; // invalid resolution ID
    else{
        *resID = VPP_GetResId(cpcbID);
    }

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Get description of the specified resolution ID
 * INPUT: handle - VPP object handle
 *        resID - the resolution ID
 *        pResDesc - the buffer to store the resolution description
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int MV_VPPOBJ_GetResolutionDescription(int handle, int resID, VPP_RESOLUTION_DESCRIPTION* pResDesc)
{
    RESOLUTION_INFO  *pResinfo;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    if (resID < 0 || resID >= MAX_NUM_RESS) {
        return MV_VPP_EBADPARAM;
    }

    pResinfo = VPP_GetResInfo(resID);
    pResDesc->uiActiveWidth = pResinfo->active_width;
    pResDesc->uiActiveHeight = pResinfo->active_height;
    pResDesc->uiWidth = pResinfo->width;
    pResDesc->uiHeight = pResinfo->height;
    pResDesc->uiRefreshRate = pResinfo->frame_rate;
    pResDesc->uiIsInterlaced = pResinfo->scan;
    pResDesc->uiIsDDD = pResinfo->flag_3d;

    return MV_VPP_OK;
}

#ifndef VPP_DIAGS
/*******************************************************************
 * FUNCTION: get CPCB input clock in unit of HZ
 * INPUT: cpcbID - cpcb ID
 *        *pclock - pointer to clock variable
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *******************************************************************/
int MV_VPPOBJ_GetCPCBInputClock(int handle, int cpcbID, unsigned int *pclock)
{
    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    if (cpcbID<FIRST_CPCB || cpcbID>=MAX_NUM_CPCBS)
        return (MV_VPP_EBADPARAM);

    if (!pclock)
        return (MV_VPP_EBADPARAM);


    if (cpcbID == CPCB_1)
        *pclock = AVPLL_GetChannelOutputFreq(0, 1);

    return (MV_VPP_OK);

}
#endif

/*******************************************************************
 * FUNCTION: get the pixel clock by resolution
 * INPUT: cpcbID -resID - resolution ID
 *        *pixel_clock - pointer to pixel clock variable
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *******************************************************************/
int MV_VPPOBJ_GetCPCBOutputPixelClock(int handle, int resID, int *pixel_clock)
{
    RESOLUTION_INFO  *pResinfo;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    if (resID<FIRST_RES || resID>=MAX_NUM_RESS)
        return (MV_VPP_EBADPARAM);

    if (!pixel_clock)
        return (MV_VPP_EBADPARAM);

    pResinfo = VPP_GetResInfo(resID);
    *pixel_clock = pResinfo->freq;

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: get plane PTS
 * INPUT: planeID - plane id
 *        *lst_valid_PTS - contain last valid PTS
 *        *curr_PTS - contain current PTS
 *        *frame_cnt - contain frame count since last valid PTS
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *******************************************************************/
int MV_VPPOBJ_GetPTS(int handle, int planeID, PTS *lst_valid_PTS, PTS *curr_PTS, int *frame_cnt)
{
    VPP_OBJ *vpp_obj;
    unsigned int delta_lo, delta_hi;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->plane[planeID].status == STATUS_DISP_VIDEO){
        /* plane last valid PTS */
        lst_valid_PTS->lo_word = vpp_obj->plane[planeID].pts.lo_word;
        lst_valid_PTS->hi_word = vpp_obj->plane[planeID].pts.hi_word;

        /* plane current PTS */
        delta_lo = (vpp_obj->plane[planeID].delta_pts.lo_word >> 2) | (vpp_obj->plane[planeID].delta_pts.hi_word << 30); /* real delta PTS will be delta_pts/4 */
        delta_hi = vpp_obj->plane[planeID].delta_pts.hi_word >> 2;

        curr_PTS->hi_word = vpp_obj->plane[planeID].pts.hi_word + delta_hi;
        curr_PTS->lo_word = vpp_obj->plane[planeID].pts.lo_word + delta_lo;
        if (curr_PTS->lo_word < vpp_obj->plane[planeID].pts.lo_word)
            curr_PTS->hi_word ++;

        /* frame count since last valid PTS */
        *frame_cnt = (int)vpp_obj->plane[planeID].frm_count;
    } else {
        /* return invalid PTS when not in display video state */
        lst_valid_PTS->lo_word = 0;
        lst_valid_PTS->hi_word = 0x80000000;
        curr_PTS->lo_word = 0;
        curr_PTS->hi_word = 0x80000000;
        *frame_cnt = 0;
    }

    return (MV_VPP_OK);
}

/***************************************************
 * FUNCTION: mute/un-mute a plane
 * PARAMS:  planeID - plane to mute/un-mute
 *          mute - 1: mute, 0: un-mute
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
int MV_VPPOBJ_SetPlaneMute(int handle, int planeID, int mute)
{
    VPP_OBJ *vpp_obj;
    CMD cmd;
    int params[3];
    int chanID, cpcbID;
    VPPAPI_DBG_INFO("plane [%d], mute [%d] \n", planeID, mute);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if(planeID>=MAX_NUM_PLANES || planeID<FIRST_PLANE || mute > 1) {
        VPP_DEBUG_UTIL_processPrintCmd(vpp_obj, mute);
        return (MV_VPP_EBADPARAM);
    }

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID)
        return (MV_VPP_EUNSUPPORT);

    if(!VPP_ENV_SA_IsMuteAllowed(vpp_obj, planeID, mute)) {
        return (MV_VPP_OK);
    }

    params[0] = planeID;
    params[1] = cpcbID;
    params[2] = mute;
    make_cmd(&cmd, (FUNCPTR)VPP_SetPlaneMute, 3, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);

}

/*******************************************************************
 * FUNCTION: VPP  log config set
 * INPUT: cmd - vpp log cmd
 *            Parm - command related
 *            module - VPP log module
 *            level - VPP log level
 * RETURN: MV_VPP_OK - SUCCEED
 *               MV_EBADPARAM - invalid parameters
 * Note: this function has to be called after the vpp log function initialized.
 *******************************************************************/
int MV_VPPOBJ_LogCtrl(unsigned int cmd, unsigned int parm, unsigned int module, unsigned int level)
{
   return VPP_log_set(cmd, parm, module, level);
}

/**************************************************************************
 * FUNCTION: get current field ID in display of a plane
 * PARAMS:  planeID - plane ID
 *          *field - field ID, 0: first field, 1: second field
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 **************************************************************************/
int MV_VPPOBJ_GetFieldID(int handle, int planeID, int *fieldID)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    if (!fieldID)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    *fieldID = vpp_obj->plane[planeID].field;

    return (MV_VPP_OK);
}

#if defined(PE_3D_BD)
/**************************************************************************
 * FUNCTION: get current field ID in display of a plane
 * PARAMS:  planeID - plane ID
 *          *field - field ID, 0: first field, 1: second field
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 **************************************************************************/
int MV_VPPOBJ_GetViewID(int handle, int cpcbID, int *ViewID)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    if (!ViewID)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    *ViewID =  vpp_obj->dv[cpcbID].viewID;

    return (MV_VPP_OK);
}
#endif

/**************************************************************************
 * FUNCTION: set alpha polarity of a plane
 * PARAMS:  planeID - plane ID
 *          src_pol - source alpha polarity, ENUM_ALPHA_POL type input
 *          bg_pol - border alpha polarity, ENUM_ALPHA_POL type input
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 **************************************************************************/
int MV_VPPOBJ_SetAlphaPolarity(int handle, int planeID, int src_pol, int bg_pol)
{
    VPP_OBJ *vpp_obj;
    CMD cmd;
    int params[3];
    int chanID, cpcbID;

//    VPPAPI_DBG_INFO(" plane [%d], src_pol [%d], bg_pol [%d] \n",  planeID, src_pol, bg_pol);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID)
        return (MV_VPP_EUNSUPPORT);

    if (src_pol)
        src_pol = 1;

    if (bg_pol)
        bg_pol = 1;

    params[0] = planeID;
    params[1] = src_pol;
    params[2] = bg_pol;
    make_cmd(&cmd, (FUNCPTR)VPP_SetPlaneAlphaPolarity, 3, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/********************************************************
 * FUNCTION: set pattern size for patgen in main channel
 * PARAMS:  pWin - size of pattern
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ********************************************************/
int MV_VPPOBJ_SetPatternSize(int handle, VPP_WIN *pWin)
{
    VPP_OBJ *vpp_obj;
    CMD cmd;
    int params[2];
    int cpcbID;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (!pWin)
        return (MV_VPP_EBADPARAM);

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, CHAN_MAIN)) == CPCB_INVALID)
        return (MV_VPP_EUNSUPPORT);

    params[0] = pWin->width;
    params[1] = pWin->height;
    make_cmd(&cmd, (FUNCPTR)VPP_SetPatternSize, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/********************************************************
 * FUNCTION: set pattern frame type
 * PARAMS:  pType - frame type
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ********************************************************/
int MV_VPPOBJ_SetPatternFrameType(int handle, VPP_PATGEN_TYPE *pType)
{
    VPP_OBJ *vpp_obj;
    CMD cmd;
    int params[3];
    int cpcbID;

//    VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (!pType)
        return (MV_VPP_EBADPARAM);

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, CHAN_MAIN)) == CPCB_INVALID)
        return (MV_VPP_EUNSUPPORT);

    params[0] = pType->type;
    params[1] = pType->cadence;
    params[2] = pType->rff;
    make_cmd(&cmd, (FUNCPTR)VPP_SetPatternFrameType, 3, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/********************************************************
 * FUNCTION: set pattern data
 * PARAMS:  pData - pattern data
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ********************************************************/
int MV_VPPOBJ_SetPatternData(int handle, VPP_PATGEN_DATA *pData)
{
    VPP_OBJ *vpp_obj;
    CMD cmd;
    int params[7];
    int cpcbID;

//    VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (!pData)
        return (MV_VPP_EBADPARAM);

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, CHAN_MAIN)) == CPCB_INVALID)
        return (MV_VPP_EUNSUPPORT);

    params[0] = ((pData->vmode & 0x0f) << 4) | (pData->hmode & 0x0f);
    params[1] = pData->hPith;
    params[2] = pData->vPith;
    params[3] = pData->hColPith;
    params[4] = pData->vColPith;
    params[5] = pData->ColSeed1;
    params[6] = pData->ColSeed2;
    make_cmd(&cmd, (FUNCPTR)VPP_SetPatternData, 7, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/***********************************************
 * FUNCTION: change CPCB z-order dynamically
 * PARAMS:  cpcbID - CPCB to make change
 *          *pZorderCtrl - pointer to z-order for each layer
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
int MV_VPPOBJ_ChangeZOrder(int handle, int cpcbID, VPP_ZORDER_CTRL *pZorderCtrl)
{
    VPP_OBJ *vpp_obj;
    CMD cmd;
    int params[3];
    int i, chanID;

    VPPAPI_DBG_INFO(" cpcb [%d] \n",  cpcbID);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (!pZorderCtrl)
        return (MV_VPP_EBADPARAM);

    if(IS_CPCB_ID_INVALID(cpcbID))
        return (MV_VPP_EBADPARAM);

    /* change z-order of each channel */
    for (i=0; i<vpp_obj->dv[cpcbID].num_of_chans; i++){
        chanID = vpp_obj->dv[cpcbID].chanID[i];
        switch (chanID) {
            case CHAN_MAIN:
                if(pZorderCtrl->main != CPCB_ZORDER_INVALID)
                    vpp_obj->chan[CHAN_MAIN].zorder = pZorderCtrl->main;
                break;
            case CHAN_PIP:
                if(pZorderCtrl->pip != CPCB_ZORDER_INVALID)
                    vpp_obj->chan[CHAN_PIP].zorder = pZorderCtrl->pip;
                break;
            case CHAN_GFX1:
                if(pZorderCtrl->gfx1 != CPCB_ZORDER_INVALID)
                    vpp_obj->chan[CHAN_GFX1].zorder = pZorderCtrl->gfx1;
                break;
            default:
                break;
        }
    }

    params[0] = cpcbID;
    /*pack Z order values to param*/
    params[1] = (pZorderCtrl->main & 0xFF) | ((pZorderCtrl->pip  & 0xFF) << 8) | ((pZorderCtrl->gfx1  & 0xFF) << 16);

    make_cmd(&cmd, (FUNCPTR)VPP_ChangeZOrder, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/***********************************************
 * FUNCTION: get CPCB z-order, Z-order of the channels not connected to the CPCB is set to invalid
 * PARAMS:  cpcbID - CPCB to make change
 *          *pZorderCtrl - pointer to z-order for each layer
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
int MV_VPPOBJ_GetZOrder(int handle, int cpcbID, VPP_ZORDER_CTRL *pZorderCtrl)
{
    VPP_OBJ *vpp_obj;
    int i, chanID;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (!pZorderCtrl)
        return (MV_VPP_EBADPARAM);

    /*set Z-order of each plane to invalid*/
    for(i=0; i<(sizeof(*pZorderCtrl)/sizeof(int)); i++){
        *(((int*)pZorderCtrl) + i) = CPCB_ZORDER_INVALID;
    }

    if(IS_CPCB_ID_INVALID(cpcbID))
        return (MV_VPP_EBADPARAM);

    /* get z-order of each channel */
    for (i=0; i<vpp_obj->dv[cpcbID].num_of_chans; i++){
        chanID = vpp_obj->dv[cpcbID].chanID[i];
        switch (chanID) {
            case CHAN_MAIN:
                pZorderCtrl->main = vpp_obj->chan[CHAN_MAIN].zorder;
                break;
            case CHAN_PIP:
                pZorderCtrl->pip = vpp_obj->chan[CHAN_PIP].zorder;
                break;
            case CHAN_GFX1:
                pZorderCtrl->gfx1 = vpp_obj->chan[CHAN_GFX1].zorder;
                break;
            default:
                break;
        }
    }

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: get CPCB VBI interrupt counter
 * INPUT: cpcbID - CPCB id
 *        *vbi_num - vbi interrupt count
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *******************************************************************/
int MV_VPPOBJ_GetCPCBInterruptCounter(int handle, int cpcbID, int *vbi_num)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (cpcbID<FIRST_CPCB || cpcbID>=MAX_NUM_CPCBS)
        return (MV_VPP_EBADPARAM);

    *vbi_num = vpp_obj->dv[cpcbID].vbi_num;

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: set De-interlacer and VNR working mode
 * INPUT: deint_mode - de-interlacer mode: DEINT_2D, DEINT_3D, or -1 (no change)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetDeintMode(int handle, int deint_mode)
{
    CMD cmd;
    int params[2];
    VPP_OBJ *vpp_obj;
    int cpcbID;

//   VPPAPI_DBG_INFO(" deint_mode [%d] \n",  deint_mode);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((deint_mode != DEINT_2D) && (deint_mode != DEINT_3D) && (deint_mode != -1))
        return (MV_VPP_EBADPARAM);

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, CHAN_MAIN)) == CPCB_INVALID) {
          /*channel is not connected to any DV in configuration*/
        return (MV_VPP_EUNSUPPORT);
    }

    /*set deint/vnr mode */
    params[0] = deint_mode;

    make_cmd(&cmd, (FUNCPTR)VPP_SetDeintMode, 1, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);

}

/********************************************************************************
 * FUNCTION: Set Deint Param
 * INPUT: deintCmd- deint cmd
   INPUT: pDeintRegs- Deint DS parameter depend upon cmd
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetDEINTParam(int handle, int deintCmd, VPP_VDO_DEINT_REG *pDeintRegs )
{
    CMD cmd;
    int params[2];
    VPP_OBJ *vpp_obj;
    int cpcbID;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((deintCmd <= DEINT_CMD_INVALID) || (deintCmd >= MAX_NUM_DEINT_CMD))
        return (MV_VPP_EBADPARAM);

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, CHAN_MAIN)) == CPCB_INVALID) {
          /*channel is not connected to any DV in configuration*/
        return (MV_VPP_EUNSUPPORT);
    }

    /*set DEINT parameter profile */
    params[0] = deintCmd;
    switch(deintCmd)
    {
        case VPP_CMD_DEINT_SET_USERCAD:
        {
            if(pDeintRegs->DEINTUserCad.Cadence==0)
                VPP_Memcpy(&(vpp_obj->DEINTUserCad1),pDeintRegs,sizeof(VPP_DEINTUSER_CAD_PARAMS_NEW));
            if(pDeintRegs->DEINTUserCad.Cadence==1)
                VPP_Memcpy(&(vpp_obj->DEINTUserCad2),pDeintRegs,sizeof(VPP_DEINTUSER_CAD_PARAMS_NEW));
            params[1]=pDeintRegs->DEINTUserCad.Cadence;
        }
        break;
        case VPP_CMD_DEINT_SET_EXCLCTRL:
        {
            VPP_Memcpy(&(vpp_obj->DEINTExclCtrl),pDeintRegs,sizeof(VPP_DEINTEXCL_CTRL_PARAMS));
        }
        break;
        case VPP_CMD_DEINT_SET_LPFCTRL:
        {
            VPP_Memcpy(&(vpp_obj->DEINTLPFCtrl),pDeintRegs,sizeof(VPP_FE_DEINT_LPF_CTRL));
        }
        break;
        case VPP_CMD_DEINT_SET_FILM_CADCTRL:
        {
            VPP_Memcpy(&(vpp_obj->FilmCadence),pDeintRegs,sizeof(UINT16));
        }
        break;
        case VPP_CMD_DEINT_SET_PRESET_MODE:
        {
            VPP_Memcpy(&(vpp_obj->DEINTPreset),pDeintRegs,sizeof(VPP_FE_DEINT_PRESET_MODE));
        }
        break;
        case VPP_CMD_DEINT_SET_FORCED_2D:
        {
            VPP_Memcpy(&(vpp_obj->DEINTF2DCtrl),pDeintRegs,sizeof(VPP_DEINT_FORCED2D_CTRL));
        }
        break;
        case VPP_CMD_DEINT_SET_HARD_SOFT_FILM_MODE:
        {
            VPP_Memcpy(&(vpp_obj->DEINTFilmMode),pDeintRegs,sizeof(VPP_DEINT_FILM_MODE_ENTRY_PARAM));
        }
        break;
        case VPP_CMD_DEINT_SET_OSD_CTRL:
        {
            VPP_Memcpy(&(vpp_obj->DEINTOSDCtrl),pDeintRegs,sizeof(UINT8));
        }
        break;
        case VPP_CMD_DEINT_SET_DEINT_ADV_CTRL:
        {
            VPP_Memcpy(&(vpp_obj->DEINTADVCtrl),pDeintRegs,sizeof(VPP_DEINTADV_MD_PARAMS));
        }
        break;
        default:
        {
        }
        break;

    }
    make_cmd(&cmd, (FUNCPTR)VPP_SetDeintEvalCommand, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Get Deint Param
 * INPUT: deintCmd- deint cmd
   OUTPUT: pDeintRegs- Deint DS parameter depend upon cmd
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/

int MV_VPPOBJ_GetDEINTParam(int handle, int deintCmd, VPP_VDO_DEINT_REG *pDeintRegs )
{
    return (MV_VPP_OK);

}

/********************************************************************************
 * FUNCTION: set the source of video output
 * INPUT: vout - id of the video output
 *            cpcbID - id of the CPCB
 *            interlaced - OUTPUT_PROGRESSIVE or OUTPUT_INTERLACED
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetVoutSource(int handle, int vout, int cpcbID, int interlaced)
{
    CMD cmd;
    int params[3];
    VPP_OBJ *vpp_obj;
    int i, old_cpcbID;

 //   VPPAPI_DBG_INFO("vout [%d], cpcbID [%d] interlaced [%d]\n",vout, cpcbID, interlaced);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (interlaced < OUTPUT_PROGRESSIVE || interlaced > OUTPUT_AUTO_SELECT)
        return (MV_VPP_EBADPARAM);

    if (vout<FIRST_VOUT || vout>=MAX_NUM_VOUTS)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if (cpcbID<FIRST_CPCB || cpcbID>=MAX_NUM_CPCBS)
        return (MV_VPP_EBADPARAM);


    old_cpcbID = vpp_obj->vout[vout].dvID;
    if(old_cpcbID == CPCB_INVALID) {
        /*vout has not been connected to a CPCB*/
        vpp_obj->vout[vout].dvID = cpcbID;
        vpp_obj->dv[cpcbID].voutID[vpp_obj->dv[cpcbID].num_of_vouts] = vout;
        vpp_obj->dv[cpcbID].num_of_vouts++;
    }else if(old_cpcbID != cpcbID){
        /*vout has been connected to a different CPCB*/
        for (i=0; i<vpp_obj->dv[old_cpcbID].num_of_vouts; i++) {
            if (vpp_obj->dv[old_cpcbID].voutID[i] == vout) {
                vpp_obj->dv[old_cpcbID].voutID[i] = vpp_obj->dv[old_cpcbID].voutID[vpp_obj->dv[old_cpcbID].num_of_vouts-1];
                vpp_obj->dv[old_cpcbID].voutID[vpp_obj->dv[old_cpcbID].num_of_vouts-1] = VOUT_INVALID;
                vpp_obj->dv[old_cpcbID].num_of_vouts--;
                break;
            }
        }

        vpp_obj->vout[vout].dvID = cpcbID;
        vpp_obj->dv[cpcbID].voutID[vpp_obj->dv[cpcbID].num_of_vouts] = vout;
        vpp_obj->dv[cpcbID].num_of_vouts++;
    }

    if (vpp_obj->dv[cpcbID].status == STATUS_ACTIVE){
        i = cpcbID;
    } else if ((old_cpcbID != CPCB_INVALID) && (vpp_obj->dv[old_cpcbID].status == STATUS_ACTIVE)) {
        i = old_cpcbID;
    } else {
        /* The CPCB is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    /* set vout source*/
    params[0] = vout;
    params[1] = cpcbID;
    params[2] = interlaced;

    make_cmd(&cmd, (FUNCPTR)VPP_SetVoutSource, 3, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[i].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[i].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[i].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[i].cmdq_lock);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: This function is extension of MV_VPPOBJ_SetVoutMute.
 *           Compared with MV_VPPOBJ_SetVoutMute, this function will mute
 *           audio/video or both
 *           Notice, this function is only effect on HDMI port
 * INPUT: vout - id of the video output
 *        mute - 1:mute, 0:un-mute
 *        bVideo - 1:mute or un-mute video, 0:do nothing on video
 *        bAudio - 1:mute or un-mute audio, 0:do nothing on audio
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetVoutMute_Ext(int handle, int vout, int mute, int bVideo, int bAudio)
{
    CMD cmd;
    int cpcbID;
    int params[4];
    VPP_OBJ *vpp_obj;

    VPPAPI_DBG_INFO(" vout ext [%d], mute [%d] \n", vout, mute);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vout<FIRST_VOUT || vout>=MAX_NUM_VOUTS)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, vout)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if(vout != VOUT_HDMI) {
        /*Do nothing except HDMI*/
        return (MV_VPP_OK);
    }

    if ((FALSE == bVideo) && (FALSE == bAudio)) {
        return (MV_VPP_OK);
    } else if ((TRUE == bVideo) && (TRUE == bAudio)) {
        params[0] = VPP_HDMI_SET_AV_MUTE;
    } else if (FALSE == bAudio) {
        params[0] = VPP_HDMI_SET_VIDEO_MUTE;
    } else {
        params[0] = VPP_HDMI_SET_AUDIO_MUTE;
    }
    params[1] = mute;
    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: mute/un-mute a video output
 * INPUT: vout - id of the video output
 *            mute - 1:mute, 0:un-mute
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetVoutMute(int handle, int vout, int mute)
{
    CMD cmd;
    int cpcbID;
    int params[3];
    VPP_OBJ *vpp_obj;

//    VPPAPI_DBG_INFO(" vout [%d], mute [%d] \n", vout, mute);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vout<FIRST_VOUT || vout>=MAX_NUM_VOUTS)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, vout)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    /*HDMI mute*/
    params[0] = VPP_HDMI_SET_VIDEO_MUTE;
    params[1] = mute;
    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}


/********************************************************************************
 * FUNCTION: enable/disable HDMI output
 * INPUT: Enable - 1:enable, 0:disable
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHDMIEnable(int handle, int vout, int Enable)
{
        CMD cmd;
        int cpcbID;
        int params[3];
        VPP_OBJ *vpp_obj;

        VPPAPI_DBG_INFO(" vout [%d], mute [%d] \n",  vout, Enable);

        if (g_vpp->magic != VPP_MAGIC){
            return (MV_VPP_EUNCONFIG);
        }

        if (handle<0 || handle>=MAX_NUM_OBJECTS)
            return (MV_VPP_EBADPARAM);

        vpp_obj = g_vpp->vpp_objs[handle];

        if (!vpp_obj)
            return (MV_VPP_ENODEV);

        if (vout<FIRST_VOUT || vout>=MAX_NUM_VOUTS)
            return (MV_VPP_EBADPARAM);

        if (vpp_obj->status == STATUS_INACTIVE){
            /* VPP module is not configured */
            return (MV_VPP_EUNCONFIG);
        }

        if ((cpcbID = CPCB_OF_VOUT(vpp_obj, vout)) == CPCB_INVALID) {
              /* Output is not connected to any DV in configuration */
            return (MV_VPP_EUNSUPPORT);
        }

        if (!Enable)
        {
            /*HDMI mute*/
            params[0] = VPP_HDMI_SET_VIDEO_MUTE;
            params[1] = 1;
            make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 2, params);

            MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
            ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

            /* commit commands */
            cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);
            MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
        }

        MV_VPPOBJ_SetHdmiPhyPower(handle, Enable);

        if (Enable)
        {
            //HDMI mute
            params[0] = VPP_HDMI_SET_VIDEO_MUTE;
            params[1] = 0;
            make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 2, params);

            MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
            ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

            // commit commands
            cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

            MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
        }

        return (MV_VPP_OK);

}



/********************************************************************************
 * FUNCTION: power on/down an analog video output
 * INPUT: vdac - id of the video DAC
 *            enable - 1:power on, 0:power down
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetVdacPower(int handle, int vdac, int enable)
{
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: turn off Aux channel clock to save power
 *           this function should be called after AVPLL_Enable
 * INPUT:  off - 1: turn off, 0: do nothing
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_VPP_EUNCONFIG - VPP not configured
 *         MV_VPP_EBADPARAM - invalid parameters
 *         MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_TurnOffCLK(int handle, int off)
{
    VPP_OBJ *vpp_obj;

//    VPPAPI_DBG_INFO("off [%d] \n",  off);

    if(!off) return (MV_VPP_OK);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    return (MV_VPP_OK);
}

int MV_VPPOBJ_SetHdmiPhyPower(int handle, int enable)
{
    /*  This API to be called from display service only.
        TODO: change enable to BOOL from int */

    VPP_OBJ *vpp_obj;
    int cpcbID;
    int retVal;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if (enable == VPP_BE_GetHdmiPhyExpectedState(vpp_obj)) {
        VPPHDMI_DBG_INFO("HDMI Tx phy already in expected state %d\n", enable);
        return (MV_VPP_OK);
    }

    /* Configure hdmi for PHY state change */
    if (MV_VPP_OK != (retVal = VPP_BE_ConfigHdmiPhyStateChange(vpp_obj, enable)))
        return retVal;

    /* Start PHY state change */
    if (MV_VPP_OK != (retVal = VPP_BE_SetHdmiPhyPower(vpp_obj, enable)))
        return retVal;

    return (MV_VPP_OK);
}

int MV_VPPOBJ_EnableHdmiAudioFmt(int handle, int enable)
{
    CMD cmd;
    int cpcbID;
    int params[2];
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    params[0] = VPP_HDMI_ENABLE_AUDIO_FMT;
    params[1] = enable;

    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 2, params);
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Set Hdmi Audio format
 * INPUT: sampFreq - Audio sampling frequency
 *      : sampSize - Word length
 *      : mClkFactor - 128/256/384/512 times frequency
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiAudioFmt(int handle, int enable, VPP_HDMI_AUDIO_CFG *pAudioCfg)
{
    CMD cmd;
    int cpcbID;
    int params[2];
    int     retVal;
    VPP_OBJ *vpp_obj;

//    VPPAPI_DBG_INFO(" enable [%d], # of channels [%d], port # [%d], sample freq [%d], sample size [%d], mclk [%d], audioFmt [%d], HBR [%d] \n", enable, pAudioCfg->numChannels, pAudioCfg->portNum, pAudioCfg->sampFreq, pAudioCfg->sampSize, pAudioCfg->mClkFactor, pAudioCfg->audioFmt, pAudioCfg->hbrAudio);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if (MV_VPP_OK != (retVal = VPP_BE_ConfigHdmiAudioFmt (vpp_obj,enable,pAudioCfg)))
    {
        return retVal;
    }

    params[0] = VPP_HDMI_SET_AUDIO_FMT;
    params[1] = enable;

    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 2, params);
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}
/********************************************************************************
 * FUNCTION: Set Hdmi Video format
 * INPUT: color_fmt - color format (RGB, YCbCr 444, 422)
 *      : bit_depth - 8/10/12 bit color
 *      : pixel_rept - 1/2/4 repetitions of pixel
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiVideoFmt(int handle, int color_fmt, int bit_depth, int pixel_rept)
{
    CMD     cmd;
    int     cpcbID;
    int     resID;
    int     params[1];
    VPP_OBJ *vpp_obj;
    int     retVal;

    VPPAPI_DBG_INFO(" color_fmt [%d], bit_depth [%d], pixel_rept [%d] \n", color_fmt, bit_depth, pixel_rept);
    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }
    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }
    /* Configure AVPLL */
    resID = VPP_GetResId(cpcbID);

    if (resID == RES_INVALID)
        return (MV_VPP_EBADPARAM);


    /* Config video format */
    if (MV_VPP_OK != (retVal = VPP_BE_ConfigHdmiVideoFmt (vpp_obj, color_fmt, bit_depth, pixel_rept)))
        return retVal;

    if(IS_DSI_PANEL_RES(resID))
        return (MV_VPP_OK);
    params[0] = VPP_HDMI_SET_VIDEO_FMT;
    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 1, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);
    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Set Hdmi Video format
 * INPUT:  MUX VALUE for Ch3Ch2Ch1Ch0 [Input value of Hex where each Nibble corresponds to each channel]
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiTmdsMuxSel(int handle, UINT32 mux_value)
{
    /* depricated API */
    return (MV_VPP_EUNSUPPORT);
}

/********************************************************************************
 * FUNCTION: Set Hdmi 3D structure
 * INPUT: structure - structure ID
 *      : ext_data - extra data for structure 8 or above
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmi3DStructure(int handle, int structure, int ext_data)
{
    CMD     cmd;
    int     cpcbID;
    int     params[4];
    int     retCode;
    VPP_OBJ *vpp_obj;
    VPP_HDMI_PKT auxPkt;
    RESOLUTION_INFO  *pResinfo;

//    VPPAPI_DBG_INFO(" structure [%d], ext_data [%d] \n", structure, ext_data);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
        /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }
    pResinfo = VPP_GetResInfo(VPP_GetResId(cpcbID));

    if (!pResinfo->flag_3d && (VPP_GetResId(cpcbID)<FIRST_RES_3D)){
        return (MV_VPP_EBADCALL);
    }

    params[0] = VPP_HDMI_SEND_HOST_PKT;
    params[1] = VPP_HDMI_PKT_ID_VENDOR_INFOFRAME;
    params[2] = 1; /* repeat */

    auxPkt.vndrSpecInfoFrm.ieeeRegID = 0x000c03;
    if (((BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx))->edidInfo.support_3D) {
        auxPkt.vndrSpecInfoFrm.HdmiVideoFmt = VPP_HDMI_VIDEO_FMT_3D_FMT;
    }
    else {
        auxPkt.vndrSpecInfoFrm.HdmiVideoFmt = VPP_HDMI_VIDEO_FMT_NO_INFO;
    }
    auxPkt.vndrSpecInfoFrm.Hdmi_3D_Structure = structure;
    auxPkt.vndrSpecInfoFrm.Hdmi_3D_Meta_Present = 0;
    auxPkt.vndrSpecInfoFrm.Hdmi_3D_Ext_Data = ext_data;
    if (MV_VPP_OK != (retCode = VPP_BE_HdmiFormatAuxPkt (vpp_obj, TRUE, VPP_HDMI_PKT_ID_VENDOR_INFOFRAME, &auxPkt)))
        return retCode;

    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 4, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: set Hdmi audio VUC bits
 * INPUT: pVUCCfg        - pointer to VUC config structure
 *        loadOnAudioCfg - 1 for loading these data when audio
 *                       - is configured subsequently
 *                       - 0 for loading it on the next VBI
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiAudioVUCBits(int handle, VPP_HDMI_AUDIO_VUC_CFG *pVUCCfg, int loadOnAudioCfg)
{
    CMD     cmd;
    int     cpcbID;
    int     params[2];
    int     retVal;
    VPP_OBJ *vpp_obj;

//    VPPAPI_DBG_INFO(" loadOnAudioCfg [%d] \n",  loadOnAudioCfg);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    params[0] = VPP_HDMI_SET_AUDIO_VUC_BITS;
    if (MV_VPP_OK != (retVal = VPP_BE_ConfigHdmiAudioVUCBits
                               (vpp_obj, pVUCCfg, loadOnAudioCfg, &params[1])))
    {
        return retVal;
    }
    if (params[1])
    {
        make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 2, params);
        MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

        ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

        /* commit commands */
        cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

        MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
    }
    return (MV_VPP_OK);
}

int MV_VPPOBJ_SetHdmiAudioFifoReset(int handle, int bSet)
{
    /* function depricated */
    return (MV_VPP_EUNSUPPORT);
}

/********************************************************************************
 * FUNCTION: mute/un-mute hdmi audio output
 * INPUT: mute - 1:mute, 0:un-mute
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiAudioMute(int handle, int mute)
{
    CMD cmd;
    int cpcbID;
    int params[2];
    VPP_OBJ *vpp_obj;

//    VPPAPI_DBG_INFO(" mute [%d] \n",  mute);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    /*HDMI audio mute*/
    params[0] = VPP_HDMI_SET_AUDIO_MUTE;
    params[1] = mute;
    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Set Hdmi output mode (Hdmi/Dvi)
 * INPUT: hdmiMode - 0:DVI, 1:HDMI
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiMode(int handle, int hdmiMode)
{
    CMD cmd;
    int params[2];
    int cpcbID;
    VPP_OBJ *vpp_obj;

//    VPPAPI_DBG_INFO(" HdmiMode [%d] \n", hdmiMode);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    /*HDMI Mode*/
    params[0] = VPP_HDMI_SET_MODE;
    params[1] = hdmiMode;
    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Set HDMI Video Information
 * INPUT: aspect ratio
 *      : scan info
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiVideoInfo(int handle, int aspRatio, int scanInfo)
{
    CMD cmd;
    int cpcbID;
    int params[4];
    int retVal;
    VPP_OBJ *vpp_obj;

 //   VPPAPI_DBG_INFO("aspRatio [%d], scanInfo [%d] \n",  aspRatio, scanInfo);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    /*Video info*/
    if (MV_VPP_OK != (retVal = VPP_BE_SetHdmiVideoInfo (vpp_obj, aspRatio, scanInfo)))
        return retVal;

    params[0] = VPP_HDMI_SEND_HOST_PKT;
    params[1] = VPP_HDMI_PKT_ID_AVI_INFOFRAME;
    params[2] = 0;
    params[3] = TRUE; // repeat on
    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 4, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Set HDMI Audio Information
 * INPUT: Speaker Map
 *      : Level Shift to denote attenuation (0 - 15)dB
 *      : Down mix inhibit flag
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiAudioInfo(int handle, VPP_HDMI_SPKR_ALLOC *pSpkrMap,
                               int lvlShiftVal, int downMixInhFlag)
{
    CMD cmd;
    int cpcbID;
    int params[4];
    int retVal;
    VPP_OBJ *vpp_obj;

 //   VPPAPI_DBG_INFO("SpeakerMap [0x%x], lvlShiftVal [%d], MixInh [%d]\n",  (int)(*(char *)pSpkrMap), lvlShiftVal, downMixInhFlag);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    /*Audio info*/
    if (MV_VPP_OK != (retVal = VPP_BE_SetHdmiAudioInfo (vpp_obj, *(unsigned char*)(pSpkrMap), lvlShiftVal, downMixInhFlag)))
        return retVal;

    params[0] = VPP_HDMI_SEND_HOST_PKT;
    params[1] = VPP_HDMI_PKT_ID_AUDIO_INFOFRAME;
    params[2] = 1;
    params[3] = TRUE; // repeat on
    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 4, params);
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Set SRM Data
 * INPUT: pSrmData - Pointer to SRM data and length (Input)
 *      : pDevSignData - Pointer to device signature to check content
 *      :              - validity (Input)
 *      : pVernNum - Pointer to return SRM info parsed and updated (output)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetSRM (int handle, VPP_HDCP_SRM_DATA *pSrmData,
                      VPP_DEV_SIGN_DATA *pDevSignData, VPP_HDCP_SRM_INFO *pSrmInfo)
{
    VPP_OBJ *vpp_obj;
    int     retCode = MV_VPP_OK;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    if (pSrmData == NULL || pDevSignData == NULL || pSrmInfo == NULL)
        return (MV_VPP_EBADPARAM);

    retCode = VPP_BE_ConfigureSRM (vpp_obj, pSrmData, pDevSignData, pSrmInfo);

    return (retCode);
}

/********************************************************************************
 * FUNCTION: Get SRM Version
 * INPUT: pHdcp22SrmVersion - HDCP 2.2 SRM File version (Output)
 *      : pHdcp22SrmVersion - HDCP 1.4 SRM File version (Output)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetSRMVersion(int handle, UINT16 *pHdcp22SrmVersion, UINT16 *pHdcp14SrmVersion)
{
    VPP_OBJ *vpp_obj;
    int     retCode = MV_VPP_OK;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    if (pHdcp22SrmVersion == NULL || pHdcp14SrmVersion == NULL)
        return (MV_VPP_EBADPARAM);

    retCode = VPP_BE_GetSRMVersion (vpp_obj, pHdcp22SrmVersion, pHdcp14SrmVersion);

    return (retCode);
}

/********************************************************************************
 * FUNCTION: Enable/Disable HDCP SRM check. This API controls if revoked list
 *         : check would be done
 * INPUT   : enable - Enable/Disable HDCP SRM Check
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_EnableSRMCheck(int handle, int enable)
{
    VPP_OBJ *vpp_obj;
    int     retCode = MV_VPP_OK;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    retCode = VPP_BE_EnableSRMCheck (vpp_obj, enable);

    return (retCode);
}

/********************************************************************************
 * FUNCTION: Before loading HDCP Keys to the registers, check if the VPP status is ready
 * INPUT: handle    - VPP object handle
 *      : pHdcpKeys - Pointer to HDCP keys to be loaded
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - HDCP is enabled currently(Keys cannot be loaded
 *                         - with HDCP on)
 ********************************************************************************/
int MV_VPPOBJ_LoadHDCPKeys_PreCheck(int handle, UINT8 *pHdcpKeys)
{
    /* depricated API */
    return (MV_VPP_EUNSUPPORT);
}

/********************************************************************************
 * FUNCTION: Enable/Disable HDCP
 * INPUT: Enable - Control to turn HDCP on/off
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_EnableHDCP(int handle, int enable)
{
    /* should be called only from displayservice*/
    CMD cmd;
    int cpcbID;
    int params[2];
    VPP_OBJ *vpp_obj;

    VPPAPI_DBG_INFO(" enable [%d] \n",  enable);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    /*HDCP control*/
    params[0] = VPP_HDMI_SET_HDCP;
    params[1] = enable;

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
        VPP_SetHdmiCmd (vpp_obj, params);
    } else {
        make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 2, params);
        MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
        ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

        /* commit commands */
        cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

        MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
    }
    return (MV_VPP_OK);
}
/********************************************************************************
 * FUNCTION: Read HDCP Status
 * OUTPUT:  - pHDCPState (Authorization state)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDCPState(int handle, int *pHDCPState)
{
    int params[2];
    VPP_OBJ *vpp_obj;

 //   VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if(pHDCPState==NULL)
        return (MV_VPP_EBADPARAM);

    /*HDCP control*/
    params[0] = VPP_HDMI_GET_HDCP_STATE;
    params[1] = (int)(pHDCPState);

    VPP_SetHdmiCmd (vpp_obj, params);
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Read HDCP Version
 * OUTPUT:  - pHDCPVersion (value returned HDCP 1.x = 0 and HDCP2.x = 1)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDCPVersion(int handle, int *pHDCPVersion)
{
    /* This API will be depricated soon. Use MV_VPPOBJ_GetCurrentHDCPVersion for reading active HDCP version */
    VPP_OBJ *vpp_obj;

    //VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if(pHDCPVersion == NULL)
        return (MV_VPP_EBADPARAM);

    return VPP_BE_GetHdcpVersion(vpp_obj, pHDCPVersion);
}

/********************************************************************************
 * FUNCTION: Read Current HDCP Version
 * OUTPUT:  - pHDCPVersion (value returned HDCP 1.x = 0 and HDCP2.2 = 1)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 *         MV_VPP_ENOSINKCNCTED - No sink connected
 *         MV_VPP_ENOHDCPENABLED - HDCP not enabled
 ********************************************************************************/
int MV_VPPOBJ_GetCurrentHDCPVersion(int handle, VPP_HDMI_HDCP_VERSION *pHDCPVersion)
{
    int Retval = MV_VPP_OK;

#ifdef VPPDRV_INTR_MONITOR_FREEZE_ON_BOOTUP
    VPPDRV_vppIntrResetSkipCount(g_vpp->vpp_objs[handle]);
#endif //VPPDRV_INTR_MONITOR_FREEZE_ON_BOOTUP

    VPP_OBJ *vpp_obj;
    int params[2];

    VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if(pHDCPVersion == NULL)
        return (MV_VPP_EBADPARAM);


    params[0] = VPP_HDMI_GET_CUR_HDCP_VERSION;
    params[1] = (int)pHDCPVersion;
    Retval = VPP_SetHdmiCmd (vpp_obj, params);

    return Retval;
}
/********************************************************************************
 * FUNCTION: Set HDCP Version
 * INPUT :  - iHDCPVersion (value to be set: HDCP 1.x = 0 and HDCP2.x = 1)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHDCPVersion(int handle, int iHDCPVersionSel)
{

    CMD cmd;
    int cpcbID;

    VPP_OBJ *vpp_obj;
    int params[2];

    VPPAPI_DBG_INFO("enter : version : %d \n", iHDCPVersionSel);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if(VPP_HDMI_HDCP_VER_ERR == iHDCPVersionSel)
    {
        VPPAPI_DBG_ERROR("invalid value for hdcp version.\n");
        return (MV_VPP_EBADPARAM);
    }

    /*HDCP control*/
    params[0] = VPP_HDMI_SET_HDCP_VERSION;
    params[1] = (int)(iHDCPVersionSel);

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
        VPP_SetHdmiCmd (vpp_obj, params);
    } else {

        make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 2, params);
        MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
        ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

        /* commit commands */
        cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

        MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
    }

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Get HDCP Ksv and BKsv
 * INPUT: pHdcpKsv - HDCP ksv data structure
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDCPKsv(int handle, VPP_HDCP_KSV_DATA *pHdcpKsv)
{
    /* depricated API */
    return (MV_VPP_EUNSUPPORT);
}

/********************************************************************************
 * FUNCTION: Get HDCP Aksv
 * INPUT: pHdcpAksv - HDCP Aksv
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDCPAksv(int handle, VPP_HDMI_HDCP_AKSV *pHdcpAksv)
{
    /* depricated API */
    return (MV_VPP_EUNSUPPORT);
}


/********************************************************************************
 * FUNCTION: Read Max HDCP capability of Sink
 * OUTPUT:  - pHDCPVersion (value returned HDCP 1.x = 0 and HDCP2.2 = 1)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 *         MV_VPP_ENOSINKCNCTED - No sink connected
 *         MV_VPP_ENOHDCPENABLED - HDCP not enabled
 ********************************************************************************/
int MV_VPPOBJ_GetSinkHDCPCapability(int handle, VPP_HDMI_HDCP_VERSION *pHDCPVersion)
{

    int Retval = 0;
    VPP_OBJ *vpp_obj;
    int params[2];


    VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if(pHDCPVersion == NULL)
        return (MV_VPP_EBADPARAM);

    params[0] = VPP_HDMI_GET_SINK_HDCP_CAP;
    params[1] = (int)pHDCPVersion;
    Retval = VPP_SetHdmiCmd (vpp_obj, params);

    return Retval;
}

/********************************************************************************
 * FUNCTION: Set TX HDCP mode
 * INPUT:  - Mode(Repeater/Transmitter)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHDCPRepeaterMode(int handle, UINT32 Mode)
{
    /* depricated API */
    return (MV_VPP_EUNSUPPORT);
}



/********************************************************************************
 * FUNCTION: Get HPD status
 * INPUT: pHpdStatus
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHPDStatus(int handle, int *pHpdStatus)
{
    VPP_OBJ *vpp_obj;
    int retVal = MV_VPP_OK;

//   VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }
    if (pHpdStatus == NULL)
        return (MV_VPP_EBADPARAM);

    retVal = VPP_BE_GetHPDStatus (vpp_obj, pHpdStatus);

#if defined(FPGA)
    /* In FPGA ignore the HDP status */
    *pHpdStatus = true;
#endif

    return retVal;
}

/********************************************************************************
 * FUNCTION: Get HDR10 playback status
 * OUTPUT: pIsHdrPlaying (1-HDR10 playing, 0-HDR10 not playing)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_GetHDR10PlaybackStatus(int handle, BOOL *pIsHdrPlaying)
{
    VPP_OBJ *vpp_obj;

    VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }
    if (pIsHdrPlaying == NULL)
        return (MV_VPP_EBADPARAM);

    *pIsHdrPlaying = (vpp_obj->isHDRContent) &
        (vpp_obj->HDR_capable) &
        (vpp_obj->HDR_enable) &0x1;

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Get Hdmi Sink Capabilities
 * INPUT: pSinkCaps
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDMISinkCaps(int handle, VPP_HDMI_SINK_CAPS *pSinkCaps)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }
    if (pSinkCaps == NULL)
        return (MV_VPP_EBADPARAM);

    return VPP_BE_GetSinkCaps (vpp_obj, pSinkCaps);
}

/********************************************************************************
 * FUNCTION: Get Hdmi Sink Feature with mask
 * INPUT: pSinkFeatureMap
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDMISinkFeatureMap(int handle, VPP_HDMI_SINKCAP_BITMASK *pSinkFeatureMap)
{
    /* cut down version of sinkcaps used for fastlogo/linux,
     * used to check basic features like 4k_support,exposed via mask
     */
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (pSinkFeatureMap == NULL)
        return (MV_VPP_EBADPARAM);

    return VPP_BE_GetSinkFeatureMap(vpp_obj, pSinkFeatureMap);
}

/********************************************************************************
 * FUNCTION: Get Hdmi Raw EDID
 * INPUT: pRawEdid
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDMIRawEdid(int handle, VPP_HDMI_RAW_EDID *pRawEdid)
{
    VPP_OBJ *vpp_obj;
    int params[2];

//    VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }
    if (pRawEdid == NULL)
        return (MV_VPP_EBADPARAM);

    params[0] = VPP_HDMI_GET_RAW_EDID;
    params[1] = (int)(pRawEdid);
    VPP_SetHdmiCmd (vpp_obj, params);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Register for HDCP event notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 *      : contextParam - Context parameter for the registering application
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_RegisterHdcpCallback (int handle, UINT32 eventCode,
        MV_VPP_EVENT_CALLBACK callbackFn,
        void *contextParam)
{
    VPP_OBJ *vpp_obj;
    int *params = NULL;
    uintptr_t paddr = 0;
    char *paramsOffset = NULL;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    params = (int *)VPP_Malloc(2*sizeof(int) + 2*sizeof(uintptr_t));
    params[0] = VPP_HDCP_REG_CALLBACK;
    params[1] = (int)eventCode;

    paramsOffset = (char *)(&params[2]);
    paddr = (uintptr_t)callbackFn;
    VPP_Memcpy(paramsOffset, &paddr, sizeof(uintptr_t));
    paramsOffset += sizeof(uintptr_t);
    paddr = (uintptr_t)contextParam;
    VPP_Memcpy(paramsOffset, &paddr, sizeof(uintptr_t));
    paramsOffset += sizeof(uintptr_t);

    VPP_SetHdmiCmd (vpp_obj, params);
    VPP_Free(params);
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Unregister for HDCP event notifications
 * INPUT: eventCode - Event code for which callback was registered
 *      : callbackFn - Address of callback function
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_UnRegisterHdcpCallback (int handle)
{
    VPP_OBJ *vpp_obj;
    int params[1];

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    params[0] = VPP_HDCP_UNREG_CALLBACK;
    VPP_SetHdmiCmd (vpp_obj, params);
    return (MV_VPP_OK);
}


/********************************************************************************
 * FUNCTION: Register for HDMI event notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 *      : contextParam - Context parameter for the registering application
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_RegisterHdmiCallback (int handle, UINT32 eventCode,
                                    MV_VPP_EVENT_CALLBACK callbackFn,
                                    void *contextParam)
{
    VPP_OBJ *vpp_obj;
    int *params;
    uintptr_t paddr;
    char *paramsOffset;

    VPPAPI_DBG_INFO(" eventCode [%d], callbackFn [0x%x] \n",  eventCode, (int)callbackFn);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    params = (int *)VPP_Malloc(2*sizeof(int) + 2*sizeof(uintptr_t));
    if (!params)
        return (MV_VPP_ENOMEM);

    params[0] = VPP_HDMI_REG_CALLBACK;
    params[1] = (int)eventCode;

    paramsOffset = (char *)(&params[2]);
    paddr = (uintptr_t)callbackFn;
    VPP_Memcpy(paramsOffset, &paddr, sizeof(uintptr_t));
    paramsOffset += sizeof(uintptr_t);
    paddr = (uintptr_t)contextParam;
    VPP_Memcpy(paramsOffset, &paddr, sizeof(uintptr_t));
    paramsOffset += sizeof(uintptr_t);

    VPP_SetHdmiCmd (vpp_obj, params);

    VPP_Free(params);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Unregister for HDMI event notifications
 * INPUT: eventCode - Event code for which callback was registered
 *      : callbackFn - Address of callback function
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_UnRegisterHdmiCallback (int handle)
{
    VPP_OBJ *vpp_obj;
    int params[1];

//    VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }
    params[0] = VPP_HDMI_UNREG_CALLBACK;
    VPP_SetHdmiCmd (vpp_obj, params);
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Send HDMI specified auxillary packets
 * INPUT: pktType - Packet Type
 *      : pAuxPkt - Pointer to auxillary packet data
 *      : repeatPkt - Transmit packet once/repeat every VSync
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SendHdmiAuxPkt (int handle, VPP_HDMI_PKT_ID pktType,
                              VPP_HDMI_PKT *pAuxPkt, BOOL repeatPkt)
{
    VPP_OBJ *vpp_obj;
    CMD     cmd;
    int     cpcbID;
    int     params[4];
    int     retCode = MV_VPP_OK;

//    VPPAPI_DBG_INFO(" pktType [%d] \n",  pktType);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    params[0] = VPP_HDMI_SEND_HOST_PKT;
    params[1] = pktType;
    params[2] = repeatPkt;

    if (MV_VPP_OK != (retCode = VPP_BE_HdmiFormatAuxPkt (vpp_obj, TRUE, pktType, pAuxPkt)))
        return retCode;

    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 4, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (retCode);
}

/********************************************************************************
 * FUNCTION: Stop HDMI auxillary packets
 * INPUT: pktType - Packet Type
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_StopHdmiAuxPkt (int handle, VPP_HDMI_PKT_ID pktType)
{
    VPP_OBJ *vpp_obj;
    CMD     cmd;
    int     cpcbID;
    int     params[2];
    int     retCode;

//    VPPAPI_DBG_INFO(" pktType [%d] \n", pktType);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if (MV_VPP_OK != (retCode = VPP_BE_HdmiFormatAuxPkt (vpp_obj, FALSE, pktType, NULL)))
        return retCode;

    params[0] = VPP_HDMI_STOP_HOST_PKT;
    params[1] = pktType;
    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

#ifdef XVYCC_SUPPORT
/********************************************************************************
 * FUNCTION: Enable\disable xvYCC
 * INPUT: Enable - 1: enable; 0:disable
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_xvYCCEnable (int handle, BOOL Enable)
{
    VPP_OBJ *vpp_obj;
    CMD     cmd;
    int     cpcbID;
    int     params[1];
    int     retCode = MV_VPP_OK;

 //   VPPAPI_DBG_INFO(" Enable [%d] \n", Enable);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    params[0] = Enable;

    make_cmd(&cmd, (FUNCPTR)VPP_xvYCCEnable, 1, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
    return (retCode);
}

/********************************************************************************
 * FUNCTION: Set xvYCC mode
 * INPUT: mode - 0: file mode;
                          1: hdmi in mode
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetxvYccMode (int handle, int mode)
{
    VPP_OBJ *vpp_obj;
    int     cpcbID;
    int     retCode = MV_VPP_OK;

 //   VPPAPI_DBG_INFO(" mode [%d] \n", mode);

    if (g_vpp->magic != VPP_MAGIC){
    return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
    return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
    return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
    return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
      /* Output is not connected to any DV in configuration */
    return (MV_VPP_EUNSUPPORT);
    }

    vpp_obj->xvYCC_mode = mode;

    return (retCode);

}
#endif

/*******************************************************************
 * FUNCTION: set oversampling rate for HD/SD VOUT
 * INPUT:  vout - VOUT_HD or VOUT_SD
 *         ovsmpl - oversampling rate
 *                  480/576I/P: 1, 2, 4, 8
 *                  1080P: 1,2
 *                  others: 1, 2, 4
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_SetDacOversampling(int handle, int vout, int ovsmpl)
{
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: set Macrovision for HD/SD VOUT
 * INPUT:  vout - VOUT_HD or VOUT_SD
 *         mode - Macrovision mode
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_SetMacrovision(int handle, int vout, int mode)
{
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: set WSS Data
 * INPUT:  vout - VOUT_HD or VOUT_SD (obsolete, WssData will be applied to both VOUT_HD and VOUT_SD)
 *         WssData - pointer to WSS data struct
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_SetWSSData(int handle, int vout, VPP_WSS_DATA *WssData)
{
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: set CGMS Data
 * INPUT:  vout - VOUT_HD or VOUT_SD
 *         CgmsData - pointer to CGMS data struct
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_SetCGMSData(int handle, int vout, VPP_CGMS_DATA *CgmsData)
{
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: set CC Data
 * INPUT:   CCData - pointer to CC data.
 *         length - CC data length by 16-bit
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_SetCCData(int handle, unsigned short *CCData, int length)
{
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: set TT Data
 * INPUT:   TTData - pointer to TT data.
 *         length - TT data length by byte
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_SetTTData(int handle, unsigned char *TTData, int length, int TTCtrl)
{
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: set Luma Key
 * INPUT:  enable - enable control
 *         low_key - lower key (inclusive)
 *         up_key - upper key (inclusive)
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_SetLumaKey(int handle, int enable, int low_key, int high_key)
{
    CMD cmd;
    VPP_OBJ *vpp_obj;
    int cpcbID;
    int params[3];

    //VPPAPI_DBG_INFO(" enable [%d], low_key [%d], high_key [%d] \n", enable, low_key, high_key);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, CHAN_PIP)) == CPCB_INVALID) {
          /* PIP is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    params[0] = enable;
    params[1] = low_key;
    params[2] = high_key;
    make_cmd(&cmd, (FUNCPTR)VPP_SetLumaKey, 3, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}


/****************************************************************************
 * FUNCTION: set GFX0/PG plane input mode
 * INPUT: planeID - id of the GFX plane
 *        mode - GFX_INPUT_MODE_NORMAL, GFX_INPUT_MODE_MATTE
 * RETURN: MV_VPP_OK - SUCCEED
 ***************************************************************************/
int MV_VPPOBJ_SetGfxInputMode(int handle, int planeID, int mode)
{
    CMD cmd;
    VPP_OBJ *vpp_obj;
    int cpcbID, chanID;
    int params[2];

    //VPPAPI_DBG_INFO(" plane [%d], mode [%d]\n",  planeID, mode);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
          /* PIP is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    params[0] = planeID;
    params[1] = mode;
    make_cmd(&cmd, (FUNCPTR)VPP_SetGfxInputMode, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/****************************************************************************
 * FUNCTION: set csc mode for gfx plane
 * INPUT: planeID - id of the GFX plane
 *        iCscMode - csc mode
 * RETURN: MV_VPP_OK - SUCCEED
 ***************************************************************************/
int MV_VPPOBJ_SetGfxCsc(int handle, int planeID, int iCscMode)
{
    CMD cmd;
    VPP_OBJ *vpp_obj;
    int cpcbID, chanID;
    int params[2];

    //VPPAPI_DBG_INFO(" plane [%d], iCscMode [%d]\n", planeID, iCscMode);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
          /* PIP is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    params[0] = planeID;
    params[1] = iCscMode;
    make_cmd(&cmd, (FUNCPTR)VPP_SetGfxCsc, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}
#ifdef CONFIG_HDMI_CEC_SUPPORT
/********************************************************************************
 * FUNCTION: Configure the features that needs to be enabled for CEC (Feature
 *         : macros are defined in vpp_api_types.h)
 * INPUT:    handle            - VPP object handle
 *      :    featureSupportCfg - Config setting to indicate features to be
 *      :                      - enabled
 * RETURN:   MV_VPP_OK - SUCCEED
 *           MV_EBADPARAM - invalid parameters
 *           MV_EUNCONFIG - VPP not configured
 *           MV_VPP_ENODEV - no device
 *           MV_VPP_EBADCALL - CEC is already enabled, API can be called only
 *                           - if CEC is disabled
 ********************************************************************************/
int MV_VPPOBJ_CecConfigFeatureSupport (int handle, int featureSupportCfg)
{
    VPP_OBJ *vpp_obj;

    //VPPAPI_DBG_INFO(" featureSupportCfg [0x%x]\n", featureSupportCfg);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

#ifndef ENABLE_KILL_PLAYER
    if (vpp_obj->be.enableCec == TRUE) {
        return (MV_VPP_EBADCALL);
    }
#endif

    vpp_obj->be.cecFeatureSupportCfg = featureSupportCfg;
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Returns CEC feature configuration currently set
 * INPUT:    handle             - VPP object handle
 *      :    pFeatureSupportCfg - Pointer to return the feature configuration
 * RETURN:   MV_VPP_OK - SUCCEED
 *           MV_EBADPARAM - invalid parameters
 *           MV_EUNCONFIG - VPP not configured
 *           MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_CecGetFeatureSupportConfig (int handle, int *pFeatureSupportCfg)
{
    VPP_OBJ *vpp_obj;

    //VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (pFeatureSupportCfg == NULL) {
        return (MV_VPP_EBADPARAM);
    }
    *pFeatureSupportCfg = vpp_obj->be.cecFeatureSupportCfg;
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Enable/Disable HDMI CEC feature
 * INPUT:    handle - VPP object handle
 *      :    enable - TRUE, to enable CEC; FALSE, to disable CEC
 * RETURN:   MV_VPP_OK - SUCCEED
 *           MV_EBADPARAM - invalid parameters
 *           MV_EUNCONFIG - VPP not configured
 *           MV_VPP_ENODEV - no device
 *           MV_EUNSUPPORT - channel not connected in configuration
 *           MV_VPP_EBADCALL - channel not connected to DV1
 ********************************************************************************/
int MV_VPPOBJ_CecEnable (int handle, int enable)
{
    VPP_OBJ *vpp_obj;
    int retCode = MV_VPP_OK;

    //VPPAPI_DBG_INFO(" enable [%d]\n", enable);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    if (vpp_obj->be.enableCec == enable) {
        // Nothing has to be done
        return (MV_VPP_OK);
    }

    retCode = VPP_BE_CEC_Enable (vpp_obj, enable);
    return (retCode);
}
/********************************************************************************
 * FUNCTION: Set CEC device physical address - Setting to the invalid address
 *         : would put CEC in idle state
 * INPUT:    handle  - VPP object handle
 *      :    phyAddr - Physical address of the device
 * RETURN:   MV_VPP_OK - SUCCEED
 *           MV_EBADPARAM - invalid parameters
 *           MV_EUNCONFIG - VPP not configured
 *           MV_VPP_ENODEV - no device
 *           MV_EUNSUPPORT - channel not connected in configuration
 *           MV_VPP_EBADCALL - channel not connected to DV1
 ********************************************************************************/
int MV_VPPOBJ_CecSetPhysicalAddr (int handle, int phyAddr)
{
    VPP_OBJ *vpp_obj;
    int retCode = MV_VPP_OK;

    //VPPAPI_DBG_INFO(" phyAddr [0x%x]\n",  phyAddr);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    if (vpp_obj->be.enableCec == FALSE) {
        return (MV_VPP_EBADCALL);
    };

    retCode = VPP_BE_CEC_SetPhysicalAddr (vpp_obj, phyAddr);
    vpp_obj->be.cecPhyAddr =(UINT32)phyAddr;
    return (retCode);
}
/********************************************************************************
 * FUNCTION: Allocate/Unallocate logical address for the given device type
 * INPUT:    handle          - VPP object handle
 *      :    alloc           - Allocate/Unallocate logical address
 *      :    deviceType      - Device type
 *      :    initialPollAddr - Initial polling address; -1, if default is to be
 *      :                      used
 * RETURN:   MV_VPP_OK - SUCCEED
 *           MV_EBADPARAM - invalid parameters
 *           MV_EUNCONFIG - VPP not configured
 *           MV_VPP_ENODEV - no device
 *           MV_EUNSUPPORT - channel not connected in configuration
 *           MV_VPP_EBADCALL - channel not connected to DV1
 ********************************************************************************/
int MV_VPPOBJ_CecAllocLogicalAddr (int handle, BOOL alloc,
                                   VPP_CEC_DEVICE_TYPE deviceType,
                                   int initialPollAddr)
{
    VPP_OBJ *vpp_obj;
    int retCode = MV_VPP_OK;

    //VPPAPI_DBG_INFO(" alloc [%d], deviceType [%d], initialPollAddr [%d]\n",
//                        alloc, deviceType, initialPollAddr);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    if (vpp_obj->be.enableCec == FALSE) {
        return (MV_VPP_EBADCALL);
    };
    retCode = VPP_BE_CEC_AllocLogicalAddr (vpp_obj, alloc, deviceType, initialPollAddr);
    return (retCode);
}
/********************************************************************************
 * FUNCTION: Start transmission of given message on CEC line
 * INPUT:    handle  - VPP object handle
 *      :    pCecMsg   - Pointer to CEC message (Input)
 * RETURN:   MV_VPP_OK - SUCCEED
 *           MV_EBADPARAM - invalid parameters
 *           MV_EUNCONFIG - VPP not configured
 *           MV_VPP_ENODEV - no device
 *           MV_EUNSUPPORT - channel not connected in configuration
 *           MV_VPP_EBADCALL - channel not connected to DV1
 ********************************************************************************/
int MV_VPPOBJ_CecSendMessage (int handle, PVPP_CEC_MSG pCecMsg)
{
    VPP_OBJ *vpp_obj;
    int retCode = MV_VPP_OK;

    //VPPAPI_DBG_INFO(" src [%d], dest [%d], opcode [%d] \n", pCecMsg->srcAddr, pCecMsg->destAddr, pCecMsg->opcode);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    if (vpp_obj->be.enableCec == FALSE) {
        return (MV_VPP_EBADCALL);
    }

    retCode = VPP_BE_CEC_SendMessage (vpp_obj, pCecMsg);
    return (retCode);
}

/********************************************************************************
 * FUNCTION: Register for CEC event notifications
 * INPUT: handle  - VPP object handle
 *      : eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 *      : contextParam - Context parameter for the registering application
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_CecRegisterCallback (int handle, UINT32 eventCode,
                                   MV_VPP_EVENT_CALLBACK callbackFn,
                                   void *contextParam)
{
    VPP_OBJ *vpp_obj;
    int retCode = MV_VPP_OK;

    //VPPAPI_DBG_INFO(" eventCode [%d], callbackFn [0x%x] \n", eventCode, (int)callbackFn);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    retCode = VPP_BE_CEC_RegisterCB (vpp_obj, eventCode, (UINT32)callbackFn, (UINT32)contextParam);
    return (retCode);
}

/********************************************************************************
 * FUNCTION: Unregister for CEC event notifications
 * INPUT: handle  - VPP object handle
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_CecUnregisterCallback (int handle)
{
    VPP_OBJ *vpp_obj;
    int retCode = MV_VPP_OK;

    //VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    retCode = VPP_BE_CEC_UnregisterCB (vpp_obj);
    return (retCode);
}
#endif //CONFIG_HDMI_CEC_SUPPORT

#if defined(PE_3D_BD)
/***************************************************************
 * FUNCTION: set stereoscopic presentation type for the selected planes.
 * INPUT: PlaneMask - the mask to identify the planes which will be set.
 *         PresentTypes - present types for each selected plane. 1: BD mode; 0:BB mode
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_SetSSType(int handle, UINT32 PlaneMask, UINT32 PresentTypes)
{
    CMD cmd;
    int params[2];
    VPP_OBJ *vpp_obj;
    int cpcbID;

    //VPPAPI_DBG_INFO(" PlaneMask [%d], PresentTypes[%d]\n", PlaneMask, PresentTypes);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    /* get a active CPCB */
    for (cpcbID = FIRST_CPCB; cpcbID < MAX_NUM_CPCBS; cpcbID++){
        if (vpp_obj->dv[cpcbID].status != STATUS_INACTIVE)
            break;
    }

    if (cpcbID >= MAX_NUM_CPCBS) {
        return (MV_VPP_OK);
    }

    /* set video plane background window */
    params[0] = PlaneMask;
    params[1] = PresentTypes;
    make_cmd(&cmd, (FUNCPTR)VPP_SetSSType, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/***************************************************************
 * FUNCTION: set plane default offset value for stereoscopic presentation.
 * INPUT: planeID - id of plane
 *         Offset - default offset value and direction. Bit 0-6: Offset value, up to 127 pixels.
 *             Bit 7: offset direction. 0: shift closer to the viewer; 1: shift far away from the viewer.
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_SetSSDefaultOffset(int handle, UINT32 planeID, UINT32 Offset)
{
    CMD cmd;
    int params[2];
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;

    //VPPAPI_DBG_INFO(" plane [%d], offset [%d] \n", planeID, Offset);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if (Offset > 255)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    /* set video plane background window */
    params[0] = planeID;
    params[1] = Offset;
    make_cmd(&cmd, (FUNCPTR)VPP_SetSSDefaultOffset, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/***************************************************************
 * FUNCTION: set plane default offset value for stereoscopic presentation.
 * INPUT: planeID - id of plane except main plane
 *         SeqIDRef - indentify offset id in video offset meta data.
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_SetSSOffsetSeqIDRef(int handle, UINT32 planeID, UINT32 SeqIDRef)
{
    CMD cmd;
    int params[2];
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;

    //VPPAPI_DBG_INFO("plane [%d], SeqIDRef [%d] \n", planeID, SeqIDRef);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES || planeID == PLANE_MAIN)
        return (MV_VPP_EBADPARAM);

    if (SeqIDRef >= 32 && SeqIDRef != 0xFF)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    /* set video plane background window */
    params[0] = planeID;
    params[1] = SeqIDRef;
    make_cmd(&cmd, (FUNCPTR)VPP_SetSSOffsetSeqIDRef, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/***************************************************************
 * FUNCTION: set plane default offset value for stereoscopic presentation.
 * INPUT: planeID - id of plane except main plane
 *         SeqIDRef - indentify how to apply the offset of a plane
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_SetSSOffsetMode(int handle, UINT32 planeID, UINT32 OffsetMode)
{
    CMD cmd;
    int params[2];
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;

    //VPPAPI_DBG_INFO(" plane [%d], OffsetMode [%d] \n", planeID, OffsetMode);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    /* Video plane offset is always zero */
    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES || planeID == PLANE_MAIN)
        return (MV_VPP_EBADPARAM);

    if (OffsetMode < FIRST_SS_OFFSET_MODE || OffsetMode >= MAX_SS_OFFSET_MODE)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    /* set video plane background window */
    params[0] = planeID;
    params[1] = OffsetMode;
    make_cmd(&cmd, (FUNCPTR)VPP_SetSSOffsetMode, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

#endif

int MV_VPPOBJ_RegisterHdmiCpcb0Callback (int handle, void *callbackFn)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }
    vpp_obj->hdmi_cpcb0_dsr_cb = callbackFn;
    return (MV_VPP_OK);
}

int MV_VPPOBJ_UnRegisterHdmiCpcb0Callback (int handle)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }
    vpp_obj->hdmi_cpcb0_dsr_cb = NULL;
    return (MV_VPP_OK);
}

#ifdef PE_3D_FMT_CONVERT
/*******************************************************************
 * FUNCTION: set 3D video convert mode
 * INPUT:  mode - indicate 3D format convert mode (ENUM_SRC_3D_FMT)
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_Set3DConvertMode(int handle, int Mode)
{
    VPP_OBJ *vpp_obj;

    //VPPAPI_DBG_INFO(" Mode [%d]\n",  Mode);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    if ((Mode < FIRST_SRC_3D_FMT) || (Mode >= MAX_SRC_3D_FMT))
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    vpp_obj->src_3D_cvt_mode = Mode;

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: set 3D video invert mode
 * INPUT:  mode - indicate 3D format invert mode
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_Set3DInvertMode(int handle, BOOL Mode)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }
    vpp_obj->src_3D_inv_mode = Mode;
    return (MV_VPP_OK);
}
#endif

#ifdef PE_3D_FMT_CONVERT
/*******************************************************************
 * FUNCTION: Get 3D video convert mode
 * INPUT:  *pMode - indicate 3D format convert mode (ENUM_SRC_3D_FMT)
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_Get3DConvertMode(int handle, int* pMode)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    *pMode = vpp_obj->src_3D_cvt_mode;

    return (MV_VPP_OK);
}
#endif

/********************************************************************************
 * FUNCTION: Read VPP Register Group
 * INPUT: pRegGroup- Register group pointer
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_ReadRegGroup(int handle, VPP_REG_GROUP *pRegGroup)
{
    int index;
    unsigned int uiRegAddr;
    unsigned int uiRegVal;
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle < 0 || handle >= MAX_NUM_OBJECTS) {
        return (MV_VPP_EBADPARAM);
    }

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj) {
        return (MV_VPP_ENODEV);
    }

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if (!pRegGroup) {
        VPPAPI_DBG_ERROR("Invalid pRegGroup, NULL!\n");
        return (MV_VPP_EUNCONFIG);
    }

    if (pRegGroup->uiRegNum > VPP_REG_GROUP_ITEM_MAX) {
        VPPAPI_DBG_ERROR("Invalid reg NUM read!\n");
    }

    for(index = 0; index < pRegGroup->uiRegNum; index++) {
        uiRegAddr = pRegGroup->uiRegGroup[(index << 1) + 1];
        if (!((uiRegAddr >= (VPP_BASE_ADDR)
             && uiRegAddr < (VPP_BASE_ADDR + AVIO_MEMMAP_VPP_REG_SIZE)) ||
#ifdef DOLPHIN_HDMI_COMMENT
             (uiRegAddr >= (VPP_HDMITX_BASE_ADDR)
             && uiRegAddr < (VPP_HDMITX_BASE_ADDR + AVIO_MEMMAP_AVIO_HDMITX_REG_SIZE)) ||
#endif
             (uiRegAddr >= MEMMAP_OVP_REG_BASE
              && uiRegAddr < (MEMMAP_OVP_REG_BASE + MEMMAP_OVP_REG_SIZE)) ||
             (uiRegAddr >= VPP_MP_BASE_ADDR
              && uiRegAddr < (VPP_MP_BASE_ADDR + AVIO_MEMMAP_VPP_MP_REG_SIZE)) ||
             (uiRegAddr >= VPP_GFX_VOP_BASE_ADDR
              && uiRegAddr < (VPP_GFX_VOP_BASE_ADDR + AVIO_MEMMAP_VPP_GFX_VOP_REG_SIZE)))
                )
        {
            VPPAPI_DBG_ERROR("Invalid RegAddr request: 0x%x!\n", uiRegAddr);
            VPPAPI_DBG_ERROR("Invalid RegAddr request: 0x%x!\n", uiRegAddr);
            return (MV_VPP_EBADPARAM);
        } else {
            VPP_REG_WORD32_READ(uiRegAddr, &uiRegVal);
            pRegGroup->uiRegGroup[(index << 1)] = uiRegVal;
        }
    }
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Write VPP Register Group
 * INPUT: pRegGroup- Register group pointer
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/

int MV_VPPOBJ_WriteRegGroup(int handle, VPP_REG_GROUP *pRegGroup)
{
    CMD cmd;
    int params[MAX_NUM_PARAMS];
    VPP_OBJ *vpp_obj;
    int cpcbID;

    if (!pRegGroup) {
        return (MV_VPP_EBADPARAM);
    }
    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle < 0 || handle >= MAX_NUM_OBJECTS) {
        return (MV_VPP_EBADPARAM);
    }
    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj) {
        return (MV_VPP_ENODEV);
    }

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, CHAN_MAIN)) == CPCB_INVALID) {
          /*channel is not connected to any DV in configuration*/
        return (MV_VPP_EUNSUPPORT);
    }

    VPP_Memcpy(params, pRegGroup, sizeof(*pRegGroup));

    make_cmd(&cmd, (FUNCPTR)VPP_WriteRegGroup, MAX_NUM_PARAMS, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/***************************************************************
 * FUNCTION: Dump all the HDMI TX Registers
 * INPUT:
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/

int MV_VPPOBJ_DumpHdmiRegisters(int handle, VPP_HDMITX_REG_DUMP *hdmiRegDump)
{
    /* TODO: add mapping if this API is still required */
    return (MV_VPP_EUNSUPPORT);
}

/*******************************************************************
 * FUNCTION: get the output frame rate
 * INPUT: cpcbID
 *        *pts_per_cnt_4 - pointer to time interval in term of PTS for every 4 frames
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *******************************************************************/
int MV_VPPOBJ_GetCPCBOutputFrameRate(int handle, int cpcbID, int *pts_per_cnt_4)
{
    int resID;
    VPP_OBJ *vpp_obj;
    RESOLUTION_INFO  *pResinfo;

    if (g_vpp->magic != VPP_MAGIC) {
        return (MV_VPP_EUNCONFIG);
    }
    if (handle < 0 || handle >= MAX_NUM_OBJECTS) {
        return (MV_VPP_EBADPARAM);
    }
    if (cpcbID<FIRST_CPCB || cpcbID>=MAX_NUM_CPCBS) {
        return (MV_VPP_EBADPARAM);
    }

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj) {
        return (MV_VPP_ENODEV);
    }
    if (vpp_obj->status == STATUS_INACTIVE) {
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }
    if (!pts_per_cnt_4) {
        return (MV_VPP_EBADPARAM);
    }

    resID = VPP_GetResId(cpcbID);
    pResinfo = VPP_GetResInfo(resID);
    *pts_per_cnt_4 = pResinfo->pts_per_cnt_4;
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Register callback for resolution change notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 *      : contextParam - Context parameter for the registering application
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_RegisterResCallback (int handle, VPP_RES_CALLBACK *pResCallback)
{
    VPP_OBJ *vpp_obj;

    if (!pResCallback) {
        return (MV_VPP_EBADPARAM);
    }
    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS) {
        return (MV_VPP_EBADPARAM);
    }
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj) {
        return (MV_VPP_ENODEV);
    }
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    vpp_obj->ResCallback.ResNotifier =  pResCallback->ResNotifier;
    vpp_obj->ResCallback.uiUserPara =  pResCallback->uiUserPara;
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Unregister callback for resolution event notifications
 * INPUT: eventCode - Event code for which callback was registered
 *      : callbackFn - Address of callback function
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_UnRegisterResCallback (int handle)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj) {
        return (MV_VPP_ENODEV);
    }
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    vpp_obj->ResCallback.ResNotifier =  NULL;
    vpp_obj->ResCallback.uiUserPara = (uintptr_t)NULL;
    return (MV_VPP_OK);
}

/****************** Common VPP object Function Calls **********************/
/********************************************************************************
 * FUNCTION: Configure the DDD block parameter
 * INPUT: handle  - VPP object handle
 *      : strength - 3D strength setting ( 0x00 - 0xFF )
 *      : focal_point - 2D level setting ( 0x00 - 0xFF )
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetDddParams(int handle, int strength, int focal_point)
{
    //TBD : Non-TZ impplemenetation
    return MV_VPP_EUNSUPPORT;
}

/********************************************************************************
 * FUNCTION: Get the DDD block parameter
 * INPUT: handle  - VPP object handle
 * OUTPUT: *pDdd_format - current DDD mode/format
 *       : *pStrength - 3D strength setting ( 0x00 - 0xFF )
 *       : *pFocal_point - 2D level setting ( 0x00 - 0xFF )
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetDddParams(int handle, int *pDdd_format, int *pStrength, int *pFocal_point)
{
    //TBD : Non-TZ impplemenetation
    return MV_VPP_EUNSUPPORT;
}

/********************************************************************************
 * FUNCTION: Get the DDD block parameter
 * INPUT: handle  - VPP object handle
 *      : ddd_format - DDD mode/format, valid only when enableFlag=1
 *      : enableFlag - enable(1) or disable(0) DDD modeule.
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetDddCtrl(int handle, int ddd_format, int enableFlag)
{
    //TBD : Non-TZ impplemenetation
    return MV_VPP_EUNSUPPORT;
}

/********************************************************************************
 * FUNCTION: Check DDD block is enabled or disabled
 * INPUT: handle  - VPP object handle
 * RETURN: MV_VPP_OK - SUCCEED, DDD block is enabled
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 *         MV_VPP_EHARDWAREBUSY - DDD block is disabled
 ********************************************************************************/
int MV_VPPOBJ_IsDddEnabled(int handle)
{
    //TBD : Non-TZ impplemenetation
    return MV_VPP_EUNSUPPORT;
}

int MV_VPPOBJ_GetBpp(int handle, int plane, int bitDepth, int srcFmt) {
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    return VPP_OVL_GetBpp(vpp_obj, plane, bitDepth == INPUT_BIT_DEPTH_8BIT?8:10, srcFmt);
}

/******************************************************************************
 * FUNCTION: Write Scalar coefficient
 * INPUT: planeID - id of a video/grahpics plane
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_WriteSclCoeff(int handle, int planeID)
{
    CMD cmd;
    int params[1];
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;

    VPPAPI_DBG_INFO("Write Scl Coeff plane [%d],  \n", planeID);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);



    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID)
        return (MV_VPP_EUNSUPPORT);

    params[0] = planeID;
    make_cmd(&cmd, (FUNCPTR)VPP_WriteSclCoeff, 1, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}


/******************************************************************************
 * FUNCTION: Dump Scalar coefficient
 * INPUT: planeID - id of a video/grahpics plane
 *        *win - pointer to a vpp window struct
 *        *attr - pointer to a vpp window attribute struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_DumpSclCoeff(int handle, int planeID, int enPrint)
{
    CMD cmd;
    int params[3];
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID = FIRST_CPCB;
    static INT16 hlut[17][8], vlut[17][8];

    VPPAPI_DBG_INFO("plane [%d],  \n", planeID);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID<FIRST_PLANE || planeID>=MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);



    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if( 0 == enPrint)
    {
        if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID)
            return (MV_VPP_EUNSUPPORT);

        params[0] = planeID;
        params[1] = (int)hlut;
        params[2] = (int)vlut;
        make_cmd(&cmd, (FUNCPTR)VPP_ReadSclCoeff, 3, params);

        MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

        ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
        /* commit commands */
        cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

        MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
    }

    else
    {
        FRC_SCL_PrintSclCoeffShadow(vpp_obj, planeID, hlut, vlut);
    }
    return (MV_VPP_OK);
}
/******************************************************************************
 * FUNCTION: Write the val to addr on interrupt
 * INPUT: addr - address to which needs to be write
 *        val  - value to be written
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_RegWrite(int handle, unsigned int addr, unsigned int val)
{
    CMD cmd;
    int params[3];
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;

    VPPAPI_DBG_INFO("addr %x , val %x  \n", addr, val);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if ((chanID = CHAN_OF_PLANE(vpp_obj, PLANE_MAIN)) == CHAN_INVALID){

        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    params[0] = addr;
    params[1] = val;

    make_cmd(&cmd, (FUNCPTR)VPP_RegWrite, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return 0;

}

/******************************************************************************
 * FUNCTION: Retrieve the current clock/SRAM gating signature of the AVIO/VPP
 * INPUT: VPP_CLKGATE_OPT, Various bit field option
 *   VPP_CLKGATE_OPT_PRINT      - Enable debug log
 *   VPP_CLKGATE_OPT_RECORD_RAW - Enable storing raw bit from the register into corresponding bit of IP block in signatrue
                Default behaiour: Enabling bit correspoing to IP block in signatrue only if clock gated/disabled
 * OUTPUT: pSignatureCount, The number of IP blocks which are clock gated/disabled
 *         pSignature, 128 bit signature (1 bit per IP block), ensure it points to array of 4 integers
 *
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_GetClkGateSignature(int handle, UINT32 *pSignatureCount,
        VPP_CLK_GATE_SIGNATURE *pSignature, VPP_CLKGATE_OPT options)
{
    VPP_OBJ *vpp_obj;

    VPPAPI_DBG_INFO(" %s:%d = %x \n", __FUNCTION__, __LINE__, options);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    VPP_ClkGate_GetSignature(vpp_obj, pSignatureCount, pSignature, options);

    return (MV_VPP_OK);

}

/******************************************************************************
 * FUNCTION: Retrieve the current clock/SRAM gating signature of the AVIO/VPP
 * INPUT: VPP_CLKGATE_OPT, Various bit field option
 *   VPP_CLKGATE_OPT_PRINT      - Enable debug log
 *   VPP_CLKGATE_OPT_RECORD_RAW - Enable storing raw bit from the register into corresponding bit of IP block in signatrue
                Default behaiour: Enabling bit correspoing to IP block in signatrue only if clock gated/disabled
 *         pSignatureValue, 128 bit signature value(1 bit per IP block), ensure it points to array of 4 integers
 *         pSignaturemASK, 128 bit signature mask(1 bit per IP block), ensure it points to array of 4 integers
 *
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_SetClkGateSignature(int handle, VPP_CLK_GATE_SIGNATURE *pSignatureValue,
        VPP_CLK_GATE_SIGNATURE *pSignatureMask, VPP_CLKGATE_OPT options)
{
    VPP_OBJ *vpp_obj;

    VPPAPI_DBG_INFO(" %s:%d = %x \n", __FUNCTION__, __LINE__, options);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);


    VPP_ClkGate_SetSignature(vpp_obj, pSignatureValue, pSignatureMask, options);

    return (MV_VPP_OK);

}


/******************************************************************************
 * FUNCTION: Enable/disable dynamic clock gating if IPs not used or softbypass mode
 * INPUT: enable - enable/disable value
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_enClkGate(int handle, int enable)
{
    CMD cmd;
    int params[3];
    VPP_OBJ *vpp_obj;
    int cpcbID;

    VPPAPI_DBG_INFO(" enClkGate = %d \n", enable);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    /* VPP module is not configured, then immediately initialize and return*/
    if (vpp_obj->status != STATUS_ACTIVE) {
        vpp_obj->enClkGate = enable;
        return (MV_VPP_ENODEV);
    }

    cpcbID = CPCB_1;
    params[0] = enable;

    make_cmd(&cmd, (FUNCPTR)VPP_enClkGate, 1, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return 0;

}


int MV_VPPOBJ_SetThdrVitmParams(int handle, int iCmdId, int iValue)
{
    return 0;
}


/********************************************************************************
 * FUNCTION: Load SCL Customized Coeffs into cust_scl_coeffs table
 * INPUT:
 *              sclUnit - scalar unitNum
 *              coeffID - scalar coeffs mode
 *              pCoeffsTab - customized coeffs talbe's pointer
 *
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_PrepareSCLCustCoeffs(int handle, int sclUnit, int coeffID, UINT16* pCoeffsTab)
{
    VPP_OBJ *vpp_obj;
    int sclLayer = 0;

    VPPAPI_DBG_INFO(" coeffID[%d] \n",  coeffID);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if(sclUnit<VPP_FRC_SCL_MAIN || sclUnit> VPP_FRC_SCL_MAX || coeffID<FIRST_SCALAR_COEFF || coeffID>MAX_NUM_PREDEFINED_COEFFS || pCoeffsTab==NULL)
        return (MV_VPP_EBADPARAM);

    /* Calculating offset of scl_coeffs table */
    if (sclUnit == VPP_FRC_SCL_MAIN
#ifdef ENABLE_PIP_GFX0_PATH //TODO: Use static table to disable the plane if(vpp_plane_map[PlaneId].is_valid)
            || sclUnit == VPP_FRC_SCL_PIP
#endif
        )
        sclLayer = 0;
    else if (sclUnit == VPP_FRC_SCL_OSD || sclUnit == VPP_FRC_SCL_VIP_Y)
        sclLayer = 1;
    else if (sclUnit == VPP_FRC_SCL_VIP_UV)
        sclLayer = 2;
    else
        return (MV_VPP_EBADPARAM);

    VPP_Memcpy(vpp_obj->cust_scl_coeffs[sclLayer][coeffID], pCoeffsTab, VPP_FRC_SCL_COEFF_TAB_SIZE);

    vpp_obj->cust_sc_bPrepared[sclLayer][coeffID] = TRUE;
    vpp_obj->cust_scl_id_flag[sclUnit][coeffID] = TRUE;

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Load SCL Customized Coeffs
 * INPUT:
 *              coeffID - scalar coeffs mode
 *              hv      - indicating it is for horizontal(0) or vertical(1) scaler
 *              UnitNum - FRC SCL unit number the coeff is for
 *
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_LoadSCLCustCoeffs(int handle, int coeffID, int hv, INT UnitNum)
{
    int params[4];
    VPP_OBJ *vpp_obj;
    int cpcbID;

    VPPAPI_DBG_INFO("enter, coeffID: %d, hv: %d, UnitNum: %d\n", coeffID, hv, UnitNum);

    CMD_CHECK(handle)

    if(UnitNum < VPP_FRC_SCL_MAIN || UnitNum > VPP_FRC_SCL_MAX)
        return (MV_VPP_EBADPARAM);

    params[0]=VPP_CMD_SCL_LOAD_CUST_COEFFS;
    params[1]=coeffID;
    params[2]=hv;
    params[3]=UnitNum;

    VPP_SetSclCoeffs(vpp_obj, params);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Reload SCL Default Coeffs
 * INPUT:
 *              coeffID - scalar coeffs mode
 *              hv      - indicating it is for horizontal(0) or vertical(1) scaler
 *              UnitNum - FRC SCL unit number the coeff is for
 *
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/

int MV_VPPOBJ_ReloadSCLDefCoeffs(int handle, int coeffID, int hv, INT UnitNum)
{
    int params[4];
    VPP_OBJ *vpp_obj;
    int cpcbID;

    CMD_CHECK(handle)

    if(UnitNum < VPP_FRC_SCL_MAIN || UnitNum > VPP_FRC_SCL_MAX)
        return (MV_VPP_EBADPARAM);

    params[0]=VPP_CMD_SCL_RELOAD_DEF_COEFFS;
    params[1]=coeffID;
    params[2]=hv;
    params[3]=UnitNum;

    //CMD_COMMIT(4, VPP_SetSclEvalCommand)
    VPP_SetSclCoeffs(vpp_obj, params);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Get H/V SCL Coeff Mode
 * INPUT:
 *              UnitNum - FRC SCL unit index
 * OUTPUT:
 *              pHSCLCoeffMode - H SCL Coeff Mode placeholder
 *              pVSCLCoeffMode - V SCL Coeff Mode placeholder
 *
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ********************************************************************************/
int MV_VPPOBJ_GetSCLCoeffModes(int handle, INT UnitNum, INT* pHSCLCoeffMode, INT* pVSCLCoeffMode)
{
    VPP_OBJ *vpp_obj;
    int cpcbID;
    CMD_CHECK(handle);

        if(VPP_FRC_SCL_MAIN > UnitNum || UnitNum >= VPP_FRC_SCL_MAX)
            return (MV_VPP_EBADPARAM);

    *pHSCLCoeffMode = vpp_obj->hscl_coeff[UnitNum];
    *pVSCLCoeffMode = vpp_obj->vscl_coeff[UnitNum];

    return (MV_VPP_OK);
}

/***********************************************************
 * FUNCTION: get video frame info
 * PARAMS: handle - the vpp handle
 *         PlaneId - plane for video frame capture
 *         pvinfo - return video infor
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EUNCONFIG - not initialized
 *         MV_VPP_ENODEV - no device
 *         MV_VPP_ENOMEM - no memory
 *         MV_VPP_EBADCALL - not supported
 *         MV_VPP_EBADPARAM - wrong input params
 **********************************************************/
int MV_VPPOBJ_GetVideoInfo(int handle, UINT32 planeID, VPP_PLANE_VIDEO_INFO *pvinfo)
{
    VPP_OBJ *vpp_obj;
    PLANE *plane;
    VBUF_INFO *pinfo;

    VPPAPI_DBG_INFO(" enter!\n");
    if (g_vpp->magic != VPP_MAGIC){
         VPPAPI_DBG_INFO(" MV_VPP_EUNCONFIG!\n");
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS || planeID >= MAX_NUM_PLANES || !pvinfo){
        VPPAPI_DBG_INFO(" MV_VPP_EBADPARAM!\n");
        return (MV_VPP_EBADPARAM);
    }
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }
    plane = &(vpp_obj->plane[planeID]);

    if (plane->status == STATUS_DISP_VIDEO){
        pinfo = (VBUF_INFO *)plane->curr_frame;
    }
    else if (plane->status == STATUS_DISP_LOGO){
        pinfo = (VBUF_INFO *)plane->curr_still_picture;
    }
    else if (plane->status == STATUS_DISP_PATGEN){
        pinfo = (VBUF_INFO *)plane->dummy_frame;
    }
    else{
        pinfo = NULL;
    }

    if(pinfo != NULL){

        //update frame info with video buffer info
        pvinfo->uiWidth = pinfo->m_active_width;
        if((pinfo->m_is_frame_seq)
            ||(IS_GFX_SRC_FORMAT(pinfo->m_srcfmt)))
            pvinfo->uiHeight = pinfo->m_active_height;
        else
            pvinfo->uiHeight = pinfo->m_active_height/2;
        pvinfo->ColorFormat = pinfo->m_srcfmt;
        pvinfo->ColorPrimary = pinfo->m_colorprimaries;
        pvinfo->ColorOrder = pinfo->m_order;
        switch(pinfo->m_bits_per_pixel)
        {
            case 8:
                pvinfo->ColorDepth = SRC_DEPTH_8BIT;
                break;
            case 10:
                pvinfo->ColorDepth = SRC_DEPTH_10BIT;
                break;
            case 12:
                pvinfo->ColorDepth = SRC_DEPTH_12BIT;
                break;
            case 16:
                pvinfo->ColorDepth = SRC_DEPTH_16BIT;
                break;
            default:
                pvinfo->ColorDepth = SRC_DEPTH_8BIT;
                break;
        }
        pvinfo->Video3DFormat = pinfo->m_3D_source_fmt;
        if(pinfo->m_frame_rate_den != 0)
            pvinfo->FrameRate = (UINT8)((pinfo->m_frame_rate_num+pinfo->m_frame_rate_den/2)
                                        /pinfo->m_frame_rate_den);
        else
            pvinfo->FrameRate = 60;

        pvinfo->uiMaxCapWidth = pvinfo->uiWidth;
        pvinfo->uiMaxCapHeight = pvinfo->uiHeight;
    }
    else{
        memset(pvinfo, 0, sizeof(VPP_PLANE_VIDEO_INFO));
    }
    return (MV_VPP_OK);
}

#ifdef VPP_ENABLE_FRAME_CAPTURE
/***********************************************************
 * FUNCTION: capture video frame
 * PARAMS: handle - the vpp handle
 *         pCapFrmSetting - setting for video frame capture
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EUNCONFIG - not initialized
 *         MV_VPP_ENODEV - no device
 *         MV_VPP_ENOMEM - no memory
 *         MV_VPP_EBADCALL - not supported
 *         MV_VPP_EBADPARAM - wrong input params
 **********************************************************/
int MV_VPPOBJ_CaptureVideoFrame(int handle, VPP_CAPTURE_FRAME_SETTING *pCapFrmSetting)
{
    VPP_OBJ *vpp_obj;
    PLANE *plane;
    CAPTURE_VID_FRAME *pCaptureVidFrm;
    VBUF_INFO *pinfo;
    UINT32 MaxWidth,MaxHeigth;

    VPPAPI_DBG_INFO(" enter!\n");
    if (g_vpp->magic != VPP_MAGIC){
         VPPAPI_DBG_INFO(" MV_VPP_EUNCONFIG!\n");
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS
                 || !pCapFrmSetting || !pCapFrmSetting->pFrmBuf
                 || pCapFrmSetting->PlaneId >= MAX_NUM_PLANES){
        VPPAPI_DBG_INFO(" MV_VPP_EBADPARAM!\n");
        return (MV_VPP_EBADPARAM);
    }
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    plane = &(vpp_obj->plane[pCapFrmSetting->PlaneId]);
    pCaptureVidFrm = &plane->CaptureVidFrm;

    if (plane->status == STATUS_DISP_VIDEO){
        pinfo = (VBUF_INFO *)plane->curr_frame;
    }
    else if (plane->status == STATUS_DISP_LOGO){
        pinfo = (VBUF_INFO *)plane->curr_still_picture;
    }
    else if (plane->status == STATUS_DISP_PATGEN){
        pinfo = (VBUF_INFO *)plane->dummy_frame;
    }
    else{
        pinfo = NULL;
    }

    if(pinfo == NULL){
        return (MV_VPP_EBADPARAM);
    }
    MaxWidth = pinfo->m_active_width;
    if((pinfo->m_is_frame_seq)
        ||(IS_GFX_SRC_FORMAT(pinfo->m_srcfmt))){
        MaxHeigth = pinfo->m_active_height;
    }
    else{
        MaxHeigth = pinfo->m_active_height/2;
    }
    if((pCapFrmSetting->CapWin.width > MaxWidth)
        || (pCapFrmSetting->CapWin.height > MaxHeigth)
        || ((pCapFrmSetting->CapWin.x
             +pCapFrmSetting->CapWin.width) > MaxWidth)
        || ((pCapFrmSetting->CapWin.y
             +pCapFrmSetting->CapWin.height) > MaxHeigth)){
        return (MV_VPP_EBADPARAM);
    }

    if(true != pCaptureVidFrm->gCapStartFlag){
        memcpy(&pCaptureVidFrm->gCapFrameSetting, pCapFrmSetting, sizeof(VPP_CAPTURE_FRAME_SETTING));
        pCaptureVidFrm->gCapStartFlag = true;
        if(plane->status != STATUS_DISP_VIDEO){
            MV_OSAL_Sem_Post(vpp_obj->vpp_FrameCapture_sem);
        }
    }else{
        VPPAPI_DBG_INFO(" Previous Capture is running!\n");
        return (MV_VPP_EUNSUPPORT);
    }
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Register for video frame capture call back
 * INPUT: callbackFn - call back function
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_RegisterCaptureVideoCallback (int handle, VPP_CAPTURE_CALLBACK callbackFn)
{
    VPP_OBJ *vpp_obj;
    PLANE *plane;
    CAPTURE_VID_FRAME *pCaptureVidFrm;
    VPPAPI_DBG_INFO(" enter!\n");
    UINT8 i;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    for(i=FIRST_PLANE; i<MAX_NUM_PLANES; i++)
    {
        plane = &(vpp_obj->plane[i]);
        pCaptureVidFrm = &plane->CaptureVidFrm;

        pCaptureVidFrm->appCbFn = callbackFn;
    }
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: frame capture task
 * INPUT: None
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_FrameCaptureTask(int handle)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    VPP_CaptureFrameTask(vpp_obj);
    return (MV_VPP_OK);
}
#endif

int MV_VPPOBJ_StartHdcp14(int handle, int index)
{
    VPP_OBJ *vpp_obj;

    if (vpp.magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = vpp.vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    BE_HDCP_StartHdcp(vpp_obj, (UINT32 *)index, TRUE);
    return (MV_VPP_OK);
}


/********************************************************************************
 * FUNCTION: Configure Inverse Scan for the requested Plane
 * INPUT: handle  - VPP object handle
 *      : planeID - Plane for which inverse scan need to be enabled
 *      : uiMode  - Mode of scan
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/

int MV_VPPOBJ_ConfigInverseScan(int handle, int planeID,  unsigned int uiMode)
{
#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
    CMD cmd;
    int params[3];
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;

    if (g_vpp->magic != VPP_MAGIC)
    {
        return (MV_VPP_EUNCONFIG);
    }

    if (handle < 0 || handle >= MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID < FIRST_PLANE || planeID >= MAX_NUM_PLANES)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE)
    {
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }
    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if (VPP_GetResId(cpcbID) == RES_INVALID) {
        /* CPCB resolution need to be set first */
        return (MV_VPP_EBADCALL);
    }

    if(uiMode < SCAN_INVERSE_MIN || uiMode > SCAN_INVERSE_MAX)
        return (MV_VPP_EBADPARAM);

    params[0] = planeID;
    params[1] = uiMode;

    make_cmd(&cmd, (FUNCPTR)VPP_ConfigInverseScan, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif
    return (MV_VPP_OK);
}

int MV_VPPOBJ_SetRotationAngle(int handle, INT PlaneId, INT RotationAngle)
{
#ifdef CONFIG_AMP_VPP_ROTATION

    CMD_BEGIN(3);
    int chanID;
    CMD_CHECK(handle);


    if((PLANE_MAIN != PlaneId)
#ifdef ENABLE_PIP_GFX0_PATH
            && (PLANE_PIP != PlaneId)
#endif
#ifdef ENABLE_EL_PIPE_PATH
            && (PLANE_OVP_EL != PlaneId)
#endif
    )
    {
        return (MV_VPP_EBADPARAM);
    }

    if(RotationAngle < ROTATION_MIN || RotationAngle >= ROTATION_MAX)
    {
        return (MV_VPP_EBADPARAM);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, PlaneId)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if(vpp_obj->plane[PlaneId].uchRotationAngle == RotationAngle)
    {
        return MV_VPP_OK;
    }

    params[0] = PlaneId;
    params[1] = RotationAngle;

    make_cmd(&cmd, (FUNCPTR)VPP_SetRotationAngle, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif
    return MV_VPP_OK;
}
int MV_VPPOBJ_GetRotationAngle(int handle, INT PlaneId, INT *pRotationAngle)
{
#ifdef CONFIG_AMP_VPP_ROTATION
    VPP_OBJ *vpp_obj;
    int cpcbID;

    CMD_CHECK(handle);

    vpp_obj = g_vpp->vpp_objs[handle];

    if(NULL == pRotationAngle)
    {
         return (MV_VPP_EBADPARAM);
    }
    //TODO: Use static table to disable the plane if(vpp_plane_map[PlaneId].is_valid)
    if((PLANE_MAIN != PlaneId)
#ifdef ENABLE_PIP_GFX0_PATH
            && (PLANE_PIP != PlaneId)
#endif
#ifdef ENABLE_EL_PIPE_PATH
            && (PLANE_OVP_EL != PlaneId)
#endif
    )
    {
        return (MV_VPP_EBADPARAM);
    }

    *pRotationAngle = vpp_obj->plane[PlaneId].uchRotationAngle;

#endif
    return MV_VPP_OK;
}

int MV_VPPOBJ_DV_EnableDolbyDump(int handle, int enable)
{
    CMD cmd;
    int params[3];
    VPP_OBJ *vpp_obj;
    int cpcbID;

    VPPAPI_DBG_INFO(" EnableDolbyDump = %d \n", enable);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    cpcbID = CPCB_1;
    params[0] = enable;

    make_cmd(&cmd, (FUNCPTR)VPP_EnableDvDump, 1, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return 0;

}

int MV_VPPOBJ_DV_InitFrameDump(int handle, INT Path, INT Enable, VPP_DV_FRAME_DUMP *pFrameDumpConfig)
{
    CMD_BEGIN(3);
    CMD_CHECK(handle);

    if(Enable < 0 || Enable > 1)
    {
        return (MV_VPP_EBADPARAM);
    }

    params[0] = Path;
    params[1] = Enable;

#ifdef CONFIG_AMP_DOLBY_DUMP
    VPP_DV_FrameDump_Init(vpp_obj, pFrameDumpConfig);
#endif

    make_cmd(&cmd, (FUNCPTR)VPP_SetDvDataDump, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
    return 0;
}

int MV_VPPOBJ_DV_GetFrameDumpInfo(int handle, VPP_DV_FRAME_DUMP *pFrameDumpConfig)
{
    VPP_OBJ *vpp_obj;
    int cpcbID;

    CMD_CHECK(handle);

#ifdef CONFIG_AMP_DOLBY_DUMP
    VPP_DV_FrameDump_GetConfig(vpp_obj, pFrameDumpConfig);
#endif

   return 0;
}

int MV_VPPOBJ_SetDVDataDump(int handle, INT Path, INT Enable)
{
    CMD_BEGIN(3);
    CMD_CHECK(handle);

    if(Enable < 0 || Enable > 1)
    {
        return (MV_VPP_EBADPARAM);
    }

    params[0] = Path;
    params[1] = Enable;


    make_cmd(&cmd, (FUNCPTR)VPP_SetDvDataDump, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
    return 0;
}

int MV_VPPOBJ_ClearDhubCommand(int handle, INT cpcb)
{
    CMD_BEGIN(3);
    CMD_CHECK(handle);

    if(cpcb > MAX_NUM_CPCBS)
    {
        return (MV_VPP_EBADPARAM);
    }

    params[0] = cpcb;

    make_cmd(&cmd, (FUNCPTR)VPP_ClearDhubCommand, 1, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcb].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcb].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcb].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcb].cmdq_lock);
    return 0;

}

int MV_VPPOBJ_EnableCrop(int handle, int planeID,
                         int rightcropy, int bottomcropy,
                         int rightcropc, int bottomcropc)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    /* This is for debug purpose */
    /* Calculation of these parameters will be done in ISR */
    vpp_obj->plane[planeID].m_rightpad_y = rightcropy;
    vpp_obj->plane[planeID].m_bottompad_y = bottomcropy;
    vpp_obj->plane[planeID].m_rightpad_c = rightcropc;
    vpp_obj->plane[planeID].m_bottompad_c = bottomcropc;

    return 0;
}

/*******************************************************************
 * FUNCTION: display a frame for a plane
 * INPUT: planeID - id of the plane
 *        *frame - frame descriptor
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
unsigned int MV_VPPOBJ_GetFrameAddress(int handle, int planeID)
{
    VPP_OBJ *vpp_obj;

    unsigned int address;

    vpp_obj = g_vpp->vpp_objs[handle];

    address = (unsigned int)vpp_obj->plane[planeID].offline_buff_descrs[0].addr;

    return(address);
}

/********************************************************************************
 * FUNCTION: Set Hdmi Tx Phy settings
 * INPUT:  Phy setting values, Size
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiPhy(int handle, unsigned int * phySetting, unsigned int PhyBufSize)
{
    /* TODO: port to dolphin if required */
    return MV_VPP_OK;
}

int MV_VPPOBJ_SetDcmPictCtrl(int handle, int PlaneId, int Hue, int Saturation, int Lightness)
{
#ifdef CONFIG_AMP_DCM
    VPP_OBJ *vpp_obj;
    int cpcbID;
    int chanID;
    CMD cmd;
    int params[4];

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if ((chanID = CHAN_OF_PLANE(vpp_obj, PLANE_MAIN)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }
    params[0] = PlaneId;
    params[1] = Hue;
    params[2] = Saturation;
    params[3] = Lightness;

    make_cmd(&cmd, (FUNCPTR)VPP_SetDcmPQ, 4, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);
    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif
    return 0;
}

int MV_VPPOBJ_ConfigAoiWin(int handle, int planeID, int Enable, VPP_WIN *win)
{
#ifdef CONFIG_AMP_DCM
    CMD cmd;
    int params[6];
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (planeID != FIRST_PLANE)
        return (MV_VPP_EBADPARAM);

    if (!win)
        return (MV_VPP_EBADPARAM);
    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    /* set video plane background window */
    params[0] = planeID;
    params[1] = Enable;
    params[2] = win->x;
    params[3] = win->y;
    params[4] = win->width;
    params[5] = win->height;
    make_cmd(&cmd, (FUNCPTR)VPP_SetDcmAoiWin, 6, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

#endif
    return (MV_VPP_OK);
}

int MV_VPPOBJ_SetOfflineParams(int handle,int iPlaneId,int bpp,int srcfmt)
{

    CMD cmd;
    int params[6];
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID = CPCB_1;

    if (vpp.magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = vpp.vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }


    if ((chanID = CHAN_OF_PLANE(vpp_obj, iPlaneId)) == CHAN_INVALID)
    {
        /* plane is not connected to any channel in configuration */
        VPPAPI_DBG_ERROR("Invalid Plane ID\n");
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID)
    {
        VPPAPI_DBG_ERROR("Invalid chanID derived from plane ID ID\n");
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }


    /* set video plane background window */
    params[0] = iPlaneId;
    params[1] = bpp;
    params[2] = srcfmt;

    make_cmd(&cmd, (FUNCPTR)VPP_SetOfflineParams, 3, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);

}

#ifndef CONFIG_VPP_DISABLE_CMU
/***************************************************************
 * FUNCTION: enable or disable flesh tone taper
 * INPUT: chanID - id of the channel
 *        Enable - 1: enble  0:disable
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_EnableFTTaper(int handle, int chanID, int Enable)
{
    CMD cmd;
    int params[3];
    VPP_OBJ *vpp_obj;
    int cpcbID;

    VPPAPI_DBG_INFO(" chanID [%d], Enable [%d] \n", chanID, Enable);

    //TODO: Use static table to disable the plane if(vpp_plane_map[PlaneId].is_valid)
    if(chanID != CHAN_MAIN
#ifdef ENABLE_PIP_GFX0_PATH
            &&  chanID != CHAN_PIP
#endif
        )
        return (MV_VPP_EBADPARAM);

    if (vpp.magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = vpp.vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
          /*channel is not connected to any DV in configuration*/
        return (MV_VPP_EUNSUPPORT);
    }

    /*only CPCB0 has CMU*/
    if(cpcbID != CPCB_1)
        return MV_VPP_EBADCALL;

    /*send command for hardware transaction*/
    params[0] = chanID;
    params[1] = VPP_CMU_SET_FT_TAPER;
    params[2] = Enable;

    make_cmd(&cmd, (FUNCPTR)VPP_SetCmuControl, 3, params);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    return (MV_VPP_OK);
}


/***************************************************************
 * FUNCTION: set adaptive contrast enhancement level
 * INPUT: chanID - id of the channel
 *        AceLevel - ACE level
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_SetAceLevel(int handle, int chanID, int AceLevel)
{
    CMD cmd;
    int params[4];
    VPP_OBJ *vpp_obj;
    int cpcbID;

    VPPAPI_DBG_INFO("chanID [%d], AceLevel [%d] \n", chanID, AceLevel);

    //TODO: Use static table to disable the plane if(vpp_plane_map[PlaneId].is_valid)
    if(chanID != CHAN_MAIN
#ifdef ENABLE_PIP_GFX0_PATH
            &&  chanID != CHAN_PIP
#endif
        )
        return (MV_VPP_EBADPARAM);

    if(AceLevel < FIRST_ACE || AceLevel >= MAX_ACE)
        return (MV_VPP_EBADPARAM);

    if (vpp.magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = vpp.vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
          /*channel is not connected to any DV in configuration*/
        return (MV_VPP_EUNSUPPORT);
    }

    /*only CPCB0 has CMU*/
    if(cpcbID != CPCB_1)
        return MV_VPP_EBADCALL;

    /*send command for ACE adjustment*/
    params[0] = chanID;
    params[1] = VPP_CMU_SET_ACE;
    params[2] = AceLevel;
    params[3] = 1;//range [16, 236]

    make_cmd(&cmd, (FUNCPTR)VPP_SetCmuControl, 4, params);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    return (MV_VPP_OK);
}

/***************************************************************
 * FUNCTION: set brightness enhancement level
 * INPUT: chanID - id of the channel
 *        BrLevel - brightness enhancement level
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_SetBrLevel(int handle, int chanID, int BrLevel)
{
    CMD cmd;
    int params[3];
    VPP_OBJ *vpp_obj;
    int cpcbID;

    VPPAPI_DBG_INFO(" chanID [%d], BrLevel [%d] \n",  chanID, BrLevel);

    //TODO: Use static table to disable the plane if(vpp_plane_map[PlaneId].is_valid)
    if(chanID != CHAN_MAIN
#ifdef ENABLE_PIP_GFX0_PATH
            &&  chanID != CHAN_PIP
#endif
        )
        return (MV_VPP_EBADPARAM);

    if(BrLevel < FIRST_BR || BrLevel >= MAX_BR)
        return (MV_VPP_EBADPARAM);

    if (vpp.magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = vpp.vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
          /*channel is not connected to any DV in configuration*/
        return (MV_VPP_EUNSUPPORT);
    }

    /*only CPCB0 has CMU*/
    if(cpcbID != CPCB_1)
        return MV_VPP_EBADCALL;

    /*send command for brightness enhancement adjustment*/
    params[0] = chanID;
    params[1] = VPP_CMU_SET_BREN;
    params[2] = BrLevel;

    make_cmd(&cmd, (FUNCPTR)VPP_SetCmuControl, 3, params);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    return (MV_VPP_OK);
}
#endif //CONFIG_VPP_DISABLE_CMU

int MV_VPPOBJ_SetHdmiOutputMode(int handle, int mode)
{
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if ((chanID = CHAN_OF_PLANE(vpp_obj, PLANE_MAIN)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if(VPP_HDMIOutputModeSupport(vpp_obj, mode) != MV_VPP_OK) {
        /* mode not supported */
        return (MV_VPP_EUNSUPPORT);
    }

    return VPP_DVAPI_setHDMImode(vpp_obj, mode);
}

/********************************************************************************
* FUNCTION: Get Hdmi Tx Phy power status
* OUTPUT:  - pPowerState (Phy power state)
* RETURN: MV_VPP_OK - SUCCEED
*         MV_EBADPARAM - invalid parameters
*         MV_EUNCONFIG - VPP not configured
*         MV_VPP_ENODEV - no device
********************************************************************************/
int MV_VPPOBJ_GetHdmiPowerStatus(int handle, int * pPowerState)
{
    VPP_OBJ *vpp_obj;
    int params[2];

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if(pPowerState == NULL)
        return (MV_VPP_EBADPARAM);

    params[0] = VPP_HDMI_GET_POWER_STATE;
    params[1] = (int)(pPowerState);
    VPP_SetHdmiCmd (vpp_obj, params);
    return (MV_VPP_OK);
}
/********************************************************************************
* FUNCTION: MV_VPPOBJ_SetColorTransformGain
* OUTPUT:
* RETURN: MV_VPP_OK - SUCCEED
********************************************************************************/
int MV_VPPOBJ_SetColorTransformGain(int handle, VPP_COLORTEMP *ClrTempGainParam)
{
    CMD_BEGIN(3);

    CMD_CHECK(handle);

    params[0] = (int)VPP_OCSC_GAIN_FLOAT2INT(ClrTempGainParam->Rgain);
    params[1] = (int)VPP_OCSC_GAIN_FLOAT2INT(ClrTempGainParam->Ggain);
    params[2] = (int)VPP_OCSC_GAIN_FLOAT2INT(ClrTempGainParam->Bgain);

    CMD_COMMIT(3, VPP_SetColorTransformGain);

    return MV_VPP_OK;
}
/********************************************************************************
* FUNCTION: Get VPP suspended status
* OUTPUT:  - pVppState (VPP object suspended status)
* RETURN: MV_VPP_OK - SUCCEED
*         MV_EBADPARAM - invalid parameters
*         MV_EUNCONFIG - VPP not configured
*         MV_VPP_ENODEV - no device
********************************************************************************/
int MV_VPPOBJ_GetVppStatus(int handle, int * pVppState)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if(pVppState == NULL)
        return (MV_VPP_EBADPARAM);

    *pVppState = vpp_obj->suspend;

    return (MV_VPP_OK);
}

int MV_VPPOBJ_GetIsrTime(void)
{
    HRESULT Ret = MV_VPP_OK;
    return (Ret);
}

int MV_VPPOBJ_SetHDMISignalSwap(int handle, int HdmiSignalSwap)
{
    /* depricated API */
    return (MV_VPP_EUNSUPPORT);
}

int MV_VPPOBJ_GetGcsc(INT handle, UINT32 chanID, UINT32 mode, UINT32 *param_value)
{
    VPP_OBJ *vpp_obj;
    UINT32 params[2];

    if (g_vpp->magic != VPP_MAGIC)
        return (MV_VPP_EUNCONFIG);

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    if (!param_value)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    //TODO: Use static table to disable the plane if(vpp_plane_map[PlaneId].is_valid)
    if (chanID != CHAN_MAIN
#ifdef ENABLE_PIP_GFX0_PATH
            &&  chanID != CHAN_PIP
#endif
            && chanID != VPP_DP1
        )
        return (MV_VPP_EBADPARAM);


    params[0] = chanID;
    params[1] = mode;
    *param_value = VPP_GetGcsc(vpp_obj, (int *)params);
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Set Hdmi Config values
 * INPUT:  HDMI config values, Size
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiConfig(int handle, unsigned int * config, unsigned int bufSize)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    return VPP_BE_SetHDMIConfig(vpp_obj, (HDMI_CONFIG *)config, bufSize);
}

#ifndef CONFIG_VPP_DISABLE_CMU
/***************************************************************
 * FUNCTION: set color enhancement level
 * INPUT: chanID - id of the channel
 *        ColorSel - color selection
 *        ColorLevel - color enhancement level [0, 100]
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_SetClrLevel(int handle, int chanID, int ColorSel, int ColorLevel)
{
    CMD cmd;
    int params[4];
    VPP_OBJ *vpp_obj;
    int cpcbID;

    VPPAPI_DBG_INFO(" chanID [%d], ColorSel [%d], ColorLevel [%d] \n",  chanID, ColorSel, ColorLevel);

    //TODO: Use static table to disable the plane if(vpp_plane_map[PlaneId].is_valid)
    if(chanID != CHAN_MAIN
#ifdef ENABLE_PIP_GFX0_PATH
            &&  chanID != CHAN_PIP
#endif
        )
        return (MV_VPP_EBADPARAM);

    if(ColorSel < FIRST_COLOR || ColorSel >= MAX_COLOR)
        return (MV_VPP_EBADPARAM);

    if(ColorLevel < VPP_CMU_CTRL_VAL_MIN || ColorLevel > VPP_CMU_CTRL_VAL_MAX)
        return (MV_VPP_EBADPARAM);

    if (vpp.magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = vpp.vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
          /*channel is not connected to any DV in configuration*/
        return (MV_VPP_EUNSUPPORT);
    }

    /*only CPCB0 has CMU*/
    if(cpcbID != CPCB_1)
        return MV_VPP_EBADCALL;

    /*send command for color level adjustment*/
    params[0] = chanID;
    params[1] = VPP_CMU_SET_CLRSAT;
    params[2] = ColorSel;
    params[3] = ColorLevel;

    make_cmd(&cmd, (FUNCPTR)VPP_SetCmuControl, 4, params);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    return (MV_VPP_OK);
}

/***************************************************************
 * FUNCTION: set color hue
 * INPUT: chanID - id of the channel
 *        ColorSel - color selection
 *        ColorHue- color hue value [0, 100]
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_SetClrHue(int handle, int chanID, int ColorSel, int ColorHue)
{
    CMD cmd;
    int params[4];
    VPP_OBJ *vpp_obj;
    int cpcbID;

    VPPAPI_DBG_INFO("chanID [%d], ColorSel [%d], ColorHue [%d] \n",  chanID, ColorSel, ColorHue);

    //TODO: Use static table to disable the plane if(vpp_plane_map[PlaneId].is_valid)
    if(chanID != CHAN_MAIN
#ifdef ENABLE_PIP_GFX0_PATH
            &&  chanID != CHAN_PIP
#endif
        )
        return (MV_VPP_EBADPARAM);

    if(ColorSel < FIRST_COLOR || ColorSel >= MAX_COLOR)
        return (MV_VPP_EBADPARAM);

    if(ColorHue < VPP_CMU_CTRL_VAL_MIN || ColorHue > VPP_CMU_CTRL_VAL_MAX)
        return (MV_VPP_EBADPARAM);

    if (vpp.magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = vpp.vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
          /*channel is not connected to any DV in configuration*/
        return (MV_VPP_EUNSUPPORT);
    }

    /*only CPCB0 has CMU*/
    if(cpcbID != CPCB_1)
        return MV_VPP_EBADCALL;

    /*send command for color level adjustment*/
    params[0] = chanID;
    params[1] = VPP_CMU_SET_CLRHUE;
    params[2] = ColorSel;
    params[3] = ColorHue;

    make_cmd(&cmd, (FUNCPTR)VPP_SetCmuControl, 4, params);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    return (MV_VPP_OK);
}


/***************************************************************
 * FUNCTION: set flesh tone detection and correction mode
 * INPUT: chanID - id of the channel
 *        FtdcMode - FTDC mode
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_SetFtdcMode(int handle, int chanID, int FtdcMode)
{
    CMD cmd;
    int params[3];
    VPP_OBJ *vpp_obj;
    int cpcbID;

    VPPAPI_DBG_INFO(" chanID [%d], FtdcMode [%d] \n",  chanID, FtdcMode);

    //TODO: Use static table to disable the plane if(vpp_plane_map[PlaneId].is_valid)
    if(chanID != CHAN_MAIN
#ifdef ENABLE_PIP_GFX0_PATH
            &&  chanID != CHAN_PIP
#endif
        )
        return (MV_VPP_EBADPARAM);

    if(FtdcMode < FIRST_FTDC || FtdcMode >= MAX_FTDC)
        return (MV_VPP_EBADPARAM);

    if (vpp.magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = vpp.vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
          /*channel is not connected to any DV in configuration*/
        return (MV_VPP_EUNSUPPORT);
    }

    /*only CPCB0 has CMU*/
    if(cpcbID != CPCB_1)
        return MV_VPP_EBADCALL;

    /*send command for FTDC adjustment*/
    params[0] = chanID;
    params[1] = VPP_CMU_SET_FTDC;
    params[2] = FtdcMode;

    make_cmd(&cmd, (FUNCPTR)VPP_SetCmuControl, 3, params);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    return (MV_VPP_OK);
}


/***************************************************************
 * FUNCTION: set intelligent color remapping mode
 * INPUT: chanID - id of the channel
 *        IcrMode - IRC mode
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_SetIcrMode(int handle, int chanID, int IcrMode)
{
    CMD cmd;
    int params[3];
    VPP_OBJ *vpp_obj;
    int cpcbID;

    VPPAPI_DBG_INFO(" chanID [%d], IcrMode [%d] \n",  chanID, IcrMode);

    //TODO: Use static table to disable the plane if(vpp_plane_map[PlaneId].is_valid)
    if(chanID != CHAN_MAIN
#ifdef ENABLE_PIP_GFX0_PATH
            &&  chanID != CHAN_PIP
#endif
        )
        return (MV_VPP_EBADPARAM);

    if(IcrMode < FIRST_ICR_MODE || IcrMode >= MAX_ICR_MODE)
        return (MV_VPP_EBADPARAM);

    if (vpp.magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = vpp.vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
          /*channel is not connected to any DV in configuration*/
        return (MV_VPP_EUNSUPPORT);
    }

    /*only CPCB0 has CMU*/
    if(cpcbID != CPCB_1)
        return MV_VPP_EBADCALL;

    /*send command for ICR adjustment*/
    params[0] = chanID;
    params[1] = VPP_CMU_SET_ICR;
    params[2] = IcrMode;

    make_cmd(&cmd, (FUNCPTR)VPP_SetCmuControl, 3, params);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    return (MV_VPP_OK);
}

/***************************************************************
 * FUNCTION: set basic picture control for main or PIP channel
 * INPUT: chanID - id of the channel
 *        CtrlSel - control selection
 *        CtrlVal - picture control value[0, 100]; [2500, 8000] for color temperature.
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
INT MV_VPPOBJ_SetPictureControl(INT handle, INT chanID, ENUM_PICTURE_CTRL CtrlSel, INT CtrlVal)
{
    CMD_BEGIN(3);

    CMD_CHECK(handle);

    VPPAPI_DBG_INFO(" BCHS chanID [%d], CtrlSel [%d], CtrlVal [%d] \n",  chanID, CtrlSel, CtrlVal);

    if(chanID != CHAN_MAIN
#ifdef ENABLE_PIP_GFX0_PATH
            &&  chanID != CHAN_PIP
#endif
            && chanID != VPP_DP1
        )
        return (MV_VPP_EBADPARAM);

    if(IS_VALID_PICTURE_CTRL_VAL(CtrlVal)) {
        return (MV_VPP_EBADPARAM);
    }

    /*set picture control*/
    params[0] = chanID;
    params[1] = CtrlSel;
    params[2] = CtrlVal;

    CMD_COMMIT(3, (chanID == VPP_DP1) ? (FUNCPTR)VPP_SetPQOCSC : (FUNCPTR)VPP_SetPQICSC);

    return MV_VPP_OK;
}


/********************************************************************************
 * FUNCTION: Set ACE Ctrl
 * INPUT: aceCmd- ACE cmd
 * INPUT: chanID- chad ID
   INPUT: pAceRegs-ACE DS parameter depend upon cmd
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/

int MV_VPPOBJ_SetACEParam(int handle, int chanID, int aceCmd, VPP_VDO_ACE_REG *pAceRegs )
{
    CMD cmd;
    int params[2];
    VPP_OBJ *vpp_obj;
    int cpcbID;

    VPPAPI_DBG_INFO(" ace cmd [%d] \n", aceCmd);

    if (vpp.magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = vpp.vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((aceCmd <= ACE_CMD_INVALID) || (aceCmd >= MAX_NUM_ACE_CMD))
        return (MV_VPP_EBADPARAM);

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, CHAN_MAIN)) == CPCB_INVALID) {
          /*channel is not connected to any DV in configuration*/
        return (MV_VPP_EUNSUPPORT);
    }

    /*set Ace parameter profile */
    params[0] = aceCmd;
    params[1] = chanID;
    GaloisMemcpy(&(vpp_obj->cmu.ACEParam),pAceRegs,sizeof(VPP_VDO_ACE_REG));

    make_cmd(&cmd, (FUNCPTR)VPP_SetAceEvalCommand, 2, params);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Get ACE Param
 * INPUT: aceCmd- ace cmd
 * INPUT: chanID- chad ID
   OUTPUT: pAceRegs- Ace DS parameter depend upon cmd
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetACEParam(int handle, int chanID, int aceCmd, VPP_VDO_ACE_REG *pAceRegs )
{
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Set FTDC Param
   INPUT: pFtdcRegs-FTDC parameter
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/

int MV_VPPOBJ_SetFTDCParam(int handle, VPP_CMU_FTDC_PARAMS *pFtdcRegs )
{
    CMD cmd;
    int params[2];
    VPP_OBJ *vpp_obj;
    int cpcbID;

    VPPAPI_DBG_INFO(" ftdc  \n");

    if (vpp.magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = vpp.vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, CHAN_MAIN)) == CPCB_INVALID) {
          /*channel is not connected to any DV in configuration*/
        return (MV_VPP_EUNSUPPORT);
    }

    GaloisMemcpy(&(vpp_obj->cmu.FTDCParam),pFtdcRegs,sizeof(VPP_CMU_FTDC_PARAMS));

    make_cmd(&cmd, (FUNCPTR)VPP_SetFTDCEvalCommand, 2, params);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Get FTDC Param
   OUTPUT: pFtdcRegs- FTDC DS parameter depend upon cmd
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetFTDCParam(int handle, VPP_CMU_FTDC_PARAMS *pFtdcRegs )
{
    return (MV_VPP_OK);
}
#endif //CONFIG_VPP_DISABLE_CMU

#ifndef CONFIG_VPP_DISABLE_GDETEE
/********************************************************************************
 * FUNCTION: set GDet mode
 * INPUT:  mode -
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
MV_VPPOBJ_SetGDetMode(int handle, int mode)
{
    CMD cmd;
    int params[1];
    VPP_OBJ *vpp_obj;
    int cpcbID;

    if (vpp.magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = vpp.vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }


    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, CHAN_MAIN)) == CPCB_INVALID) {
          /*channel is not connected to any DV in configuration*/
        return (MV_VPP_EUNSUPPORT);
    }

    /*set GDet parameter mode */
    params[0] = mode;

    make_cmd(&cmd, (FUNCPTR)VPP_SetGDetParams, 1, params);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

#ifndef VPP_DIAGS
    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
#endif

    return (MV_VPP_OK);

}

/********************************************************************************
 * FUNCTION: set EE mode
 * INPUT:  mode -
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetEEMode(int handle, UINT32 mode, VPP_CMU_EE_CTRL_PARAM *pCmuEeCtrlParam)
{
    CMD_BEGIN(3);

    CMD_CHECK(handle);

    if (mode >= VPP_CMU_EE_MAX_MODE) {
        VPPAPI_DBG_ERROR("EDGE_EN Invalid EE Mode: %d \n", mode);
        return MV_VPP_EBADPARAM;
    }

    VPPAPI_DBG_INFO("EDGE_EN mode: %d  \n", mode);

    /*send command for hardware transaction*/
    params[0] = mode;

    if (VPP_CMU_EE_NEW_LTI_CTI_WITH_CUSTOM_GAIN_CONTROL == mode) {
        params[1] = pCmuEeCtrlParam->stVppCmuLtiCtiCustomCtrlParam.u8Ltigain;
        params[2] = pCmuEeCtrlParam->stVppCmuLtiCtiCustomCtrlParam.u8Ctigain;

        VPPAPI_DBG_INFO("EDGE_EN eeLtiGain [%d] eeCtiGain [%d] \n", \
                                pCmuEeCtrlParam->stVppCmuLtiCtiCustomCtrlParam.u8Ltigain, \
                                pCmuEeCtrlParam->stVppCmuLtiCtiCustomCtrlParam.u8Ctigain);
    }

    CMD_COMMIT(3, VPP_SetEEParams);

    return MV_VPP_OK;
}
#endif //CONFIG_VPP_DISABLE_GDETEE
/***************************************************************
 * FUNCTION: set gamma correction
 * INPUT: vopID - id of the channel
 *        GmVal - user gamma value
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
INT MV_VPPOBJ_SetGmMode(INT handle, VPP_VOP_TYPE vopID, ENUM_GAMMA_MODE GmMode, \
                                                                UINT32 u32GammaPhyAddr)
{
    CMD_BEGIN(2);
    UINT32 AllocAddr=0;

    VPPAPI_DBG_INFO("GAMMA vopID [%d], GmMode [%d] \n",  vopID, GmMode);

    if (vopID >= VPP_MAX_VOP)
        return (MV_VPP_EBADPARAM);

    if (GmMode >= MAX_GM_MODE)
        return (MV_VPP_EBADPARAM);

    CMD_CHECK(handle);

    vpp_obj->dv[vopID].gammaLutinfo.GammaCurrentMode = GmMode;
    if (GM_CUSTOM == GmMode) {
            if (NULL == vpp_obj->dv[vopID].gammaLutinfo.pstgammaLut) {
                VPP_SHM32_MALLOC(VPP_SHM_SECURE,GAMMA_LUT_SIZE, &AllocAddr);
                vpp_obj->dv[vopID].gammaLutinfo.pstgammaLut = (VPP_GAMMA_LUT_STRUCT *)AllocAddr;
            }
            VPP_Memcpy((void*)&vpp_obj->dv[vopID].gammaLutinfo.pstgammaLut, (void*)&u32GammaPhyAddr, \
                                                                            sizeof(GAMMA_LUT_SIZE));
    }
    /*write register value & address to gamma bcm buffer*/
    VPP_BE_SetGammaCorrection(vpp_obj, vopID, GmMode);

    /*send command for hardware transaction*/
    params[0] = vopID;
    params[1] = GmMode;

    CMD_COMMIT(2, VPP_SetGamma);

    return (MV_VPP_OK);
}

int MV_VPPOBJ_SetAxiLmeterParams(int handle, unsigned int uiEnable, unsigned int uiCaptureCycles,        unsigned int uiHistWidth, unsigned int uiHistHeight, unsigned int uiNuOfFrames)
{
    VPP_OBJ *vpp_obj;
    CMD     cmd;
    int     cpcbID;
    int     params[8];
    int     retCode = MV_VPP_OK;

    if (vpp.magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = vpp.vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE)
    {
        return (MV_VPP_EUNCONFIG);
    }
    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
        /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    params[0] = uiEnable;
    params[1] = uiCaptureCycles;
    params[2] = uiHistWidth;
    params[3] = uiHistHeight;
    params[4] = uiNuOfFrames;

    make_cmd(&cmd, (FUNCPTR)VPP_AxiLatencyMeterParams, 5, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (retCode);


}

int MV_VPPOBJ_SetFormat(int handle, int cpcbID, VPP_DISP_OUT_PARAMS *pDispParams)
{
    VPP_OBJ *vpp_obj;
    int retVal;
    CMD cmd;
    int params[8], i;
    int vco_freq;
    int resID;
    int bit_depth;
    int pixel_rept;
    int color_fmt;
    int isForceResChange;
    int isQuiescentMode;
    unsigned int uiDisplayMode;
#ifdef VPPDRV_VPP_INTR_FREQ_MONITOR
    int vppIntrMonStatus = VPPDRV_vppIntrMonGetStatus(g_vpp->vpp_objs[handle]);
#endif //VPPDRV_VPP_INTR_FREQ_MONITOR

    if (g_vpp->magic != VPP_MAGIC) {
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (!pDispParams)
        return (MV_VPP_EBADPARAM);

    resID = pDispParams->uiResId;
    bit_depth = pDispParams->uiBitDepth;
    pixel_rept = pDispParams->iPixelRepeat;
    color_fmt = pDispParams->uiColorFmt;

    isForceResChange = (pDispParams->uiResId & 0x80000000);
    isQuiescentMode = pDispParams->uiResId & 0x40000000;
    uiDisplayMode = pDispParams->uiDisplayMode;

    VPPAPI_DBG_INFO(" handle [%x], cpcb [%d], resID [%d], bit_depth [%d], clr_fmt [%d], pixel_rep [%d]\n", handle, cpcbID, resID, bit_depth, color_fmt, pixel_rept);

    if (resID<FIRST_RES || resID>=MAX_NUM_RESS)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if (cpcbID<FIRST_CPCB || cpcbID>=MAX_NUM_CPCBS)
        return (MV_VPP_EBADPARAM);

    /*For DSI format resid, configure the panel here and then set the format to custom*/
    if ((resID == RES_DSI_CUSTOM) || ((pDispParams->uiDispId == VOUT_DSI) && (!IS_DSI_PANEL_RES(resID)))) {
        if ((resID !=  vpp_obj->dsiInfo.CurrentResID) || isForceResChange) {
            if (VPP_BE_ConfigMIPIResinfo(handle, resID)) {
                VPPAPI_DBG_ERROR("MIPI setformat failed resid[%d]\n",resID);
                return (MV_VPP_EBADPARAM);
            }
            vpp_obj->dsiInfo.CurrentResID = resID;
        }
        else {
                VPPAPI_DBG_INFO("MIPI is already set with same resid[%d]\n",resID);
        }
        resID = RES_DSI_CUSTOM;
    }
    vco_freq = GetVCOFreq(resID, bit_depth);

    VPPDRV_vppIntrMonSetStatus(vpp_obj, 0);

    if((pDispParams->uiDispId == VOUT_HDMI) || (uiDisplayMode == VOUT_DISP_SINGLE_MODE_SEC))
        VPP_BE_SetHdmiPhyPower(vpp_obj, FALSE);
#ifdef CONFIG_VPP_FASTLOGO_SUPPORT
    if (resID == RES_RESET)
        goto setFormat_skiphdmi_clear;
    //This is bug fix for LGU - recovery mode shows no signal sometime, solution ensure recieve resync   with transmitter
    /*Disable Scrambling*/
    VPP_BE_HDMI_ScramblingEnable(vpp_obj, FALSE);
    /* clear SCDC reserved bit for LGE UHD TV */
    if (MV_VPP_OK == VPP_BE_HDMI_ClearSCDCReservedBit(vpp_obj))
    {
        MV_OSAL_Task_Sleep(50); //50ms
    }

setFormat_skiphdmi_clear:
#endif //CONFIG_VPP_FASTLOGO_SUPPORT

    /*RES_RESET will not clear output_res, so use output_res_updated to check reset state*/
    if (((VPP_GetResId(cpcbID) != resID) ||
        (vpp_obj->dv[cpcbID].vco_freq != vco_freq)) || isForceResChange ||
        (vpp_obj->dv[cpcbID].output_res_updated == 0) ||
        (vpp_obj->m_VoutMode != uiDisplayMode) ||
        ((pDispParams->uiDispId == VOUT_HDMI) && HDMI_COLORFMT420_CHANGE(vpp_obj->be.hdmiTx, color_fmt)))
    {
        /* update CPCB vco frequency */
        vpp_obj->dv[cpcbID].vco_freq = vco_freq;

        /*backward compatibility, allow primary - secondary overlay switch based on resolution*/
        uiDisplayMode = ((uiDisplayMode == VOUT_DISP_SINGLE_MODE_PRI) &&
                IS_DSI_PANEL_RES(resID))? VOUT_DISP_SINGLE_MODE_SEC : uiDisplayMode;

        for(i=FIRST_CPCB; i<MAX_NUM_CPCBS; i++) {
            if (vpp_obj->dv[i].status != STATUS_INACTIVE) {
                /*Reset both CPCB in case of mode change*/
                if((vpp_obj->m_VoutMode != uiDisplayMode) || (cpcbID == i)) {
                    /* reset CPCB TG */
                    ResetCPCBTG(vpp_obj, i, uiDisplayMode);
                }
            }
        }

        if (resID != RES_RESET) {

            VPP_ResetPlane(vpp_obj, uiDisplayMode);
            vpp_obj->m_VoutMode = uiDisplayMode;

            if (vpp_obj->iVppFeatureCfg[0] & VPP_FEATURE_MIPI_DSI) {
                /*Handle dynamic switching between DSI & HDMI*/
                if(IS_DSI_PANEL_RES(resID)) {
                    VPP_BE_MIPI_PanelInit(vpp_obj, (resID - RES_MIN_DSI));
                } else {
                    if (!(vpp_obj->iVppFeatureCfg[0] & VPP_FEATURE_HDMITX)) {
                        VPPAPI_DBG_ERROR("Invalid MIPI-DSI Res ID = %d\n", resID);
                        return (MV_VPP_EBADPARAM);
                    }
                }
            }

            VPP_SetClk(vpp_obj, cpcbID, resID);

            MV_OSAL_Mutex_Lock(vpp_obj->bcmbuf_lock);

            //Enable the canvas change flag, so that inline line dropping is forced for 1st frame, when ofline scaling is required
            vpp_obj->resIdUpdated[cpcbID] = 1;

            /* select BCM sub-buffer to dump register settings */
            vpp_obj->pVbiBcmBuf = vpp_obj->pVbiBcmBufCpcb[cpcbID];
            VPP_BCMBUF_Select(vpp_obj->pVbiBcmBuf, cpcbID);

            VPPAPI_DBG_INFO("vppdrv: %s %d\n",__FUNCTION__,__LINE__);

            /* set CPCB resolution */
            params[0] = cpcbID;
            params[1] = resID;
            params[2] = color_fmt;
            params[3] = bit_depth;
            params[4] = pixel_rept;
            params[5] = isQuiescentMode;
            params[6] = uiDisplayMode;
            params[7] = pDispParams->uiDispId;
            VPP_SetCPCBOutputResolution(vpp_obj, params);

            /* start BCM engine to program VPP registers */
            VPP_BCMBUF_HardwareTrans(vpp_obj->pVbiBcmBuf, 1/*block*/);

#ifndef CONFIG_VPP_ENABLE_INTERRUPT_ON_PUSHFRAME
#ifdef ENABLE_GFX2_VOP2_PATH
            /*DP1->DP2 case, VOP2 DTG interupt used for all programming*/
            if(IS_VOUT_DISP_MODE(SINGLE_MODE_SEC)) {
                VPP_Enable_ISR_Interrupt(vpp_obj, CPCB_2);
            }
            else
#endif
            {
                VPP_Enable_VDE_Interrupt(vpp_obj, cpcbID);
                VPP_Enable_ISR_Interrupt(vpp_obj, cpcbID);
            }

            VPP_GlobalIntr_Config(vpp_obj, cpcbID);
            VPP_dhub_sem_clear(vpp_obj);
#endif //CONFIG_VPP_ENABLE_INTERRUPT_ON_PUSHFRAME

            /* switch BCM buffer for the CPCB */
            if (vpp_obj->pVbiBcmBufCpcb[cpcbID] == &vpp_obj->vbi_bcm_buf[cpcbID][0])
                vpp_obj->pVbiBcmBufCpcb[cpcbID] = &vpp_obj->vbi_bcm_buf[cpcbID][1];
            else
                vpp_obj->pVbiBcmBufCpcb[cpcbID] = &vpp_obj->vbi_bcm_buf[cpcbID][0];

            MV_OSAL_Mutex_Unlock(vpp_obj->bcmbuf_lock);

            /* set DV status to active */
            vpp_obj->dv[cpcbID].status = STATUS_ACTIVE;
        }
#ifdef BERLIN_DTV_MERGE
        AudioDAC_ReSetSampRate();
#endif
    }

#ifdef VPPDRV_VPP_INTR_FREQ_MONITOR
    //Restore monitoring status after resolution change
    VPPDRV_vppIntrMonSetStatus(vpp_obj, vppIntrMonStatus);
#endif //VPPDRV_VPP_INTR_FREQ_MONITOR

//##################################################
//############### SetHdmiVideoFmt ##################
//##################################################
    resID = VPP_GetResId(cpcbID);

    if (resID == RES_INVALID)
        return (MV_VPP_EBADPARAM);

    /* Config video format */
    if((pDispParams->uiDispId == VOUT_HDMI) || IS_VOUT_DISP_MODE(SINGLE_MODE_SEC)) {
        if (MV_VPP_OK != (retVal = VPP_BE_ConfigHdmiVideoFmt (vpp_obj, color_fmt,
                        bit_depth, pixel_rept)))
            return retVal;
    }

    if(IS_DSI_PANEL_RES(resID))
        return (MV_VPP_OK);

    params[0] = VPP_HDMI_SET_VIDEO_FMT;
    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 1, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);
    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
    return (MV_VPP_OK);
}

int MV_VPPOBJ_Stop (int handle)
{
    VPP_OBJ *vpp_obj;
    int cpcbID;

    VPPAPI_DBG_INFO("enter\n");

    CMD_CHECK(handle);

    MV_VPP_Stop(g_vpp->vpp_objs[handle]);

    g_vpp->vpp_objs[handle]->vpp_isr_need_exit = 1;

    return MV_VPP_OK;
}

int MV_VPPOBJ_GetDispOutParams(int handle, VPP_DISP_OUT_PARAMS *pdispParams, int size)
{
    int i;
    VPP_OBJ *vpp_obj;
    int cpcbID;
    VPP_DISP_OUT_PARAMS *pDispOutParams;
    int nCpcb;

    CMD_CHECK(handle);

    if (!pdispParams)
        return MV_VPP_EBADPARAM;

#ifdef ENABLE_GFX2_VOP2_PATH
    nCpcb = MAX_NUM_CPCBS;
#else
    nCpcb = 1;
#endif

    if (size < (nCpcb * sizeof(VPP_DISP_OUT_PARAMS))) {
        VPPAPI_DBG_ERROR("Size mismatch\n");
        return MV_VPP_EBADPARAM;
    }

    for (i= 0; i < nCpcb; i++) {
        pDispOutParams = (vpp_obj->dv[i].dispOutParams.uiResId == RES_INVALID) ?
                         &vpp_obj->dispOutParams[i] : &vpp_obj->dv[i].dispOutParams;
        VPP_Memcpy(&pdispParams[i], pDispOutParams, sizeof(VPP_DISP_OUT_PARAMS));
    }

    return (MV_VPP_OK);
}

int MV_VPPOBJ_SetDispOutParams(int handle, VPP_DISP_OUT_PARAMS *pdispParams, int cpcbID)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC)
        return (MV_VPP_EUNCONFIG);

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE)
        return (MV_VPP_EUNCONFIG);

    if (!pdispParams)
        return MV_VPP_EBADPARAM;

    VPP_Memcpy(&vpp_obj->dispOutParams[cpcbID], pdispParams, sizeof(VPP_DISP_OUT_PARAMS));

    if (vpp_obj->dispOutParams[cpcbID].uiDispId == VOUT_DSI)
        VPP_BE_ConfigMIPIResinfo(handle, vpp_obj->dispOutParams[cpcbID].uiResId);

    return (MV_VPP_OK);
}

int MV_VPPOBJ_LoadMipiConfig(int handle, VPP_MIPI_LOAD_CONFIG *pMipiLoadConfigParams)
{
    uintptr_t CmdAllocAddr=0;
    VPP_OBJ *vpp_obj;
    VPP_MIPI_CONFIG_PARAMS *pMipiCfg;
    int i;
    void *shmVirtAddr;
    int mipiCfgSize;
    int shmSize;
    VPP_SHM_HANDLE shmHandle;
    VPP_BUFF_HANDLE bufHandle;
    VPP_SHM_BUFFER_DESC bufDesc;


    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (!pMipiLoadConfigParams)
        return (MV_VPP_EBADPARAM);

    mipiCfgSize = pMipiLoadConfigParams->noOfresID * sizeof(VPP_MIPI_CONFIG_PARAMS);
    mipiCfgSize = VPP_SHM_ALIGN_ROUNDUP(mipiCfgSize);

    VPP_SHM32_MALLOC(VPP_SHM_NONSECURE, mipiCfgSize, (void *) &pMipiCfg);

    if (!pMipiCfg) {
        VPPAPI_DBG_ERROR("Error Allocating memory for MIPI \n");
        return (MV_VPP_ENOMEM);
    }

    //memid/phy-addr is passed in shmHandle
    shmHandle = (VPP_SHM_HANDLE)pMipiLoadConfigParams->vppMipiCfgPA;
    shmSize = mipiCfgSize;

    VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmHandle, shmSize, VPP_SHM_BUFFER_TYPE_INPUT);
    VPP_MapAndValidate_SingleBuffer(&bufHandle, &bufDesc);
    //Retrieve the value from VPP_BUFF_HANDLE
    shmVirtAddr = (void*)bufHandle.virt_addr;

#ifndef VPP_ENABLE_MEMID_BY_VIRT_ADDR
    //If continue to use physical address, then override virt addr with phy addr
    shmVirtAddr = (void*)pMipiLoadConfigParams->vppMipiCfgPA;
#endif

    VPP_Memcpy(pMipiCfg, (void*)shmVirtAddr,
        (pMipiLoadConfigParams->noOfresID * sizeof(VPP_MIPI_CONFIG_PARAMS)));

    VPP_UnMapUnRegister_SingleBuffer(&bufHandle);

    /* Copy the Command buffer to TA local memory*/
    for (i = 0; i < pMipiLoadConfigParams->noOfresID; i++) {
        VPP_SHM32_MALLOC(VPP_SHM_NONSECURE, MAX_CMD_BUF_SIZE, &CmdAllocAddr);

        if (!CmdAllocAddr)
            goto EXIT_FREE;

        //memid/phy-addr is passed in shmHandle
        shmHandle = (VPP_SHM_HANDLE)pMipiCfg[i].vppMipiCmd.pcmd;
        shmSize = VPP_SHM_ALIGN_ROUNDUP(MAX_CMD_BUF_SIZE);

        VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmHandle, shmSize, VPP_SHM_BUFFER_TYPE_INPUT);
        VPP_MapAndValidate_SingleBuffer(&bufHandle, &bufDesc);
        //Retrieve the value from VPP_BUFF_HANDLE
        shmVirtAddr = (void*)bufHandle.virt_addr;

#ifndef VPP_ENABLE_MEMID_BY_VIRT_ADDR
        //If continue to use physical address, then override virt addr with phy addr
        shmVirtAddr = (void*)pMipiCfg[i].vppMipiCmd.pcmd;
#endif

        VPP_Memcpy((void*) CmdAllocAddr, (void*)shmVirtAddr, MAX_CMD_BUF_SIZE);

        pMipiCfg[i].vppMipiCmd.pcmd = (ARCH_PTR_TYPE) CmdAllocAddr;
        VPP_UnMapUnRegister_SingleBuffer(&bufHandle);
    }

    vpp_obj->dsiInfo.MipiCfg.noOfresID = pMipiLoadConfigParams->noOfresID;
    vpp_obj->dsiInfo.MipiCfg.vppMipiCfgPA = (ARCH_PTR_TYPE) pMipiCfg;

    return MV_VPP_OK;

EXIT_FREE:
    for (;i > 0; i--) {
        VPP_SHM_Free((VPP_SHM_HANDLE) pMipiCfg[i-1].vppMipiCmd.pcmd);
    }
    VPP_SHM_Free((VPP_SHM_HANDLE) pMipiCfg);

    return (MV_VPP_ENOMEM);
}

int MV_VPPOBJ_GetBlockStatus(int handle, ENUM_VPP_BLOCK blkId, int blkIdArg, int *status)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (!status)
        return (MV_VPP_EBADPARAM);

    if ((blkId < VPP_BLOCK_START) || (blkId > VPP_BLOCK_MAX))
        return (MV_VPP_EBADPARAM);

    switch (blkId) {
        case VPP_BLOCK_CPCB_TG:
            MV_VPP_GetCpcbStatus(vpp_obj, blkIdArg, status);
            break;

        default:
            VPPAPI_DBG_ERROR("Unspported block ID %d for status\n", blkId);
            return MV_VPP_EUNSUPPORT;

    }

    return (MV_VPP_OK);
}

int MV_VPPOBJ_MipiPanelSendCmd(int handle, UINT8 *pcmd, UINT16 bufsize)
{
    VPP_OBJ *vpp_obj;
    UINT8 *shmVirtAddr;
    int ret;
    VPP_SHM_HANDLE shmHandle;
    VPP_BUFF_HANDLE bufHandle;
    VPP_SHM_BUFFER_DESC bufDesc;
    int shmsize;

    if (g_vpp->magic != VPP_MAGIC)
    {
        return (MV_VPP_EUNCONFIG);
    }

    if (handle < 0 || handle >= MAX_NUM_OBJECTS)
    {
        return (MV_VPP_EBADPARAM);
    }

    if (!pcmd || !bufsize)
    {
        return (MV_VPP_EBADPARAM);
    }

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj) {
        return (MV_VPP_ENODEV);
    }

    //memid/phy-addr is passed in shmHandle
    shmHandle = (VPP_SHM_HANDLE)(ARCH_PTR_TYPE)pcmd;
    shmsize = VPP_SHM_ALIGN_ROUNDUP(bufsize);

    VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmHandle, shmsize, VPP_SHM_BUFFER_TYPE_INPUT);
    VPP_MapAndValidate_SingleBuffer(&bufHandle, &bufDesc);

    //Retrieve the value from VPP_BUFF_HANDLE
    shmVirtAddr = (void*)bufHandle.virt_addr;

#ifndef VPP_ENABLE_MEMID_BY_VIRT_ADDR
    //If continue to use physical address, then override virt addr with phy addr
    shmVirtAddr = (void*)pcmd;
#endif

    ret = VPP_BE_MIPI_PanelSendCmdBuf(vpp_obj, shmVirtAddr, bufsize);

    VPP_UnMapUnRegister_SingleBuffer(&bufHandle);

    return ret;
}
