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

#ifndef _TSP_SHARED_H_
#define _TSP_SHARED_H_

#include "Galois_memmap.h"
#include "tsp.h"

#define DRM_KEY_TABLE_BASE_INVALID            (0xFFFF)

#define TSP_KEY_TABLE_BASE_ADDR_MASK          (TspKeyTblAdr_TspKeyTblBase)

/* total 256 entries, 8bytes of key data per entry
 * currently one 128bit key uses 2 entries, so it can support up to 128 128bit keys
 * in the future the keytable may be extended to 256 (per Android's requirement)
 */
#define TSP_KEY_TABLE_MAX_OFFSET              (256 * 8)

#define NAGRA_KEY_TABLE_BASE_ADDR_MASK        (TspKeyTblAdr_NagraKeyTblBase)

#if (BERLIN_CHIP_VERSION>=BERLIN_VS680)
#define NSK2_TS_KEY_TABLE_BASE_ADDR_MASK        (TspKeyTblAdr_Nsk2TsKeyTblBase)
#define NSK2_M2M_KEY_TABLE_BASE_ADDR_MASK       (TspKeyTblAdr_Nsk2M2mKeyTblBase)
#define IFCP_KEY_TABLE_BASE_ADDR_MASK           (TspKeyTblAdr_IFCPKeyTblBase)
#define GKL_KEY_TABLE_BASE_ADDR_MASK            (TspKeyTblAdr_GKLKeyTblBase)
#else
#define NSK2_TS_KEY_TABLE_BASE_ADDR_MASK        (DRM_KEY_TABLE_BASE_INVALID)
#define NSK2_M2M_KEY_TABLE_BASE_ADDR_MASK       (DRM_KEY_TABLE_BASE_INVALID)
#define IFCP_KEY_TABLE_BASE_ADDR_MASK           (DRM_KEY_TABLE_BASE_INVALID)
#define GKL_KEY_TABLE_BASE_ADDR_MASK            (DRM_KEY_TABLE_BASE_INVALID)
#endif

/*
 currently all the hw key areas are defined at drm se ta as enum MV_KEYTABLE_AREA_TYPE,
 so in dmx our definition of HW_KEYAREA should be aligned with it
*/
#define HW_KEYAREA_GENERAL   0
#define HW_KEYAREA_NAGRA     1
#define HW_KEYAREA_NSK2      3   //MV_KEYTABLE_AREA_NSK_HWKL
#define HW_KEYAREA_NSK2_M2M  4   //MV_KEYTABLE_AREA_NSK_M2M_HWKL
#define HW_KEYAREA_GKL       5
#define HW_KEYAREA_IFCP_BRD  6
#define HW_KEYAREA_IFCP_OTT  7
#define HW_KEYAREA_IFCP_PVR  8

#define IS_IFCP_KEYAREA(keyArea) ((keyArea == HW_KEYAREA_IFCP_BRD) || \
                                 (keyArea == HW_KEYAREA_IFCP_OTT) || \
                                 (keyArea == HW_KEYAREA_IFCP_PVR))

//should be algined with the related CAS TAs and drmSe TA
#define TSP_KEYTABLE_ENTRY_SIZE        0x10  //uTSPKeyTableOffset = uDrmKeyTableOffset/2
#define NAGRA_KEYTABLE_ENTRY_SIZE      0x20  //uTSPKeyTableOffset = uDrmKeyTableOffset + 32 - uKeySize
#define IFCP_KEYTABLE_ENTRY_SIZE       0x20  //uTSPKeyTableOffset = uDrmKeyTableOffset + 32 - uKeySize
#define GKL_KEYTABLE_ENTRY_SIZE        0x20  //uTSPKeyTableOffset = uDrmKeyTableOffset + 32 - uKeySize
#define NSK_KEYTABLE_ENTRY_SIZE        0x40

#define IFCP_KEY_BROADCAST_START             0
#define IFCP_KEY_BROADCAST_SIZE             (32 * IFCP_KEYTABLE_ENTRY_SIZE)   //0~31
#define IFCP_KEY_OTT_START                  (IFCP_KEY_BROADCAST_START + IFCP_KEY_BROADCAST_SIZE)
#define IFCP_KEY_OTT_SIZE                   (16 * IFCP_KEYTABLE_ENTRY_SIZE)   //32~47
#define IFCP_KEY_PVR_START                  (IFCP_KEY_OTT_START + IFCP_KEY_OTT_SIZE)
#define IFCP_KEY_PVR_SIZE                   (14 * IFCP_KEYTABLE_ENTRY_SIZE)   //48~61
#define IFCP_KEY_IRDETO_PRIVATE_START       (IFCP_KEY_PVR_START + IFCP_KEY_PVR_SIZE)
#define IFCP_KEY_IRDETO_PRIVATE_SIZE        (2 * IFCP_KEYTABLE_ENTRY_SIZE)   //62~63

