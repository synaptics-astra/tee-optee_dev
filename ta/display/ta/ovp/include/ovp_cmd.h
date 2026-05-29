/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#ifndef __OVP_CMD_H__
#define __OVP_CMD_H__

#define TAOVP_UUID {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x03, 0xf4}}

/* enum for VPP commands */
typedef enum {
	OVP_CREATE,
	OVP_DESTROY,
	OVP_GETFRAMESWAIT,
	OVP_PASSSHM,
	OVP_GET_CLR_INTR_STS,
	OVP_SUSPEND,
	OVP_RESUME,
}OVP_CMD_ID;

/* enum for VPP callbacks */
typedef enum {
	OVP_CB_MALLOCSHM,
	OVP_CB_FREESHM
}OVP_CB_ID;

/* enum for VPP shm ID */
typedef enum {
	OVP_SHM_PUSHFRAME,
	OVP_SHM_PROCESSFRAME,
	OVP_SHM_RELEASEBUFF
}OVP_SHM_ID;


#endif /* __OVP_CMD_H__ */

