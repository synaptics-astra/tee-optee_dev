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
//! \file    DRM_FIRSTBOOT_AGENT.c
//! \brief   newfirstboot client program
//! \author
//! \date
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DRM_FIRSTBOOT_AGENT_Api.h"
#include "DRM_OTP_Agent_API.h"
#include "IUImageLayout.h"

#define FIRSTBOOT_ERR_PRINTF
#ifdef FIRSTBOOT_ERR_PRINTF
#define ERR_printf printf
#else
#define ERR_printf
#endif
#define FIRSTBOOT_INFO_PRINTF
#ifdef FIRSTBOOT_INFO_PRINTF
#define INFO_printf printf
#else
#define INFO_printf
#endif
#define FACTORY_SETTING_DIR "/factory_setting/"

#define OTP_PACKAGE_SIZE        (1280)
#define CUSTK_SIZE              (1024)
#define PREBOOT_UPDATE_SIZE     (262144)
#define KEY_1_2_UPDATE_SIZE     (5120)

UINT8 *gp_OtpLayout = NULL;
Key_Injection_Image gKeyInjectData;

extern int mtd_read(unsigned char* buffer, const char* path, unsigned int addr, unsigned int size);
extern int mtd_preboot_write(unsigned char* buffer, unsigned int size);
extern int mtd_keys_write(unsigned char* buffer, unsigned int size);

//! Port the SHA-1 Here to avoid using CPU1 image
static UINT32 SHA1_Transform(UINT32 state[5], UINT8 buffer[SHA1_BLOCK_BYTES_LEN])
{
    UINT32 a, b, c, d, e;

    BYTE64QUAD16 *block;

    block = (BYTE64QUAD16*)buffer;
    /* Copy sha->state[] to working vars */

    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];

    /* 4 rounds of 20 operations each. Loop unrolled. */
    R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1);  R0(d,e,a,b,c, 2);  R0(c,d,e,a,b, 3);
    R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5);  R0(e,a,b,c,d, 6);  R0(d,e,a,b,c, 7);
    R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9);  R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
    R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
    R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
    R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
    R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
    R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
    R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
    R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
    R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
    R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
    R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
    R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
    R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
    R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
    R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
    R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
    R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
    R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);

    /* Add the working vars back into sha.state[] */
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;

    /* Wipe variables */
    a = b = c = d = e = 0;

    return 1;
}


static UINT32 SHA1_Init(SHA1CTX *sha)
{
    sha->state[0] = 0x67452301;
    sha->state[1] = 0xEFCDAB89;
    sha->state[2] = 0x98BADCFE;
    sha->state[3] = 0x10325476;
    sha->state[4] = 0xC3D2E1F0;
    sha->count[0] = sha->count[1] = 0;
    return 1;
}



static UINT32 SHA1_Update(SHA1CTX *sha, const UINT8 *input, UINT32 const inputLen)
{
    UINT32 i, j;
    j = (sha->count[0] >> 3) & 63;
    if ((sha->count[0] += inputLen << 3) < (inputLen << 3)) sha->count[1]++;

    sha->count[1] += (inputLen >> 29);
    if ((j + inputLen) > (SHA1_BLOCK_BYTES_LEN - 1)) {
        memcpy(&sha->buffer[j], input, (i = SHA1_BLOCK_BYTES_LEN - j));
        SHA1_Transform(sha->state, sha->buffer);

        for ( ; i + SHA1_BLOCK_BYTES_LEN - 1 < inputLen; i += SHA1_BLOCK_BYTES_LEN) {
                memcpy(sha->buffer, &input[i], SHA1_BLOCK_BYTES_LEN);
                SHA1_Transform(sha->state, sha->buffer);
            }
            j = 0;
    } else i = 0;

    memcpy(&sha->buffer[j], &input[i], inputLen - i);
    return 1;
}



static UINT32 SHA1_Final(UINT8 digest[20], SHA1CTX *sha)
{
    UINT32 i, j;
    UINT8 finalcount[8];
    for (i = 0; i < 8; i++) {
        finalcount[i] = (UINT8)((sha->count[(i >= 4 ? 0 : 1)]    >> ((3-(i & 3)) * 8) ) & 255);
        /* Endian independent */
    }

    SHA1_Update(sha, (UINT8 *)"\200", 1);

    while ((sha->count[0] & 504) != 448) {
        SHA1_Update(sha, (UINT8 *)"\0", 1);
    }

    /* Should cause a SHA1_Transform() */
    SHA1_Update(sha, finalcount, 8);
    for (i = 0; i < SHA1_DIGEST_BYTES_LEN; i++) {
        digest[i] = (UINT8)((sha->state[i >> 2] >> ((3 - (i & 3)) * 8) ) & 255);
    }

    /* Wipe variables */
    i = j = 0;
    memset(sha->buffer, 0, SHA1_BLOCK_BYTES_LEN);
    memset(sha->state, 0, SHA1_DIGEST_BYTES_LEN);
    memset(sha->count, 0, 8);
    memset(&finalcount, 0, 8);

    return 1;
}



static HRESULT Write_KeyStore(INT8* pbFileName, UINT8* pbData, UINT32 uSize)
{
    HRESULT lRes = S_OK;
    FILE* pFile = NULL;

    //Device Certificate
    INT8   pDstFileName[128] = {0};
    INT8   pDstFilePATH[128] = {0};

    char* sKeystorePath = getenv("BERLIN_KEYSTORE_PATH");
    if (sKeystorePath == NULL) {
        sKeystorePath = FACTORY_SETTING_DIR;
    }
    snprintf(pDstFilePATH, 128, "%s", sKeystorePath);
    snprintf(pDstFileName, 128, "%s/%s", pDstFilePATH, pbFileName);

    pFile = fopen(pDstFileName, "wb");
    if (pFile == NULL) {
        ERR_printf("Failed to open %s\n", pbFileName);
        lRes = E_FAIL;
        goto func_ret;
    }
    if (fwrite(pbData, 1, uSize, pFile ) != uSize) {
        ERR_printf("Failed to write %s\n", pbFileName);
        lRes = E_FAIL;
        goto func_ret;
    }

func_ret:
    if (pFile != NULL) {
        fclose(pFile);
    }

    return lRes;
}


/**
 * Return Value:
 * pbFileName NULL: E_INVALIDARG
 * File not exist : E_NOTEXIST
 * Fail to malloc : E_OUTOFMEMORY
 * Fail to read   : E_FILEREAD
 * Fail to open   : E_FILEOPEN
 * File corrupts  : E_FAIL
 * File corrects  : S_OK
 */
static HRESULT CheckKeysIntegraty_Status(INT8 * pbFileName)
{
    INT8 pbHashFileName[128] = {0};
    FILE* pFile = NULL;
    FILE* pHashFile = NULL;
    HRESULT lRes = S_OK;
    UINT32 uFileLen = 0;
    UINT32 uReadLen = 0;
    UINT8 * pFileData = NULL;

    INT8   pDstFileName[128] = {0};
    INT8   pDstFilePATH[128] = {0};
//  UINT32  status = 0;
//  UINT8   bCRC32Value[4] = {0};
//  UINT32  uCheckCRC32Value = 0;
    SHA1CTX shaCtx;
    UINT8 pbDigest[20] = {0};
    UINT8 pbReferDigest[20] = {0};

    if (pbFileName == NULL) {
        ERR_printf("Invalid argument\n");
        return E_INVALIDARG;
    }

    char* sKeystorePath = getenv("BERLIN_KEYSTORE_PATH");
    if (sKeystorePath == NULL) {
        sKeystorePath = FACTORY_SETTING_DIR;
    }
    snprintf(pDstFilePATH, 128, "%s", sKeystorePath);
    snprintf(pDstFileName, 128, "%s/%s", pDstFilePATH, pbFileName);
    snprintf(pbHashFileName, 128, "%s/%s.hash", pDstFilePATH, pbFileName);

    pFile = fopen(pDstFileName, "rb");
    if (pFile == NULL) {
        lRes = E_NOTEXIST;
        ERR_printf("File %s does not exist\n", pDstFileName);
        goto func_ret;
    }
    fseek(pFile, 0, SEEK_END);
    uFileLen = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    pFileData = (UINT8 *)malloc(uFileLen);
    if (pFileData == NULL) {
        lRes = E_OUTOFMEMORY;
        ERR_printf("Failed to malloc buffer\n");
        goto func_ret;
    }

    uReadLen = fread(pFileData, 1, uFileLen, pFile);
    if (uReadLen != uFileLen) {
        lRes = E_FILEREAD;
        ERR_printf("Failed to read data from file %s", pbFileName);
        goto func_ret;
    }

    SHA1_Init(&shaCtx);

    SHA1_Update(&shaCtx, pFileData, uFileLen);

    SHA1_Final(pbDigest, &shaCtx);

    pHashFile = fopen(pbHashFileName, "rb");
    if (pHashFile == NULL) {
        lRes = E_FILEOPEN;
        ERR_printf("Failed to open %s\n", pbHashFileName);
        goto func_ret;
    }

    uReadLen = fread(pbReferDigest, 1, 20, pHashFile);
    if (uReadLen != 20) {
        lRes = E_FILEREAD;
        ERR_printf("Failed to read data from file %s", pbHashFileName);
        goto func_ret;
    }

    if (memcmp(pbDigest, pbReferDigest, 20) != 0) {
        lRes = E_FAIL;
        ERR_printf("\tStatus %s :   Crashed\n", pbFileName);
    }
    else {
        lRes = S_OK;
        INFO_printf("\tStatus %s :   Valid\n", pbFileName);
    }

func_ret:
    if(pFile != NULL) {
        fclose(pFile);
    }
    if(pHashFile != NULL) {
        fclose(pHashFile);
    }
    if (pFileData != NULL) {
        free(pFileData);
    }
    return lRes;
}

static HRESULT FIRSTBOOT_Delete_Key(INT8 * pbFileName)
{
    INT8 pbHashFileName[128] = {0};
    FILE* pFile = NULL;
    FILE* pHashFile = NULL;
    HRESULT lRes = S_OK;
    UINT8 * pFileData = NULL;
    INT8   pDstFileName[128] = {0};
    INT8   pDstFilePATH[128] = {0};

    if (pbFileName == NULL) {
        ERR_printf("Invalid argument\n");
        return E_INVALIDARG;
    }

    char* sKeystorePath = getenv("BERLIN_KEYSTORE_PATH");
    if (sKeystorePath == NULL) {
         sKeystorePath = FACTORY_SETTING_DIR;
    }
    snprintf(pDstFilePATH, 128, "%s", sKeystorePath);
    snprintf(pDstFileName, 128, "%s/%s", pDstFilePATH, pbFileName);
    snprintf(pbHashFileName, 128, "%s/%s.hash", pDstFilePATH, pbFileName);

    pFile = fopen(pDstFileName, "rb");
    if (pFile == NULL) {
        ERR_printf("\t Not Exist\n");
        goto func_ret;
    }
    else {
        remove(pDstFileName);
        remove(pbHashFileName);
    }
func_ret:
    if (pFile != NULL) {
        fclose(pFile);
    }
    if (pHashFile != NULL) {
        fclose(pHashFile);
    }
    if (pFileData != NULL) {
        free(pFileData);
    }
    return lRes;
}

