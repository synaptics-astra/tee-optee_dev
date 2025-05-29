/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef __VPP_BE_HDCP_PRV_H__
#define __VPP_BE_HDCP_PRV_H__

#include "com_type.h"

/*-----------------------------------------------------------------------------
 * Macros and Constants
 *-----------------------------------------------------------------------------
 */

#ifdef VPP_DEBUG
#define HDCP_RETCODE_CHK(RetCode) \
    if (RetCode != HDCP_ERR_OK) \
    { \
        VPPHDMI_DBG_ERROR(("Hdcp error: %d\n", RetCode)); \
        return RetCode;\
    }
#else
#define HDCP_RETCODE_CHK(RetCode) \
    if (RetCode != HDCP_ERR_OK) \
        return RetCode;
#endif

#define IS_BIT_SET(Num,Pos) (((Num) & (1 << Pos)) == (1 << Pos))
/*-----------------------------------------------------------------------------
 * TypeDefs
 *-----------------------------------------------------------------------------
 */
// HDCP
typedef enum tagHDCP_CMD
{
    HDCP_CMD_START_AUTH,
    HDCP_CMD_STOP_AUTH,
    HDCP_CMD_MAX
}HDCP_CMD;

/*-----------------------------------------------------------------------------
 * Function Prototypes
 *-----------------------------------------------------------------------------
 */

#endif // __VPP_BE_HDCP_PRV_H__
