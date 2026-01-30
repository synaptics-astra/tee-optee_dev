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



#ifndef __DRM_COMMON_TYPE_H__
#define __DRM_COMMON_TYPE_H__

typedef char                CHAR;
typedef unsigned char       UCHAR;
typedef short               SHORT;
typedef unsigned short      USHORT;
typedef int                 INT;
typedef unsigned int        UINT;
typedef long long           LONGLONG;
typedef unsigned long long  ULONGLONG;
typedef void                VOID;
typedef void*               PTR;
typedef void**              PHANDLE;
typedef void*               HANDLE;
typedef void*               PVOID;
typedef UCHAR               BYTE;
typedef UCHAR               BOOL;

#ifndef _typedef___INT8
#define _typedef___INT8
typedef CHAR                INT8;
#endif /* _typedef___INT8 */
#ifndef _typedef___UINT8
#define _typedef___UINT8
typedef UCHAR               UINT8;
#endif /* _typedef___UINT8 */
#ifndef _typedef___INT16
#define _typedef___INT16
typedef short               INT16;
#endif /* _typedef___INT16 */
#ifndef _typedef___UINT16
#define _typedef___UINT16
typedef unsigned short      UINT16;
#endif /* _typedef___UINT16 */
#ifndef _typedef___INT32
#define _typedef___INT32
typedef int                 INT32;
#endif /* _typedef___INT32 */
#ifndef _typedef___UINT32
#define _typedef___UINT32
typedef unsigned int        UINT32;
#endif /* _typedef___UINT32 */

#ifndef _typedef___INT64
#define _typedef___INT64
typedef long long           INT64;
#endif

#ifndef _typedef___UINT64
#define _typedef___UINT64
typedef unsigned long long  UINT64;
#endif

typedef unsigned int        SIZE_T;

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef _typedef___HRESULT
#define _typedef___HRESULT
typedef signed int          HRESULT;
#endif

#define TEE2H(r) r

typedef struct
{
    UINT    Data1;
    UINT16  Data2;
    UINT16  Data3;
    UCHAR   Data4[8];
} GUID;


#define E_GENERIC_BASE      (0xF0000000)
// generic error code macro
#define E_GEN_ERR(code)     (E_GENERIC_BASE | ( code & 0x0000FFFF))


#define S_OK                0x00000000 // Success

#define E_FAIL              E_GEN_ERR( 0x0001 ) // Unspecified failure
#define E_INVALIDARG        E_GEN_ERR( 0x0002 ) // No such interface supported
#define E_OUTOFMEMORY       E_GEN_ERR( 0x0003 ) // Failed to allocate necessary memory
#define E_NOTIMPL           E_GEN_ERR( 0x0004 ) // Not implemented
#define E_NOINTERFACE       E_GEN_ERR( 0x0005 ) // No such interface supported
#define E_UNEXPECTED        E_GEN_ERR( 0x0006 ) // Unexpected failure
#define E_ACCESSDENIED      E_GEN_ERR( 0x0007 ) // General access denied error
#define E_HANDLE            E_GEN_ERR( 0x0008 ) // Handle that is not valid

#define E_BADVALUE          E_GEN_ERR( 0x0009 ) // The value is illegal (general)
#define E_OUTOFRANGE        E_GEN_ERR( 0x000A ) // The value is out of range
#define E_TIMEOUT           E_GEN_ERR( 0x000B ) // Operation time Out
#define E_FULL              E_GEN_ERR( 0x000C ) // Item is full (Queue or table etc...)
#define E_EMPTY             E_GEN_ERR( 0x000D ) // Item is empty (Queue or table etc...)
#define E_NOTREADY          E_GEN_ERR( 0x000E ) // Operation or item is not ready yet
#define E_ALREADYEXIST      E_GEN_ERR( 0x000F ) // Tried to create existing item
#define E_NOTEXIST          E_GEN_ERR( 0x0010 ) // Tried to operate not existing item
#define E_FILEOPEN          E_GEN_ERR( 0x0011 ) // open file failed
#define E_FILEWRITE         E_GEN_ERR( 0x0012 ) // write file failed
#define E_FILEREAD          E_GEN_ERR( 0x0013 ) // read file failed
#define E_FILESEEK          E_GEN_ERR( 0x0014 ) // seek file failed
#define E_FILECLOSE         E_GEN_ERR( 0x0015 ) // close file failed

#endif  // __COMMON_TYPE__
