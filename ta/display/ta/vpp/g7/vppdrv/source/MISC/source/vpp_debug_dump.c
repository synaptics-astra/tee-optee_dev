/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_debug_dump.c
//! \brief      Various vpp debug dump utility routine to debug VPP issues
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#include "vpp_module.h"
#include "vpp_debug.h"
#include "vpp_debug_util.h"
#include "vpp_debug_dump.h"
#include "vpp_dv_api.h"
#include "api_avio_dhub.h"
#include "avio.h"

#ifdef VPP_DEBUG_UTIL_ENABLE_printRegDump
#define VPP_DEBUG_UTIL_REGDUMP_CMD_VPP       0
#define VPP_DEBUG_UTIL_REGDUMP_CMD_BCM       1
#define VPP_DEBUG_UTIL_REGDUMP_CMD_DHUB      2
#define VPP_DEBUG_UTIL_REGDUMP_CMD_HDMI      3
#define VPP_DEBUG_UTIL_REGDUMP_CMD_VPP_All   4
#define VPP_DEBUG_UTIL_REGDUMP_CMD_VPP_MP    5
#define VPP_DEBUG_UTIL_REGDUMP_CMD_VPP_GFX   6
#define VPP_DEBUG_UTIL_REGDUMP_CMD_VPP_HDRX  7
#define VPP_DEBUG_UTIL_REGDUMP_CMD_VPP_MTR   8

#define VPP_DEBUG_UTIL_PP_2_VPP         VPP_DEBUG_UTIL_PP_2 "VPP"
#define VPP_DEBUG_UTIL_PP_2_BCM         VPP_DEBUG_UTIL_PP_2 "BCM"
#define VPP_DEBUG_UTIL_PP_2_DHUB        VPP_DEBUG_UTIL_PP_2 "DHUB"
#define VPP_DEBUG_UTIL_PP_2_HDMI        VPP_DEBUG_UTIL_PP_2 "HDMI"
#define VPP_DEBUG_UTIL_PP_2_CPCB        VPP_DEBUG_UTIL_PP_2 "CPCB"

typedef struct _tag_bcm_reg_dump_table {
    //Flag, If the offset_addr is repeated for each queue or not
    BOOL    isRepeatedForQ;
    UINT32  offset_addr;
    char    *addr_name;
} VPP_DEBUG_UTIL_BCM_REG_INFO;

static void VPP_DEBUG_UTIL_RegDump_bcm(VPP_OBJ *vpp_obj) {
    const VPP_DEBUG_UTIL_BCM_REG_INFO bcm_reg_table[] = {
        {1, 0x00100, "BCM_Q"},
        {0, 0x00144, "BCM_FULL_STS"},
        {0, 0x00148, "BCM_EMP_STS"},
        {0, 0x0014C, "BCM_FLUSH"},
        {0, 0x00150, "BCM_AUTOPUSH_CNT"},
        {1, 0x00154, "OCCURRENCE_Q"},
        {0, 0x00198, "BCM_AUTOPUSH"},
        {0, 0x0019c, "BCM_FULL_STS_STICKY"},
        {0, 0x001a0, "BCM_ERROR"},
        {0, 0x001a4, "BCM_LOG_ADDR"},
        {0, 0x001a8, "BCM_ERROR_DATA"},
    };

    UINT32 size = 0;
    UINT32 idx = 0;
    UINT32 regVal = 0;
    UINT32 i;
    UINT32 base_addr;
    //Iterator variables
    UINT32 iter_offset;
    const UINT32 maxQueues = 18;

    base_addr = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_BCM_REG_BASE;

    size = sizeof(bcm_reg_table)/sizeof(VPP_DEBUG_UTIL_BCM_REG_INFO);

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_BCM, "@@@@@@@ BCM REG DUMP START @@@@@@@\n");

    for(idx=0; idx<size; idx++){
        if(bcm_reg_table[idx].isRepeatedForQ) {
            iter_offset = 0;
            //Iterate for maxQueues
            for(i=0; i<maxQueues; i++,iter_offset+=4) {
                GA_REG_WORD32_READ(base_addr + bcm_reg_table[idx].offset_addr + iter_offset, &regVal);
                VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_BCM, "%s%d\t 0x%X 0x%X\n",
                        bcm_reg_table[idx].addr_name, i,
                        base_addr + bcm_reg_table[idx].offset_addr + iter_offset, regVal);
            }
        } else {
                GA_REG_WORD32_READ(base_addr + bcm_reg_table[idx].offset_addr, &regVal);
                VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_BCM, "%s\t 0x%X 0x%X\n",
                        bcm_reg_table[idx].addr_name,
                        base_addr + bcm_reg_table[idx].offset_addr, regVal);
        }
    }

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_BCM, "@@@@@@@ BCM REG DUMP END @@@@@@@\n");
}