HRESULT mv_drm_firstboot_delete_old_appkeys()
{
    INFO_printf("Delete HDCP Key:\n");
    FIRSTBOOT_Delete_Key("HDCP_Key.store");

    INFO_printf("Delete HDCPRX Key:\n");
    FIRSTBOOT_Delete_Key("HDCPRX_Key.store");

    //printf("Delete AACS Device Key:\n");
    //FIRSTBOOT_Delete_Key("AACS_Marvell_DKey.store");

    //printf("Delete AACS Host Cert:\n");
    //FIRSTBOOT_Delete_Key("AACS_Marvell_HCert.store");

    //printf("Delete BDPLUS Device Key:\n");
    //FIRSTBOOT_Delete_Key("BDPLUS_Marvell_DKey.store");

    //printf("Delete BDPLUS Device Cert:\n");
    //FIRSTBOOT_Delete_Key("BDPLUS_Marvell_DevCert.store");

    //printf("Delete BDPLUS Device Public:\n");
    //FIRSTBOOT_Delete_Key("BDPLUS_Marvell_DevCert");

    //printf("Delete BDPLUS Model/Version Key:\n");
    //FIRSTBOOT_Delete_Key("BDPLUS_Marvell_MVKey.store");

    //printf("Delete BDPLUS Model/Version Cert:\n");
    //FIRSTBOOT_Delete_Key("BDPLUS_Marvell_ModVerCert.store");

    //printf("Delete BDPLUS Model/Version Public:\n");
    //FIRSTBOOT_Delete_Key("BDPLUS_Marvell_ModVerCert");

    //INFO_printf("Delete MAC:\n");
    //FIRSTBOOT_Delete_Key("MAC_ADDR");

    INFO_printf("Delete Netflix ESN Secret:\n");
    FIRSTBOOT_Delete_Key("Netflix_ESN_PreShareKey.store");

    INFO_printf("Delete Netflix Web Secret:\n");
    FIRSTBOOT_Delete_Key("Netflix_Web_Secret.store");

    INFO_printf("Delete WMDRM Private Key:\n");
    FIRSTBOOT_Delete_Key("WMDRM_Key.store");

    INFO_printf("Delete WMDRM Device Cert Template:\n");
    FIRSTBOOT_Delete_Key("devcerttemplate.dat");

    INFO_printf("Delete CSS Key:\n");
    FIRSTBOOT_Delete_Key("CSS_Key.store");

    INFO_printf("Delete PANDORA Secret:\n");
    FIRSTBOOT_Delete_Key("PANDORA_Secret.store");

    INFO_printf("Delete SUNEXT Key:\n");
    FIRSTBOOT_Delete_Key("SUNEXT_Key.store");

    INFO_printf("Delete ULPK Key:\n");
    FIRSTBOOT_Delete_Key("ulpk.all");
    FIRSTBOOT_Delete_Key("ulpk.dat");
    FIRSTBOOT_Delete_Key("ulpk_transit.dat");

    INFO_printf("Delete Widevine Key:\n");
    FIRSTBOOT_Delete_Key("kb.bin");

    INFO_printf("Delete GTV CA Key:\n");
    FIRSTBOOT_Delete_Key("client-ca-0.pem");

    INFO_printf("Delete GTV Gen3 ICA Key:\n");
    FIRSTBOOT_Delete_Key("client-ca-1.pem");

    INFO_printf("Delete OEM ICA Key:\n");
    FIRSTBOOT_Delete_Key("client-ca-2.pem");

    INFO_printf("Delete GCA Client Certificate:\n");
    FIRSTBOOT_Delete_Key("client.crt");

    INFO_printf("Delete GCA Client Key:\n");
    FIRSTBOOT_Delete_Key("client.key");

    INFO_printf("Delete PlayReady Key:\n");
    FIRSTBOOT_Delete_Key("PLAYREADY_Key.store");

    INFO_printf("Delete bgroup Certificate:\n");
    FIRSTBOOT_Delete_Key("bgroupcert.dat");

    INFO_printf("Delete HDCP2X private Key:\n");
    FIRSTBOOT_Delete_Key("HDCP20RX_Priv.store");

    INFO_printf("Delete HDCP2X Local constant:\n");
    FIRSTBOOT_Delete_Key("HDCP20_LC.store");

    INFO_printf("Delete HDCP2X dn key:\n");
    FIRSTBOOT_Delete_Key("HDCP20RX_dn.store");

    INFO_printf("Delete HDCP2X certificate:\n");
    FIRSTBOOT_Delete_Key("HDCP20RX_Certx");

    INFO_printf("Delete Customer blob:\n");
    FIRSTBOOT_Delete_Key("CustBlob.bin");

    INFO_printf("Delete Customer blob 1:\n");
    FIRSTBOOT_Delete_Key("CustBlob1.bin");

    INFO_printf("Delete Customer blob 2:\n");
    FIRSTBOOT_Delete_Key("CustBlob2.bin");

    INFO_printf("Delete UEI Key:\n");
    FIRSTBOOT_Delete_Key("UEI.Key");

    INFO_printf("Delete Vudu Key:\n");
    FIRSTBOOT_Delete_Key("VuduCommonKey.store");

    INFO_printf("Delete MARLIN Key:\n");
    FIRSTBOOT_Delete_Key("MARLIN_SKB_KEY.store");

    INFO_printf("Delete MARLIN Cert:\n");
    FIRSTBOOT_Delete_Key("MARLIN_PRIV_KEY.bin");

    INFO_printf("Delete CIPLUS Key:\n");
    FIRSTBOOT_Delete_Key("CIP_TZ_KEY.store");

    INFO_printf("Delete CIPLUS Cert:\n");
    FIRSTBOOT_Delete_Key("CIP_NTZ_KEY.bin");

    INFO_printf("Delete DTCP PKG:\n");
    FIRSTBOOT_Delete_Key("DTCPEL_KEY.bin");

    INFO_printf("Delete ICP PKG:\n");
    FIRSTBOOT_Delete_Key("ICP_KEY.bin");

    INFO_printf("Delete HDCP2X Reserved private Key:\n");
    FIRSTBOOT_Delete_Key("HDCP20RX_Priv_R.store");

    INFO_printf("Delete HDCP2X Reserved Local constant:\n");
    FIRSTBOOT_Delete_Key("HDCP20_LC_R.store");

    INFO_printf("Delete HDCP2X Reserved dn key:\n");
    FIRSTBOOT_Delete_Key("HDCP20RX_dn_R.store");

    INFO_printf("Delete HDCP2X Reserved certificate:\n");
    FIRSTBOOT_Delete_Key("HDCP20RX_Certx_R");

    INFO_printf("Delete RPMB KEY:\n");
    FIRSTBOOT_Delete_Key("RPMB_Key.store");

    INFO_printf("Delete GKEY KEY:\n");
    FIRSTBOOT_Delete_Key("GKEY_key.store");

    INFO_printf("Delete CUSTOMER TA:\n");
    FIRSTBOOT_Delete_Key("CUSTOMER_TA.store");

    INFO_printf("Delete HDCP2X private Key Padded:\n");
    FIRSTBOOT_Delete_Key("HDCP20RX_Priv_Padded.store");

    INFO_printf("Delete Direct Carrier Billing key:\n");
    FIRSTBOOT_Delete_Key("CarrierBilling_rsakey.store");

    INFO_printf("Delete Key Master key:\n");
    FIRSTBOOT_Delete_Key("KM_key.store");
    FIRSTBOOT_Delete_Key("KM.cert");

    return S_OK;
}

HRESULT mv_drm_firstboot_get_appkeys_status(UINT8 bAppKey)
{
    HRESULT hResult = S_OK;

    switch(bAppKey) {
        case APPKEY_AACS_KEY:
            INFO_printf("No Check Now:\n");
            hResult = S_FALSE;
            break;

        case APPKEY_AACS_CERT:
            INFO_printf("No Check Now:\n");
            hResult = S_FALSE;
            break;

        case APPKEY_BDPLUS_DEVKEY:
            INFO_printf("No Check Now:\n");
            hResult = S_FALSE;
            break;

        case APPKEY_BDPLUS_DEVCERT:
            INFO_printf("No Check Now:\n");
            hResult = S_FALSE;
            break;

        case APPKEY_BDPLUS_DEVPUB:
            INFO_printf("No Check Now:\n");
            hResult = S_FALSE;
            break;

        case APPKEY_BDPLUS_MVKEY:
            INFO_printf("No Check Now:\n");
            hResult = S_FALSE;
            break;

        case APPKEY_BDPLUS_MVCERT:
            INFO_printf("No Check Now:\n");
            hResult = S_FALSE;
            break;

        case APPKEY_BDPLUS_MVPUB:
            INFO_printf("No Check Now:\n");
            hResult = S_FALSE;
            break;

        case APPKEY_CUSTKEY:
            INFO_printf("No Check Now:\n");
            hResult = S_FALSE;
            break;

        case APPKEY_HDCP:
            INFO_printf("Check HDCP Key:\n");
            hResult = CheckKeysIntegraty_Status("HDCP_Key.store");
            break;

        /**
        case APPKEY_MAC_ADDR:
            printf("Check MAC:\n");
            CheckKeysIntegraty_Status("MAC_ADDR");
            break;
        */

        case APPKEY_NETFLIX_ESN:
            INFO_printf("Check Netflix ESN Secret:\n");
            hResult = CheckKeysIntegraty_Status("Netflix_ESN_PreShareKey.store");

            /**
            printf("Check Netflix Web Secret:\n");
            CheckKeysIntegraty_Status("Netflix_Web_Secret.store");
            */
            break;

        case APPKEY_NETFLIX_MODEL:
            INFO_printf("No Check Now:\n");
            hResult = S_FALSE;
            break;

        case APPKEY_WMDRM_JANUS:
            INFO_printf("Check WMDRM Private Key:\n");
            hResult = CheckKeysIntegraty_Status("WMDRM_Key.store");
            break;

        case APPKEY_WMDRM_JANUS_CERT_TEMPLATE:
            INFO_printf("Check WMDRM Device Cert Template:\n");
            hResult = CheckKeysIntegraty_Status("devcerttemplate.dat");
            break;

        /**
        case APPKEY_CSS:
            printf("Check CSS Key:\n");
            CheckKeysIntegraty_Status("CSS_Key.store");
            break;
        */

        case APPKEY_CPRM:
            INFO_printf("No Check Now:\n");
            hResult = S_FALSE;
            break;

        /**
        case APPKEY_PANDORA:
            printf("Check PANDORA Secret:\n");
            CheckKeysIntegraty_Status("PANDORA_Secret.store");
            break;
        */

        /**
        case APPKEY_SUNEXT_DRIVE:
            printf("Check SUNEXT Key:\n");
            CheckKeysIntegraty_Status("SUNEXT_Key.store");
            break;
        */

        /**
        case APPKEY_ULI:
            printf("Check ULPK Key:\n");
            CheckKeysIntegraty_Status("ulpk.all");
            break;
        */

        case APPKEY_WV:
            INFO_printf("Check WVK Key:\n");
            hResult = CheckKeysIntegraty_Status("kb.bin");
            break;

        case APPKEY_CLIENTCA0:
            INFO_printf("No Check Now:\n");
            hResult = S_FALSE;
            break;

        case APPKEY_CLIENTCA1:
            INFO_printf("No Check Now:\n");
            hResult = S_FALSE;
            break;

        case APPKEY_CLIENTCA2:
            INFO_printf("No Check Now:\n");
            hResult = S_FALSE;
            break;

        case APPKEY_CLIENTCER:
            INFO_printf("Check GCA Client Certificate:\n");
            hResult = CheckKeysIntegraty_Status("client.crt");
            break;

        case APPKEY_CLIENTKEY:
            INFO_printf("Check GCA Client Certificate:\n");
            hResult = CheckKeysIntegraty_Status("client.key");
            break;

        case APPKEY_PRKEY:
            INFO_printf("Check PlayReady Key:\n");
            hResult = CheckKeysIntegraty_Status("PLAYREADY_Key.store");
            break;

        case APPKEY_BGROUPCER:
            INFO_printf("Check bgroup Certificate:\n");
            hResult = CheckKeysIntegraty_Status("bgroupcert.dat");
            break;

        case APPKEY_HDCPPRIV:
            INFO_printf("Check HDCP2X private Key:\n");
            hResult = CheckKeysIntegraty_Status("HDCP20RX_Priv.store");
            break;

        case APPKEY_HDCPLC:
            INFO_printf("Check HDCP2X Local constant:\n");
            hResult = CheckKeysIntegraty_Status("HDCP20_LC.store");
            break;

        case APPKEY_HDCPDN:
            INFO_printf("Check HDCP2X dn key:\n");
            hResult = CheckKeysIntegraty_Status("HDCP20RX_dn.store");
            break;

        case APPKEY_HDCPCER:
            INFO_printf("Check HDCP2X certificate:\n");
            hResult = CheckKeysIntegraty_Status("HDCP20RX_Certx");
        break;

        case APPKEY_CUSTBLOB:
            INFO_printf("Check Customer blob:\n");
            hResult = CheckKeysIntegraty_Status("CustBlob.bin");
            break;

        case APPKEY_EXTENDED_KEY5:
            INFO_printf("Check Customer blob 1:\n");
            hResult = CheckKeysIntegraty_Status("CustBlob1.bin");
            break;

        case APPKEY_EXTENDED_KEY6:
            INFO_printf("Check Customer blob 2:\n");
            hResult = CheckKeysIntegraty_Status("CustBlob2.bin");
            break;

        case APPKEY_UEIKEY:
            INFO_printf("Check UEI Key:\n");
            hResult = CheckKeysIntegraty_Status("UEI.Key");
            break;

        /**
        case APPKEY_HDCP2X_WIDI:
            printf("Check HDCP2X_WIDI Key:\n");
            CheckKeysIntegraty_Status("HDCP2X_WIDI.store");

            printf("Check HDCP2X Reserved private Key:\n");
            CheckKeysIntegraty_Status("HDCP20RX_Priv_R.store");

            printf("Check HDCP2X Reserved Local constant:\n");
            CheckKeysIntegraty_Status("HDCP20_LC_R.store");

            printf("Check HDCP2X Reserved dn key:\n");
            CheckKeysIntegraty_Status("HDCP20RX_dn_R.store");

            printf("Check HDCP2X Reserved certificate:\n");
            CheckKeysIntegraty_Status("HDCP20RX_Certx_R");

            break;
        */

        case APPKEY_MOBITV_KEY:
            INFO_printf("Check MobiTV Key:\n");
            hResult = CheckKeysIntegraty_Status("MobiTV_key.bin");
            break;

        case APPKEY_MOBITV_CERT:
            INFO_printf("Check MobiTV Cert:\n");
            hResult = CheckKeysIntegraty_Status("MobiTV_cert.pem");
            break;

        case APPKEY_DTCP_PVK:
            INFO_printf("Check DTCP PVK Key:\n");
            hResult = CheckKeysIntegraty_Status("DTCP_PVK.store");
            break;

        case APPKEY_DTCP_CRT:
            INFO_printf("Check DTCP Cert:\n");
            hResult = CheckKeysIntegraty_Status("DTCP_cert");
            break;

        case APPKEY_DTCP_RNG:
            INFO_printf("Check DTCP RNG :\n");
            hResult = CheckKeysIntegraty_Status("DTCP_RNG.store");
            break;

        case APPKEY_VUDU_KEY:
            INFO_printf("Check VUDU KEY :\n");
            hResult = CheckKeysIntegraty_Status("VuduCommonKey.store");
            break;

        case APPKEY_HDCPRX_KEY:
            INFO_printf("Check HDCPRX Key:\n");
            hResult = CheckKeysIntegraty_Status("HDCPRX_Key.store");
            break;

        case APPKEY_MARLIN_KEYS:
            INFO_printf("Check MARLIN KEY :\n");
            hResult = CheckKeysIntegraty_Status("MARLIN_SKB_KEY.store");
            break;

        case APPKEY_MARLIN_CERTS:
            INFO_printf("No Check Now:\n");
            hResult = S_FALSE;
            break;

        case APPKEY_CIPLUS_KEYS:
            INFO_printf("Check CIPLUS KEY :\n");
            hResult = CheckKeysIntegraty_Status("CIP_TZ_KEY.store");
            break;

        case APPKEY_CIPLUS_CERTS:
            INFO_printf("Check CIPLUS CERT :\n");
            hResult = CheckKeysIntegraty_Status("MARLIN_PRIV_KEY.bin");
            if(hResult != S_OK) {
                ERR_printf("MARLIN_PRIV_KEY error!\n");
                break;
            }

            INFO_printf("Check CIPLUS CERT :\n");
            hResult = CheckKeysIntegraty_Status("CIP_NTZ_KEY.bin");
            break;

        case APPKEY_ICP_PKG:
            INFO_printf("Check ICP PKG :\n");
            hResult = CheckKeysIntegraty_Status("ICP_KEY.bin");
            break;

        case APPKEY_DTCP_PKG:
            INFO_printf("Check DTCP PKG :\n");
            hResult = CheckKeysIntegraty_Status("DTCPEL_KEY.bin");
            break;

        case APPKEY_HDCPPRIV_R:
            hResult = S_FALSE;
            break;

        case APPKEY_HDCPLC_R:
            hResult = S_FALSE;
            break;

        case APPKEY_HDCPDN_R:
            hResult = S_FALSE;
            break;

        case APPKEY_HDCPCER_R:
            hResult = S_FALSE;
            break;

        /**
        case APPKEY_GKEY:
            printf("Check GKEY KEY :\n");
            CheckKeysIntegraty_Status("GKEY_Key.store");

            printf("Check GTV CA Key:\n");
            CheckKeysIntegraty_Status("client-ca-0.pem");

            printf("Check GTV Gen3 ICA Key:\n");
            CheckKeysIntegraty_Status("client-ca-1.pem");

            printf("Check OEM ICA Key:\n");
            CheckKeysIntegraty_Status("client-ca-2.pem");

            printf("Check GTV Client Certificate:\n");
            CheckKeysIntegraty_Status("client.pem");

            printf("Check GTC Client Key:\n");
            CheckKeysIntegraty_Status("client.key");

            break;
        */

        case APPKEY_CUSTTA:
            INFO_printf("Check CUSTOMER TA:\n");
            hResult = CheckKeysIntegraty_Status("CUSTOMER_TA.store");
            break;

        case APPKEY_HDCPPRIV_PADDED:
            INFO_printf("Check HDCP2X private Key Padded:\n");
            hResult = CheckKeysIntegraty_Status("HDCP20RX_Priv_Padded.store");
            break;

        case APPKEY_WMDRM_JANUS30:
            INFO_printf("Check WMDRM 3.0 Private Key:\n");
            hResult = CheckKeysIntegraty_Status("WMDRM30_Key.store");
            break;

        case APPKEY_WMDRM_JANUS_CERT_TEMPLATE30:
            INFO_printf("Check WMDRM Device Cert Template 3.0:\n");
            hResult = CheckKeysIntegraty_Status("devcerttemplate30.dat");
            break;

        case APPKEY_PR30KEY:
            INFO_printf("Check PlayReady 3.0 Key:\n");
            hResult = CheckKeysIntegraty_Status("PLAYREADY30_Key.store");
            break;

        case APPKEY_BGROUPCER30:
            INFO_printf("Check bgroup Certificate 3.0:\n");
            hResult = CheckKeysIntegraty_Status("bgroupcert30.dat");
            break;

        case APPKEY_EXTENDED_KEY3:
            INFO_printf("Check PlayReady 4.0 Key:\n");
            hResult = CheckKeysIntegraty_Status("PLAYREADY40_Key.store");
            break;

        case APPKEY_EXTENDED_KEY4:
            INFO_printf("Check bgroup Certificate 4.0:\n");
            hResult = CheckKeysIntegraty_Status("bgroupcert40.dat");
            break;

        case APPKEY_CARRIER_BILLING:
            INFO_printf("Check Carrier Billing:\n");
            hResult = CheckKeysIntegraty_Status("CarrierBilling_rsakey.store");
            break;

        case APPKEY_EXTENDED_KEY0:
            INFO_printf("Check Carrier Billing Public Key:\n");
            hResult = CheckKeysIntegraty_Status("CarrierBilling_rsakey.pub");
            break;

        case APPKEY_EXTENDED_KEY1:
            INFO_printf("Check KM2 Key:\n");
            hResult = CheckKeysIntegraty_Status("KM_key.store");
            break;

        case APPKEY_EXTENDED_KEY2:
            INFO_printf("Check KM2 Certificate:\n");
            hResult = CheckKeysIntegraty_Status("KM.cert");
            break;

        case APPKEY_CISCO_VSSS_KEY:
            INFO_printf("Check Cisco VSSS Key:\n");
            hResult = CheckKeysIntegraty_Status("CiscoVSSS_rsakey.store");
            break;

        case APPKEY_LAST_KEYTYPE:
            hResult = S_FALSE;
            break;

        default:
            hResult = S_FALSE;
    }

    return hResult;
}

