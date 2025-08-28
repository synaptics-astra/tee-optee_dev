/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright © 2013-2017 Synaptics Incorporated. All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated (“Synaptics”). The holder of this file shall treat all
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
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS,” AND
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
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS’ TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */

////////////////////////////////////////////////////////////////////////////////
//! \file DRM_FIRSTBOOT_CLIENT_API.h
//! \brief Header file declare the FIRSTBOOT CLIENT APIs
//!
//! Purpose:
//!
//!  Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef __DRM_FIRSTBOOT_AGENT_API_H__
#define __DRM_FIRSTBOOT_AGENT_API_H__

#ifndef _typedef___UCHAR
#define _typedef___UCHAR
typedef unsigned char       UCHAR;
#endif /* _typedef___UCHAR */

#ifndef _typedef__INT8
#define _typedef__INT8
typedef char INT8;
#endif /* _typedef__INIT8 */

#ifndef _typedef___UINT8
#define _typedef___UINT8
typedef UCHAR               UINT8;
#endif /* _typedef___UINT8 */

#ifndef _typedef___UINT16
#define _typedef___UINT16
typedef unsigned short      UINT16;
#endif /* _typedef___UINT16 */

#ifndef _typedef___UINT32
#define _typedef___UINT32
typedef unsigned int        UINT32;
#endif /* _typedef___UINT32 */

#ifndef _typedef___HRESULT
#define _typedef___HRESULT
typedef signed int HRESULT;
#endif /* _typedef___HRESULT */

#if !defined(S_OK) && !defined(S_FALSE)
#define S_OK 0
#define S_FALSE 1
#endif

enum FIRSTBOOT_ERROR {
    E_FAIL = -1,
    E_FILEOPEN = -2,
    E_FILEREAD = -3,
    E_OUTOFMEMORY = -4,
    E_NOTEXIST = -5,
    E_INVALIDARG = -6,
};

typedef struct __Key_Injection_Image{
    unsigned char *pucOemRootRsa;
    unsigned int  uOemRootRsaSize;
    unsigned char *pucSysInit;
    unsigned int  uSysInitSize;
    unsigned char *pucOemCustk;
    unsigned int  uOemCustkSize;
    unsigned char *pucOemExtRsa;
    unsigned int  uOemExtRsaSize;
} Key_Injection_Image;

//! SHA-1 Releated definition

#define      SHA1_DIGEST_BYTES_LEN     20
#define      SHA1_BLOCK_BYTES_LEN      64

typedef struct {
    UINT32  state[5];
    UINT32  count[2];
    UINT8   buffer[SHA1_BLOCK_BYTES_LEN];
} SHA1CTX;

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

#define blk0(i) \
(block->l[i] = (rol(block->l[i],24)&(UINT32)0xFF00FF00)|(rol(block->l[i],8)&(UINT32)0x00FF00FF))
#define blk(i) \
(block->l[i&15] = rol(block->l[(i+13)&15]^block->l[(i+8)&15]^block->l[(i+2)&15]^block->l[i&15],1))


typedef union {
UINT8   c[SHA1_BLOCK_BYTES_LEN];
UINT32  l[SHA1_DIGEST_BYTES_LEN];
} BYTE64QUAD16;


/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);


int kms_request_kms_status();

int kms_retrieve_initimg(unsigned long long device_sid,
                         unsigned long unique_id,
                         unsigned char * mainboard_id,
                         unsigned int mainboard_id_len,
                         unsigned char **data, unsigned int * datalen);

HRESULT mv_drm_firstboot_delete_old_appkeys();

HRESULT mv_drm_firstboot_get_appkeys_status(UINT8 bAppKey);

HRESULT mv_drm_firstboot_get_serialid(UINT8* pbSerialID, UINT32 uSize);

HRESULT mv_drm_firstboot_check_initimage(UINT8* pbPackage, UINT32 uPackSize);

HRESULT mv_drm_firstboot_check_initimage_hash(UINT8* pbPackage, UINT32 uPackSize,
                                               UINT8* pbHash, UINT32 uHashSize);

HRESULT mv_drm_firstboot_parse_initimage(UINT8* pbPackage, UINT32 uPackSize);

HRESULT mv_drm_firstboot_init_finish();

HRESULT mv_drm_firstboot_get_uniqueid(unsigned long * unique_id);

HRESULT mv_drm_firstboot_update_preboot(void);

HRESULT mv_drm_firstboot_check_keyinjection(void);

HRESULT mv_drm_firstboot_load_production_preboot(void);

HRESULT mv_drm_firstboot_check_appkeys(UINT8* pbPackage, UINT32 uPackSize);

HRESULT mv_drm_load_secure_boot_keys(UINT8* pbPackage, UINT32 uPackSize);

HRESULT mv_drm_load_otp_layout(UINT8* pbPackage, UINT32 uPackSize);

HRESULT mv_drm_firstboot_write_appkeys(UINT8* pbPackage, UINT32 uPackSize);

HRESULT mv_drm_cipher_otp_burner(void);

HRESULT mv_drm_firstboot_update_key1_key2(void);
#endif