static void VPP_DEBUG_DumpDhubConfig_Func(UNSG32 dHubBaseAddr, UNSG32 hboSramAddr, HDL_dhub2d *pdhubHandle, DHUB_channel_config *dhub_config, SIGN32 numOfChans)
{
    char *DhubNameList[2] = {
        "VPP DHUB",
        "AG DHUB",
    };
    char *DhubName = pdhubHandle == &VPP_dhubHandle?DhubNameList[0]:DhubNameList[1];
    char *notDefinedStr="-NA-";
    SIGN32 i;
    SIGN32 chanId;
    char **ChanName = NULL;

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_DHUB, "\nDHUB: %s,   hboSramAddr : %08X, dHubBaseAddr : %08X\n", DhubName?DhubName:notDefinedStr,hboSramAddr, dHubBaseAddr);
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_DHUB, "\t\t\t\t: chanId:, CmdBase:, DataBase:, CmdSize:, DataSize:, MtuSize:, Qos:, SelfLoop:, Enable:\n");

    for (i = 0; i< numOfChans; i++) {
        chanId = dhub_config[i].chanId;
        char *tabSeparator = strlen(ChanName?ChanName[chanId]:notDefinedStr) >= 12 ? "\t" : "\t\t";
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_DHUB, "%d(%s) %s: %02X,      %08X, %08X, %08X, %08X, %08X, %08X, %d, \t%d\n",
                i, ChanName?ChanName[chanId]:notDefinedStr, tabSeparator,
                dhub_config[i].chanId, dhub_config[i].chanCmdBase,  dhub_config[i].chanDataBase,
                dhub_config[i].chanCmdSize, dhub_config[i].chanDataSize, dhub_config[i].chanMtuSize,
                dhub_config[i].chanQos, dhub_config[i].chanSelfLoop, dhub_config[i].chanEnable );
    }
}

static void VPP_DEBUG_UTIL_RegDump_dhub(VPP_OBJ *vpp_obj) {
    VPP_DEBUG_DumpDhubConfig_Func(VPP_DHUB_BASE, VPP_HBO_SRAM_BASE, &VPP_dhubHandle, VPP_config, VPP_NUM_OF_CHANNELS);
    VPP_DEBUG_DumpDhubConfig_Func(AG_DHUB_BASE, AG_HBO_SRAM_BASE, &AG_dhubHandle, AG_config, AG_NUM_OF_CHANNELS);
}

static void VPP_DEBUG_UTIL_RegDump_hdmi(VPP_OBJ *vpp_obj) {
}

void VPP_DEBUG_UTIL_printRegDumpMain(VPP_OBJ *vpp_obj, UINT32 printMainSubCmd) {
    switch(printMainSubCmd) {
        case VPP_DEBUG_UTIL_REGDUMP_CMD_VPP:
            VPP_DEBUG_DumpRegs(vpp_obj->base_addr, VPP_DEBUG_REG_DUMP_ENABLE_VPP);
        break;
        case VPP_DEBUG_UTIL_REGDUMP_CMD_BCM:
            VPP_DEBUG_UTIL_RegDump_bcm(vpp_obj);
        break;
        case VPP_DEBUG_UTIL_REGDUMP_CMD_DHUB:
            VPP_DEBUG_UTIL_RegDump_dhub(vpp_obj);
        break;
        case VPP_DEBUG_UTIL_REGDUMP_CMD_HDMI:
            VPP_DEBUG_UTIL_RegDump_hdmi(vpp_obj);
        break;
        case VPP_DEBUG_UTIL_REGDUMP_CMD_VPP_All:
            VPP_DEBUG_DumpRegs(vpp_obj->base_addr, VPP_DEBUG_REG_DUMP_ENABLE_ALL);
        break;
        case VPP_DEBUG_UTIL_REGDUMP_CMD_VPP_MP:
            VPP_DEBUG_DumpRegs(vpp_obj->base_addr, VPP_DEBUG_REG_DUMP_ENABLE_MP);
        break;
        case VPP_DEBUG_UTIL_REGDUMP_CMD_VPP_GFX:
            VPP_DEBUG_DumpRegs(vpp_obj->base_addr, VPP_DEBUG_REG_DUMP_ENABLE_GFX);
        break;
        case VPP_DEBUG_UTIL_REGDUMP_CMD_VPP_HDRX:
            VPP_DEBUG_DumpRegs(vpp_obj->base_addr, VPP_DEBUG_REG_DUMP_ENABLE_HDRX);
        break;
        case VPP_DEBUG_UTIL_REGDUMP_CMD_VPP_MTR:
            VPP_DEBUG_DumpRegs(vpp_obj->base_addr, VPP_DEBUG_REG_DUMP_ENABLE_MTR);
        break;

    }
}
#endif //VPP_DEBUG_UTIL_ENABLE_printRegDump

