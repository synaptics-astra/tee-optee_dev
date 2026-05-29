/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "vpp_apifuncs_fastlogo.h"
#include "vpp_fe_dlr.h"
#include "hdmi_tx_wrap.h"
#include "vpp_be.h"
#include "cinclude.h"

#define avioDhubSemMap_vpp_vppCPCB0_intr avioDhubSemMap_vpp128b_vpp_inr0
#define vpp_vop2_DTG_frst_intr avioDhubSemMap_vpp128b_vpp_inr14
#define avioDhubSemMap_vpp_vppOUT4_intr avioDhubSemMap_vpp128b_vpp_inr6
#define avioDhubSemMap_vpp_CH10_intr avioDhubSemMap_vpp128b_CH8_intr
#define avioDhubSemMap_vpp_vppOUT0_intr avioDhubSemMap_vpp128b_vpp_inr2
#define avioDhubSemMap_vpp_vppOUT3_intr avioDhubSemMap_vpp128b_vpp_inr5

#define HDMI_FC_AVICONF0 0x4064

#define bTST(x, b) (((x) >> (b)) & 1)

#ifdef DEBUG_PRINT_ENABLE
#define VPPDBG_PRINT(...) VPP_log(VPP_LOG_API, VPP_LOG_INFO, "DEBUG: TA:", __VA_ARGS__)
#else
#define VPPDBG_PRINT(...)
#endif

static void MV_VPP_dhub_sem_clear(VPP_OBJ *vpp_obj);

void MV_VPP_EnableSemIntr(VPP_OBJ *vpp_obj, int cpcbID, int flag) {

    VPPDBG_PRINT("Before Entered %s at %d \n",__FUNCTION__,__LINE__);

#ifdef ENABLE_GFX2_VOP2_PATH
   if (IS_CPCB_VOUT_DISP_MODE(cpcbID, SINGLE_MODE_SEC)) {
        semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp128b_vpp_inr14, 1, 0);
        semaphore_pop(vpp_obj->pSemHandle, avioDhubSemMap_vpp128b_vpp_inr14, 1);
        semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp128b_vpp_inr14);
        semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp128b_vpp_inr14, 0/*empty*/, flag/*full*/, 0/*almost empty*/, 0/*almost full*/, 0/*cpu id*/);
    }
    else
#endif
    {
        /* configure and enable CPCB0 interrupt
            * Enable VDE interrupt only for HDMI
            */
        if (CPCB_1 == cpcbID) {
            if (!IS_CPCB_VOUT_DISP_MODE(cpcbID, SINGLE_MODE_SEC)) {
                semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr, 1, 0);
                semaphore_pop(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr, 1);
                semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr);
                semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr, 0/*empty*/, flag/*full*/, 0/*almost empty*/, 0/*almost full*/, 0/*cpu id*/);
                VPPDBG_PRINT("at %s at %d \n",__FUNCTION__,__LINE__);
            }
            semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp128b_vpp_inr0, 1, 0);
            semaphore_pop(vpp_obj->pSemHandle, avioDhubSemMap_vpp128b_vpp_inr0, 1);
            semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp128b_vpp_inr0);
            semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp128b_vpp_inr0, 0/*empty*/, flag/*full*/, 0/*almost empty*/, 0/*almost full*/, 0/*cpu id*/);
        }

#ifdef ENABLE_GFX2_VOP2_PATH
        if (CPCB_2 == cpcbID) {
            if (IS_CPCB_VOUT_DISP_MODE(cpcbID, DUAL_MODE_PIP)) {
                semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp128b_vpp_inr14, 1, 0);
                semaphore_pop(vpp_obj->pSemHandle, avioDhubSemMap_vpp128b_vpp_inr14, 1);
                semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp128b_vpp_inr14);
                semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp128b_vpp_inr14, 0/*empty*/, flag/*full*/, 0/*almost empty*/, 0/*almost full*/, 0/*cpu id*/);
            }
        }
