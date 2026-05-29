/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#ifndef USER_TA_HEADER_DEFINES_H
#define USER_TA_HEADER_DEFINES_H

/* To get the TA UUID definition */
#include <ovp_cmd.h>
#define TA_UUID				TAOVP_UUID

/*
 * TA properties: single-instance TA, no specific attribute
 * TA_FLAG_EXEC_DDR is meaningless but mandated.
 */
#define TA_FLAGS			(TA_FLAG_EXEC_DDR + TA_FLAG_CACHE_MAINTENANCE | TA_FLAG_SINGLE_INSTANCE | TA_FLAG_MULTI_SESSION)

/* Provisioned stack size */
#define TA_STACK_SIZE			(32 * 1024)

/* Provisioned heap size for TEE_Malloc() and friends */
#define TA_DATA_SIZE			(512 * 1024)

/* The gpd.ta.version property */
#define TA_VERSION	"1.0"

/* The gpd.ta.description property */
#define TA_DESCRIPTION	"OVP Trusted Application"

#endif /* USER_TA_HEADER_DEFINES_H */