#if defined(VPP_DEBUG_UTIL_ENABLE_printVbufInfo) || defined(VPP_DEBUG_UTIL_ENABLE_StoreVbufInfo)
#define VPP_DEBUG_UTIL_PRINTF_VBUFINFO(...)     VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_3, __VA_ARGS__)
void VPP_DEBUG_UTIL_printVbufInfo(VPP_OBJ *vpp_obj, UINT32 planeID, VBUF_INFO *pVBInfo) {
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("VBUFINFO : planeID:%d-%x\n", planeID, pVBInfo);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_pbuf_start                 =   %p\n",  pVBInfo->m_pbuf_start   );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_buf_size                   =   %d\n",  pVBInfo->m_buf_size     );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_srcfmt                     =   %d\n",  pVBInfo->m_srcfmt   );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_order                      =   %d\n",  pVBInfo->m_order    );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_bytes_per_pixel            =   %d\n",  pVBInfo->m_bytes_per_pixel  );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_content_offset             =   %d\n",  pVBInfo->m_content_offset   );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_content_width              =   %d\n",  pVBInfo->m_content_width    );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_content_height             =   %d\n",  pVBInfo->m_content_height   );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_buf_stride                 =   %d\n",  pVBInfo->m_buf_stride   );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_active_left                =   %d\n",  pVBInfo->m_active_left  );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_active_top                 =   %d\n",  pVBInfo->m_active_top   );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_active_width               =   %d\n",  pVBInfo->m_active_width     );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_active_height              =   %d\n",  pVBInfo->m_active_height    );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_disp_offset                =   %d\n",  pVBInfo->m_disp_offset  );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_is_frame_seq               =   %d\n",  pVBInfo->m_is_frame_seq     );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_is_top_field_first         =   %d\n",  pVBInfo->m_is_top_field_first   );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_is_repeat_first_field      =   %d\n",  pVBInfo->m_is_repeat_first_field    );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_is_progressive_pic         =   %d\n",  pVBInfo->m_is_progressive_pic   );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_pixel_aspect_ratio         =   %d\n",  pVBInfo->m_pixel_aspect_ratio   );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_frame_rate_num             =   %d\n",  pVBInfo->m_frame_rate_num   );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_frame_rate_den             =   %d\n",  pVBInfo->m_frame_rate_den   );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_pan_left[0]                =   %d\n",  pVBInfo->m_pan_left[0]  );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_pan_top[0]                 =   %d\n",  pVBInfo->m_pan_top[0]   );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_pan_width[0]               =   %d\n",  pVBInfo->m_pan_width[0]     );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_pan_height[0]              =   %d\n",  pVBInfo->m_pan_height[0]    );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_pan_left[1]                =   %d\n",  pVBInfo->m_pan_left[1]  );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_pan_top[1]                 =   %d\n",  pVBInfo->m_pan_top[1]   );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_pan_width[1]               =   %d\n",  pVBInfo->m_pan_width[1]     );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_pan_height[1]              =   %d\n",  pVBInfo->m_pan_height[1]    );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_clut_start_index           =   %d\n",  pVBInfo->m_clut_start_index     );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_clut_num_items             =   %d\n",  pVBInfo->m_clut_num_items   );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_is_right_eye_view_valid    =   %d\n",  pVBInfo->m_is_right_eye_view_valid  );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_right_eye_view_buf_start   =   0x%x\n",  pVBInfo->m_right_eye_view_buf_start     );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_3D_source_fmt              =   %d\n",  pVBInfo->m_3D_source_fmt    );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_3D_interpretation_type     =   %d\n",  pVBInfo->m_3D_interpretation_type   );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_3D_SBS_sampling_mode       =   %d\n",  pVBInfo->m_3D_SBS_sampling_mode     );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_3D_convert_mode_from_SEI   =   %d\n",  pVBInfo->m_3D_convert_mode_from_SEI     );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_3D_SwitchLeftRightView     =   %d\n",  pVBInfo->m_3D_SwitchLeftRightView   );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_bits_per_pixel             =   %d\n",  pVBInfo->m_bits_per_pixel   );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_buf_pbuf_start_UV          =   0x%x\n",  pVBInfo->m_buf_pbuf_start_UV    );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_buf_stride_UV              =   0x%x\n",  pVBInfo->m_buf_stride_UV    );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_flags                      =   %d\n",  pVBInfo->m_flags    );
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_is_compressed              =   %d\n",  pVBInfo->m_is_compressed);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_luma_left_ofst             =   %d\n",  pVBInfo->m_luma_left_ofst);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_luma_top_ofst              =   %d\n",  pVBInfo->m_luma_top_ofst);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_chroma_left_ofst           =   %d\n",  pVBInfo->m_chroma_left_ofst);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_chroma_top_ofst            =   %d\n",  pVBInfo->m_chroma_top_ofst);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_colorprimaries             =   %d\n",  pVBInfo->m_colorprimaries);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("builtinFrame                 =   %d\n",  pVBInfo->builtinFrame);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("m_dv_present_mode            =   %d\n",  pVBInfo->m_dv_present_mode);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("transfer_characteristics     =   %d\n",  pVBInfo->transfer_characteristics);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-Y m_MtrrCfgCfg_stride    =   %d\n",  pVBInfo->m_mtr_buf_desc_Y.m_MtrrCfgCfg_stride);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-Y m_MtrrCfgCfg_format    =   %d\n",  pVBInfo->m_mtr_buf_desc_Y.m_MtrrCfgCfg_format);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-Y m_MtrrCfgCfg_mode      =   %d\n",  pVBInfo->m_mtr_buf_desc_Y.m_MtrrCfgCfg_mode);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-Y m_MtrrCfgCfg_weave     =   %d\n",  pVBInfo->m_mtr_buf_desc_Y.m_MtrrCfgCfg_weave);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-Y m_MtrrCfgBase_addr     =   %d\n",  pVBInfo->m_mtr_buf_desc_Y.m_MtrrCfgBase_addr);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-Y m_MmuCfgPbm_shy_bw     =   %d\n",  pVBInfo->m_mtr_buf_desc_Y.m_MmuCfgPbm_shy_bw);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-Y m_MmuCfgPbm_shy_pos    =   %d\n",  pVBInfo->m_mtr_buf_desc_Y.m_MmuCfgPbm_shy_pos);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-Y m_MmuCfgPbm_pm_enable  =   %d\n",  pVBInfo->m_mtr_buf_desc_Y.m_MmuCfgPbm_pm_enable);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-Y m_MmuCfgPbm_shuffle_en =   %d\n",  pVBInfo->m_mtr_buf_desc_Y.m_MmuCfgPbm_shuffle_en);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-Y m_MmuCfgPbm_bm_enable  =   %d\n",  pVBInfo->m_mtr_buf_desc_Y.m_MmuCfgPbm_bm_enable);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-Y m_MmuCfgPbm_weave      =   %d\n",  pVBInfo->m_mtr_buf_desc_Y.m_MmuCfgPbm_weave);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-Y m_MmuCfgVm_enable      =   %d\n",  pVBInfo->m_mtr_buf_desc_Y.m_MmuCfgVm_enable);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-UV m_MtrrCfgCfg_stride   =   %d\n",  pVBInfo->m_mtr_buf_desc_UV.m_MtrrCfgCfg_stride);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-UV m_MtrrCfgCfg_format   =   %d\n",  pVBInfo->m_mtr_buf_desc_UV.m_MtrrCfgCfg_format);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-UV m_MtrrCfgCfg_mode     =   %d\n",  pVBInfo->m_mtr_buf_desc_UV.m_MtrrCfgCfg_mode);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-UV m_MtrrCfgCfg_weave    =   %d\n",  pVBInfo->m_mtr_buf_desc_UV.m_MtrrCfgCfg_weave);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-UV m_MtrrCfgBase_addr    =   %d\n",  pVBInfo->m_mtr_buf_desc_UV.m_MtrrCfgBase_addr);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-UV m_MmuCfgPbm_shy_bw    =   %d\n",  pVBInfo->m_mtr_buf_desc_UV.m_MmuCfgPbm_shy_bw);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-UV m_MmuCfgPbm_shy_pos   =   %d\n",  pVBInfo->m_mtr_buf_desc_UV.m_MmuCfgPbm_shy_pos);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-UV m_MmuCfgPbm_pm_enable =   %d\n",  pVBInfo->m_mtr_buf_desc_UV.m_MmuCfgPbm_pm_enable);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-UV m_MmuCfgPbm_shuffle_en=   %d\n",  pVBInfo->m_mtr_buf_desc_UV.m_MmuCfgPbm_shuffle_en);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-UV m_MmuCfgPbm_bm_enable =   %d\n",  pVBInfo->m_mtr_buf_desc_UV.m_MmuCfgPbm_bm_enable);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-UV m_MmuCfgPbm_weave     =   %d\n",  pVBInfo->m_mtr_buf_desc_UV.m_MmuCfgPbm_weave);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-UV m_MmuCfgVm_enable     =   %d\n",  pVBInfo->m_mtr_buf_desc_UV.m_MmuCfgVm_enable);
}