#endif
    }

    VPP_GlobalIntr_Config(vpp_obj, cpcbID);
    VPPDBG_PRINT("Before Exited %s at %d \n",__FUNCTION__,__LINE__);
}

void MV_VPP_Enable_ISR_Interrupt(VPP_OBJ *vpp_obj, int cpcbID, int flag)
{
        MV_VPP_EnableSemIntr(vpp_obj, cpcbID, flag);
        if (cpcbID == CPCB_1) {
            vpp_obj->vde_int_enable[CPCB_1] = 1;
        }
    return;
}

void MV_VPP_DisableInterrupt(VPP_OBJ *vpp_obj, int cpcbID)
{
    if(g_vpp->VPP_fl.enableInterrupt[cpcbID])
    {
        MV_VPP_EnableSemIntr(vpp_obj, cpcbID, 0);
        MV_VPP_dhub_sem_clear(vpp_obj);
        g_vpp->VPP_fl.enableInterrupt[cpcbID] = 0;
    }
}

//Enable the interrupt after pushframe by frame/still_pic
void MV_VPP_EnableInterrupt(VPP_OBJ *vpp_obj, int cpcbID)
{
    if(!g_vpp->VPP_fl.enableInterrupt[cpcbID]) {
        VDB_print("%s:%d: enableInterrupt=%d\n", __func__, __LINE__, g_vpp->VPP_fl.enableInterrupt[cpcbID]);

        g_vpp->VPP_fl.enableInterrupt[cpcbID]= 1;
        MV_VPP_dhub_sem_clear(vpp_obj);
        MV_VPP_EnableSemIntr(vpp_obj, cpcbID, 1);
    }
}

//Need this interface, if we want to enable/disable sem intr from CA side
int MV_VPPOBJ_EnableInterrupt(int handle, int flag)
{
    VPP_OBJ *vpp_obj;
    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    MV_VPP_EnableSemIntr(vpp_obj, CPCB_1, flag);
#ifdef ENABLE_GFX2_VOP2_PATH
    MV_VPP_EnableSemIntr(vpp_obj, CPCB_2, flag);
#endif //ENABLE_GFX2_VOP2_PATH

    return (MV_VPP_OK);
}

static void MV_VPP_dhub_sem_clear(VPP_OBJ *vpp_obj)
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

int MV_VPP_Stop(VPP_OBJ *vpp_obj)
{
    volatile int j;
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    /* wait for CPCB TG reset done */
    MV_VPP_EnableSemIntr(vpp_obj, CPCB_1, 0);
#ifdef ENABLE_GFX2_VOP2_PATH
    MV_VPP_EnableSemIntr(vpp_obj, CPCB_2, 0);
#endif
    for(j=0;j<2;j++)
    {
        volatile unsigned int i=0;
        for(i=0;i<100000;i++);
    }
    MV_VPP_dhub_sem_clear(vpp_obj);
    return (MV_VPP_OK);
}