//IFCP keyTable partitions:  BROADCAST/OTT/PVR/IRDETO_PRIVATE
#define VALIDATE_IFCP_KEY_IN_PARTITION(keyIndex, partition) \
   ((keyIndex >= IFCP_KEY_##partition##_START) && \
    (keyIndex < (IFCP_KEY_##partition##_START + IFCP_KEY_##partition##_SIZE)))

#define HW_KEY_USAGE_ENC     0x1
#define HW_KEY_USAGE_DEC     0x2

#define TSP_MAX_FIGO_NUM    2

#define TSP_FIGO_REG_BASE   MEMMAP_TSP_REG_BASE
#define TSP_FIGO_REG_SIZE   RA_TspReg_Figo1Dtcm
#define TSP_FIGO_DTCM_SIZE  RA_TspReg_FigoReg
#define TSP_FIGO_ITCM_SIZE  (RA_TspReg_Figo1Dtcm -RA_TspReg_FigoItcm)
#define TSP_FIGO0_ITCM_BASE   RA_TspReg_FigoItcm
#define TSP_FIGO1_ITCM_BASE   RA_TspReg_Figo1Itcm

#define IS_CLEAR_KEY(keyAddr)    (keyAddr > 0 && keyAddr < TspKeyTblAdr_TspKeyTblBase)

/**
 *  HBO is accessed in Entry unit. 8Btyes per Entry.
 */
#define FilterFifoEntrySize        8
/**
 *  Configure 16 Entries to each HBO FIFO
 */
#define FilterFifoDepth            16
/**
 *  Total HBO FIFO capacity in Bytes.
 */
#define FilterFifoTotalSize        (FilterFifoEntrySize * FilterFifoDepth)

// import from DMX_KEY_OPTION in dmx_drmwrap.h
typedef enum __tsp_key_option__ {
    DMX_TSP_KEY_INVALID = 0,
    DMX_TSP_KEY_CONTENT = 1,
    DMX_TSP_KEY_TBLIDX  = 2,
    DMX_TSP_KEY_MAX     = 3
} DMX_TSP_KEY_OPTION;

/**
 *  TSP config macro when startup.
 */
#define INVALID_DTCM_ADDR               (0xffff)

typedef enum __tsp_input_type__ {
    TSP_INPUT_TYPE_TS_MEM     = 0x0,
    TSP_INPUT_TYPE_TS_FE      = 0x1,
    TSP_INPUT_TYPE_ES         = 0x2,
    TSP_INPUT_TYPE_RAW        = 0x3,
    TSP_INPUT_TYPE_M2M        = 0x4,
    TSP_INPUT_TYPE_INVALID,
    TSP_INPUT_TYPE_MAX        = TSP_INPUT_TYPE_INVALID
}TSP_INPUT_TYPE;

typedef enum __tsp_scheme_type__ {
    TSP_CRYPTO_SCHEME_TYPE_NONE    = 0x0,
    TSP_AES_128_CBC_CLR_CLR,
    TSP_AES_128_CTR_NO_NO,
    TSP_AES_128_CBC_CTS_CLR,
    TSP_AES_128_CBC_RP_CLR,
    TSP_AES_128_CBC_RBT_CLR,
    TSP_AES_128_CBC_NO_NO,
    TSP_AES_128_CBC_NO_CLR,
    TSP_DVB_CSA_NO_NO,
    TSP_AES_128_ECB_CLR_CLR,
    TSP_DES_56_ECB_CLR_CLR,
    TSP_TDES_128_CBC_EXOR_EXOR,
    TSP_TDES_128_ECB_CLR_CLR,
    TSP_DVB_CSA3_NO_NO,
    TSP_ASA_NO_NO,
    TSP_SCHEME_TYPE_MAX
}TSP_SCHEME_TYPE;

typedef enum __tsp_src_crypto_level__ {
    TSP_SRC_CRYPTO_WHOLE_PACKET    = 0x0,
    TSP_SRC_CRYPTO_TS_PAYLOAD      = 0x1,
    TSP_SRC_CRYPTO_LEVEL_MAX
}TSP_SRC_CRYPTO_LEVEL;

//refer to MV_DRMContentKeyUnitAttr in drm_contentkey.h
typedef struct __tsp_content_key_unit_attr__
{
    unsigned int e_AlgType;
    unsigned int e_AlgMode;
    unsigned int e_ResidualMode;
    unsigned int e_SolitaryMode;
    unsigned int e_Polarity;
    unsigned int e_ValueType;
    unsigned int e_IVType;
    unsigned int e_KeyUsage;
    unsigned int uKeyIDLen;
    unsigned int uHWKeyIndex;
    unsigned int uHWIVIndex;
    unsigned int uKeyDataLen;
    unsigned int uIVLen;
    unsigned int uHWKeyArea;
}TSP_ContentKeyUnitAttr;

#define TSP_MAX_KEY_LENGTH 16

typedef struct __key_unit_info_ {
    TSP_ContentKeyUnitAttr keyAttr;

    //for clear key and iv
    unsigned char  key[TSP_MAX_KEY_LENGTH];
    unsigned char  iv[TSP_MAX_KEY_LENGTH];
}KEY_UNIT_INFO;

#endif

