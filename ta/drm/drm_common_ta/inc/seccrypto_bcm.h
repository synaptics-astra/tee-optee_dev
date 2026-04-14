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
//! \file    seccrypto_bcm.h
//! \brief   Extended Secure Crypto DRM BCM Entry Functions
//! \author  Girish Shetty
//! \date    Dec 2014
///////////////////////////////////////////////////////////////////////////////


#ifndef __SECCRYPTO_BCM_H__
#define __SECCRYPTO_BCM_H__

//BCM AES modes
#define BCM_AES_ECB      0
#define BCM_AES_CBC      1
#define BCM_AES_CTR      2

//Algo
#define CRYPTO_DVBCSA2_TYPE 2
#define CRYPTO_DVBCSA3_TYPE 3
#define CRYPTO_AES_TYPE     4
#define CRYPTO_INVAES_TYPE  5
#define CRYPTO_TDES_TYPE    6
#define CRYPTO_INVTDES_TYPE 7

typedef enum {
    BCM_SRAM_READ = 0,
    BCM_SRAM_WRITE = 1
} BCM_SRAM_ACCESS_T;

/*
 * Function: BCM_GENERATE_SECUREKEY_V2
 *   Generate Secrete Key per chip unique ID (CUID)
 *
 * Arguments:
 *   phyAddrUUID          : Phy Address of UUID
 *   phyAddrAppID         : Phy Address of APPID
 *   uAppVariant          : Application variant
 *   phyAddrSecureKey     : Output buffer key
 *
 * Returns:
 *   SUCCESS or one of the BCM error code
 */

HRESULT BCM_GENERATE_SECUREKEY_V2(UINT32 phyAddrUUID,
                        UINT32 phyAddrAppID , UINT32 uAppVariant ,
                        UINT32 phyAddrSecureKey);

/*===============================================================================================
 * Verimatrix
 *===============================================================================================*/
/*
 * Function: BCM_SECUREVMX_INIT
 *   BCM routine to initialize Verimatrix DRM
 *
 * Arguments: None
 *
 * Returns:
 *   SUCCESS or one of the BCM error code
 */
HRESULT BCM_SECUREVMX_INIT(void);

/*
 * Function: BCM_SECUREVMX_GETKEYID
 * Read the Verimatrix KeyID from OTP fields
 * Verimatrix Key ID is 8 bytes value in OTP
 *
 * Arguments:
 * - AddrKeyID  (OUT) - buffer to copy the VMX keyID, should be at least 8 bytes in size
 *
 * Return:
 *         STATUS_SUCCESS or one of the BCM error code
 */
HRESULT BCM_SECUREVMX_GETKEYID(UINT8* AddrKeyID);

/*
 * Function: BCM_SECUREVMX_KEYLADDER
 *   Setup the key ladder for en/decryption of input data
 *   This API is used for encrypting/decrypting data using Level 2 key laddering
 *
 * Arguments:
 * - AddrSCHEMEKey  (IN) -  Secure Kvmx
 * - uSCHEMEKeylen     (IN) -  Size of secure Kvmx - 16 bytes
 * - AddrEMMKey     (IN) -  Secure Kcwp - Control Word Protection key - Kvmx[Kcwp]
 * - uEMMKeyLen        (IN) -  Size of secure Kcwp - 16 bytes
 * - AddrECMKey     (IN) -  Secure Control Word - Kcwp[Kcw]
 * - uECMKeylen        (IN) -  Size of Kcwp[Kcw] - 16 bytes
 * - AddrCWC        (IN) -  Control Word Control Info - CWC
 * - uCWCLen           (IN) -  Size of CWC
 * - uKeyOffset        (IN) -  offset within tsp key table
 * - uAlgo             (IN) -  content key algorithm
 * - uMode             (IN) -  content key mode
 *
 * Return:
 *         STATUS_SUCCESS or one of the BCM error code
 */