void MV_VPP_get_interrupt_message(VPP_OBJ *vpp_obj, UINT32 *pIntrMsg)
{
    UINT32 msg_para = *pIntrMsg;

    if(msg_para==0) {
        HDL_semaphore *pSemHandle = vpp_obj->pSemHandle;
        //bootloader does not pass the instant/msg_para; but amp_core driver will pass it through CA
        msg_para = semaphore_chk_full(pSemHandle, -1);
        if (bTST(msg_para, vpp_vop2_DTG_frst_intr)) {
            semaphore_pop(pSemHandle, vpp_vop2_DTG_frst_intr, 1);
            semaphore_clr_full(pSemHandle, vpp_vop2_DTG_frst_intr);
            msg_para = bSETMASK(vpp_vop2_DTG_frst_intr);
        } else if (bTST(msg_para, avioDhubSemMap_vpp_vppCPCB0_intr)) {
            semaphore_pop(pSemHandle, avioDhubSemMap_vpp_vppCPCB0_intr, 1);
            semaphore_clr_full(pSemHandle, avioDhubSemMap_vpp_vppCPCB0_intr);
            msg_para = bSETMASK(avioDhubSemMap_vpp_vppCPCB0_intr);
        } else if (bTST(msg_para, avioDhubSemMap_vpp_vppOUT4_intr)) {
            semaphore_pop(pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr, 1);
            semaphore_clr_full(pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr);
            msg_para = bSETMASK(avioDhubSemMap_vpp_vppOUT4_intr);
        } else if (bTST(msg_para, avioDhubSemMap_vpp_vppOUT0_intr)) {
            semaphore_pop(pSemHandle, avioDhubSemMap_vpp_vppOUT0_intr, 1);
            semaphore_clr_full(pSemHandle, avioDhubSemMap_vpp_vppOUT0_intr);
            msg_para = bSETMASK(avioDhubSemMap_vpp_vppOUT0_intr);
        }
        *pIntrMsg = msg_para;
    }

    if(msg_para==0) {
        VDB_print("%s:%d: msg_para:%x\n", __func__, __LINE__, msg_para);
        //If no interrupt, then return immediately
        return;
    }
}

/*******************************************************************
 * FUNCTION: clear SCDC reserved bit according to LGE UHD TV requirement
 *******************************************************************/
int MV_VPP_ClearSCDCReservedBit(VPP_OBJ *vpp_obj)
{
    /* TBD : For LG TV will clear bit 7 (reserved bit) of offset 0x20 (TMDS CNTRL CONFIG)
     *          to clear the pop up JIRA:6953
     */
    return (MV_VPP_OK);
}

int MV_VPPOBJ_SetHdmiTxControl(int handle, int enable)
{
    return (MV_VPP_OK);
}

#ifdef CONFIG_VPP_ENABLE_DELAY_BY_LOOP
/*
 * Poll TEE_GetSystemTime until at least 'ms' milliseconds have elapsed.
 * TEE_Time resolution is milliseconds, so sub-millisecond delays are
 * rounded up to 1 ms.
 */
static void mdelay(unsigned int ms)
{
    TEE_Time t0, t1;
    uint32_t elapsed_ms;

    if (ms == 0)
        return;

    TEE_GetSystemTime(&t0);
    do {
        TEE_GetSystemTime(&t1);
        elapsed_ms = (t1.seconds - t0.seconds) * 1000u +
                     (uint32_t)((int32_t)t1.millis - (int32_t)t0.millis);
    } while (elapsed_ms < ms);
}

static void udelay(unsigned int us)
{
    /* TEE_Time is ms-granular; round up to at least 1 ms */
    mdelay(us < 1000u ? 1u : us / 1000u);
}
#endif //CONFIG_VPP_ENABLE_DELAY_BY_LOOP

HRESULT  VPP_OSAL_Task_Sleep(UINT32 TimeOutMS)
{
#ifndef CONFIG_AMP_TEE_ENABLE

    MV_TimeSpec_t   TimeSpec;

    TimeSpec.tv_sec = sec;
    TimeSpec.tv_nsec = (TimeOutMS * 1000 * 1000);
    MV_TimeSpec_Delay (&TimeSpec);

#else

#ifdef CONFIG_VPP_ENABLE_DELAY_BY_LOOP
    mdelay(TimeOutMS);
#else
    TEE_Wait(TimeOutMS);
#endif //CONFIG_VPP_ENABLE_DELAY_BY_LOOP

#endif //CONFIG_AMP_TEE_ENABLE
    return S_OK;
}

