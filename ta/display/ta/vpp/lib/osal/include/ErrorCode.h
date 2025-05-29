/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


////////////////////////////////////////////////////////////////////////////////
//! \file ErrorCode.h
//! \brief Error Code for Galois System
//!
//!
//! Purpose:
//!	   Shared the Error code of the Galois System
//!
//!	Version, Date and Author :
//!		V 1.00,	        	FengLei Wang
//!		V 1.10,	5/28/2007,	Fang Bao	, add some more error codes
//!		V 1.11,	5/29/2007,	Fang Bao	, add the S_FALSE comment to avoid confusing
//!		V 1.12,	6/05/2008,	Fang Bao	, remove error code E_NOPERMISSION and replace E_NOPERMISSION by E_ACCESSDENIED
//!
//!
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef _ERRORCODE_H_
#define _ERRORCODE_H_

#include "com_type.h"

#ifndef WIN32
#define SUCCEEDED(hr)		((HRESULT)(hr) >= 0)
#define FAILED(hr)			((HRESULT)(hr) < 0)
#endif

#define E_SUC				( 0x00000000 )
#define E_ERR				( 0x80000000 )

// error code base list
#define ERRORCODE_BASE		(0)
#define E_GENERIC_BASE		( 0x0000 << 16 )
#define E_SYSTEM_BASE		( 0x0001 << 16 )
#define E_DEBUG_BASE		( 0x0002 << 16 )


#define E_WMDRM_BASE		( 0x0004 << 16 )
#define E_AUDIO_BASE		( 0x0010 << 16 )
#define E_VIDEO_BASE		( 0x0020 << 16 )
#define E_SUB_BASE			( 0x0030 << 16 )	// Generic subtitle
#define E_DEMUX_BASE		( 0x0040 << 16 )
#define E_BD_BASE			( 0x0050 << 16 )
#define E_HDDVD_BASE		( 0x0060 << 16 )	// HDDVD: Standard Content
#define E_HDI_BASE			( 0x0070 << 16 )	// HDDVD: Advanced Content
#define E_GFX_BASE			( 0x0080 << 16 )	// GFXDS in PE
#define E_DC_BASE			( 0x0090 << 16)		// DC Base
#define E_DOM_BASE			( 0x00A0 << 16)		// MV DOM
#define E_BDG_BASE          ( 0x00B0 << 16 )    // BD Graphics (IG/PG/TS)
#define E_PE_BASE			( 0x00C0 << 16)		// PE Base
#define E_DVDSPU_BASE		( 0x00D0 << 16)		// GraphDec  HDDVD&DVD SPU
#define E_VFD_BASE			( 0x00E0 << 16)		// GraphDec  HDDVD&DVD SPU
#define E_NET_BASE			( 0x00F0 << 16)		//Network
#define E_OU_BASE			( 0x0100 << 16)		//online upgrade base
#define E_VPP_BASE                 ( 0x0200 << 16)   //VPP base
#define E_DIVX_BASE			( 0x0400 << 16)		// DivX base
#define E_BDRESUB_BASE          ( 0x0800 << 16 )    // BD-RE SUBs (DVB/ISDB/ATSC)
#define E_VIP_BASE                 ( 0x1000 << 16)   //VIP base

// #define E_XXX_BASE		( 0x00a0 << 16 )	// Add your module base here


// generic error code macro
#define E_GEN_SUC( code ) 	( E_SUC | E_GENERIC_BASE | ( code & 0x0000FFFF ) )
#define E_GEN_ERR( code ) 	( E_ERR | E_GENERIC_BASE | ( code & 0x0000FFFF ) )

// generic error code list

// S_XXX = Success Status
#if defined( WIN32 )
// S_OK and S_FALSE are defined in windows.h
#else
#define S_OK				E_GEN_SUC( 0x0000 ) // Success
#define S_FALSE				E_GEN_SUC( 0x0001 )	// Success but return false status
#endif

// E_XXX = Error Status
#if defined( WIN32 )
// The following error codes are defined in windows.h
#else
#define E_NOTIMPL			E_GEN_ERR( 0x4001 )	// Not implemented
#define E_NOINTERFACE		E_GEN_ERR( 0x4002 )	// No such interface supported
#define E_POINTER			E_GEN_ERR( 0x4003 )	// Pointer that is not valid
#define E_ABORT				E_GEN_ERR( 0x4004 )	// Operation aborted
#define E_FAIL				E_GEN_ERR( 0x4005 )	// Unspecified failure
//#define E_NOPERMISSION		E_GEN_ERR( 0x4006 )	// No permission to do sth
#define E_UNEXPECTED		E_GEN_ERR( 0xFFFF )	// Unexpected failure
#define E_ACCESSDENIED		E_GEN_ERR( 0x7005 )	// General access denied error
#define E_HANDLE			E_GEN_ERR( 0x7006 )	// Handle that is not valid
#define E_OUTOFMEMORY		E_GEN_ERR( 0x700E )	// Failed to allocate necessary memory
#define E_INVALIDARG		E_GEN_ERR( 0x7057 )	// One or more arguments are not valid
#endif

#define E_BADVALUE			E_GEN_ERR( 0x7060 )	// The value is illegal (general)
#define E_OUTOFRANGE		E_GEN_ERR( 0x7061 )	// The value is out of range
#define E_TIMEOUT			E_GEN_ERR( 0x7062 )	// Operation time Out
#define E_FULL				E_GEN_ERR( 0x7063 )	// Item is full (Queue or table etc...)
#define E_EMPTY				E_GEN_ERR( 0x7064 )	// Item is empty (Queue or table etc...)
#define E_NOTREADY			E_GEN_ERR( 0x7065 )	// Operation or item is not ready yet
#define E_ALREADYEXIST		E_GEN_ERR( 0x7066 )	// Tried to create existing item
#define E_NOTEXIST			E_GEN_ERR( 0x7067 )	// Tried to operate not existing item
#define E_FILEOPEN			E_GEN_ERR( 0x7068 )	// open file failed
#define E_FILEWRITE			E_GEN_ERR( 0x7069 )	// write file failed
#define E_FILEREAD			E_GEN_ERR( 0x7070 )	// read file failed
#define E_FILESEEK			E_GEN_ERR( 0x7071 )	// seek file failed
#define E_FILECLOSE			E_GEN_ERR( 0x7072 )	// close file failed

#define E_TASKFAIL			E_GEN_ERR( 0x7080 )	// task error (general)
#define E_TASKCREATE		E_GEN_ERR( 0x7081 )	// task creation failed
#define E_TASKDESTROY		E_GEN_ERR( 0x7082 )	// task destroying failed

// error handling
#define CHKHR			{ if (FAILED(hr)) { return hr; }}
#define CHKHR_OUT		{ if (FAILED(hr)) { goto out; }}
#define CHKPTR(ptr)		{ if (!ptr) { return hr; }}
#define CHKPTR_OUT(ptr)	{ if (!ptr) { goto out; }}

#define CHKPTR_OUT_HR(ptr, out_hr)	{ if (!(ptr)) { hr = (out_hr); goto out; }}
#define CHKHR_RETURN(ret) 			{ if (FAILED(hr)) { return (ret); }}
#define CHKPTR_RETURN(ptr, ret) 	{ if (!(ptr)) { return (ret); }}

#endif	// _ERRORCODE_H_