HRESULT BCM_SECUREVMX_KEYLADDER(UINT8* AddrSCHEMEKey, UINT32 uSCHEMEKeylen,
        UINT8* AddrEMMKey, UINT32 uEMMKeyLen, UINT8* AddrECMKey, UINT32 uECMKeylen,
        UINT8* AddrCWC, UINT32 uCWCLen, UINT32 uKeyOffset, UINT32 uAlgo, UINT32 uMode);

/*
 * Function: BCM_SECUREVMX_LOADCW
 *   Loads the VMX Control Word (CW) key into BCM tsp key table
 *
 * Arguments:
 * - AddrKeyBuffer  (IN) - CW key
 * - uKeySize          (IN) - length of CW key, should be 16 bytes
 * - uKeyOffset        (IN) - offset within tsp key table
 * - uAlgo             (IN) -  content key algorithm
 * - uMode             (IN) - content key algorithm mode
 *
 * Return:
 *         STATUS_SUCCESS or one of the BCM error code
 */
HRESULT BCM_SECUREVMX_LOADCW(UINT8* AddrKeyBuffer, UINT32 uKeySize,
        UINT8 uKeyOffset,UINT32 uAlgo, UINT32 uMode);

/*
 * Function: BCM_SECUREVMX_GETROMKSTATUS
 * Read the ROMK_DISABLE OTP field
 *
 * Arguments:
 * - AddrRomkStatus  (OUT) - pointer to copy the ROMK_DISABLE status
 *
 * Return:
 *         STATUS_SUCCESS or one of the BCM error code
 */
HRESULT BCM_SECUREVMX_GETROMKSTATUS(UINT32* AddrRomkStatus);

/*
 * Function: BCM_SECUREVMX_GETRANDOM
 *   Generates a random byte stream of desired length
 *
 * Arguments:
 *   addrRandom(IN): address of buffer to store random bumber
 *   uRandomSize(IN):   size (in bytes) of random number
 */
HRESULT BCM_SECUREVMX_GETRANDOM(UINT8* addrRandom, UINT32 uRandomSize);

/*
 * Function: BCM_SECUREVMX_LOADCLEARIV
 *   Loads the Clear IV into BCM tsp key table
 *
 * Arguments:
 *   addrIV          : the buffer containing the clear IV
 *   uIVSize         : the length of the IV in the buffer addrIVBuffer
 *   uKeyTableOffset : the key table offset of entry in which the IV will be stored
 *   uAlgo           : the algorithm of the IV used for
 *   uMode           : the mode of the IV used for
 *
 * Returns:
 *   SUCCESS or one of the BCM error code
 */

HRESULT BCM_SECUREVMX_LOADCLEARIV(UINT8* addrIV, UINT32 uIVSize, UINT8 uKeyTableIndex,
                                  UINT32 uAlgo, UINT32 uMode);

/*
 * Function: BCM_SECURE_LOADCONTENTKEY
 * Loads the content key
 *
 * Arguments:
 *   uKeyTableIndex  : the key table index
 *   uMode           : the mode
 *   AddrIn          : the buffer containing the key
 *   uKeySize        : the data type
 *
 * Returns:
 *   SUCCESS or one of the BCM error code
 */

HRESULT BCM_LOADCONTENTKEY(UINT32 keyIndexOffset, UINT32 uMode, UINT32 uKeySize, UINT8* AddrIn);


/* Possible Data type for KEY/IV :- CLEAR or SECURE */
typedef enum {
    EClearKey = 0,
    ESecureKey,
    EClearIV,
    ESecureIV
    } DataType;

/*
 * Function: BCM_SECURE_LOADCONTENTKEY
 * Loads the content key
 *
 * Arguments:
 *   addrData        : the buffer containing the data
 *   uDataType       : the data type
 *   uSize           : the data size
 *   uKeyTableIndex  : the key table index
 *   uAlgo           : the algorithm
 *   uMode           : the mode
 *
 * Returns:
 *   SUCCESS or one of the BCM error code
 */
