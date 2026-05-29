/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#ifndef __OVP_INTERNAL_H__
#define __OVP_INTERNAL_H__
#include "com_type.h"

#define MAX_FRAMES_PENDING  10
#define MAX_OVP_OBJECTS     4

typedef struct OVP_FRAMEDESC_LIST_s
{
	UCHAR	m_uchClientId;
	void	*m_pbufInFrameDescr;
	struct OVP_FRAMEDESC_LIST_s *pNext;
	struct OVP_FRAMEDESC_LIST_s *pPrev;
}OVP_FRAMEDESC_LIST;

#endif


