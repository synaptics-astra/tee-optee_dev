// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Synaptics Incorporated

#if !defined (PVR_HEADER_H)
#define PVR_HEADER_H

#if defined(DOLPHIN)
#include "rgxconfig_km_29.V.108.208.h"
#include "rgxcore_km_29.14.108.208.h"
#elif defined(VS550)
#include "rgxconfig_km_22.V.54.330.h"
#include "rgxcore_km_22.46.54.330.h"
#elif defined(PLATYPUS)
#include "rgxconfig_km_29.V.52.202.h"
#include "rgxcore_km_29.19.52.202.h"
#else
#error "Unspport chip type, please check and add a new one!"
#endif

#define IMG_EXPLICIT_INCLUDE_HWDEFS 1
#include "rgx_cr_defs_km.h"
#undef IMG_EXPLICIT_INCLUDE_HWDEFS

#ifndef CONFIG_OPTEE
#include "tee_log.h"
#endif
#endif