HRESULT BCM_SECURE_LOADCONTENTKEY(UINT8* addrData, UINT32 uDataType, UINT32 uSize,
                                  UINT8 uKeyTableIndex, UINT32 uAlgo, UINT32 uMode);

/*===============================================================================================
 * R2R function API
 *===============================================================================================*/
 /*
 * Function: BCM_SECUREVMX_LOADCW
 *   Setup the verimatrix key laddering and move the Control Word key to the TSP's key table
 *
 * Arguments:
 * - AddrL1Key   (IN)  -  buffer containing 16 bytes of ciphertext of Level 1 key
 * - AddrL2Key   (IN)  -  buffer containing 16 bytes of ciphertext of Level 2 key
 * - AddrDataIn  (IN)  -  buffer containing value of input data to be en/decrypted
 * - AddrDataOut (OUT) -  buffer which will contains en/decrypted data
 * - uDataInLen     (IN)  -  length of input data
 * - uCryptoCmd     (IN)  -  operation type - encryption or decryption
 *
 * Return:
 *         STATUS_SUCCESS or one of the BCM error code
 */
HRESULT BCM_SECURER2R_KEYLADDER (UINT8* AddrL1Key, UINT8* AddrL2Key,
        UINT8* AddrDataIn, UINT8* AddrDataOut, UINT32 uDataInLen, UINT32 uCryptoCmd);


/*===============================================================================================
 * PVR
 *===============================================================================================*/
/*
 * Function: BCM_SECUREPVR_GENERATEKEY
 *   Generate random PVR key and return it in secure keystore format
 *
 * Arguments:
 * - uKeySize       (IN)  - Size of the key to be generated
 * - AddrKey     (OUT) - Buffer for PVR secure keystore
 * - AddrKeySize (OUT) - Size of PVR secure keystore
 *
 * Return:
 *         STATUS_SUCCESS or one of the BCM error code
 */
HRESULT BCM_SECUREPVR_GENERATEKEY(UINT32 uKeyLength, UINT8* AddrKey, UINT8* AddrKeySize);

/*
 * Function: BCM_SECUREPVR_LOADKEY
 *   Loads the PVR content key from secure keystore format into BCM tsp key table
 *
 * Arguments:
 * - AddrKey     (IN) - PVR secure keystore
 * - uKeySize       (IN) - length of PVR keystore
 * - uKeyTableIndex (IN) - offset within tsp key table
 * - uMode          (IN) - content key mode
 *
 * Return:
 *         STATUS_SUCCESS or one of the BCM error code
 */
HRESULT BCM_SECUREPVR_LOADKEY(UINT8* AddrKey, UINT32 uKeySize,
        UINT32 uKeyTableIndex, UINT32 uMode);

/*
 * Function: BCM_SECUREPVR_LOADKEY
 *   Loads the PVR content key from secure keystore format into BCM tsp key table
 *
 * Arguments:
 * - AddrKey     (IN) - PVR secure keystore
 * - uKeySize       (IN) - length of PVR keystore
 * - uKeyOffset     (IN) - offset within tsp key table
 * - uFlag          (IN) - algorithm mode (should be AES_CBC) and the data type (content KEY or IV)
 *
 * Return:
 *         STATUS_SUCCESS or one of the BCM error code
 */
HRESULT BCM_SECUREPVR_LOADKEY(UINT8* AddrKey, UINT32 uKeySize,
        UINT32 uKeyOffset, UINT32 uFlag);

/*
 * Function: BCM_HDCP1X_LOADTXKEY
 *   Load the HDCP 1.X TX key to HDMI register
 *
 * Arguments:
 * - AddrKey     (IN) - HDCP 1.X TX secure keystore
 * - uKeySize       (IN) - length of HDCP 1.X secure  keystore
 *
 * Return:
 *         STATUS_SUCCESS or one of the BCM error code
 */
HRESULT BCM_HDCP1X_LOADTXKEY(UINT8* AddrKey, UINT32 uKeySize);

