/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "vpp_module.h"
#include "vpp_be.h"
#include "global.h"

void VPP_GlobalIntr_Config(VPP_OBJ *vpp_obj, int cpcbID)
{
    T32Vpp_VPP_INTR_EN vpp_int_en;
    UINT32 RegAddr;

    RegAddr = vpp_obj->base_addr + RA_Vpp_VPP_INTR_EN;

    GA_REG_WORD32_READ(RegAddr, &vpp_int_en.u32);
    if(IS_VOUT_DISP_MODE(SINGLE_MODE_SEC)) {
#ifdef ENABLE_GFX2_VOP2_PATH
        vpp_int_en.uVPP_INTR_EN_vppOUT6 = 1; //DSI only
        vpp_int_en.uVPP_INTR_EN_vppVBI =  0;
#else
        vpp_int_en.uVPP_INTR_EN_vppVBI =  1;
#endif
        vpp_int_en.uVPP_INTR_EN_vppVDE0 = 0;
        vpp_int_en.uVPP_INTR_EN_vppOUT0 = 0;
    }
    else if(IS_VOUT_DISP_MODE(SINGLE_MODE_PRI)){
        vpp_int_en.uVPP_INTR_EN_vppOUT6 = 0; //HDMI only
        vpp_int_en.uVPP_INTR_EN_vppVBI =  1;
        vpp_int_en.uVPP_INTR_EN_vppVDE0 = 1;
        vpp_int_en.uVPP_INTR_EN_vppOUT0 = 1;
    }
    else {
        vpp_int_en.uVPP_INTR_EN_vppOUT6 = 1; //DSI + HDMI
        vpp_int_en.uVPP_INTR_EN_vppVBI =  1;
        vpp_int_en.uVPP_INTR_EN_vppVDE0 = 1;
        vpp_int_en.uVPP_INTR_EN_vppOUT0 = 1;
    }
    GA_REG_WORD32_WRITE(RegAddr, vpp_int_en.u32);
    VPPAPI_DBG_INFO("Interrupt: Val: 0x%x\r\n", vpp_int_en.u32);
}

void VPP_DTGUF_Handler(VPP_OBJ *vpp_obj, int cpcbID)
{
    UINT32 addr, regval;
    T32DTG_status stDtgStatus;

    stDtgStatus.u32 = 0;
    if((cpcbID == CPCB_1) && (!IS_VOUT_DISP_MODE(SINGLE_MODE_SEC)))
    {
        addr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_DTG;
        GA_REG_WORD32_READ(addr + RA_DTG_status, &stDtgStatus.u32);
        /*clear underflow status here*/
        if(stDtgStatus.ustatus_underflow) {
            VPPAPI_DBG_FIELD("VPPDBG:UF[%d]: Count:%d vbinum[%d] \n", cpcbID,
                    vpp_obj->uiUfCount[cpcbID]++, vpp_obj->dv[cpcbID].vbi_num);
            GA_REG_WORD32_READ(addr + RA_DTG_CTRL0, &regval);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (addr + RA_DTG_CTRL0), regval&0xFFFFFFFC);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (addr + RA_DTG_CTRL0), regval|0x3);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (addr + RA_DTG_CTRL0), regval&0xFFFFFFFC);
        }
    }
#ifdef ENABLE_GFX2_VOP2_PATH
    else
    {
        addr = MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_VPP_GFX_VOP_REG_BASE+RA_VPP_GFX_VOP_VOP2+RA_VOP2_DTG;
        GA_REG_WORD32_READ(addr + RA_DTG_status, &stDtgStatus.u32);
        if(stDtgStatus.ustatus_underflow) {
            GA_REG_WORD32_READ(addr + RA_DTG_CTRL0,&regval);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,addr+RA_DTG_CTRL0, regval&0xFFFFFFFC);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,addr+RA_DTG_CTRL0, regval|0x3);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,addr+RA_DTG_CTRL0, regval|0x3);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,addr+RA_DTG_CTRL0, regval|0x3);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,addr+RA_DTG_CTRL0, regval&0xFFFFFFFC);
        }
    }
#endif

#ifdef ENABLE_DHUB_CLEAR_UPON_UF
    int i;
    int chanID, planeID;
    DV *pDV;
    PLANE *plane;

    pDV = &(vpp_obj->dv[cpcbID]);
    if(pDV->status == STATUS_ACTIVE) {
        /*check for atleast one completed dhub command for valid underflow status*/
        for(i = 0; i < pDV->num_of_chans; i++) {
            chanID = pDV->chanID[i];
            if (vpp_obj->chan[chanID].status == STATUS_ACTIVE) {
                planeID = PLANE_OF_CHAN(vpp_obj, chanID);
                plane = &vpp_obj->plane[planeID];
                /*dhub command is execute and completed when frame becomes prev_frame*/
                if(plane->prev_curr_frame != NULL) {
                    vpp_obj->is_uf_triggered[cpcbID] = stDtgStatus.ustatus_underflow;
                    break;
                }
            }
        }
    }
#endif /* #ifdef ENABLE_DHUB_CLEAR_UPON_UF */
}

void VPP_AVIO_Reset(void)
{
    TGbl_avioReset reg;

    reg.uavioReset_SyncReset = 1;
    GA_REG_WORD32_WRITE((MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_avioReset), reg.u32[0]);
    reg.uavioReset_SyncReset = 0;
    GA_REG_WORD32_WRITE((MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_avioReset), reg.u32[0]);
}

