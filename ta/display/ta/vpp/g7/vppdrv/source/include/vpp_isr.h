/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_ISR_H_
#define _VPP_ISR_H_

#ifdef __cplusplus
extern "C" {
#endif

#define VPP_DHUB_SIZE (16)
#define VPP_DHUB_BURST_OPT (256)
#define VPP_DHUB_BURST_MMU_ONLY (64)
#define VPP_STRIDE_ALIGNMNET (1024)
#define VPP_IS_MULTIPLE_OF_BURST(width) (!((width) & (VPP_DHUB_BURST_OPT-1)))
#define VPP_IS_MULTIPLE_OF_MMU_BURST(width)  (!((width) & (VPP_DHUB_BURST_MMU_ONLY-1)))
#define VPP_IS_STRIDE_ALIGN_TO_BURST(stride)  (!((stride) & (VPP_STRIDE_ALIGNMNET-1)))
#define VPP_IS_PORTRAIT_FRAME_RESTRICT(iHRes, iVRes, oHRes, oVRes, scan) ((iHRes < iVRes)           \
                                                        && ((iVRes * 1.0 / oVRes) > 2.0)            \
                                                        && (SCAN_INTERLACED == scan))
#define MAX_VBI_TO_RESTRICT_DNSCL_AFTER_RES_CHANGE (2+3)
/* MTR crop is not supported now.
   So excluding compressed inputs from limiting on downScl on res change */
#define VPP_IS_LIMIT_DOWNSCL_AFTER_RES_CHANGE(PLANEID, iHRes, oHRes, PRESINFO, VBI_NUM, MTR_EN) \
    (!(IS_MTR_ENABLED(MTR_EN)) && \
     VBI_NUM < MAX_VBI_TO_RESTRICT_DNSCL_AFTER_RES_CHANGE && \
     (PLANE_MAIN == PLANEID || PLANE_PIP == PLANEID) && \
     (PRESINFO->freq < 148500 || SCAN_INTERLACED == PRESINFO->scan) && \
     (iHRes > oHRes))

//#define ENABLE_CROP_CALC_DEBUG_PRINTS
//Ensure to crop only even pixels from FETG and do odd left/right pixel crop using scalar cropping
#define ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH

//Enable to the allow power saving by doing line dropping on GFX1 plane during >2k playback (4K)
//#define CONFIG_FORCE_LINEDROPPING_FOR_GFX1

#ifdef ENABLE_CROP_CALC_DEBUG_PRINTS
static int display_win=1;
#define VDBG_IF_DISP_ENABLE(enable)    display_win=enable
#define VDBG_IF_DISP(...)              if(display_win) VPPAPI_DBG_INFO(VPP_DEBUG_CROP_PREFIX __VA_ARGS__)
#define VDBG_IF_DISP_WIN(window)       if(display_win) PRINT_WIN1(window)
#define VDBG_DISP(...)                 VPPAPI_DBG_INFO(VPP_DEBUG_CROP_PREFIX __VA_ARGS__)
#else //ENABLE_CROP_CALC_DEBUG_PRINTS
#define VDBG_IF_DISP_ENABLE(enable)
#define VDBG_IF_DISP(...)
#define VDBG_IF_DISP_WIN(window)
#define VDBG_DISP(...)
#endif //ENABLE_CROP_CALC_DEBUG_PRINTS

#define VPP_PIP_BUILTIN_FRAME_WIDTH 64
#define VPP_PIP_BUILTIN_FRAME_HEIGHT 36
#ifdef CONFIG_HARDCODE_BUILT_IN_FRAME_SCL_TO_1_1

//Enable below line to get debug prints for built-in frame SCL/PWR optimization
//#define ENABLE_VDBG_DISP_BUILTIN_FRAME

#ifdef  ENABLE_VDBG_DISP_BUILTIN_FRAME
#define VPP_DEBUG_BUILTIN_FRAME_PREFIX  "VPPDBG:BUILTIN:"
#define VDBG_DISP_BUILTIN_FRAME(...)    VPPAPI_DBG_INFO(VPP_DEBUG_BUILTIN_FRAME_PREFIX __VA_ARGS__)
#else   //ENABLE_VDBG_DISP_BUILTIN_FRAME
#define VDBG_DISP_BUILTIN_FRAME(...)
#endif  //!ENABLE_VDBG_DISP_BUILTIN_FRAME

#endif //CONFIG_HARDCODE_BUILT_IN_FRAME_SCL_TO_1_1

#define ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH

#define VPP_IS_DEALPHA_SRC_FMT(fmt, cpcb) ((SRCFMT_ARGB32_PM == fmt) && (cpcb == CPCB_1))

/*************************************************************
 * FUNCTION: register VPP interrupt service routine
 * PARAMS: none
 * RETURN: none
 *************************************************************/
void VPP_ISR_Registration(VPP_OBJ *vpp_obj);

void VPP_ISR_TaskHandler(VPP_OBJ *vpp_obj);
void VPP_DSR_Task(VPP_OBJ *vpp_obj);

void VPP_ISR_UnRegistration(VPP_OBJ *vpp_obj);

void VPP_CEC_ISR_Registration(VPP_OBJ *vpp_obj);
void VPP_CEC_ISR_UnRegistration(VPP_OBJ *vpp_obj);

void VPP_Enable_VDE_Interrupt(VPP_OBJ *vpp_obj, int cpcbID);
void VPP_Disable_VDE_Interrupt(VPP_OBJ *vpp_obj, int cpcbID);

void VPP_Enable_ISR_Interrupt(VPP_OBJ *vpp_obj, int cpcbID);
void VPP_Disable_ISR_Interrupt(VPP_OBJ *vpp_obj, int cpcbID);
void VPP_Early_Update_CurrentFrame(VPP_OBJ *vpp_obj, int chanID, void *frame);

int configVppWriteBackMode(VPP_OBJ *vpp_obj, UINT32 uiMode,UINT32 uiWidth, UINT32 uiHeight);

int startVppWriteBackClients(VPP_OBJ *vpp_obj, UINT32 uiMode, UINT32 uiWidth, UINT32 uiHeight);

int vppSetDhubChannelCfg(VPP_OBJ *vpp_obj, INT dmaId, UINT8 uchInverseScanMode, SIGN32 mtuByte);

UINT32 vppMtrR_ConfigStride(UINT8 *semId, UINT32 stride);

int VPP_VIP_TG_SetTGParams(VPP_OBJ *vpp_obj, UINT32 uiWidth, UINT32 uiHeight);

UNSG32 vppDhubCfg2NDDMA(VPP_OBJ *vpp_obj, VPP_DHUB_CFG_2NDDMA_PARAM *pstCfg2NDDMAParam);

void VPP_ISR_Handler(UINT32 msg_id, UINT32 msg_para);

void CEC_ISR_Handler(UINT32 msg_id, UINT32 msg_para);

#ifdef __cplusplus
}
#endif

#endif