void VPP_DEBUG_UTIL_printVbufInfo_opt(VPP_OBJ *vpp_obj, UINT32 planeID, VBUF_INFO *pVBInfo) {
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("VBUFINFO : planeID:%d-%x\n", planeID, pVBInfo);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("buf[Y:%p/%x, U:%x/%x, S:%d],  fmt[s:%d, o:%d, B:%d, b:%d, c:%d]\n",
            pVBInfo->m_pbuf_start,pVBInfo->m_buf_stride, pVBInfo->m_buf_pbuf_start_UV, pVBInfo->m_buf_stride_UV, pVBInfo->m_buf_size,
            pVBInfo->m_srcfmt, pVBInfo->m_order, pVBInfo->m_bytes_per_pixel, pVBInfo->m_bits_per_pixel, pVBInfo->m_colorprimaries);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("content[do:%d, co:%d, c:%dx%d], active[%dx%d, %dx%d]\n",
            pVBInfo->m_disp_offset, pVBInfo->m_content_offset, pVBInfo->m_content_width, pVBInfo->m_content_height,
            pVBInfo->m_active_left, pVBInfo->m_active_top, pVBInfo->m_active_width, pVBInfo->m_active_height);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("pan_0[%dx%d, %dx%d], pan_1[%dx%d, %dx%d]\n",
            pVBInfo->m_pan_left[0], pVBInfo->m_pan_top[0], pVBInfo->m_pan_width[0], pVBInfo->m_pan_height[0],
            pVBInfo->m_pan_left[1], pVBInfo->m_pan_top[1], pVBInfo->m_pan_width[1], pVBInfo->m_pan_height[1]);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("s_crop[ll:%d, lt:%d, cl:%d, ct:%d]\n",
            pVBInfo->m_luma_left_ofst, pVBInfo->m_luma_top_ofst, pVBInfo->m_chroma_left_ofst, pVBInfo->m_chroma_top_ofst);
    VPP_DEBUG_UTIL_PRINTF_VBUFINFO("flags[p:%d/%d, c:%d, dv:%d, b:%d, tr:%d, f:%d, t:%d, r:%d]\n",
            pVBInfo->m_is_frame_seq, pVBInfo->m_is_progressive_pic, pVBInfo->m_is_compressed, pVBInfo->m_dv_present_mode,
            pVBInfo->builtinFrame, pVBInfo->transfer_characteristics, pVBInfo->m_flags, pVBInfo->m_is_top_field_first, pVBInfo->m_is_repeat_first_field);

    if(pVBInfo->m_is_compressed)
    {
            VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-Y [s:%d, f:%d, m:%d, a:0x%x] [sb:%d, sp:%d, pe:%d, se:%d, be:%d, ve:%d]\n",
                    pVBInfo->m_mtr_buf_desc_Y.m_MtrrCfgCfg_stride, pVBInfo->m_mtr_buf_desc_Y.m_MtrrCfgCfg_format,
                    pVBInfo->m_mtr_buf_desc_Y.m_MtrrCfgCfg_mode, pVBInfo->m_mtr_buf_desc_Y.m_MtrrCfgBase_addr,
                    pVBInfo->m_mtr_buf_desc_Y.m_MmuCfgPbm_shy_bw, pVBInfo->m_mtr_buf_desc_Y.m_MmuCfgPbm_shy_pos,
                    pVBInfo->m_mtr_buf_desc_Y.m_MmuCfgPbm_pm_enable, pVBInfo->m_mtr_buf_desc_Y.m_MmuCfgPbm_shuffle_en,
                    pVBInfo->m_mtr_buf_desc_Y.m_MmuCfgPbm_bm_enable, pVBInfo->m_mtr_buf_desc_Y.m_MmuCfgVm_enable);
            VPP_DEBUG_UTIL_PRINTF_VBUFINFO("MTR-UV [s:%d, f:%d, m:%d, a:0x%x] [sb:%d, sp:%d, pe:%d, se:%d, be:%d, ve:%d]\n",
                    pVBInfo->m_mtr_buf_desc_UV.m_MtrrCfgCfg_stride, pVBInfo->m_mtr_buf_desc_UV.m_MtrrCfgCfg_format,
                    pVBInfo->m_mtr_buf_desc_UV.m_MtrrCfgCfg_mode, pVBInfo->m_mtr_buf_desc_UV.m_MtrrCfgBase_addr,
                    pVBInfo->m_mtr_buf_desc_UV.m_MmuCfgPbm_shy_bw, pVBInfo->m_mtr_buf_desc_UV.m_MmuCfgPbm_shy_pos,
                    pVBInfo->m_mtr_buf_desc_UV.m_MmuCfgPbm_pm_enable, pVBInfo->m_mtr_buf_desc_UV.m_MmuCfgPbm_shuffle_en,
                    pVBInfo->m_mtr_buf_desc_UV.m_MmuCfgPbm_bm_enable, pVBInfo->m_mtr_buf_desc_UV.m_MmuCfgVm_enable);
    }

}
#endif //VPP_DEBUG_UTIL_ENABLE_printVbufInfo