int MV_VPPOBJ_EnableAutoPush(int handle, int enable)
{
    int cpcbID;

    /* Set HDMI output to fix color, Otherwise show pixel generated from pipe
     *        otherwise after autopush disable lastpixel of bootlogo
     *        will result in faint color screen before android logo is seen
     *
     */

    T32HDMI_TX_WRAP_VID_CTRL vid_ctrl;
    T32HDMI_TX_WRAP_VID_CTRL1 vid_ctrl1;
    UINT32 base_addr_txwrap = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMITX_WRAP_REG_BASE;
    UINT32 base_addr_tx = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_SNPS_HDMITX_REG_BASE;
    int color_space;
    int fix_color_flag = enable?0x00:0x01;

    GA_REG_WORD32_READ(base_addr_tx + HDMI_FC_AVICONF0, &color_space);
    color_space &= 0x03;
    GA_REG_WORD32_READ(base_addr_txwrap + RA_HDMI_TX_WRAP_VID_CTRL, &vid_ctrl.u32);
    GA_REG_WORD32_READ(base_addr_txwrap + RA_HDMI_TX_WRAP_VID_CTRL1, &vid_ctrl1.u32);
    /* enable fix color transmission from hdmi tx wrapper.
       Below values configure black screen for different formats
     */
    if (color_space == OUTPUT_COLOR_FMT_RGB888) {
        vid_ctrl.uVID_CTRL_fix_clr_ch0 = 0x00;
        vid_ctrl.uVID_CTRL_fix_clr_ch1 = 0x00;
        vid_ctrl1.uVID_CTRL_fix_clr_ch2 = 0x00;
    } else if (color_space == OUTPUT_COLOR_FMT_YCBCR420) {
        vid_ctrl.uVID_CTRL_fix_clr_ch0 = 0x00;
        vid_ctrl.uVID_CTRL_fix_clr_ch1 = 0x00;
        vid_ctrl1.uVID_CTRL_fix_clr_ch2 = 0x800;
    } else {
        //YUV444 or YUV422
        vid_ctrl.uVID_CTRL_fix_clr_ch0 = 0x800;
        vid_ctrl.uVID_CTRL_fix_clr_ch1 = 0x00;
        vid_ctrl1.uVID_CTRL_fix_clr_ch2 = 0x800;
    }
    vid_ctrl.uVID_CTRL_fix_clr_sel = fix_color_flag;
    GA_REG_WORD32_WRITE(base_addr_txwrap + RA_HDMI_TX_WRAP_VID_CTRL1, vid_ctrl1.u32);
    GA_REG_WORD32_WRITE(base_addr_txwrap + RA_HDMI_TX_WRAP_VID_CTRL, vid_ctrl.u32);

    /*Enable/Disable the autopush feature for BCM Q0*/
    for(cpcbID = FIRST_CPCB; cpcbID < MAX_NUM_CPCBS; cpcbID++) {
        VPP_BCMDHUB_AutoPush(cpcbID, 0, enable);
    }

    /*TBD: Better wait for next frame (wost case 20ms for 50Hz)*/
#ifdef CONFIG_FASTLOGO_DETAILED_LOG
    VDB_print("%s:%d: Autopush enable : %d FixColor flag : %d/%d, FixColor:%x\n",
        __func__, __LINE__, enable, fixColorFlag, stHDTX_VIDEO_CTRL.uVIDEO_CTRL_in_yc, stHDTX_IN_FIX_CLR.u32);
#else
    VDB_print("%s:%d: Autopush enable : %d \n",  __func__, __LINE__, enable);
#endif

    return MV_VPP_OK;
}

unsigned int VPP_SHM_CleanCache(VPP_SHM_HANDLE  hShm, unsigned int offset, unsigned int  size)
{
    int retVal = MV_VPP_OK;
    void *vir_addr;

    vir_addr = VPP_SHM_GetVirtualAddress(hShm, offset);

    if(g_vpp->VPP_fl.enableFlushCache)
    {
        retVal = TEE_FlushCache(vir_addr, size);
    }

    return retVal;
}

int VPP_BE_CEC_Init (VPP_OBJ *vpp_obj)
{
    return MV_VPP_OK;
}

int VPP_BE_CEC_Term (VPP_OBJ *vpp_obj)
{
    return MV_VPP_OK;
}

int VPP_BE_CEC_Enable (VPP_OBJ *vpp_obj, BOOL enable)
{
    return MV_VPP_OK;
}

