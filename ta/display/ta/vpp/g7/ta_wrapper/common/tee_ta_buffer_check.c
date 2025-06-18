/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

////////////////////////////////////////////////////////////////////////////////
//! \file tee_ta_buffer_check.c
//!
//! \brief Implementation of buffer validation/checking functions for TrustZone.
//!
//! Purpose: This is used for check/validate access permission of various
//!               buffers shared from CA to TA (i/p & o/p)
//!
//!     Version    Date                     Author
//!     V 1.00,    Aug 24 2022,             Shanmugam Ramachandran
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include "tee_ta_buffer_check.h"
#include "vpp_module.h"

TEE_Result VPP_ValidateInput(VPP_SHM_BUFFER_DESC *pBuffDec, bool outputToNonsecure)
{
    TEE_Result res = TEE_ERROR_BAD_PARAMETERS;
    TEE_MemRegion mr;

    if(TEE_SUCCESS != TEE_FindMemRegion(&mr, pBuffDec->bufAddr, pBuffDec->bufSize))
        return res;
    if (TEE_MR_IS_RESTRICTED(&mr))
        return res;
    if (outputToNonsecure && TEE_MR_IS_SECURE(&mr))
        return res;
    return TEE_SUCCESS;
}

TEE_Result VPP_ValidateOutput(VPP_SHM_BUFFER_DESC *pBuffDec, bool inputFromSecure)
{
    TEE_Result res = TEE_ERROR_BAD_PARAMETERS;
    TEE_MemRegion mr;

    if(TEE_SUCCESS != TEE_FindMemRegion(&mr, pBuffDec->bufAddr, pBuffDec->bufSize))
        return res;
    if (TEE_MR_IS_RESTRICTED(&mr))
        return res;
    if (inputFromSecure && TEE_MR_IS_NONSECURE(&mr))
        return res;
    return TEE_SUCCESS;
}

TEE_Result VPP_Validate_SingleBuffer(VPP_SHM_BUFFER_DESC *pBuffDec)
{
    TEE_Result res = TEE_SUCCESS;

    if (pBuffDec->bufType & VPP_SHM_BUFFER_TYPE_INPUT ||
        pBuffDec->bufType & VPP_SHM_BUFFER_TYPE_SECURE_INPUT) {

        res = VPP_ValidateInput(pBuffDec, 0);

        //If secure, then check fruther input rights
        if (res == TEE_SUCCESS && (pBuffDec->bufType & VPP_SHM_BUFFER_TYPE_SECURE_INPUT)) {
            res = TEE_CheckMemoryInputRights(pBuffDec->bufAddr, pBuffDec->bufSize, TEE_SECURE, TEE_MT_COPY);
        }
    }

    if (res == TEE_SUCCESS && (pBuffDec->bufType & VPP_SHM_BUFFER_TYPE_OUTPUT)) {
        res = VPP_ValidateOutput(pBuffDec, 0);
    }

    if (res == TEE_SUCCESS && (pBuffDec->bufType & VPP_SHM_BUFFER_TYPE_NON_SECURE_OUTPUT)) {
        res = TEE_CheckMemoryOutputRights(pBuffDec->bufAddr, pBuffDec->bufSize, TEE_NONSECURE, TEE_MT_COPY);
    }

    return res;
}

TEE_Result VPP_MapAndValidate_SingleBuffer(VPP_BUFF_HANDLE *pBuffHandle, VPP_SHM_BUFFER_DESC *pBuffDesc)
{
    UINT32 shmSize;
#ifdef VPP_ENABLE_MEMID_BY_VIRT_ADDR
    const UINT8 bIsMapToAllInstances = 0;
#else
    const UINT8 bIsMapToAllInstances = 1;
#endif
    const UINT8 bIsPhyAddrNeeded = 1;

    //memid/phy-addr is passed in pBuffDesc->bufAddr
    shmSize = VPP_SHM_ALIGN_ROUNDUP(pBuffDesc->bufSize);

    VPP_BUFF_GetHandle(pBuffHandle, (UINT32)pBuffDesc->bufAddr, shmSize, bIsMapToAllInstances, bIsPhyAddrNeeded);

    //Retrieve the value from VPP_BUFF_HANDLE
#ifdef VPP_ENABLE_MEMID_BY_VIRT_ADDR
    pBuffDesc->bufAddr = pBuffHandle->virt_addr;
#else
    //If continue to use physical address, then override virtual with physical address
    pBuffDesc->bufAddr = pBuffHandle->phy_addr;
#endif //VPP_ENABLE_MEMID_BY_VIRT_ADDR

    return VPP_Validate_SingleBuffer(pBuffDesc);
}
