/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef	__GALOIS_CFG_H__
#define	__GALOIS_CFG_H__

/*!
 * IO read/write through software/hardware socket
 * #define	__MEMIO_SOCKET
 */

/*!
 * IO read/write through PCI-Express bus
 * #define	__MEMIO_PCIE
 */

/*!
 * IO read/write directly using MEMI/O
 */
#define	__MEMIO_DIRECT

/*!
 * Program is running under little endian system.
 */
#ifndef __LITTLE_ENDIAN
#define	__LITTLE_ENDIAN
#endif

/*!
 * Program is running under big endian system.
 * #define	__BIG_ENDIAN
 */

/*!
 * Program is running under Linux on ARM processor 
 */
#define	__LINUX_ARM


/*!
 * Program is running under VxWorks on ARM processor 
 * #define	__VxWORKS_ARM
 */

#endif	/* __GALOIS_CFG_H__	*/
