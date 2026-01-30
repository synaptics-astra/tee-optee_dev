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
#ifndef _TEE_TSP_NSK_ALG_H_
#define _TEE_TSP_NSK_ALG_H_

//See Doc HWSEC-REG-8014-Transport Scrambling and Descrambling Algorithms.pdf

//M2M part:

typedef enum {
    NSK_ALG_M2M_START      =    0, //17+3 items
    DES_ECB_CLR_CLR        =    1,
    DES_CBC_SCTE52_IV1,
    DES_CBC_SCTE52_IV2,
    DES_CBC_CLR_CLR,

    TDES_ECB_CLR_CLR       =    5,
    TDES_CBC_SCTE52_IV1,
    TDES_CBC_SCTE52_IV2,
    TDES_CBC_CLR_CLR,

    AES_ECB_CLR_CLR        =    9,
    AES_CBC_CTS_IV1,
    AES_CBC_CTS_CLR,
    AES_CBC_SCTE52_IV1,
    AES_CBC_SCTE52_CLR,
    AES_CBC_CLR_CLR,

    RC4_64                  =    15,
    AES_CTR                 =    16,
    NSK_ALG_M2M_RESERVE0    =    17,
    NSK_ALG_M2M_RESERVE1    =    18,
    AES_CTR64               =    19, //    NSK_ALG_M2M_END
} NSK_ALG_M2M;

#define NSK_ALG_TS_OFFSET    80

//TS Part:
typedef enum {
    //NSK_ALG_TS_START, 17 items
    DVB_CSA2            =    100,
    DVB_CSA2_CONFORMANCE,
    DVB_CSA3,

    CPCM_LSA_MDI_CBC    =    103,
    CPCM_LSA_MDI_RCBC,
    CPCM_LSA_MDD_CBC,
    CPCM_LSA_MDD_RCBC,

    SYNAMEDIA_AES       =    107,
    AES_ECB_CLEARTAIL,
    CIPLUS_AES,
    SCTE41_DES,
    SCTE52_DES,
    TDES_ECB_CLEARTAIL,
    MULTI2_TS,
    TDES_ECB_CTS,
    DES_ECB_CTS,
    IDSA_AES            =    116, //    NSK_ALG_TS_END
} NSK_ALG_TS;

#define NSK_ALG_TS_DIRECTV_OFFSET    83

typedef enum {
    //NSK_ALG_TS_DIRECTV_START, 10 items
    NSA_DSS_MDI_MSC4              =    200,
    NSA_DSS_MDI_MSC18,
    NSA_DSS_MDD_MSC4,
    NSA_DSS_MDD_MSC18,

    DIRECTV_A3_AES                =    204,
    AVLOCAL_MPEG_MSC12,
    AVLOCAL_MPEG_MSC28,
    AVLOCAL_DSS_MSC2,
    AVLOCAL_DSS_MSC18,
    DIRECTV_DSS_DES //    NSK_ALG_TS_DIRECTV_END
} NSK_ALG_TS_DIRECTV;

#define NSK_ALG_TS_DIRECTV_END    209

//209-80-83+1= 47; also 3+17+17+10=47 items
#define NSK_ALG_TABLE_USED_SIZE (NSK_ALG_TS_DIRECTV_END - NSK_ALG_TS_OFFSET - NSK_ALG_TS_DIRECTV_OFFSET + 1)

int TSPTA_InitNSKAlgTable(void *secureContext);

bool TSPTA_shallAllocateFixedZeroIV(uint32_t algId);

#endif
