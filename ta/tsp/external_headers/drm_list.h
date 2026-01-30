/*
 * INTERNAL USE ONLY
 *
 * Unpublished Work Copyright (C) 2013-2017 Synaptics Incorporated.
 * All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 */

///////////////////////////////////////////////////////////////////////////////
//! \file drm_list.h
//! \brief Header file for DRM List
///////////////////////////////////////////////////////////////////////////////

#ifndef __DRM_LIST_INCLUDE_H__
#define __DRM_LIST_INCLUDE_H__

#include "drm_types.h"

typedef enum MV_DRMListStatus
{
    DRMLIST_INVALID,
    DRMLIST_EMPTY,
    DRMLIST_NORMAL,
} MV_DRMListStatus;

typedef struct MV_DRMListNode MV_DRMListNode;
struct MV_DRMListNode
{
    MV_DRMListNode *next;
    MV_DRMListNode *prev;
    UINT32 ucount;
};

typedef MV_DRMListNode MV_DRMListHead;

HRESULT MV_DRMListInit(MV_DRMListHead * t);
UINT32 MV_DRMListIsEmpty(MV_DRMListHead * t);
HRESULT MV_DRMListAdd(MV_DRMListHead * t, MV_DRMListNode * o_node);
HRESULT MV_DRMListDelete (MV_DRMListHead *t, MV_DRMListNode * o_node);
HRESULT MV_DRMListGetFirstNode(MV_DRMListHead * t, MV_DRMListNode ** o_curnode);
HRESULT MV_DRMListGetNextNode(MV_DRMListHead * t, MV_DRMListNode ** o_curnode);
#endif
