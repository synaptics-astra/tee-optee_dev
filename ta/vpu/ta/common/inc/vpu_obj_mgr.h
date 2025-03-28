/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2017 Synaptics Incorporated. All rights reserved.
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
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS", AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */

////////////////////////////////////////////////////////////////////////////////
//! \file vpu_obj_mgr.h
//!
//! \brief Header file of VPU object manager.
//!
//! Purpose: These APIs will facilitate managing objects in TZ.
//!
//!     Version    Date                     Author
//!     V 1.00,    Feb 24 2014,             Lei Wang
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef __VPU_OBJECTS_MANAGER__
#define __VPU_OBJECTS_MANAGER__

/** \fn void* vpuObjMgrCreate(void);
 *
 *  \brief Create a object manger.
 *
 *  \retval the object manager pointer.
 */
void* vpuObjMgrCreate(void);

/** \fn void *vpuObjMgrDestroy(void *mgr);
 *
 *  \brief Destroy a object manger.
 *
 *  \param[in] the object manager pointer.
 *
 *  \retval void.
 */
void vpuObjMgrDestroy(void *mgr);

/** \fn int vpuObjMgrSet(
 *      void *key,
 *      void *value);
 *
 *  \brief Save a object with a new key.
 *
 *  \param[in] mgr the object manager.
 *
 *  \param[in] key the key value.
 *
 *  \param[in] value the object pointer.
 *
 *  \retval 0 succeeded.
 *  \retval -1 already have the same key.
 *  \retval -2 running out of space for the new key.
 *
 * \sa VMETA_CLIENT_CONFIG
 */
int vpuObjMgrSet(void *mgr, void *key, void *value);

/** \fn void *vpuObjMgrGet(
 *      void *key);
 *
 *  \brief Load a object from a key.
 *
 *  \param[in] mgr the object manager.
 *
 *  \param[in] key the key value.
 *
 *  \retval the object pointer.
 *  \retval NULL fails to find the key..
 */
void *vpuObjMgrGet(void *mgr, void *key);

/** \fn void *vpuObjMgrRelease(
 *      void *key);
 *
 *  \brief Release a key slot.
 *
 *  \param[in] mgr the object manager.
 *
 *  \param[in] key the key value.
 *
 *  \retval the object pointer.
 *  \retval NULL fails to find the key..
 */
void *vpuObjMgrRelease(void *mgr, void *key);

/** \fn int vpuObjMgrGetTotalNumber(void *mgr);
 *
 *  \brief Get total instance number of object manger.
 *
 *  \param[in] the object manager pointer.
 *
 *  \retval void.
 */
int vpuObjMgrGetTotalNumber(void *mgr);

/** \fn void *vpuObjMgrGetWithIndex(
 *      void *mgr, unsigned int index);
 *
 *  \brief Load a object from index.
 *
 *  \param[in] mgr the object manager.
 *
 *  \param[in] index the index.
 *
 *  \retval the object pointer.
 *  \retval NULL fails to find the key..
 */
void *vpuObjMgrGetWithIndex(void *mgr, unsigned int index);

#endif