static HRESULT FIRSTBOOT_Process_HDCP_Key(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process HDCP_Key\n");
    Write_KeyStore("HDCP_Key.store", pbKeyData, uKeySize);
    return Write_KeyStore("HDCP_Key.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_HDCPRX_Key(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process HDCPRX_Key\n");
    Write_KeyStore("HDCPRX_Key.store", pbKeyData, uKeySize);
    return Write_KeyStore("HDCPRX_Key.store.hash", pbHashValue, 20);
}

#if 0 // AMP code

static HRESULT FIRSTBOOT_Process_AACS_KEY(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
        printf("process AACS_Marvell_DKey\n");
        Write_KeyStore("AACS_Marvell_DKey.store", pbKeyData, uKeySize);
        return Write_KeyStore("AACS_Marvell_DKey.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_AACS_CERT(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
        printf("process AACS_Marvell_HCert\n");
        Write_KeyStore("AACS_Marvell_HCert.store", pbKeyData, uKeySize);
        return Write_KeyStore("AACS_Marvell_HCert.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_BDPLUS_DEVKEY(UINT8* pbKeyData,
                                               UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
        printf("process BDPLUS_Marvell_DKey\n");
        Write_KeyStore("BDPLUS_Marvell_DKey.store", pbKeyData, uKeySize);
        return Write_KeyStore("BDPLUS_Marvell_DKey.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_BDPLUS_DEVCERT(UINT8* pbKeyData,
                                                UINT32 uKeySize,
                                                UINT8* pbHashValue)
{
        printf("process BDPLUS_Marvell_DevCert\n");
        Write_KeyStore("BDPLUS_Marvell_DevCert.store", pbKeyData, uKeySize);
        return Write_KeyStore("BDPLUS_Marvell_DevCert.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_BDPLUS_DEVPUB(UINT8* pbKeyData,
                                               UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
        printf("process BDPLUS_Marvell_DevCert\n");
        Write_KeyStore("BDPLUS_Marvell_DevCert", pbKeyData, uKeySize);
        return Write_KeyStore("BDPLUS_Marvell_DevCert.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_BDPLUS_MVKEY(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
        printf("process BDPLUS_Marvell_MVKey\n");
        Write_KeyStore("BDPLUS_Marvell_MVKey.store", pbKeyData, uKeySize);
        return Write_KeyStore("BDPLUS_Marvell_MVKey.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_BDPLUS_MVCERT(UINT8* pbKeyData,
                                               UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
        printf("process BDPLUS_Marvell_MVCert\n");
        Write_KeyStore("BDPLUS_Marvell_ModVerCert.store", pbKeyData, uKeySize);
        return Write_KeyStore("BDPLUS_Marvell_ModVerCert.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_BDPLUS_MVPUB(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
        printf("process BDPLUS_Marvell_ModVerCert\n");
        Write_KeyStore("BDPLUS_Marvell_ModVerCert", pbKeyData, uKeySize);
        return Write_KeyStore("BDPLUS_Marvell_ModVerCert.hash", pbHashValue, 20);
}

#endif

static HRESULT FIRSTBOOT_Process_MacAddr(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Store the Mac address data\n");
    //! For alignment Mac data is 20 bytes, but available data is 17 bytes
    if(uKeySize != 20) {
         ERR_printf("Invalid Mac address length=%d, It should be 20\n", uKeySize);
         return E_FAIL;
    }
    uKeySize = uKeySize - 3; //!trsuncate the the padding data in the tail.
    Write_KeyStore("MAC_ADDR", pbKeyData, uKeySize);
    return Write_KeyStore("MAC_ADDR.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_NetflixESN_KEY(UINT8* pbKeyData,
                                                UINT32 uKeySize,
                                                UINT8* pbHashValue)
{
    INFO_printf("process Store the Netflix ESN key data\n");
    Write_KeyStore("Netflix_ESN_PreShareKey.store", pbKeyData, uKeySize);
    return Write_KeyStore("Netflix_ESN_PreShareKey.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_NetflixWebSecret_KEY(UINT8* pbKeyData,
                                                      UINT32 uKeySize,
                                                      UINT8* pbHashValue)
{
    INFO_printf("process Secure Store: Netflix Web Secret\n");
    Write_KeyStore("Netflix_Web_Secret.store",pbKeyData, uKeySize);
    return Write_KeyStore("Netflix_Web_Secret.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_WMDRM_KeyStore(UINT8* pbKeyData,
                                                UINT32 uKeySize,
                                                UINT8* pbHashValue)
{
    INFO_printf("process Secure store: WM DRM Keys:\n");
    Write_KeyStore("WMDRM_Key.store", pbKeyData, uKeySize);
    return Write_KeyStore("WMDRM_Key.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_WMDRM_CertTemplate(UINT8* pbKeyData,
                                                    UINT32 uKeySize,
                                                    UINT8* pbHashValue)
{
    INFO_printf("process secure store: Windows Media DRM Cert template\n");
    Write_KeyStore("devcerttemplate.dat", pbKeyData, uKeySize);
    return Write_KeyStore("devcerttemplate.dat.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_WMDRM30_KeyStore(UINT8* pbKeyData,
                                                UINT32 uKeySize,
                                                UINT8* pbHashValue)
{
    INFO_printf("process Secure store: WM DRM 3.0 Keys:\n");
    Write_KeyStore("WMDRM30_Key.store", pbKeyData, uKeySize);
    return Write_KeyStore("WMDRM30_Key.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_WMDRM_CertTemplate30(UINT8* pbKeyData,
                                                    UINT32 uKeySize,
                                                    UINT8* pbHashValue)
{
    INFO_printf("process secure store: Windows Media DRM Cert template 30\n");
    Write_KeyStore("devcerttemplate30.dat", pbKeyData, uKeySize);
    return Write_KeyStore("devcerttemplate30.dat.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_CSS_Key(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: CSS Keys:\n");
    Write_KeyStore("CSS_Key.store", pbKeyData, uKeySize);
    return Write_KeyStore("CSS_Key.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_PANDORA_Secret(UINT8* pbKeyData,
                                                UINT32 uKeySize,
                                                UINT8* pbHashValue)
{
    INFO_printf("process Secure store: PANDORA Secret:\n");
    Write_KeyStore("PANDORA_Secret.store", pbKeyData, uKeySize);
    return Write_KeyStore("PANDORA_Secret.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_SUNEXT_Key(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: SUNEXT Key:\n");
    Write_KeyStore("SUNEXT_Key.store", pbKeyData, uKeySize);
    return Write_KeyStore("SUNEXT_Key.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_ULPK_Key(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: ULPK Key (ULI Format):\n");
    Write_KeyStore("ulpk.all", pbKeyData, uKeySize);
    Write_KeyStore("ulpk.dat", pbKeyData, 96);
    Write_KeyStore("ulpk_transit.dat", pbKeyData + 96, 144);
    return Write_KeyStore("ulpk.all.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_WVK_Key(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: WVK Key (version 2):\n");
    Write_KeyStore("kb.bin", pbKeyData, uKeySize);
    return Write_KeyStore("kb.bin.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_CLIENTCA0_Key(UINT8* pbKeyData,
                                               UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
    INFO_printf("process Secure store: GTV CA Key:\n");
    Write_KeyStore("client-ca-0.pem", pbKeyData, uKeySize);
    return Write_KeyStore("client-ca-0.pem.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_CLIENTCA1_Key(UINT8* pbKeyData,
                                               UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
    INFO_printf("process Secure store: GTV Gen3 ICA Key:\n");
    Write_KeyStore("client-ca-1.pem", pbKeyData, uKeySize);
    return Write_KeyStore("client-ca-1.pem.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_CLIENTCA2_Key(UINT8* pbKeyData,
                                               UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
    INFO_printf("process Secure store: OEM ICA Key:\n");
    Write_KeyStore("client-ca-2.pem", pbKeyData, uKeySize);
    return Write_KeyStore("client-ca-2.pem.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_CLIENTCER_Key(UINT8* pbKeyData,
                                               UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
    INFO_printf("process Secure store: GCA Client Certificate:\n");
    Write_KeyStore("client.crt", pbKeyData, uKeySize);
    return Write_KeyStore("client.crt.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_CLIENTKEY_Key(UINT8* pbKeyData,
                                               UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
    INFO_printf("process Secure store: GCA Client Key:\n");
    Write_KeyStore("client.key", pbKeyData, uKeySize);
    return Write_KeyStore("client.key.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_PRKEY_Key(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: PlayReady Key:\n");
    Write_KeyStore("PLAYREADY_Key.store", pbKeyData, uKeySize);
    return Write_KeyStore("PLAYREADY_Key.store.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_BGROUPCER_Key(UINT8* pbKeyData,
                                               UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
    INFO_printf("process Secure store: bgroup Certificate:\n");
    Write_KeyStore("bgroupcert.dat", pbKeyData, uKeySize);
    return Write_KeyStore("bgroupcert.dat.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_PR30KEY_Key(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: PlayReady 3.0 Key:\n");
    Write_KeyStore("PLAYREADY30_Key.store", pbKeyData, uKeySize);
    return Write_KeyStore("PLAYREADY30_Key.store.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_BGROUPCER30_Key(UINT8* pbKeyData,
                                               UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
    INFO_printf("process Secure store: bgroup Certificate 3.0:\n");
    Write_KeyStore("bgroupcert30.dat", pbKeyData, uKeySize);
    return Write_KeyStore("bgroupcert30.dat.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_PR40KEY_Key(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: PlayReady 4.0 Key:\n");
    Write_KeyStore("PLAYREADY40_Key.store", pbKeyData, uKeySize);
    return Write_KeyStore("PLAYREADY40_Key.store.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_BGROUPCER40_Key(UINT8* pbKeyData,
                                               UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
    INFO_printf("process Secure store: bgroup Certificate 4.0:\n");
    Write_KeyStore("bgroupcert40.dat", pbKeyData, uKeySize);
    return Write_KeyStore("bgroupcert40.dat.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_HDCPPRIV_Key(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: HDCP2X private Key:\n");
    Write_KeyStore("HDCP20RX_Priv.store", pbKeyData, uKeySize);
    return Write_KeyStore("HDCP20RX_Priv.store.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_HDCPLC_Key(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: HDCP2X Local constant:\n");
    Write_KeyStore("HDCP20_LC.store", pbKeyData, uKeySize);
    return Write_KeyStore("HDCP20_LC.store.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_HDCPDN_Key(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: HDCP2X dn key:\n");
    Write_KeyStore("HDCP20RX_dn.store", pbKeyData, uKeySize);
    return Write_KeyStore("HDCP20RX_dn.store.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_HDCPCER_Key(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: HDCP2X certificate:\n");
    Write_KeyStore("HDCP20RX_Certx", pbKeyData, uKeySize);
    return Write_KeyStore("HDCP20RX_Certx.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_CUSTBLOB_Key(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: Customer blob:\n");
    Write_KeyStore("CustBlob.bin", pbKeyData, uKeySize);
    return Write_KeyStore("CustBlob.bin.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_CUSTBLOB1_Key(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: Customer blob 1:\n");
    Write_KeyStore("CustBlob1.bin", pbKeyData, uKeySize);
    return Write_KeyStore("CustBlob1.bin.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_CUSTBLOB2_Key(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: Customer blob 2:\n");
    Write_KeyStore("CustBlob2.bin", pbKeyData, uKeySize);
    return Write_KeyStore("CustBlob2.bin.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_UEI_Key(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: UEI Key:\n");
    Write_KeyStore("UEI.Key", pbKeyData, uKeySize);
    return Write_KeyStore("UEI.Key.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_HDCP2X_WIDI_Key(UINT8* pbKeyData,
                                                 UINT32 uKeySize,
                                                 UINT8* pbHashValue)
{
    INFO_printf("process Secure store: UEI Key:\n");
    Write_KeyStore("HDCP2X_WIDI.store", pbKeyData, uKeySize);
    return Write_KeyStore("HDCP2X_WIDI.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_MOBITV_Key(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: UEI Key:\n");
    Write_KeyStore("MobiTV_key.bin", pbKeyData, uKeySize);
    return Write_KeyStore("MobiTV_key.bin.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_MOBITV_Cert(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: UEI Key:\n");
    Write_KeyStore("MobiTV_cert.pem", pbKeyData, uKeySize);
    return Write_KeyStore("MobiTV_cert.pem.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_DTCP_PVK(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: UEI Key:\n");
    Write_KeyStore("DTCP_PVK.store", pbKeyData, uKeySize);
    return Write_KeyStore("DTCP_PVK.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_DTCP_CRT(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: UEI Key:\n");
    Write_KeyStore("DTCP_cert", pbKeyData, uKeySize);
    return Write_KeyStore("DTCP_cert.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_DTCP_RNG(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: UEI Key:\n");
    Write_KeyStore("DTCP_RNG.store", pbKeyData, uKeySize);
    return Write_KeyStore("DTCP_RNG.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_VUDU_KEY(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: VUDU Key:\n");
    Write_KeyStore("VuduCommonKey.store", pbKeyData, uKeySize);
    return Write_KeyStore("VuduCommonKey.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_MARLIN_KEY(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: MARLIN Key:\n");
    Write_KeyStore("MARLIN_SKB_KEY.store", pbKeyData, uKeySize);
    return Write_KeyStore("MARLIN_SKB_KEY.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_MARLIN_CERT(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: MARLIN CERT:\n");
    Write_KeyStore("MARLIN_PRIV_KEY.bin", pbKeyData, uKeySize);
    return Write_KeyStore("MARLIN_PRIV_KEY.bin.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_CIPLUS_KEY(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: CIPLUS Key:\n");
    Write_KeyStore("CIP_TZ_KEY.store", pbKeyData, uKeySize);
    return Write_KeyStore("CIP_TZ_KEY.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_CIPLUS_CERT(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: CIPLUS CERT:\n");
    Write_KeyStore("CIP_NTZ_KEY.bin", pbKeyData, uKeySize);
    return Write_KeyStore("CIP_NTZ_KEY.bin.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_DTCP_PKG(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: DTCP PKG:\n");
    Write_KeyStore("DTCPEL_KEY.bin", pbKeyData, uKeySize);
    return Write_KeyStore("DTCPEL_KEY.bin.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_ICP_PKG(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: ICP PKG:\n");
    Write_KeyStore("ICP_KEY.bin", pbKeyData, uKeySize);
    return Write_KeyStore("ICP_KEY.bin.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_HDCPPRIV_Key_R(UINT8* pbKeyData, UINT32 uKeySize,
                                                UINT8* pbHashValue)
{
    INFO_printf("process Secure store: HDCP2X Reserved private Key:\n");
    Write_KeyStore("HDCP20RX_Priv_R.store", pbKeyData, uKeySize);
    return Write_KeyStore("HDCP20RX_Priv_R.store.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_HDCPLC_Key_R(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: HDCP2X Reserved Local constant:\n");
    Write_KeyStore("HDCP20_LC_R.store", pbKeyData, uKeySize);
    return Write_KeyStore("HDCP20_LC_R.store.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_HDCPDN_Key_R(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: HDCP2X Reserved dn key:\n");
    Write_KeyStore("HDCP20RX_dn_R.store", pbKeyData, uKeySize);
    return Write_KeyStore("HDCP20RX_dn_R.store.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_HDCPCER_Key_R(UINT8* pbKeyData, UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
    INFO_printf("process Secure store: HDCP2X Reserved certificate:\n");
    Write_KeyStore("HDCP20RX_Certx_R", pbKeyData, uKeySize);
    return Write_KeyStore("HDCP20RX_Certx_R.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_GKEY_KEY(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: GKEY Key:\n");
    Write_KeyStore("GKEY_Key.store", pbKeyData, uKeySize);
    return Write_KeyStore("GKEY_Key.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_CUSTTA(UINT8* pbKeyData, UINT32 uKeySize, UINT8* pbHashValue)
{
    INFO_printf("process Secure store: CUSTOMER TA:\n");
    Write_KeyStore("CUSTOMER_TA.store", pbKeyData, uKeySize);
    return Write_KeyStore("CUSTOMER_TA.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_HDCPPRIV_PADDED(UINT8* pbKeyData, UINT32 uKeySize,
                                                 UINT8* pbHashValue)
{
    INFO_printf("process Secure store: HDCP2X private Key Padded:\n");
    Write_KeyStore("HDCP20RX_Priv_Padded.store", pbKeyData, uKeySize);
    return Write_KeyStore("HDCP20RX_Priv_Padded.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_RPMB_Key(UINT8* pbKeyData,
                                               UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
    INFO_printf("process Secure store: RPMB Key:\n");
    Write_KeyStore("RPMB_Key.store", pbKeyData, uKeySize);
    return Write_KeyStore("RPMB_Key.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_CB_Key(UINT8* pbKeyData,
                                               UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
    INFO_printf("process Secure store: Carrier Billing:\n");
    Write_KeyStore("CarrierBilling_rsakey.store", pbKeyData, uKeySize);
    return Write_KeyStore("CarrierBilling_rsakey.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_CB_Public_Key(UINT8* pbKeyData,
                                               UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
    INFO_printf("process Secure store: Carrier Billing Public Key:\n");
    Write_KeyStore("CarrierBilling_rsakey.pub", pbKeyData, uKeySize);
    return Write_KeyStore("CarrierBilling_rsakey.pub.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_KM_Key(UINT8* pbKeyData,
                                               UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
    INFO_printf("process Secure store: KM2 Key:\n");
    Write_KeyStore("KM_key.store", pbKeyData, uKeySize);
    return Write_KeyStore("KM_key.store.hash", pbHashValue, 20);
}
static HRESULT FIRSTBOOT_Process_KM_Cert(UINT8* pbKeyData,
                                               UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
    INFO_printf("process Secure store: KM2 Certificate:\n");
    Write_KeyStore("KM.cert", pbKeyData, uKeySize);
    return Write_KeyStore("KM.cert.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_CiscoVSSS_Key(UINT8* pbKeyData,
                                               UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
    INFO_printf("process Secure store: Cisco VSSS Key:\n");
    Write_KeyStore("CiscoVSSS_rsakey.store", pbKeyData, uKeySize);
    return Write_KeyStore("CiscoVSSS_rsakey.store.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_CiscoVSSS_Certificate(UINT8* pbKeyData,
                                               UINT32 uKeySize,
                                               UINT8* pbHashValue)
{
    INFO_printf("process Secure store: Cisco VSSS Certificate:\n");
    Write_KeyStore("cisco_vsss_cert.bin", pbKeyData, uKeySize);
    return Write_KeyStore("cisco_vsss_cert.bin.hash", pbHashValue, 20);
}

static HRESULT FIRSTBOOT_Process_OEM_ROOT_RSA(UINT8* pbKeyData, UINT32 uKeySize)
{
    INFO_printf("process OEM_ROOT_RSA.store, uKeySize = %d\n", uKeySize);

    if (gKeyInjectData.pucOemRootRsa != NULL) {
        free(gKeyInjectData.pucOemRootRsa);
    }

    if (uKeySize != 0) {
        gKeyInjectData.pucOemRootRsa = (UINT8*)malloc(uKeySize);
        if (gKeyInjectData.pucOemRootRsa == NULL) {
            ERR_printf("Out of memory\n");
            return E_OUTOFMEMORY;
        }
        memcpy(gKeyInjectData.pucOemRootRsa, pbKeyData, uKeySize);
        gKeyInjectData.uOemRootRsaSize = uKeySize;
    } else {
        ERR_printf("Invalid data package\n");
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT FIRSTBOOT_Process_CUST_KEY(UINT8* pbKeyData, UINT32 uKeySize)
{
    INFO_printf("process OEM_CUSTK.store, uKeySize = %d\n", uKeySize);

    if (gKeyInjectData.pucOemCustk != NULL) {
        free(gKeyInjectData.pucOemCustk);
    }

    if (uKeySize != 0) {
        gKeyInjectData.pucOemCustk = (UINT8*)malloc(uKeySize);
        if (gKeyInjectData.pucOemCustk == NULL) {
            ERR_printf("Out of memory\n");
            return E_OUTOFMEMORY;
        }
        memcpy(gKeyInjectData.pucOemCustk, pbKeyData, uKeySize);
        gKeyInjectData.uOemCustkSize = uKeySize;
    } else {
        ERR_printf("Invalid data package\n");
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT FIRSTBOOT_Process_OEM_EXT_RSA(UINT8* pbKeyData, UINT32 uKeySize)
{
    INFO_printf("process OEM_EXT_RSA.store, uKeySize = %d\n", uKeySize);

    if (gKeyInjectData.pucOemExtRsa != NULL) {
        free(gKeyInjectData.pucOemExtRsa);
    }

    if (uKeySize != 0) {
        gKeyInjectData.pucOemExtRsa = (UINT8*)malloc(uKeySize);
        if (gKeyInjectData.pucOemExtRsa == NULL) {
            ERR_printf("Out of memory\n");
            return E_OUTOFMEMORY;
        }
        memcpy(gKeyInjectData.pucOemExtRsa, pbKeyData, uKeySize);
        gKeyInjectData.uOemExtRsaSize = uKeySize;
    } else {
        ERR_printf("Invalid data package\n");
        return E_FAIL;
    }

    return S_OK;
}

HRESULT mv_drm_firstboot_load_production_preboot(void)
{
    INT8 filename[128] = {0};
    FILE* pFile = NULL;
    UINT32 uImageSize = 0;
    UINT32 uReadSize = 0;

    INFO_printf("process production preboot\n");

    char* sKeystorePath = getenv("BERLIN_KEYSTORE_PATH");
    if (sKeystorePath == NULL) {
          sKeystorePath = FACTORY_SETTING_DIR;
    }
    snprintf(filename, 128, "%s/%s", sKeystorePath, "production.subimg");
    pFile = fopen(filename, "rb");
    if (pFile == NULL) {
        printf("open %s failed\n", filename);
        return E_FAIL;
    }
    fseek(pFile, 0, SEEK_END);
    uImageSize = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    if (gKeyInjectData.pucSysInit != NULL) {
        free(gKeyInjectData.pucSysInit);
    }

    if (uImageSize != 0) {
        gKeyInjectData.pucSysInit = (UINT8*)malloc(uImageSize);
        if (gKeyInjectData.pucSysInit == NULL) {
            ERR_printf("Failed to malloc the memory\n");
            return E_FAIL;
        }
        uReadSize = fread(gKeyInjectData.pucSysInit, 1, uImageSize, pFile);
        if (uReadSize != uImageSize) {
            ERR_printf("Failed to read init image from the file\n");
            return E_FAIL;
        }
        fclose(pFile);
        gKeyInjectData.uSysInitSize = uImageSize;
    } else {
         ERR_printf("Invalid data package\n");
         return E_FAIL;
    }

    return S_OK;
}

static HRESULT MV_DRM_FIRSTBOOT_Init_CustomerOtpKeys(UINT8* pbSectPtr)
{
//  UINT32 i = 0;
//  HRESULT lRes = S_OK;
    UINT8* pbKeyData = NULL;
//  UINT64 uOTPData = 0;
//  UINT32 uEntryIdx = 0;



    IUImage_Section_OTPKeys* pOTPKeyPtr = (IUImage_Section_OTPKeys*)pbSectPtr;
//  IUImage_Section_Common* pSecCommPtr = (IUImage_Section_Common*)&pOTPKeyPtr->tCommon;

    pbKeyData = pOTPKeyPtr->bOTPKeyData;

    INFO_printf("process Store the OTP key Layout\n");

    if (gp_OtpLayout != NULL) {
        free(gp_OtpLayout);
    }

    gp_OtpLayout = (UINT8*)malloc(OTP_PACKAGE_SIZE);
    if (NULL == gp_OtpLayout) {
        return E_OUTOFMEMORY;
    }
    memcpy(gp_OtpLayout, pbKeyData, OTP_PACKAGE_SIZE);
    Write_KeyStore("otp_pack.bin", gp_OtpLayout, OTP_PACKAGE_SIZE);
    return S_OK;
}

static HRESULT MV_DRM_FIRSTBOOT_Check_CustomerSecureBootKeys(UINT8* pbSectPtr)
{
    UINT32 uKeyNum = 0;
    UINT32 i = 0;
    HRESULT lRes = S_OK;
    UINT8* pbKeyPtr = NULL;
    IUImage_Section_ApplicationKeys* pAppKeySectPtr = (IUImage_Section_ApplicationKeys*)pbSectPtr;
    IUImage_Section_Common* pSecCommPtr = (IUImage_Section_Common*)&pAppKeySectPtr->tCommon;
    pbKeyPtr = (UINT8 *)&pAppKeySectPtr->bAppKey;
    Application_Key* pAppKeyPtr = NULL;
    UINT32 uAppKeyType = 0;

    if (pSecCommPtr->uSectionType != IUIMAGE_APPKEYS) {
        printf("Should be a AppKey Section, but the section type is0x%x\n",
                pSecCommPtr->uSectionType);
        return E_INVALIDARG;
    }

    uKeyNum = pAppKeySectPtr->uKeyNum;

    int custk_found = 0;
    int ext_rsa_found = 0;
    int root_rsa_found = 0;

    for (i = 0; i< uKeyNum; i++) {
        pAppKeyPtr = (Application_Key*)pbKeyPtr;
        memcpy((UINT8*)&uAppKeyType, (UINT8*)&pAppKeyPtr->uAppKeyType, sizeof(UINT32));
        switch (uAppKeyType) {
            case APPKEY_CUSTKEY:
                    custk_found = 1;
                    break;
            case APPKEY_OEM_EXT_RSA:
                    ext_rsa_found = 1;
                    break;
            case APPKEY_OEM_ROOT_RSA:
                    root_rsa_found = 1;
                    break;
            default:
                    break;
        }
        // uLen is only key data length,
        // so need add 8 = sizeof(uLen) + sizeof(uAppKeyType);
        // in addition 20 bytes hash value
        pbKeyPtr = pbKeyPtr + ((pAppKeyPtr->uLen + 0x3) & ~0x3) + 8 + 20;

    }

    if ( !custk_found || !ext_rsa_found || !root_rsa_found) {
        return E_FAIL;
    }

    return lRes;
}

static HRESULT MV_DRM_FIRSTBOOT_Check_ValidAppKeys(UINT8* pbSectPtr)
{
    UINT32 uKeyNum = 0;
    UINT32 i = 0;
    UINT8* pbKeyPtr = NULL;
    IUImage_Section_ApplicationKeys* pAppKeySectPtr = (IUImage_Section_ApplicationKeys*)pbSectPtr;
    IUImage_Section_Common* pSecCommPtr = (IUImage_Section_Common*)&pAppKeySectPtr->tCommon;
    pbKeyPtr = (UINT8 *)&pAppKeySectPtr->bAppKey;
    Application_Key* pAppKeyPtr = NULL;
    UINT32 uAppKeyType = 0;

    if (pSecCommPtr->uSectionType != IUIMAGE_APPKEYS) {
        printf("Should be a AppKey Section, but the section type is0x%x\n",
                pSecCommPtr->uSectionType);
        return E_INVALIDARG;
    }

    uKeyNum = pAppKeySectPtr->uKeyNum;

    for (i = 0; i< uKeyNum; i++) {

        HRESULT lRes = E_FAIL;

        pAppKeyPtr = (Application_Key*)pbKeyPtr;
        memcpy((UINT8*)&uAppKeyType, (UINT8*)&pAppKeyPtr->uAppKeyType, sizeof(UINT32));
        switch (uAppKeyType) {

            /* skip secure boot keys */
            case APPKEY_CUSTKEY:
            case APPKEY_OEM_EXT_RSA:
            case APPKEY_OEM_ROOT_RSA:
            case APPKEY_MAC_ADDR:
            case APPKEY_HDCP:
            case APPKEY_NETFLIX_ESN:
            case APPKEY_NETFLIX_MODEL:
            case APPKEY_WMDRM_JANUS:
            case APPKEY_WMDRM_JANUS_CERT_TEMPLATE:
            case APPKEY_CSS:
            case APPKEY_PANDORA:
            case APPKEY_SUNEXT_DRIVE:
            case APPKEY_ULI:
            case APPKEY_WV:
            case APPKEY_CLIENTCA2:
            case APPKEY_CLIENTCA1:
            case APPKEY_CLIENTCA0:
            case APPKEY_CLIENTCER:
            case APPKEY_CLIENTKEY:
            case APPKEY_PRKEY:
            case APPKEY_BGROUPCER:
            case APPKEY_HDCPPRIV:
            case APPKEY_HDCPLC:
            case APPKEY_HDCPDN:
            case APPKEY_HDCPCER:
            case APPKEY_CUSTBLOB:
            case APPKEY_UEIKEY:
            case APPKEY_HDCP2X_WIDI:
            case APPKEY_MOBITV_KEY:
            case APPKEY_MOBITV_CERT:
            case APPKEY_DTCP_PVK:
            case APPKEY_DTCP_CRT:
            case APPKEY_DTCP_RNG:
            case APPKEY_VUDU_KEY:
            case APPKEY_HDCPRX_KEY:
            case APPKEY_MARLIN_KEYS:
            case APPKEY_MARLIN_CERTS:
            case APPKEY_CIPLUS_KEYS:
            case APPKEY_CIPLUS_CERTS:
            case APPKEY_ICP_PKG:
            case APPKEY_DTCP_PKG:
            case APPKEY_HDCPPRIV_R:
            case APPKEY_HDCPLC_R:
            case APPKEY_HDCPDN_R:
            case APPKEY_HDCPCER_R:
            case APPKEY_GKEY:
            case APPKEY_CUSTTA:
            case APPKEY_HDCPPRIV_PADDED:
            case APPKEY_RPMB:
            case APPKEY_WMDRM_JANUS30:
            case APPKEY_WMDRM_JANUS_CERT_TEMPLATE30:
            case APPKEY_PR30KEY:
            case APPKEY_BGROUPCER30:
            case APPKEY_CARRIER_BILLING:
            case APPKEY_CISCO_VSSS_KEY:
            case APPKEY_CISCO_VSSS_CERT:
            case APPKEY_EXTENDED_KEY0:
            case APPKEY_EXTENDED_KEY1:
            case APPKEY_EXTENDED_KEY2:
            case APPKEY_EXTENDED_KEY3:
            case APPKEY_EXTENDED_KEY4:
            case APPKEY_EXTENDED_KEY5:
            case APPKEY_EXTENDED_KEY6:
                    lRes = S_OK;
                    break;
            default:
                    ERR_printf("Invalid App Key type: 0x%x\n", pAppKeyPtr->uAppKeyType);
                    lRes = E_FAIL;
                    break;
        }
        if( lRes != S_OK) {
            ERR_printf("Failed to process APP Key 0x%x\n", pAppKeyPtr->uAppKeyType);
            return E_FAIL;
        }
        // uLen is only key data length,
        // so need add 8 = sizeof(uLen) + sizeof(uAppKeyType);
        // in addition 20 bytes hash value
        pbKeyPtr = pbKeyPtr + ((pAppKeyPtr->uLen + 0x3) & ~0x3) + 8 + 20;

    }

    return S_OK;
}

static HRESULT MV_DRM_FIRSTBOOT_Check_CustomerAppKeys(UINT8* pbSectPtr)
{
    UINT32 uKeyNum = 0;
    UINT32 i = 0;
    UINT8* pbKeyPtr = NULL;
    IUImage_Section_ApplicationKeys* pAppKeySectPtr = (IUImage_Section_ApplicationKeys*)pbSectPtr;
    IUImage_Section_Common* pSecCommPtr = (IUImage_Section_Common*)&pAppKeySectPtr->tCommon;
    pbKeyPtr = (UINT8 *)&pAppKeySectPtr->bAppKey;
    Application_Key* pAppKeyPtr = NULL;
    UINT32 uAppKeyType = 0;

    if (pSecCommPtr->uSectionType != IUIMAGE_APPKEYS) {
        printf("Should be a AppKey Section, but the section type is0x%x\n",
                pSecCommPtr->uSectionType);
        return E_INVALIDARG;
    }

    uKeyNum = pAppKeySectPtr->uKeyNum;

    for (i = 0; i< uKeyNum; i++) {

        HRESULT lRes = E_FAIL;

        pAppKeyPtr = (Application_Key*)pbKeyPtr;
        memcpy((UINT8*)&uAppKeyType, (UINT8*)&pAppKeyPtr->uAppKeyType, sizeof(UINT32));
        switch (uAppKeyType) {

            /* skip secure boot keys */
            case APPKEY_CUSTKEY:
                    lRes = S_OK;
                    break;

            case APPKEY_OEM_EXT_RSA:
                    lRes = S_OK;
                    break;

            case APPKEY_OEM_ROOT_RSA:
                    lRes = S_OK;
                    break;

            case APPKEY_MAC_ADDR:
                    lRes = CheckKeysIntegraty_Status("MAC_ADDR");
                    break;

            case APPKEY_HDCP:
                    lRes = CheckKeysIntegraty_Status("HDCP_Key.store");
                    break;

            case APPKEY_NETFLIX_ESN:
                    lRes = CheckKeysIntegraty_Status("Netflix_ESN_PreShareKey.store");
                    break;

            case APPKEY_NETFLIX_MODEL:
                    lRes = CheckKeysIntegraty_Status("Netflix_Web_Secret.store");
                    break;

            case APPKEY_WMDRM_JANUS:
                    lRes = CheckKeysIntegraty_Status("WMDRM_Key.store");
                    break;

            case APPKEY_WMDRM_JANUS_CERT_TEMPLATE:
                    lRes = CheckKeysIntegraty_Status("devcerttemplate.dat");
                    break;

            case APPKEY_CSS:
                    lRes = CheckKeysIntegraty_Status("CSS_Key.store");
                    break;

            case APPKEY_PANDORA:
                    lRes = CheckKeysIntegraty_Status("PANDORA_Secret.store");
                    break;

            case APPKEY_SUNEXT_DRIVE:
                    lRes = CheckKeysIntegraty_Status("SUNEXT_Key.store");
                    break;

            case APPKEY_ULI:
                    lRes = CheckKeysIntegraty_Status("ulpk.all");
                    break;

            case APPKEY_WV:
                    lRes = CheckKeysIntegraty_Status("kb.bin");
                    break;

            case APPKEY_CLIENTCA2:
                    lRes = CheckKeysIntegraty_Status("client-ca-0.pem");
                    break;

            case APPKEY_CLIENTCA1:
                    lRes = CheckKeysIntegraty_Status("client-ca-1.pem");
                    break;

            case APPKEY_CLIENTCA0:
                    lRes = CheckKeysIntegraty_Status("client-ca-2.pem");
                    break;

            case APPKEY_CLIENTCER:
                    lRes = CheckKeysIntegraty_Status("client.crt");
                    break;

            case APPKEY_CLIENTKEY:
                    lRes = CheckKeysIntegraty_Status("client.key");
                    break;

            case APPKEY_PRKEY:
                    lRes = CheckKeysIntegraty_Status("PLAYREADY_Key.store");
                    break;

            case APPKEY_BGROUPCER:
                    lRes = CheckKeysIntegraty_Status("bgroupcert.dat");
                    break;

            case APPKEY_HDCPPRIV:
                    lRes = CheckKeysIntegraty_Status("HDCP20RX_Priv.store");
                    break;

            case APPKEY_HDCPLC:
                    lRes = CheckKeysIntegraty_Status("HDCP20_LC.store");
                    break;

            case APPKEY_HDCPDN:
                    lRes = CheckKeysIntegraty_Status("HDCP20RX_dn.store");
                    break;

            case APPKEY_HDCPCER:
                    lRes = CheckKeysIntegraty_Status("HDCP20RX_Certx");
                    break;

            case APPKEY_CUSTBLOB:
                    lRes = CheckKeysIntegraty_Status("CustBlob.bin");
                    break;

            case APPKEY_EXTENDED_KEY5:
                    lRes = CheckKeysIntegraty_Status("CustBlob1.bin");
                    break;

            case APPKEY_EXTENDED_KEY6:
                    lRes = CheckKeysIntegraty_Status("CustBlob2.bin");
                    break;

            case APPKEY_UEIKEY:
                    lRes = CheckKeysIntegraty_Status("UEI.Key");
                    break;

            case APPKEY_HDCP2X_WIDI:
                    lRes = CheckKeysIntegraty_Status("HDCP2X_WIDI.store");
                    break;

            case APPKEY_MOBITV_KEY:
                    lRes = CheckKeysIntegraty_Status("MobiTV_key.bin");
                    break;

            case APPKEY_MOBITV_CERT:
                    lRes = CheckKeysIntegraty_Status("MobiTV_cert.pem");
                    break;

            case APPKEY_DTCP_PVK:
                    lRes = CheckKeysIntegraty_Status("DTCP_PVK.store");
                    break;

            case APPKEY_DTCP_CRT:
                    lRes = CheckKeysIntegraty_Status("DTCP_cert");
                    break;

            case APPKEY_DTCP_RNG:
                    lRes = CheckKeysIntegraty_Status("DTCP_RNG.store");
                    break;

            case APPKEY_VUDU_KEY:
                    lRes = CheckKeysIntegraty_Status("VuduCommonKey.store");
                    break;

            case APPKEY_HDCPRX_KEY:
                    lRes = CheckKeysIntegraty_Status("HDCPRX_Key.store");
                    break;

            case APPKEY_MARLIN_KEYS:
                    lRes = CheckKeysIntegraty_Status("MARLIN_SKB_KEY.store");
                    break;

            case APPKEY_MARLIN_CERTS:
                    lRes = CheckKeysIntegraty_Status("MARLIN_PRIV_KEY.bin");
                    break;

            case APPKEY_CIPLUS_KEYS:
                    lRes = CheckKeysIntegraty_Status("CIP_TZ_KEY.store");
                    break;

            case APPKEY_CIPLUS_CERTS:
                    lRes = CheckKeysIntegraty_Status("CIP_NTZ_KEY.bin");
                    break;

            case APPKEY_ICP_PKG:
                    lRes = CheckKeysIntegraty_Status("ICP_KEY.bin");
                    break;

            case APPKEY_DTCP_PKG:
                    lRes = CheckKeysIntegraty_Status("DTCPEL_KEY.bin");
                    break;

            case APPKEY_HDCPPRIV_R:
                    lRes = CheckKeysIntegraty_Status("HDCP20RX_Priv_R.store");
                    break;

            case APPKEY_HDCPLC_R:
                    lRes = CheckKeysIntegraty_Status("HDCP20_LC_R.store");
                    break;

            case APPKEY_HDCPDN_R:
                    lRes = CheckKeysIntegraty_Status("HDCP20RX_dn_R.store");
                    break;

            case APPKEY_HDCPCER_R:
                    lRes = CheckKeysIntegraty_Status("HDCP20RX_Certx_R");
                    break;

            case APPKEY_GKEY:
                    lRes = CheckKeysIntegraty_Status("GKEY_Key.store");
                    break;

            case APPKEY_CUSTTA:
                    lRes = CheckKeysIntegraty_Status("CUSTOMER_TA.store");
                    break;

            case APPKEY_HDCPPRIV_PADDED:
                    lRes = CheckKeysIntegraty_Status("HDCP20RX_Priv_Padded.store");
                    break;

            case APPKEY_RPMB:
                    lRes = CheckKeysIntegraty_Status("RPMB_Key.store");
                    break;

            case APPKEY_WMDRM_JANUS30:
                    lRes = CheckKeysIntegraty_Status("WMDRM30_Key.store");
                    break;

            case APPKEY_WMDRM_JANUS_CERT_TEMPLATE30:
                    lRes = CheckKeysIntegraty_Status("devcerttemplate30.dat");
                    break;

            case APPKEY_PR30KEY:
                    lRes = CheckKeysIntegraty_Status("PLAYREADY30_Key.store");
                    break;

            case APPKEY_BGROUPCER30:
                    lRes = CheckKeysIntegraty_Status("bgroupcert30.dat");
                    break;

            case APPKEY_EXTENDED_KEY3:
                    lRes = CheckKeysIntegraty_Status("PLAYREADY40_Key.store");
                    break;

            case APPKEY_EXTENDED_KEY4:
                    lRes = CheckKeysIntegraty_Status("bgroupcert40.dat");
                    break;

            case APPKEY_CARRIER_BILLING:
                    lRes = CheckKeysIntegraty_Status("CarrierBilling_rsakey.store");
                    break;

            case APPKEY_EXTENDED_KEY0:
                    lRes = CheckKeysIntegraty_Status("CarrierBilling_rsakey.pub");
                    break;

            case APPKEY_EXTENDED_KEY1:
                    lRes = CheckKeysIntegraty_Status("KM_key.store");
                    break;

            case APPKEY_EXTENDED_KEY2:
                    lRes = CheckKeysIntegraty_Status("KM.cert");
                    break;

            case APPKEY_CISCO_VSSS_KEY:
                    lRes = CheckKeysIntegraty_Status("CiscoVSSS_rsakey.store");
                    break;

            case APPKEY_CISCO_VSSS_CERT:
                    lRes = CheckKeysIntegraty_Status("cisco_vsss_cert.bin");
                    break;

            default:
                    ERR_printf("Invalid App Key type: 0x%x\n", pAppKeyPtr->uAppKeyType);
                    lRes = E_FAIL;
                    break;
        }
        if( lRes != S_OK) {
            ERR_printf("Failed while processing APP Key 0x%x\n", pAppKeyPtr->uAppKeyType);
            return E_FAIL;
        }
        // uLen is only key data length,
        // so need add 8 = sizeof(uLen) + sizeof(uAppKeyType);
        // in addition 20 bytes hash value
        pbKeyPtr = pbKeyPtr + ((pAppKeyPtr->uLen + 0x3) & ~0x3) + 8 + 20;

    }

    return S_OK;
}

static HRESULT MV_DRM_FIRSTBOOT_Init_CustomerSecureBootKeys(UINT8* pbSectPtr)
{
    UINT32 uKeyNum = 0;
    UINT32 i = 0;
    HRESULT lRes = S_OK;
    UINT8* pbKeyPtr = NULL;
    IUImage_Section_ApplicationKeys* pAppKeySectPtr = (IUImage_Section_ApplicationKeys*)pbSectPtr;
    IUImage_Section_Common* pSecCommPtr = (IUImage_Section_Common*)&pAppKeySectPtr->tCommon;
    pbKeyPtr = (UINT8 *)&pAppKeySectPtr->bAppKey;
    Application_Key* pAppKeyPtr = NULL;
    UINT32 uAppKeyType = 0;

    if (pSecCommPtr->uSectionType != IUIMAGE_APPKEYS) {
        printf("Should be a AppKey Section, but the section type is0x%x\n",
                pSecCommPtr->uSectionType);
        return E_INVALIDARG;
    }

    uKeyNum = pAppKeySectPtr->uKeyNum;

    for (i = 0; i< uKeyNum; i++) {
        pAppKeyPtr = (Application_Key*)pbKeyPtr;
        memcpy((UINT8*)&uAppKeyType, (UINT8*)&pAppKeyPtr->uAppKeyType, sizeof(UINT32));
        switch (uAppKeyType) {
            case APPKEY_CUSTKEY:
                    lRes = FIRSTBOOT_Process_CUST_KEY(pAppKeyPtr->bAppKeyData,
                                                      pAppKeyPtr->uLen);
                    break;

            case APPKEY_OEM_EXT_RSA:
                    lRes = FIRSTBOOT_Process_OEM_EXT_RSA(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen);
                    break;
            case APPKEY_OEM_ROOT_RSA:
                    lRes = FIRSTBOOT_Process_OEM_ROOT_RSA(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen);
            default:
                    break;
        }
        if(lRes != S_OK) {
            ERR_printf("Failed to process APP Key 0x%x\n", pAppKeyPtr->uAppKeyType);
            return E_FAIL;
        }
        // uLen is only key data length,
        // so need add 8 = sizeof(uLen) + sizeof(uAppKeyType);
        // in addition 20 bytes hash value
        pbKeyPtr = pbKeyPtr + ((pAppKeyPtr->uLen + 0x3) & ~0x3) + 8 + 20;
    }

    return lRes;
}

static HRESULT MV_DRM_FIRSTBOOT_Init_CustomerAppKeys(UINT8* pbSectPtr)
{
    UINT32 uKeyNum = 0;
    UINT32 i = 0;
    HRESULT lRes = S_OK;
    UINT8* pbKeyPtr = NULL;
    IUImage_Section_ApplicationKeys* pAppKeySectPtr = (IUImage_Section_ApplicationKeys*)pbSectPtr;
    IUImage_Section_Common* pSecCommPtr = (IUImage_Section_Common*)&pAppKeySectPtr->tCommon;
    pbKeyPtr = (UINT8 *)&pAppKeySectPtr->bAppKey;
    Application_Key* pAppKeyPtr = NULL;
    UINT32 uAppKeyType = 0;

    if (pSecCommPtr->uSectionType != IUIMAGE_APPKEYS) {
        printf("Should be a AppKey Section, but the section type is0x%x\n",
                pSecCommPtr->uSectionType);
        return E_INVALIDARG;
    }

    uKeyNum = pAppKeySectPtr->uKeyNum;

    for (i = 0; i< uKeyNum; i++) {
        pAppKeyPtr = (Application_Key*)pbKeyPtr;
        memcpy((UINT8*)&uAppKeyType, (UINT8*)&pAppKeyPtr->uAppKeyType, sizeof(UINT32));
        switch (uAppKeyType) {
            /* skip secure boot keys */
            case APPKEY_CUSTKEY:
                    break;
            case APPKEY_OEM_EXT_RSA:
                    break;
            case APPKEY_OEM_ROOT_RSA:
                    break;
            case APPKEY_HDCP:
                    lRes = FIRSTBOOT_Process_HDCP_Key(pAppKeyPtr->bAppKeyData,
                                                      pAppKeyPtr->uLen,
                                                      &pAppKeyPtr->bHashValue[0]);
                    break;

            case APPKEY_MAC_ADDR:
                    lRes = FIRSTBOOT_Process_MacAddr(pAppKeyPtr->bAppKeyData,
                                                     pAppKeyPtr->uLen,
                                                     &pAppKeyPtr->bHashValue[0]);
                    break;

            case APPKEY_NETFLIX_ESN:
                    lRes = FIRSTBOOT_Process_NetflixESN_KEY(pAppKeyPtr->bAppKeyData,
                                                            pAppKeyPtr->uLen,
                                                            &pAppKeyPtr->bHashValue[0]);
                    break;

            case APPKEY_NETFLIX_MODEL:
                    lRes = FIRSTBOOT_Process_NetflixWebSecret_KEY(pAppKeyPtr->bAppKeyData,
                                                                  pAppKeyPtr->uLen,
                                                                  &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_WMDRM_JANUS:
                    lRes = FIRSTBOOT_Process_WMDRM_KeyStore(pAppKeyPtr->bAppKeyData,
                                                            pAppKeyPtr->uLen,
                                                            &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_WMDRM_JANUS_CERT_TEMPLATE:
                    lRes = FIRSTBOOT_Process_WMDRM_CertTemplate(pAppKeyPtr->bAppKeyData,
                                                                pAppKeyPtr->uLen,
                                                                &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_CSS:
                    lRes = FIRSTBOOT_Process_CSS_Key(pAppKeyPtr->bAppKeyData,
                                                     pAppKeyPtr->uLen,
                                                     &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_PANDORA:
                    lRes = FIRSTBOOT_Process_PANDORA_Secret(pAppKeyPtr->bAppKeyData,
                                                            pAppKeyPtr->uLen,
                                                            &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_SUNEXT_DRIVE:
                    lRes = FIRSTBOOT_Process_SUNEXT_Key(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen,
                                                        &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_ULI:
                    lRes = FIRSTBOOT_Process_ULPK_Key(pAppKeyPtr->bAppKeyData,
                                                      pAppKeyPtr->uLen,
                                                      &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_WV:
                    lRes = FIRSTBOOT_Process_WVK_Key(pAppKeyPtr->bAppKeyData,
                                                     pAppKeyPtr->uLen,
                                                     &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_CLIENTCA2:
                    lRes = FIRSTBOOT_Process_CLIENTCA0_Key(pAppKeyPtr->bAppKeyData,
                                                           pAppKeyPtr->uLen,
                                                           &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_CLIENTCA1:
                    lRes = FIRSTBOOT_Process_CLIENTCA1_Key(pAppKeyPtr->bAppKeyData,
                                                           pAppKeyPtr->uLen,
                                                           &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_CLIENTCA0:
                    lRes = FIRSTBOOT_Process_CLIENTCA2_Key(pAppKeyPtr->bAppKeyData,
                                                           pAppKeyPtr->uLen,
                                                           &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_CLIENTCER:
                    lRes = FIRSTBOOT_Process_CLIENTCER_Key(pAppKeyPtr->bAppKeyData,
                                                           pAppKeyPtr->uLen,
                                                           &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_CLIENTKEY:
                    lRes = FIRSTBOOT_Process_CLIENTKEY_Key(pAppKeyPtr->bAppKeyData,
                                                           pAppKeyPtr->uLen,
                                                           &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_PRKEY:
                    lRes = FIRSTBOOT_Process_PRKEY_Key(pAppKeyPtr->bAppKeyData,
                                                       pAppKeyPtr->uLen,
                                                       &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_BGROUPCER:
                    lRes = FIRSTBOOT_Process_BGROUPCER_Key(pAppKeyPtr->bAppKeyData,
                                                           pAppKeyPtr->uLen,
                                                           &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_HDCPPRIV:
                    lRes = FIRSTBOOT_Process_HDCPPRIV_Key(pAppKeyPtr->bAppKeyData,
                                                          pAppKeyPtr->uLen,
                                                          &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_HDCPLC:
                    lRes = FIRSTBOOT_Process_HDCPLC_Key(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen,
                                                        &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_HDCPDN:
                    lRes = FIRSTBOOT_Process_HDCPDN_Key(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen,
                                                        &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_HDCPCER:
                    lRes = FIRSTBOOT_Process_HDCPCER_Key(pAppKeyPtr->bAppKeyData,
                                                         pAppKeyPtr->uLen,
                                                         &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_CUSTBLOB:
                    lRes = FIRSTBOOT_Process_CUSTBLOB_Key(pAppKeyPtr->bAppKeyData,
                                                          pAppKeyPtr->uLen,
                                                          &pAppKeyPtr->bHashValue[0]);

            case APPKEY_EXTENDED_KEY5:
                    lRes = FIRSTBOOT_Process_CUSTBLOB1_Key(pAppKeyPtr->bAppKeyData,
                                                          pAppKeyPtr->uLen,
                                                          &pAppKeyPtr->bHashValue[0]);
            case APPKEY_EXTENDED_KEY6:
                    lRes = FIRSTBOOT_Process_CUSTBLOB2_Key(pAppKeyPtr->bAppKeyData,
                                                          pAppKeyPtr->uLen,
                                                          &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_UEIKEY:
                    lRes = FIRSTBOOT_Process_UEI_Key(pAppKeyPtr->bAppKeyData,
                                                     pAppKeyPtr->uLen,
                                                     &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_HDCP2X_WIDI:
                    lRes = FIRSTBOOT_Process_HDCP2X_WIDI_Key(pAppKeyPtr->bAppKeyData,
                                                             pAppKeyPtr->uLen,
                                                             &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_MOBITV_KEY:
                    lRes = FIRSTBOOT_Process_MOBITV_Key(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen,
                                                        &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_MOBITV_CERT:
                    lRes = FIRSTBOOT_Process_MOBITV_Cert(pAppKeyPtr->bAppKeyData,
                                                         pAppKeyPtr->uLen,
                                                         &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_DTCP_PVK:
                    lRes = FIRSTBOOT_Process_DTCP_PVK(pAppKeyPtr->bAppKeyData,
                                                      pAppKeyPtr->uLen,
                                                      &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_DTCP_CRT:
                    lRes = FIRSTBOOT_Process_DTCP_CRT(pAppKeyPtr->bAppKeyData,
                                                      pAppKeyPtr->uLen,
                                                      &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_DTCP_RNG:
                    lRes = FIRSTBOOT_Process_DTCP_RNG(pAppKeyPtr->bAppKeyData,
                                                      pAppKeyPtr->uLen,
                                                      &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_VUDU_KEY:
                    lRes = FIRSTBOOT_Process_VUDU_KEY(pAppKeyPtr->bAppKeyData,
                                                      pAppKeyPtr->uLen,
                                                      &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_HDCPRX_KEY:
                    lRes = FIRSTBOOT_Process_HDCPRX_Key(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen,
                                                        &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_MARLIN_KEYS:
                    lRes = FIRSTBOOT_Process_MARLIN_KEY(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen,
                                                        &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_MARLIN_CERTS:
                    lRes = FIRSTBOOT_Process_MARLIN_CERT(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen,
                                                        &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_CIPLUS_KEYS:
                    lRes = FIRSTBOOT_Process_CIPLUS_KEY(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen,
                                                        &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_CIPLUS_CERTS:
                    lRes = FIRSTBOOT_Process_CIPLUS_CERT(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen,
                                                        &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_ICP_PKG:
                    lRes = FIRSTBOOT_Process_ICP_PKG(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen,
                                                        &pAppKeyPtr->bHashValue[0]);
                    break;

            case APPKEY_DTCP_PKG:
                    lRes = FIRSTBOOT_Process_DTCP_PKG(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen,
                                                        &pAppKeyPtr->bHashValue[0]);
                    break;

            case APPKEY_HDCPPRIV_R:
                    lRes = FIRSTBOOT_Process_HDCPPRIV_Key_R(pAppKeyPtr->bAppKeyData,
                                                          pAppKeyPtr->uLen,
                                                          &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_HDCPLC_R:
                    lRes = FIRSTBOOT_Process_HDCPLC_Key_R(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen,
                                                        &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_HDCPDN_R:
                    lRes = FIRSTBOOT_Process_HDCPDN_Key_R(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen,
                                                        &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_HDCPCER_R:
                    lRes = FIRSTBOOT_Process_HDCPCER_Key_R(pAppKeyPtr->bAppKeyData,
                                                         pAppKeyPtr->uLen,
                                                         &pAppKeyPtr->bHashValue[0]);
            case APPKEY_GKEY:
                    lRes = FIRSTBOOT_Process_GKEY_KEY(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen,
                                                        &pAppKeyPtr->bHashValue[0]);
                    break;

            case APPKEY_CUSTTA:
                    lRes = FIRSTBOOT_Process_CUSTTA(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen,
                                                        &pAppKeyPtr->bHashValue[0]);
                    break;

            case APPKEY_HDCPPRIV_PADDED:
                    lRes = FIRSTBOOT_Process_HDCPPRIV_PADDED(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen,
                                                        &pAppKeyPtr->bHashValue[0]);
                    break;

            case APPKEY_RPMB:
                    lRes = FIRSTBOOT_Process_RPMB_Key(pAppKeyPtr->bAppKeyData,
                                                        pAppKeyPtr->uLen,
                                                        &pAppKeyPtr->bHashValue[0]);
                    break;

            case APPKEY_WMDRM_JANUS30:
                    lRes = FIRSTBOOT_Process_WMDRM30_KeyStore(pAppKeyPtr->bAppKeyData,
                                                            pAppKeyPtr->uLen,
                                                            &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_WMDRM_JANUS_CERT_TEMPLATE30:
                    lRes = FIRSTBOOT_Process_WMDRM_CertTemplate30(pAppKeyPtr->bAppKeyData,
                                                                pAppKeyPtr->uLen,
                                                                &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_PR30KEY:
                    lRes = FIRSTBOOT_Process_PR30KEY_Key(pAppKeyPtr->bAppKeyData,
                                                       pAppKeyPtr->uLen,
                                                       &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_BGROUPCER30:
                    lRes = FIRSTBOOT_Process_BGROUPCER30_Key(pAppKeyPtr->bAppKeyData,
                                                           pAppKeyPtr->uLen,
                                                           &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_EXTENDED_KEY3:
                    lRes = FIRSTBOOT_Process_PR40KEY_Key(pAppKeyPtr->bAppKeyData,
                                                       pAppKeyPtr->uLen,
                                                       &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_EXTENDED_KEY4:
                    lRes = FIRSTBOOT_Process_BGROUPCER40_Key(pAppKeyPtr->bAppKeyData,
                                                           pAppKeyPtr->uLen,
                                                           &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_CARRIER_BILLING:
                    lRes = FIRSTBOOT_Process_CB_Key(pAppKeyPtr->bAppKeyData,
                                                           pAppKeyPtr->uLen,
                                                           &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_EXTENDED_KEY0:
                    lRes = FIRSTBOOT_Process_CB_Public_Key(pAppKeyPtr->bAppKeyData,
                                                           pAppKeyPtr->uLen,
                                                           &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_EXTENDED_KEY1:
                    lRes = FIRSTBOOT_Process_KM_Key(pAppKeyPtr->bAppKeyData,
                                                       pAppKeyPtr->uLen,
                                                       &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_EXTENDED_KEY2:
                    lRes = FIRSTBOOT_Process_KM_Cert(pAppKeyPtr->bAppKeyData,
                                                           pAppKeyPtr->uLen,
                                                           &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_CISCO_VSSS_KEY:
                    lRes = FIRSTBOOT_Process_CiscoVSSS_Key(pAppKeyPtr->bAppKeyData,
                                                           pAppKeyPtr->uLen,
                                                           &pAppKeyPtr->bHashValue[0]);
                    break;
            case APPKEY_CISCO_VSSS_CERT:
                    lRes = FIRSTBOOT_Process_CiscoVSSS_Certificate(pAppKeyPtr->bAppKeyData,
                                                           pAppKeyPtr->uLen,
                                                           &pAppKeyPtr->bHashValue[0]);
                    break;
            default:
                    ERR_printf("Invalid App Key type: 0x%x\n", pAppKeyPtr->uAppKeyType);
                    break;
        }
        if(lRes != S_OK) {
            ERR_printf("Failed to process APP Key 0x%x\n", pAppKeyPtr->uAppKeyType);
            return E_FAIL;
        }
        // uLen is only key data length,
        // so need add 8 = sizeof(uLen) + sizeof(uAppKeyType);
        // in addition 20 bytes hash value
        pbKeyPtr = pbKeyPtr + ((pAppKeyPtr->uLen + 0x3) & ~0x3) + 8 + 20;

    }

    return lRes;
}

HRESULT mv_drm_firstboot_get_serialid(UINT8* pbSerialID, UINT32 uSize)
{
     /* todo */
     return S_OK;
}

HRESULT mv_drm_firstboot_check_initimage_hash(UINT8* pbPackage, UINT32 uPackSize,
                                           UINT8* pbHash, UINT32 uHashSize )
{
    HRESULT lRes = S_OK;
    UINT8 pbDigest[20] ={0};
    SHA1CTX shaCtx;

    if (pbPackage == NULL || pbHash == NULL || uHashSize != 20) {
        ERR_printf("Invalid arguments."
               "pbPackage = %p, pbHash =%p, uHashSize = 0x%x\n",
                pbPackage, pbHash, uHashSize);
        return E_INVALIDARG;
    }

    SHA1_Init(&shaCtx);
    SHA1_Update(&shaCtx, pbPackage, uPackSize);
    SHA1_Final(pbDigest, &shaCtx);

    if (0 != memcmp(pbHash, pbDigest, uHashSize))
    {
        ERR_printf("IUImage Hash check Failed\n");
        return E_FAIL;
    } else {
        INFO_printf("Hash check is ok\n");
    }

    return lRes;

}

HRESULT mv_drm_firstboot_check_initimage(UINT8* pbPackage, UINT32 uPackSize)
{
    HRESULT lRes = S_OK;
    UINT8*  pbPackPtr = pbPackage;
    IUImage_Section_Common* pSecCommPtr = NULL;
    IUImage_Section_Header*  pImageHeader = NULL;

    //! Step 1: Check the Hash value in Image Header
    pSecCommPtr = (IUImage_Section_Common*)pbPackPtr;
    if (pSecCommPtr->uSectionType != IUIMAGE_HEADER) {
        ERR_printf("Error: Invalid Image package!"
               "The first section is not a Image hader, Section Type = 0x%x\n",
                pSecCommPtr->uSectionType);
        return E_INVALIDARG;
    }

    pImageHeader = (IUImage_Section_Header*)pbPackPtr;
    if ((pImageHeader->uTotalLen + pSecCommPtr->uSectionLength) != uPackSize) {
        ERR_printf("Error: Invalid Image Size!"
                "Total Size should be = 0x%x while transferred size = 0x%x\n",
                pImageHeader->uTotalLen + pSecCommPtr->uSectionLength, uPackSize);
        return E_INVALIDARG;
    }
    if (S_OK != mv_drm_firstboot_check_initimage_hash(pbPackage + pSecCommPtr->uSectionLength,
                                                  pImageHeader->uTotalLen,
                                                  pImageHeader->bHashValue, 20)) {
        ERR_printf("Error: Image Hash Check Failed, Invalid IU image package\n");
        return E_INVALIDARG;
    }
    pbPackPtr = pbPackPtr + pSecCommPtr->uSectionLength;   // Move to the Next Section

    int otp_found = 0;
    int app_keys_found = 0;

    //! Step 2: Parse each part of the IU image
    while(pbPackPtr != (pbPackage + uPackSize)) {
        pSecCommPtr =(IUImage_Section_Common*)pbPackPtr;

        switch(pSecCommPtr->uSectionType) {
            case IUIMAGE_OTPKEYS:
                otp_found = 1;
                break;

            case IUIMAGE_APPKEYS:
#ifndef CONFIG_MV_AMP_DRM_NAGRA_UPDATE_TSM_ONLY
                if ( MV_DRM_FIRSTBOOT_Check_CustomerSecureBootKeys(pbPackPtr) != S_OK ) {
                    ERR_printf("Missing required customer app keys\n");
                    return E_INVALIDARG;
                }
#endif
                if ( MV_DRM_FIRSTBOOT_Check_ValidAppKeys(pbPackPtr) != S_OK ) {
                    ERR_printf("Unsupported app keys present\n");
                    return E_INVALIDARG;
                }
                app_keys_found = 1;
                break;

            case IUIMAGE_SIGNATURES:
                break;

            default:
                ERR_printf("Unrecognized section\n");
                return E_INVALIDARG;
        }

        pbPackPtr = pbPackPtr + pSecCommPtr->uSectionLength;
    }

    if ( !otp_found || !app_keys_found ) {
        ERR_printf("Missing required section\n");
        return E_INVALIDARG;
    }

    return lRes;

}

HRESULT mv_drm_firstboot_check_appkeys(UINT8* pbPackage, UINT32 uPackSize)
{
    UINT8*  pbPackPtr = pbPackage;
    IUImage_Section_Common* pSecCommPtr = NULL;
    IUImage_Section_Header*  pImageHeader = NULL;

    pSecCommPtr = (IUImage_Section_Common*)pbPackPtr;
    pImageHeader = (IUImage_Section_Header*)pbPackPtr;
    pbPackPtr = pbPackPtr + pSecCommPtr->uSectionLength;   // Move to the Next Section
    pImageHeader = (IUImage_Section_Header*)pbPackPtr;

    while(pbPackPtr != (pbPackage + uPackSize)) {
        pSecCommPtr =(IUImage_Section_Common*)pbPackPtr;

        if ( pSecCommPtr->uSectionType == IUIMAGE_APPKEYS ) {

            if (MV_DRM_FIRSTBOOT_Check_CustomerAppKeys(pbPackPtr) != S_OK) {
              ERR_printf("Failed to check app keys\n");
              return E_FAIL;
            }
        }
        pbPackPtr = pbPackPtr + pSecCommPtr->uSectionLength;
    }

    return S_OK;
}

HRESULT mv_drm_load_secure_boot_keys(UINT8* pbPackage, UINT32 uPackSize)
{
    UINT8*  pbPackPtr = pbPackage;
    IUImage_Section_Common* pSecCommPtr = NULL;
    IUImage_Section_Header*  pImageHeader = NULL;

    pSecCommPtr = (IUImage_Section_Common*)pbPackPtr;
    pImageHeader = (IUImage_Section_Header*)pbPackPtr;
    pbPackPtr = pbPackPtr + pSecCommPtr->uSectionLength;   // Move to the Next Section

    while(pbPackPtr != (pbPackage + uPackSize)) {
        pSecCommPtr =(IUImage_Section_Common*)pbPackPtr;
        if ( pSecCommPtr->uSectionType == IUIMAGE_APPKEYS) {
            if ( MV_DRM_FIRSTBOOT_Init_CustomerSecureBootKeys(pbPackPtr) != S_OK) {
                ERR_printf("Failed to load secure boot keys\n");
                return E_FAIL;
            }
        }
        pbPackPtr = pbPackPtr + pSecCommPtr->uSectionLength;
    }

    return S_OK;
}

HRESULT mv_drm_load_otp_layout(UINT8* pbPackage, UINT32 uPackSize)
{
    UINT8*  pbPackPtr = pbPackage;
    IUImage_Section_Common* pSecCommPtr = NULL;
    IUImage_Section_Header*  pImageHeader = NULL;

    pSecCommPtr = (IUImage_Section_Common*)pbPackPtr;
    pImageHeader = (IUImage_Section_Header*)pbPackPtr;
    pbPackPtr = pbPackPtr + pSecCommPtr->uSectionLength;   // Move to the Next Section

    while(pbPackPtr != (pbPackage + uPackSize)) {
        pSecCommPtr =(IUImage_Section_Common*)pbPackPtr;

        if ( pSecCommPtr->uSectionType == IUIMAGE_OTPKEYS) {

            if ( MV_DRM_FIRSTBOOT_Init_CustomerOtpKeys(pbPackPtr) != S_OK) {
                ERR_printf("Failed to load otp layout\n");
                return E_FAIL;
            }
        }

        pbPackPtr = pbPackPtr + pSecCommPtr->uSectionLength;
    }

    return S_OK;

}

HRESULT mv_drm_load_otp_layout_evt(UINT8* pbPackage, UINT32 uPackSize)
{
    INFO_printf("process Store the OTP key Layout\n");

    if (pbPackage == NULL) {
        return E_FAIL;
    }

    if (gp_OtpLayout != NULL) {
        free(gp_OtpLayout);
    }

    gp_OtpLayout = (UINT8*)malloc(OTP_PACKAGE_SIZE);
    if (NULL == gp_OtpLayout) {
        return E_OUTOFMEMORY;
    }
    memcpy(gp_OtpLayout, pbPackage, OTP_PACKAGE_SIZE);

    return S_OK;
}

HRESULT mv_drm_firstboot_write_appkeys(UINT8* pbPackage, UINT32 uPackSize)
{
    UINT8*  pbPackPtr = pbPackage;
    IUImage_Section_Common* pSecCommPtr = NULL;
    IUImage_Section_Header*  pImageHeader = NULL;

    pSecCommPtr = (IUImage_Section_Common*)pbPackPtr;
    pImageHeader = (IUImage_Section_Header*)pbPackPtr;

    if (Write_KeyStore("Unique_ID.bin", (UINT8 *)&pImageHeader->uUniqueID, 4) != S_OK) {
        ERR_printf("Failed to write Unique ID to board, please check it\n");
        return E_FAIL;
    }

    pbPackPtr = pbPackPtr + pSecCommPtr->uSectionLength;   // Move to the Next Section

    while(pbPackPtr != (pbPackage + uPackSize)) {
        pSecCommPtr =(IUImage_Section_Common*)pbPackPtr;

        if ( pSecCommPtr->uSectionType == IUIMAGE_APPKEYS) {

            if ( MV_DRM_FIRSTBOOT_Init_CustomerAppKeys(pbPackPtr) != S_OK) {
                ERR_printf("Failed to write app keys\n");
                return E_FAIL;
            }

        }
        pbPackPtr = pbPackPtr + pSecCommPtr->uSectionLength;
    }

    return S_OK;

}

HRESULT mv_drm_firstboot_init_finish()
{
    HRESULT lRes = S_OK;
    UINT8 pKeyData[4] = {0};

    Write_KeyStore("keyinjection.done", pKeyData, 4);
    FIRSTBOOT_Delete_Key("production.subimg");
    FIRSTBOOT_Delete_Key("otp_pack.bin");

    //! Release the resource alloc in section parse
    if (gp_OtpLayout!= NULL) {
        free(gp_OtpLayout);
        gp_OtpLayout = NULL;
    }

    return lRes;
}

HRESULT mv_drm_firstboot_get_uniqueid(unsigned long * unique_id)
{
    HRESULT lRes = S_OK;
    FILE* pFile = NULL;
//  UINT32 uFileLen = 0;
    UINT32 uReadLen = 0;
    UINT8 * pFileData = NULL;

    INT8   pDstFileName[128] = {0};
    INT8   pDstFilePATH[128] = {0};
//  UINT32  status = 0;
//  UINT8   bCRC32Value[4] = {0};
//  UINT32  uCheckCRC32Value = 0;

    char* sKeystorePath = getenv("BERLIN_KEYSTORE_PATH");
    if (sKeystorePath == NULL) {
         sKeystorePath = FACTORY_SETTING_DIR;
    }
    snprintf(pDstFilePATH, 128, "%s", sKeystorePath);
    snprintf(pDstFileName, 128, "%s/%s", pDstFilePATH, "Unique_ID.bin");

    pFile = fopen(pDstFileName, "rb");
    if (pFile == NULL) {
        ERR_printf("No %s existed, this maybe the first time key injection\n", pDstFileName);
        *unique_id = 0;
        goto func_ret;
    }
    uReadLen = fread(unique_id, 1, 4, pFile);
    if (uReadLen != 4) {
        memset(unique_id, 0, 4);
        ERR_printf("Failed to read Unique_ID.bin, please check it\n");
        lRes = E_FAIL;
    }
func_ret:
    if (pFile != NULL) {
        fclose(pFile);
    }

    if (pFileData != NULL) {
        free(pFileData);
    }
    return lRes;

}

HRESULT mv_drm_cipher_otp_burner(void)
{
    INT8 filename[128] = {0};
    char* sKeystorePath = getenv("BERLIN_KEYSTORE_PATH");

    if (sKeystorePath == NULL) {
        sKeystorePath = FACTORY_SETTING_DIR;
    }
    snprintf(filename, 128, "%s/%s", sKeystorePath, "otp_pack.bin");

    return MV_OTP_BCM_Layout_Program((UCHAR*)filename);
}

HRESULT mv_drm_firstboot_check_keyinjection(void)
{
    FILE* pFile = NULL;
    HRESULT lRes = S_OK;
    UINT32 uFileLen = 0;
    UINT32 uReadLen = 0;
    UINT8 * pFileData = NULL;
    INT8 pDstFileName[128] = {0};
    INT8 pDstFilePATH[128] = {0};

    char* sKeystorePath = getenv("BERLIN_KEYSTORE_PATH");
    if (sKeystorePath == NULL) {
        sKeystorePath = FACTORY_SETTING_DIR;
    }
    snprintf(pDstFilePATH, 128, "%s", sKeystorePath);
    snprintf(pDstFileName, 128, "%s/keyinjection.done", pDstFilePATH);

    pFile = fopen(pDstFileName, "rb");
    if (pFile != NULL) {
        lRes = E_FAIL;
        ERR_printf("Already run key injection on this board!");
        goto func_ret;
    }

func_ret:
    if(pFile != NULL) {
        fclose(pFile);
    }

    return lRes;
}

HRESULT mv_drm_firstboot_update_preboot(void)
{
    HRESULT lRes = S_OK;
    UINT8 *pbpreboot = NULL;

    pbpreboot = (UINT8*)malloc(PREBOOT_UPDATE_SIZE);
    if (pbpreboot == NULL) {
        ERR_printf("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    memcpy(pbpreboot, gKeyInjectData.pucSysInit, gKeyInjectData.uSysInitSize);
    memcpy(pbpreboot, gKeyInjectData.pucOemRootRsa, gKeyInjectData.uOemRootRsaSize);
    lRes = mtd_preboot_write(pbpreboot, PREBOOT_UPDATE_SIZE);
    if (lRes != S_OK) {
        ERR_printf("Can't update preboot area! Error code = %d\n", lRes);
    }

    free(pbpreboot);

    return lRes;
}

HRESULT mv_drm_firstboot_update_key1_key2(void)
{
    HRESULT lRes = S_OK;
    UINT8 *pbkeys = NULL;

    pbkeys = (UINT8*)malloc(KEY_1_2_UPDATE_SIZE);
    if (pbkeys == NULL) {
        ERR_printf("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    memcpy(pbkeys, gKeyInjectData.pucOemCustk, gKeyInjectData.uOemCustkSize);
    memcpy(pbkeys + CUSTK_SIZE, gKeyInjectData.pucOemExtRsa, gKeyInjectData.uOemExtRsaSize);
    lRes = mtd_keys_write(pbkeys, KEY_1_2_UPDATE_SIZE);
    if (lRes != S_OK) {
        ERR_printf("Can't update key1st/key2nd! Error code = %d\n", lRes);
    }

    free(pbkeys);

    return lRes;
}

HRESULT mv_drm_firstboot_read_spi(int addr)
{
    HRESULT lRes = S_OK;
    UINT8 *pbBuf = NULL;
    UINT8 i = 0;

    pbBuf = (UINT8*)malloc(128);
    if (pbBuf == NULL) {
        ERR_printf("Out of memory\n");
        return E_OUTOFMEMORY;
    }

//    lRes = mtd_read(pbBuf, "/dev/mtd1", addr, 128);
    lRes = mtd_read(pbBuf, "/dev/mtd13", addr, 128);
    if (lRes != S_OK) {
        ERR_printf("Can't read SPI!\n");
        goto func_ret;
    }

    for (i = 0; i < 128/4; i++) {
        printf("%08x ", *(UINT32 *)(pbBuf + i * 4));
        if (((i + 1)% 4) == 0) {
            printf("\n");
        }
    }

    free(pbBuf);

func_ret:
    return lRes;
}

