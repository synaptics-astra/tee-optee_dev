/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#include "vpp_module.h"
#include "vpp_debug.h"
#include "vpp_debug_util_data.h"

#define VPP_SW_BRINUP_IRQ_DEBUG
#define VPP_SW_BRINGUP_REGDUMP_ALL

extern const VPP_REG_INFO vpp_reg_table[];
extern const VPP_REG_INFO vpp_mp_reg_table[];
extern const VPP_REG_INFO vpp_gfx_vop_reg_table[];
extern const VPP_REG_INFO vpp_hdrx_pipe_reg_table[];
extern const VPP_REG_INFO mtr_reg_table[];

void printfVBInfo(VBUF_INFO *pVBInfo) {
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_pbuf_start   =   %p\n",  pVBInfo->m_pbuf_start   );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_buf_size     =   %d\n",  pVBInfo->m_buf_size     );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_srcfmt       =   %d\n",  pVBInfo->m_srcfmt   );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_order        =   %d\n",  pVBInfo->m_order    );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_bytes_per_pixel  =   %d\n",  pVBInfo->m_bytes_per_pixel  );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_content_offset   =   %d\n",  pVBInfo->m_content_offset   );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_content_width    =   %d\n",  pVBInfo->m_content_width    );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_content_height   =   %d\n",  pVBInfo->m_content_height   );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_buf_stride   =   %d\n",  pVBInfo->m_buf_stride   );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_active_left  =   %d\n",  pVBInfo->m_active_left  );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_active_top   =   %d\n",  pVBInfo->m_active_top   );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_active_width     =   %d\n",  pVBInfo->m_active_width     );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_active_height    =   %d\n",  pVBInfo->m_active_height    );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_disp_offset      =   %d\n",  pVBInfo->m_disp_offset  );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_is_frame_seq     =   %d\n",  pVBInfo->m_is_frame_seq     );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_is_top_field_first   =   %d\n",  pVBInfo->m_is_top_field_first   );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_is_repeat_first_field    =   %d\n",  pVBInfo->m_is_repeat_first_field    );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_is_progressive_pic   =   %d\n",  pVBInfo->m_is_progressive_pic   );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_pixel_aspect_ratio   =   %d\n",  pVBInfo->m_pixel_aspect_ratio   );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_frame_rate_num   =   %d\n",  pVBInfo->m_frame_rate_num   );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_frame_rate_den   =   %d\n",  pVBInfo->m_frame_rate_den   );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_pan_left[0]      =   %d\n",  pVBInfo->m_pan_left[0]  );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_pan_top[0]       =   %d\n",  pVBInfo->m_pan_top[0]   );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_pan_width[0]     =   %d\n",  pVBInfo->m_pan_width[0]     );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_pan_height[0]    =   %d\n",  pVBInfo->m_pan_height[0]    );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_pan_left[1]      =   %d\n",  pVBInfo->m_pan_left[1]  );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_pan_top[1]       =   %d\n",  pVBInfo->m_pan_top[1]   );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_pan_width[1]     =   %d\n",  pVBInfo->m_pan_width[1]     );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_pan_height[1]    =   %d\n",  pVBInfo->m_pan_height[1]    );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_clut_start_index =   %d\n",  pVBInfo->m_clut_start_index     );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_clut_num_items   =   %d\n",  pVBInfo->m_clut_num_items   );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_is_right_eye_view_valid  =   %d\n",  pVBInfo->m_is_right_eye_view_valid  );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_right_eye_view_buf_start =   %d\n",  pVBInfo->m_right_eye_view_buf_start     );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_3D_source_fmt            =   %d\n",  pVBInfo->m_3D_source_fmt    );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_3D_interpretation_type   =   %d\n",  pVBInfo->m_3D_interpretation_type   );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_3D_SBS_sampling_mode     =   %d\n",  pVBInfo->m_3D_SBS_sampling_mode     );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_3D_convert_mode_from_SEI =   %d\n",  pVBInfo->m_3D_convert_mode_from_SEI     );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_3D_SwitchLeftRightView   =   %d\n",  pVBInfo->m_3D_SwitchLeftRightView   );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_bits_per_pixel       =   %d\n",  pVBInfo->m_bits_per_pixel   );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_buf_pbuf_start_UV    =   %d\n",  pVBInfo->m_buf_pbuf_start_UV    );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_buf_stride_UV        =   %d\n",  pVBInfo->m_buf_stride_UV    );
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "    pVBInfo->m_flags                =   %d\n",  pVBInfo->m_flags    );
}

UINT32 VPP_DEBUG_DumpRegs(UINT32 base_addr, UINT32 sel)
{
    UINT32 idx = 0;
    UINT32 DelayTime = 1;
    UINT32 regVal = 0;

    VPP_REG_INFO_GROUP vpp_reg_info_group[] = {
        {vpp_reg_table, MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_VPP_REG_BASE, \
            VPP_DEBUG_GetDumpSize(VPP_LOG_REG)/sizeof(VPP_REG_INFO), VPP_DEBUG_UTIL_PP_2_VPP_VPP},
        {vpp_mp_reg_table, MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_VPP_MP_REG_BASE, \
            VPP_DEBUG_GetDumpSize(VPP_LOG_MP_REG)/sizeof(VPP_REG_INFO), VPP_DEBUG_UTIL_PP_2_VPP_MP},
        {vpp_gfx_vop_reg_table, MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_VPP_GFX_VOP_REG_BASE, \
            VPP_DEBUG_GetDumpSize(VPP_LOG_GFX_VOP_REG)/sizeof(VPP_REG_INFO), VPP_DEBUG_UTIL_PP_2_VPP_GFX},
        {vpp_hdrx_pipe_reg_table, MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE, \
            VPP_DEBUG_GetDumpSize(VPP_LOG_HDRX_PIPE_REG)/sizeof(VPP_REG_INFO), VPP_DEBUG_UTIL_PP_2_VPP_HDRX},
        {mtr_reg_table, MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_GBL_BASE + RA_avioGbl_MEMMAP_MTR, \
            VPP_DEBUG_GetDumpSize(VPP_LOG_MTR_REG)/sizeof(VPP_REG_INFO), VPP_DEBUG_UTIL_PP_2_VPP_MTR}
    };

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "@@@@@@@ VPP REG DUMP START SEL = 0x%02X @@@@@@@\n", sel);
    for (idx=0; sel; idx++) {
        UINT32 mask = 1<<idx;
        if (sel & mask) {
            UINT32 tidx = 0;
            for(tidx=0; tidx<vpp_reg_info_group[idx].regSize; tidx++) {
                GA_REG_WORD32_READ(vpp_reg_info_group[idx].regBase + vpp_reg_info_group[idx].pRegInfo[tidx].addr, &regVal);
                VPP_DEBUG_UTIL_PRINTF(vpp_reg_info_group[idx].prefix, "%s 0x%X 0x%X\n\r", \
                        vpp_reg_info_group[idx].pRegInfo[tidx].addr_name, \
                        vpp_reg_info_group[idx].regBase + vpp_reg_info_group[idx].pRegInfo[tidx].addr, regVal);
                if((tidx %10) == 0) {
                    MV_OSAL_Task_Sleep(DelayTime);
                }
            }
            sel &= ~mask;
        }
    }
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_2_VPP, "@@@@@@@ VPP REG DUMP END @@@@@@@\n");
    return 0;
}

int MV_VPPOBJ_DumpRegs(int handle)
{
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;
    UINT32 sel=0xF;

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

    VPP_DEBUG_DumpRegs(vpp_obj->base_addr, sel);
    return 0;
}