#define VPP_DBG_UTIL_BCMBUF_SIZE_MAX        (256)
#define VPP_DBG_UTIL_BCMBUF_REG_PER_LINE    6
static void VPP_DEBUG_UTIL_bcmBufferDumpLocal(UINT32 start, UINT32 size, const char *pPrnPrefix) {
    UINT32 i, regValAddr, regValData, regNdx, curLoopRegNdx;
    char localBuffer[VPP_DBG_UTIL_BCMBUF_SIZE_MAX];
    char regPairBuffer[VPP_DBG_UTIL_BCMBUF_SIZE_MAX/VPP_DBG_UTIL_BCMBUF_REG_PER_LINE];

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_8, "%s: virt-addr:%x size:%x\n", pPrnPrefix, start, size);

    localBuffer[0]='\0';
    for(i=1, curLoopRegNdx=regNdx=0; regNdx<size; i++, regNdx+=8, start+=8) {
        regValData = *((UINT32*)(start));
        regValAddr = *((UINT32*)(start+4));
#ifdef VPP_DEBUG_UTIL_BCM_BUFFER_DUMP_SIMPLE
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_8, "BCM_BUFF[%02x-%08x] => %8x = %8x\n", regNdx, start, regValAddr, regValData);
#else
        sprintf(regPairBuffer, "%08x=%08x\t", regValAddr, regValData);
        strcat(localBuffer, regPairBuffer);
        if(i>= VPP_DBG_UTIL_BCMBUF_REG_PER_LINE || (regNdx+8) >= size) {
            VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_8, "[%04x]:%s\n", curLoopRegNdx, localBuffer);
            curLoopRegNdx = regNdx+8;
            localBuffer[0]='\0';
            i=0;
        }
