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

////////////////////////////////////////////////////////////////////////////////
//! \file ptm_ta.h
//!
//! \brief PTM TA CMD defines
//!
//!
//!     Version    Date                     Author
//!     V 1.00,    JUL 2019,                Zhwan
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef __PTMCMD_H__
#define __PTMCMD_H__

#define TA_PTM_UUID {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x2d}}

enum {
        PTM_INVALID,
        PTM_DEBUG,
        PTM_CONSTRUCT_PT,
        PTM_FREE_PT,
        PTM_REF_PT,
        PTM_UNREF_PT,
        PTM_QUERY_PT,
        PTM_DUMP_PT,
        PTM_CMD_MAX
};

#endif /* __PTMCMD_H__ */
