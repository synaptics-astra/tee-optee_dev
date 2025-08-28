/*******************************************************************************
 *                 2017 Synaptics Incorporated. All Rights Reserved            *
 * THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF Synaptics.                   *
 * NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT *
 * OF Synaptics OR ANY THIRD PARTY. Synaptics RESERVES THE RIGHT AT ITS SOLE   *
 * DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO Synaptics.  *
 * THIS CODE IS PROVIDED "AS IS". Synaptics MAKES NO WARRANTIES, EXPRESSED,    *
 * IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.  *
 *                                                                             *
 *******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
//! \file    factory_def.h
//! \brief   Factory TA debug macro
//! \author  Frank Ho
//! \date    Dec 2017
////////////////////////////////////////////////////////////////////////////////

#ifndef __FACTORY_DEF_H__
#define __FACTORY_DEF_H__
#include <stdio.h>

#define FACTORY_DEBUG_TA 1
#ifdef  FACTORY_DEBUG_TA
#define FACTORY_LOG printf
#define FACTORY_ASSERT DRM_ASSERT
#define FACTORY_ERR printf
#define FACTORY_TRACEHEX DRM_HEXDUMP
#else

#define FACTORY_LOG(...)
#define FACTORY_ASSERT(...)
#define FACTORY_ERR DRM_ERR
#define FACTORY_TRACEHEX(...)

#endif

#endif //__FACTORY_DEF_H__