#endif
    }
}

#define IS_BCMBUFF_REG_ADDR_VALID(uiRegAddr)    ((uiRegAddr>=MEMMAP_AVIO_REG_BASE) && (uiRegAddr<(MEMMAP_AVIO_REG_BASE+MEMMAP_AVIO_REG_SIZE)))
static BOOL VPP_DEBUG_UTIL_isBcmBufferValidLocal(UINT32 start, UINT32 size, const char *pPrnPrefix) {
    UINT32 regValAddr, regValData, regNdx;

    for(regNdx=0; regNdx<size; regNdx+=8, start+=8) {
        regValData = *((UINT32*)(start));
        regValAddr = *((UINT32*)(start+4));
        if(!IS_BCMBUFF_REG_ADDR_VALID(regValAddr)) {
            VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_8, "INVALID:%s: BCM_BUFF[%02x-%08x] => %8x=%8x\n", pPrnPrefix, regNdx, start, regValAddr, regValData);
            return 0;
        }
    }

    return 1;
}

void VPP_DEBUG_UTIL_bcmBufferDump(BCMBUF *pbcmbuf, const char *pPrnPrefix) {
    UINT32 start;
    INT32 size;

    if(pbcmbuf) {
        start= (UINT32)pbcmbuf->dv1_head;
        size = (int)pbcmbuf->writer-(int)start;
        if (size > 0) {
            VPP_DEBUG_UTIL_bcmBufferDumpLocal(start, size, pPrnPrefix);
        }
    }
}

