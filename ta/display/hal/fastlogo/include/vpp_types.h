/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
////////////////////////////////////////////////////////////////////////////////
//! \file vpp_types.h
//!
//! \brief  typedef specific to VPP/Fastlogo TA
//!
//! Purpose: - This file has types related to VPP/Fastlogo TA.
//!
//!
//! Version, Date and Author :
//!   V 1.00,    Aug 04 2022,   Prem Anand
//!
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef __VPP_TYPES_H__
#define __VPP_TYPES_H__

#ifndef CONFIG_OPTEE
typedef UINT32 ARCH_PTR_TYPE;
#else
typedef void * ARCH_PTR_TYPE;
#endif

// HDR10+
#define MAX_NUM_DISTRIBUTIONS 9
#define MAX_NUM_BEZIER_CURVE_ANCHORS 9

typedef INT32   ENUM_TYPE;

#endif /* __VPP_TYPES_H__ */
