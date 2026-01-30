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
//! \file drm_sheme.h
//! \brief Header file for DRM Scheme
///////////////////////////////////////////////////////////////////////////////

#ifndef __DRM_SCHEME_H__
#define __DRM_SCHEME_H__

typedef enum
{
    DRMScheme_INVALID,    //Invalid type
    DRMScheme_HDCP2X,     //HDCP 2.0
    DRMScheme_DTCPIP,     //DTCP on IP
    DRMScheme_CIPLUS,     //CI+
    DRMScheme_WIDEVINE,   //WideVine
    DRMScheme_PLAYREADY,  //WideVine
    DRMScheme_VUDU,       //VUDU
    DRMScheme_VRMTX,      //VeriMatrix
    DRMScheme_NDS,        //NDS
    DRMScheme_MLA,        /**< MARLIN Audio */
    DRMScheme_MOBITV,     //MOBITV
    DRMScheme_PVR,        //PVR
    DRMScheme_DUMMY,      //DUMMY
    DRMScheme_STUB,       //STUB
    DRMScheme_PVR_GENERAL,       //PVR GENERAL
    DRMScheme_MARLIN,     //MARLIN
    DRMScheme_RAW,
    DRMScheme_NAGRA,      //Nagra
    DRMScheme_CENC3,
    DRMScheme_ALTICAST,
    DRMScheme_IRDETO,
    DRMScheme_GKL,
    DRMScheme_MAX = 100,
} MV_DRMSchemeType;

#endif