#ifdef VPP_DEBUG_UTIL_ENABLE_validateBcmBuffer
BOOL VPP_DEBUG_UTIL_isRawBcmBufferValid(UINT32 start, UINT32 size, const char *pPrnPrefix, BOOL dumpFlag) {
    BOOL flag;

    flag = VPP_DEBUG_UTIL_isBcmBufferValidLocal(start, size, pPrnPrefix);
    if(flag && start && size>0) {
        VPP_DEBUG_UTIL_bcmBufferDumpLocal(start, size, pPrnPrefix);
    }
    return flag;
}

BOOL VPP_DEBUG_UTIL_isBcmBufferValid(BCMBUF *pbcmbuf, const char *pPrnPrefix, BOOL dumpFlag) {
    BOOL retFlag;
    UINT32 start;
    INT32 size;

    if(pbcmbuf) {
        start= (UINT32)pbcmbuf->dv1_head;
        size = (int)pbcmbuf->writer-(int)start;
        if (size > 0) {
            retFlag = VPP_DEBUG_UTIL_isBcmBufferValidLocal(start, size, pPrnPrefix);
            if((VPP_DEBUG_UTIL_BCMBUFF_DUMP_ENABLE == dumpFlag) && !retFlag) {
                //Dump the BCM register if not valid
                VPP_DEBUG_UTIL_bcmBufferDumpLocal(start, size, pPrnPrefix);
            }
        }
    }

    return dumpFlag;
}

void VPP_DEBUG_UTIL_checkBcmBuffer(VPP_OBJ *vpp_obj, BCMBUF *pbcmbuf, const char *pPrnPrefix, BOOL dumpFlag) {
    if(VPP_DEBUG_UTIL_IS_FF_ENABLED_VALIDATE_BCMBUFF(vpp_obj)) {
        VPP_DEBUG_UTIL_isBcmBufferValid(pbcmbuf, pPrnPrefix, dumpFlag);
    }
}

void VPP_DEBUG_UTIL_checkRawBcmBuffer(VPP_OBJ *vpp_obj, UINT32 start, UINT32 size, const char *pPrnPrefix, BOOL dumpFlag) {
    if(VPP_DEBUG_UTIL_IS_FF_ENABLED_VALIDATE_BCMBUFF(vpp_obj)) {
        VPP_DEBUG_UTIL_isRawBcmBufferValid(start, size, pPrnPrefix, dumpFlag);
    }
}

#endif //VPP_DEBUG_UTIL_ENABLE_validateBcmBuffer

#ifdef VPP_DEBUG_UTIL_ENABLE_monitorBcmError
#define VPP_DEBUG_UTIL_BCM_ERROR_MONITOR_TIMEOUT_CNT    50
void VPP_DEBUG_UTIL_monitorBcmErrorStart(BCMBUF *pbcmbuf, const char *pPrnPrefix, BOOL dumpFlag) {
    VPP_DEBUG_GEN_MONITOR *pState=&VPPDBG_GMON_STATE;

    pState->bcmErrorMonitorCount = 0;
    //VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_BCM, "Monitor:%s: iter:%d\n", pPrnPrefix, pState->bcmErrorMonitorCount);
    //Validate the BCM buffer, before submiting to BCM
    VPP_DEBUG_UTIL_isBcmBufferValid(pbcmbuf, pPrnPrefix, dumpFlag);
}