/*
 * Function: BCM_HDCP2X_LOADLC
 *   Load the HDCP 2.X LC to HDMI register
 *
 * Arguments:
 * - AddrKey     (IN) - HDCP 2.X LC keystore
 * - uKeySize       (IN) - length of HDCP 2.X LC keystore
 *
 * Return:
 *         STATUS_SUCCESS or one of the BCM error code
 */
HRESULT BCM_HDCP2X_LOADLC(UINT8* AddrKey, UINT32 uKeySize);

/*
 * Function: BCM_OTP_BURNLAYOUT
 *   Burn the OTP layout
 *
 * Arguments:
 * - AddrLayout     (IN) - OTP layout secure store
 * - uLayoutSize       (IN) - length of OTP layout secure store
 * - AddrResp      (OUT) - BCM response buffer
 * - uRespSize         (IN) - BCM response buffer length
 *
 * Return:
 *         STATUS_SUCCESS or one of the BCM error code
 */
HRESULT BCM_OTP_BURNLAYOUT(UINT8* AddrLayout, UINT32 uLayoutSize, UINT8* AddrResp, UINT32 uRespSize);
/*===============================================================================================
 * CISCODRM
 *===============================================================================================*/
/*
 * Function: BCM_CISCODRM_GETRANDOM
 *   Loads the Clear IV into BCM tsp key table
 *
 * Arguments:
 *   phyAddr          : Phy Address of input buffer
 *   uPhySize         : Size of buffer
 * Returns:
 *   SUCCESS or one of the BCM error code
 */

HRESULT BCM_CISCODRM_GETRANDOM(UINT8* phyAddr, UINT32 uPhySize);
/*
 * Function: BCM_RSA_CISCODRM_SIGNKEY
 *   Loads CISCO specific key,sign and unload
 *
 * Arguments:
 *   phyKeyStore          : Phy Address of input buffer
 *   uKeyStoreSize         : Size of buffer
     phyCtx               : Context param
     uMsg                 : Input Message
     uMsgSize             : Message Size
     uSig                 : Output buffer
 * Returns:
 *   SUCCESS or one of the BCM error code
 */

HRESULT BCM_RSA_CISCODRM_SIGNKEY(UINT8* phyKeyStore, UINT32 uKeyStoreSize,UINT8* phyCtx , UINT8*uMsg ,UINT32 uMsgSize ,UINT8 *uSig);
/*
 * Function: BCM_GENERATE_SECUREKEY
 *   Generate Secrete Key
 *
 * Arguments:
 *   phyAddrUUID          : Phy Address of UUID
 *   phyAddrAppID         : Phy Address of APPID
     uAppVariant          : Application variant
     phyAddrSecureKey     : Output buffer key

 * Returns:
 *   SUCCESS or one of the BCM error code
 */

HRESULT BCM_GENERATE_SECUREKEY(UINT32 phyAddrUUID,UINT32 phyAddrAppID , UINT32 uAppVariant ,UINT32 phyAddrSecureKey);

/*
 * Function: BCM_READ_PUB_OTP
*   PI is called to read OTP Value.
*
* Arguments:
* - uTag  (IN) -          The tag of OTP value to be fetched
* - AddrOtpBuffer (OUT) - Buffer containing the OTP value
* - uOtpBufferLen (IN) -  Length of buffer containing OTP
*
* Returns:
*   SUCCESS or one of the BCM error code
*/
HRESULT BCM_READ_PUB_OTP(UINT32 uTag, UINT8* AddrOtpBuffer, UINT32 uOtpBufferLen);

