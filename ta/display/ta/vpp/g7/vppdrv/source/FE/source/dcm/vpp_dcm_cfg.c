/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_DCM_CFG_C_
#define _VPP_DCM_CFG_C_

#ifdef CONFIG_AMP_DCM
#include "Galois_memmap.h"
#include "vpp_mp.h"
#include "vpp_gfx_vop.h"
#include "vpp_module.h"
#include "vpp_fe_dcm_common.h"

/*
   ***************** Register Addr Mapping ******************
   * dcm_base_addr_mp = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_VPP_MP_REG_BASE
   * dcm_base_addr_gfx = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_VPP_GFX_VOP_REG_BASE
   *
   * vdcm_base = RA_VPP_MP_VDCM
   * gdcm_base_gfx0 = RA_VPP_GFX_VOP_VPP_GDCM_GFX0
   * gdcm_base_gfx1 = RA_VPP_GFX_VOP_VPP_GDCM_GFX1
   *
   * vdcm_lutbase = RA_VPP_MP_HDRLUT
   * gdcm_lutbase_gfx0 = RA_VPP_GFX_VOP_HDRLUT_GFX0
   * gdcm_lutbase_gfx1 = RA_VPP_GFX_VOP_HDRLUT_GFX1
   *
   *
 */

/*
 * VDCM IY2R and VDCM/GDCM CY2R CSC's are of type CSC_C15O18F12
 */
unsigned int DCM_CSC_C15O18F12_BASE(VPP_OBJ *vpp_obj, int planeID, int csc_type)
{
    unsigned int base_addr = 0;

    if (planeID == PLANE_MAIN) {
        base_addr = vpp_obj->dcm_base_addr_mp + RA_VPP_MP_VDCM;
        base_addr += (csc_type == DCM_CSC_TYPE_CMAP)? RA_VDCM_CY2R: RA_VDCM_IY2R;
    }
#ifdef ENABLE_PIP_GFX0_PATH
    else if ((planeID == PLANE_PIP) && (csc_type == DCM_CSC_TYPE_CMAP)) {
        base_addr = vpp_obj->dcm_base_addr_gfx + RA_VPP_GFX_VOP_VPP_GDCM_GFX0 + RA_GDCM_CY2R;
    }
#endif
    else if ((planeID == PLANE_GFX1) && (csc_type == DCM_CSC_TYPE_CMAP)) {
        base_addr = vpp_obj->dcm_base_addr_gfx + RA_VPP_GFX_VOP_VPP_GDCM_GFX1 + RA_GDCM_CY2R;
    }
    return base_addr;
}

/*
 * VDCM OR2Y, GDCM IR2Y and VDCM/GDCM CR2Y CSC's are of type CSC_C13O18F12
 */
unsigned int DCM_CSC_C13O18F12_BASE(VPP_OBJ *vpp_obj, int planeID, int csc_type)
{
    unsigned int base_addr = 0;

    if (planeID == PLANE_MAIN) {
        base_addr = vpp_obj->dcm_base_addr_mp + RA_VPP_MP_VDCM;
        base_addr += (csc_type == DCM_CSC_TYPE_CMAP)? RA_VDCM_CR2Y: RA_VDCM_OR2Y;
    }
#ifdef ENABLE_PIP_GFX0_PATH
    else if (planeID == PLANE_PIP) {
        base_addr = vpp_obj->dcm_base_addr_gfx + RA_VPP_GFX_VOP_VPP_GDCM_GFX0;
        base_addr += (csc_type == DCM_CSC_TYPE_CMAP)? RA_GDCM_CR2Y: RA_GDCM_IR2Y;
    }
#endif
    else if (planeID == PLANE_GFX1) {
        base_addr = vpp_obj->dcm_base_addr_gfx + RA_VPP_GFX_VOP_VPP_GDCM_GFX1;
        base_addr += (csc_type == DCM_CSC_TYPE_CMAP)? RA_GDCM_CR2Y: RA_GDCM_IR2Y;
    }
    return base_addr;
}

unsigned int DCM_REG_BASE(VPP_OBJ *vpp_obj, int planeID)
{
    unsigned int base_addr = 0;

    if (planeID == PLANE_MAIN) {
        base_addr = vpp_obj->dcm_base_addr_mp + RA_VPP_MP_VDCM;
    }
#ifdef ENABLE_PIP_GFX0_PATH
    else if (planeID == PLANE_PIP) {
        base_addr = vpp_obj->dcm_base_addr_gfx + RA_VPP_GFX_VOP_VPP_GDCM_GFX0;
    }
#endif
    else if (planeID == PLANE_GFX1) {
        base_addr = vpp_obj->dcm_base_addr_gfx + RA_VPP_GFX_VOP_VPP_GDCM_GFX1;
    }
    return base_addr;
}

unsigned int DCM_LUT_BASE(VPP_OBJ *vpp_obj, int planeID)
{
    unsigned int base_addr = 0;

    if (planeID == PLANE_MAIN) {
        base_addr = vpp_obj->dcm_base_addr_mp + RA_VPP_MP_HDRLUT;
    }
#ifdef ENABLE_PIP_GFX0_PATH
    else if (planeID == PLANE_PIP) {
        base_addr = vpp_obj->dcm_base_addr_gfx + RA_VPP_GFX_VOP_HDRLUT_GFX0;
    }
#endif
    else if (planeID == PLANE_GFX1) {
        base_addr = vpp_obj->dcm_base_addr_gfx + RA_VPP_GFX_VOP_HDRLUT_GFX1;
    }
    return base_addr;
}
#endif
#endif