void VPP_DEBUG_UTIL_monitorBcmError(BCMBUF *pbcmbuf, const char *pPrnPrefix, BOOL dumpFlag) {
    VPP_DEBUG_GEN_MONITOR *pState=&VPPDBG_GMON_STATE;
    UINT32 regVal=0, regValAddr, regValData, base_addr;
    BOOL timeoutErrFlag = 0;
    base_addr = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_BCM_REG_BASE;
    base_addr += RA_AVIO_BCM_ERROR;

    GA_REG_WORD32_READ(base_addr, &regVal);

    pState->bcmErrorMonitorCount++;
    timeoutErrFlag = (regVal) || ((pState->bcmErrorMonitorCount%VPP_DEBUG_UTIL_BCM_ERROR_MONITOR_TIMEOUT_CNT)==0);
    if((VPP_DEBUG_UTIL_BCMBUFF_DUMP_ENABLE == dumpFlag) || (timeoutErrFlag)) {
        //Log the error, if error occured or after timeout iteration
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_BCM, "Monitor:%s:%s: iter:%d\n", timeoutErrFlag?"Err":"Info",pPrnPrefix, pState->bcmErrorMonitorCount);
    }

    if(regVal) {
        GA_REG_WORD32_READ(base_addr+4, &regValAddr);
        GA_REG_WORD32_READ(base_addr+8, &regValData);

        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_BCM, "Monitor:%s: err:%x, addr:%x, val:%x\n", pPrnPrefix, regVal, regValAddr, regValData);
        if((VPP_DEBUG_UTIL_BCMBUFF_DUMP_ENABLE == dumpFlag)) {
            VPP_DEBUG_UTIL_bcmBufferDump(pbcmbuf, "MonBcmBuff");

            //Clear the error stick bit - Clean only after atleast one dump is done
            GA_REG_WORD32_WRITE(base_addr, 0x1);
        }
    }
}

void VPP_DEBUG_UTIL_resetDhubPtr(HDL_dhub2d *pDhubHandle, int dhubID) {
    unsigned int dhub_cnt, dhub_ptr;

    dhub_cnt = dhub_data_query(&(pDhubHandle->dhub), dhubID, 1, &dhub_ptr);
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_DHUB, "1:dhubID:%d, dhub_cnt:%d, dhub_ptr:%d\n", dhubID, dhub_cnt, dhub_ptr);
    if(dhub_ptr != 0) {
        dhub2d_channel_clear_seq(&(pDhubHandle->dhub), dhubID);
        dhub2d_channel_start_seq(&(pDhubHandle->dhub), dhubID);
        dhub_cnt = dhub_data_query(&(pDhubHandle->dhub), dhubID, 1, &dhub_ptr);
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_DHUB, "2:dhubID:%d, dhub_cnt:%d, dhub_ptr:%d\n", dhubID, dhub_cnt, dhub_ptr);
    }
}

#endif //VPP_DEBUG_UTIL_ENABLE_monitorBcmError

#ifdef VPP_DEBUG_UTIL_ENABLE_monitorResetCPCB
//Set the timeout to 2 second since watchdog will timeout in 5 sec.
#define VPP_DEBUG_UTIL_CPCB_RESET_MONITOR_TIMEOUT_CNT    2
void VPP_DEBUG_UTIL_monitorResetCPCBStart(VPP_OBJ *vpp_obj, const char *pPrnPrefix, BOOL dumpFlag) {
    VPP_DEBUG_GEN_MONITOR *pState=&VPPDBG_GMON_STATE;
    pState->resetCPCBMonitorCount = 0;
    if(VPP_DEBUG_UTIL_BCMBUFF_DUMP_ENABLE == dumpFlag) {
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_CPCB, "Monitor:%s: iter:%d\n", pPrnPrefix, pState->resetCPCBMonitorCount);
    }
}

void VPP_DEBUG_UTIL_monitorResetCPCB(VPP_OBJ *vpp_obj, const char *pPrnPrefix, BOOL dumpFlag) {
    VPP_DEBUG_GEN_MONITOR *pState=&VPPDBG_GMON_STATE;
    pState->resetCPCBMonitorCount++;
    if(((pState->resetCPCBMonitorCount%VPP_DEBUG_UTIL_BCM_ERROR_MONITOR_TIMEOUT_CNT)==0)) {
        if(VPP_DEBUG_UTIL_BCMBUFF_DUMP_ENABLE == dumpFlag) {
            VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_CPCB, "Monitor:%s: iter:%d\n", pPrnPrefix, pState->resetCPCBMonitorCount);
        }
    }
}
#endif //VPP_DEBUG_UTIL_ENABLE_monitorResetCPCB
