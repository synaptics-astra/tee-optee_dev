/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_devkey.h
//! \brief      VPP device security key generation header file
//! \author     Dharini
//-----------------------------------------------------------------------------

#ifndef _VPP_DEVKEY_H_
#define _VPP_DEVKEY_H_

#ifdef CONFIG_VPP_DEV_KEY_SUPPORT

#define VPP_DEVKEY_CMD_SIZE     (48)
#define VPP_DEVKEY_SIZE         (16)

int VPP_DEVKEY_AllocMem (uintptr_t *pShmHandle, unsigned char **ppPhyAddr);
int VPP_DEVKEY_FreeMem (uintptr_t shmHandle);

int VPP_DEVKEY_Generate(unsigned char *pPhyAddr);


#endif // CONFIG_VPP_DEV_KEY_SUPPORT
#endif // _VPP_DEVKEY_H_