/*
 * Function: BCM_READ_OTP_JTAG
*   PI is called to read OTP Jtag status.
*
* Arguments:
* - AddrJtag (OUT) - Buffer containing the OTP Jtag status
*
* Returns:
*   SUCCESS or one of the BCM error code
*/
HRESULT BCM_READ_OTP_JTAG(UINT8 *AddrJtag);
/**
 * Function: HRESULT BCM_ACCESS_SECURITY_SRAM(BCM_SRAM_ACCESS_T access_type, uint32_t idx, uint64_t *pBuf)
 *
 *   PI is called to access Secure SRAM Index
 *
 * Arguments:
 * - access_type (IN) - BCM_SRAM_READ or BCM_SRAM_WRITE
 * - idx         (IN) - Buffer index
 * - pBuf        (IN/OUT) - Pointer to 64-bit data buffer
 *
 * Returns:
 *   SUCCESS or one of the BCM error code
 */
TEE_Result BCM_ACCESS_SECURITY_SRAM(BCM_SRAM_ACCESS_T access_type, uint32_t idx, uint64_t *pBuf);
/*
 * Function: BCM_GENX_OEM_COMMAND
 *   Load the GenX OEM command to BCM
 *
 * Arguments:
 * - AddrOEMCommand     (IN) - OEM Command binary
 * - uOEMCommandSize    (IN) - length of OEM Command binary
 *
 * Return:
 *         STATUS_SUCCESS or one of the BCM error code
 */
HRESULT BCM_GENX_OEM_COMMAND(UINT8* AddrOEMCommand, UINT32 uOEMCommandSize);

/*
 * Function: BCM_READ_OTP_PUBLIC_ITEMS
 *   PI is called to read pre-defined public OTP items.
 *
 * Arguments:
 * - AddrPubOTP (OUT) - Buffer containing the pre-defined public OTP items
 * - uPubOTPLen (IN)  - Length of buffer containing OTP
 *
 * Returns:
 *   SUCCESS or one of the BCM error code
 */
HRESULT BCM_READ_PUBLIC_OTP_ITEMS(UINT8 *AddrPubOTP, UINT32 uPubOTPLen);

/*
 * Function: BCM_READ_OTP_SOC_UID
 *   PI is called to read SOC UID from OTP.
 *
 * Arguments:
 * - AddrSOCUID (OUT) - Buffer containing the SOC UID
 * - uSOCUIDLen (IN)  - Length of buffer containing SOC UID
 *
 * Returns:
 *   SUCCESS or one of the BCM error code
 */
HRESULT BCM_READ_SOC_UID(UINT8 *AddrSOCUID, UINT32 uSOCUIDLen);

/*
 * Function: BCM_WRITE_RKEKID
 *   PI is called to write RKEKID to OTP.
 *
 * Arguments:
 * - AddrRKEKID (IN) - Buffer containing the RKEKID
 * - uRKEKIDLen (IN) - Length of buffer containing RKEKID
 *
 * Returns:
 *   SUCCESS or one of the BCM error code
 */
HRESULT BCM_WRITE_RKEKID(UINT8 *AddrRKEKID, UINT32 uRKEKIDLen);

/*
 * Function: HRESULT BCM_OTP_READ(UINT32 type, UINT32 *data, UINT32 *mask)
 *
 *   PI is called to read OTP data.
 *
 * Arguments:
 * - type (IN)   - Type of OTP data to read
 * - pdata (OUT)  - Buffer to store the read OTP data
 * - pmask (OUT)  - Buffer to store the mask for the read OTP data
 *
 * Returns:
 *   SUCCESS or one of the BCM error code
 */
HRESULT BCM_READ_OTP(UINT32 type, UINT32 *pdata, UINT32 *pmask);

/*
 * Function: HRESULT BCM_WRITE_OTP(UINT32 type, UINT32 data, UINT32 mask)
 *
 *   PI is called to write OTP data.
 *
 * Arguments:
 * - type (IN)   - Type of OTP data to write
 * - data (IN)   - OTP data to write
 * - mask (IN)   - mask for the OTP data
 *
 * Returns:
 *   SUCCESS or one of the BCM error code
 */
HRESULT BCM_WRITE_OTP(UINT32 type, UINT32 data, UINT32 mask);

#endif
