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
//! \file    seccrypto_bcm.c
//! \brief   Extended Secure Crypto DRM BCM Entry Functions
//! \author  Girish Shetty
//! \date    Dec 2014
///////////////////////////////////////////////////////////////////////////////

#include "bcm_primitive.h"
#include "bcm_mailbox.h"
#include "bcm_status.h"
#include "drm_bcm_com.h"
#include "drm_tee_log.h"
#include "seccrypto_bcm.h"
#include "tee_internal_api.h"

//#define BCM_DEBUG

/*===============================================================================================
 * Generic
 *===============================================================================================*/
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
                        UINT32 phyAddrSecureKey)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT32)phyAddrUUID;
    cmd.arg1 = (UINT32)phyAddrAppID;
    cmd.arg2 = uAppVariant;
    cmd.arg3 = (UINT32)phyAddrSecureKey;

    cmd.command = DRM_GENERATE_SECUREKEY_V2;
    execute_bcm_command(&cmd, &status);
    return status.return_status;
}

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
HRESULT BCM_SECUREVMX_INIT()
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);

    cmd.command = DRM_SECUREVMX_INIT;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}

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
HRESULT BCM_SECUREVMX_GETKEYID(UINT8* AddrKeyID)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8* phyAddrKeyID = NULL;

    phyAddrKeyID  = TEE_VirtToPhys(AddrKeyID);

#ifdef BCM_DEBUG
    printf("%s: %d", __func__, __LINE__);
    printf("show the address as below:\n");
    printf("vir: %08x phy: %08x\n", AddrKeyID, phyAddrKeyID);
#endif
    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT)phyAddrKeyID;          //VMX key ID

    cmd.command = DRM_SECUREVMX_GETKEYID;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}

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
        UINT8* AddrCWC, UINT32 uCWCLen, UINT32 uKeyOffset, UINT32 uAlgo, UINT32 uMode)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8*  phyAddrSCHEMEKey = NULL;
    UINT8*  phyAddrEMMKey= NULL;
    UINT8*  phyAddrECMKey = NULL;
    UINT8*  phyAddrCWC = NULL;

    phyAddrSCHEMEKey = TEE_VirtToPhys(AddrSCHEMEKey);
    phyAddrEMMKey = TEE_VirtToPhys(AddrEMMKey);
    phyAddrECMKey = TEE_VirtToPhys(AddrECMKey);
    phyAddrCWC    = TEE_VirtToPhys(AddrCWC);
#ifdef BCM_DEBUG
    printf("%s: %d", __func__, __LINE__);
    printf("show the address as below:\n");
    printf("vir: %08x phy: %08x\n", AddrSCHEMEKey, phyAddrSCHEMEKey);
    printf("vir: %08x phy: %08x\n", AddrEMMKey, phyAddrEMMKey);
    printf("vir: %08x phy: %08x\n", AddrECMKey, phyAddrECMKey);
    printf("vir: %08x phy: %08x\n", AddrCWC, phyAddrCWC);
#endif

    //Load the Secure Key store into BCM
    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT)phyAddrSCHEMEKey;      //secure vmx key
    cmd.arg1 = uSCHEMEKeylen;               //size of kvmx
    cmd.arg2 = (UINT)phyAddrEMMKey;         //secure kcwp
    cmd.arg3 = uEMMKeyLen;                  //size of kcwp
    cmd.arg4 = (UINT)phyAddrECMKey;         //secure control word
    cmd.arg5 = uECMKeylen;                  //size of control word
    cmd.arg6 = (UINT)phyAddrCWC;            //Control Word Control Info
    cmd.arg7 = uCWCLen;                     //Size of CWC
    cmd.arg8 = uKeyOffset;                  //offset within tsp key table
    cmd.arg9 = uAlgo;                       //content key algorithm
    cmd.arg10 = uMode;                      //content key mode

    cmd.command = DRM_SECUREVMX_KEYLADDER;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}

/*
 * Function: BCM_SECUREVMX_LOADCW
 *   Loads the VMX Control Word (CW) key into BCM tsp key table
 *
 * Arguments:
 * - AddrKeyBuffer  (IN) - CW key
 * - uKeySize          (IN) - length of CW key, should be 16 bytes
 * - uKeyOffset        (IN) - offset within tsp key table
 * - uAlgo             (IN) - content key algorithm
 * - uMode             (IN) - content key algorithm mode
 *
 * Return:
 *         STATUS_SUCCESS or one of the BCM error code
 */
HRESULT BCM_SECUREVMX_LOADCW(UINT8* AddrKeyBuffer, UINT32 uKeySize,
        UINT8 uKeyOffset, UINT32 uAlgo, UINT32 uMode)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8* phyAddrKeyBuffer = NULL;

    phyAddrKeyBuffer  = TEE_VirtToPhys(AddrKeyBuffer);
#ifdef BCM_DEBUG
    printf("%s: %d", __func__, __LINE__);
    printf("show the address as below:\n");
    printf("vir: %08x phy: %08x\n", AddrKeyBuffer, phyAddrKeyBuffer);
#endif

    //Load the Secure Key store into BCM
    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT)phyAddrKeyBuffer;      //cw key
    cmd.arg1 = uKeySize;                    //size of cw
    cmd.arg2 = uKeyOffset;                  //offset within tsp key table
    cmd.arg3 = uAlgo;                       //content key algorithm
    cmd.arg4 = uMode;                       //content key mode

    cmd.command = DRM_SECUREVMX_LOADCW;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}

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
        UINT8* AddrDataIn, UINT8* AddrDataOut, UINT32 uDataInLen, UINT32 uCryptoCmd)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8*  phyAddrL1Key = NULL;
    UINT8*  phyAddrL2Key= NULL;
    UINT8*  phyAddrDataIn = NULL;
    UINT8*  phyAddrDataOut = NULL;

    phyAddrL1Key = TEE_VirtToPhys(AddrL1Key);
    phyAddrL2Key = TEE_VirtToPhys(AddrL2Key);
    phyAddrDataIn = TEE_VirtToPhys(AddrDataIn);
    phyAddrDataOut = TEE_VirtToPhys(AddrDataOut);
#ifdef BCM_DEBUG
    printf("%s: %d\n", __func__, __LINE__);
    printf("show the address as below:\n");
    printf("vir: %08x phy: %08x \n", AddrL1Key, phyAddrL1Key);
    printf("vir: %08x phy: %08x \n", AddrL2Key, phyAddrL2Key);
    printf("vir: %08x phy: %08x \n", AddrDataIn, phyAddrDataIn );
    printf("vir: %08x phy: %08x \n", AddrDataOut, phyAddrDataOut);
#endif

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT)phyAddrL1Key;          //L1 key
    cmd.arg1 = (UINT)phyAddrL2Key;          //L2 key
    cmd.arg2 = (UINT)phyAddrDataIn;         //input message
    cmd.arg3 = uDataInLen;                  //size of input buffer
    cmd.arg4 = (UINT)phyAddrDataOut;        //output buffer
    cmd.arg5 = uCryptoCmd;                  //crypto operation (encrypt/decrypt)

    cmd.command = DRM_SECURER2R_KEYLADDER;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}

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
HRESULT BCM_SECUREPVR_GENERATEKEY(UINT32 uKeyLength, UINT8* AddrKey, UINT8* AddrKeySize)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8* phyAddrKey = NULL;
    UINT8* phyAddrKeySize =NULL;

    phyAddrKey  = TEE_VirtToPhys(AddrKey);
    phyAddrKeySize  = TEE_VirtToPhys(AddrKeySize);
#ifdef BCM_DEBUG
    printf("%s: %d", __func__, __LINE__);
    printf("show the address as below:\n");
    printf("vir:%08x phy:%08x\n", AddrKey, phyAddrKey);
    printf("vir:%08x phy:%08x\n", phyAddrKeySize, AddrKeySize);
#endif

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = uKeyLength;            //size of the key to be generated
    cmd.arg1 = (UINT)phyAddrKey;      //output buffer for the secure keystore
    cmd.arg2 = (UINT)phyAddrKeySize;  //size of output keystore
    DRM_LOG("uKeyLength=0x%x, phyAddrKey=%p, phyAddrKeySize=%p\n",uKeyLength, phyAddrKey, phyAddrKeySize);
    cmd.command = DRM_SECUREPVR_GENKEY;

    execute_bcm_command(&cmd, &status);
    DRM_LOG("Return from GenKey command\n");
    return status.return_status;
}

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
        UINT32 uKeyOffset, UINT32 uFlag)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8* phyAddrKey = NULL;

    phyAddrKey  = TEE_VirtToPhys(AddrKey);
#ifdef BCM_DEBUG
    printf("%s: %d", __func__, __LINE__);
    printf("show the address as below:\n");
    printf("vir:%08x phy:%08x\n", AddrKey, phyAddrKey);
#endif

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT)phyAddrKey;            //secure key
    cmd.arg1 = uKeySize;                    //size of key
    cmd.arg2 = uKeyOffset;                  //offset within tsp key table
    cmd.arg3 = uFlag;                       //content key flag

    cmd.command = DRM_SECUREPVR_LOADKEY;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}


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
HRESULT BCM_HDCP1X_LOADTXKEY(UINT8* AddrKey, UINT32 uKeySize)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8* phyAddrKey = NULL;

    phyAddrKey  = TEE_VirtToPhys(AddrKey);
#ifdef BCM_DEBUG
    printf("%s: %d", __func__, __LINE__);

    printf("show the address as below:\n");
    printf("vir: %08x\n", AddrKey);
#endif

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT)phyAddrKey;            //secure key
    cmd.arg1 = uKeySize;                    //size of key

    cmd.command = DRM_HDCP_HDMI_TX_LOADKEYS;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}

/*
 * Function: BCM_HDCP1X_LOADRXKEY
 *   Load the HDCP 1.X RX key to HDMI register
 *
 * Arguments:
 * - AddrKey     (IN) - HDCP 1.X RX secure keystore
 * - uKeySize       (IN) - length of HDCP 1.X secure  keystore
 *
 * Return:
 *         STATUS_SUCCESS or one of the BCM error code
 */
HRESULT BCM_HDCP1X_LOADRXKEY(UINT8* AddrKey, UINT32 uKeySize)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8* phyAddrKey = NULL;

    phyAddrKey  = TEE_VirtToPhys(AddrKey);
#ifdef BCM_DEBUG
    printf("%s: %d", __func__, __LINE__);
    printf("show the address as below:\n");
    printf("vir:%08x phy:%08x\n", AddrKey, phyAddrKey);
#endif

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT)phyAddrKey;            //secure key
    cmd.arg1 = uKeySize;                    //size of key

    cmd.command = DRM_HDCP_HDMI_RX_LOADKEYS;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}

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
HRESULT BCM_HDCP2X_LOADLC(UINT8* AddrKey, UINT32 uKeySize)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8* phyAddrKey = NULL;

    phyAddrKey  = TEE_VirtToPhys(AddrKey);
#ifdef BCM_DEBUG
    printf("%s: %d", __func__, __LINE__);
    printf("show the address as below:\n");
    printf("vir:%08x phy:%08x \n", AddrKey, phyAddrKey);
#endif

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT)phyAddrKey;            //secure key
    cmd.arg1 = uKeySize;                    //size of key

    //TO: Change to right BCM PI ID
    cmd.command = DRM_HDCP2X_HDMI_TX_LOADKEYS;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}

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
HRESULT BCM_OTP_BURNLAYOUT(UINT8* AddrLayout, UINT32 uLayoutSize, UINT8* AddrResp, UINT32 uRespSize)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8* phyAddrLayout = NULL;
    UINT8* phyAddrResp = NULL;

    phyAddrLayout  = TEE_VirtToPhys(AddrLayout);
    phyAddrResp  = TEE_VirtToPhys(AddrResp);
#ifdef BCM_DEBUG
    printf("%s: %d", __func__, __LINE__);

    printf("show the address as below:\n");
    printf("vir:%08x phy:%08x \n", AddrLayout, phyAddrLayout);
    printf("vir:%08x phy:%08x \n", AddrResp, phyAddrResp);
#endif

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT)phyAddrLayout;
    cmd.arg1 = uLayoutSize;
    cmd.arg2 = (UINT)phyAddrResp;
    cmd.arg3 = uRespSize;
    cmd.arg4 = 0x0;

    cmd.command = DRM_OTP_PROGRAM_OTP;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}

/*
 * Function: BCM_GET_RANDOM
 *   Generates a random byte stream of desired length
 *
 * Arguments:
 *   addrRandom(IN): address of buffer to store random bumber
 *   uRandomSize(IN):   size (in bytes) of random number
 */
HRESULT BCM_SECUREVMX_GETRANDOM(UINT8* addrRandom, UINT32 uRandomSize)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8* phyAddrRandom = NULL;

    phyAddrRandom  = TEE_VirtToPhys(addrRandom);
#ifdef BCM_DEBUG
    printf("%s: %d", __func__, __LINE__);
    printf("show the address as below:\n");
    printf("vir:%08x phy:%08x \n", addrRandom, phyAddrRandom);
#endif

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);

    cmd.arg0 = (UINT)phyAddrRandom;
    cmd.arg1 = uRandomSize;

    cmd.command = DRM_GET_RANDOM;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}

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

HRESULT BCM_LOADCONTENTKEY(UINT32 keyIndexOffset, UINT32 uMode, UINT32 uKeySize, UINT8* AddrIn)
{
    HRESULT lRes = S_OK;
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8* phyAddrIn = NULL;

    phyAddrIn  = TEE_VirtToPhys(AddrIn);
#ifdef BCM_DEBUG
    printf("%s: %d\n", __func__, __LINE__);
    printf("show the address as below:\n");
    printf("vir:%08x phy:%08x \n", AddrIn, phyAddrIn);
#endif

    //Load the content key into BCM's TSP table
    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT)phyAddrIn;               //clear key that we want to be loaded to BCM's TSP
    cmd.arg1 = uKeySize;                      //length of the clear content key
    cmd.arg2 = keyIndexOffset;                //index offset of in the TSP table
    cmd.arg3 = uMode;                         //mode of the content key
    cmd.command = DRM_LOAD_CONTENT_KEY;
    execute_bcm_command(&cmd, &status);

    lRes = status.return_status;
    return lRes;
}

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
                                  UINT8 uKeyTableIndex, UINT32 uAlgo, UINT32 uMode)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8* phyAddrData = NULL;

    phyAddrData  = TEE_VirtToPhys(addrData);
#ifdef BCM_DEBUG
    printf("%s: %d", __func__, __LINE__);
    printf("show the address as below:\n");
    printf("vir:%08x phy:%08x \n", addrData, phyAddrData);
#endif

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT)phyAddrData;      //physical address of data (key or iv)
    cmd.arg1 = uDataType;              //flag specifying data type := key or iv and clear or secure
    cmd.arg2 = uSize;                  //size of key or iv
    cmd.arg3 = uKeyTableIndex;         //offset within tsp key table
    cmd.arg4 = uAlgo;                  //crypto algorithm associated with the data
    cmd.arg5 = uMode;                  //crypto mode associated with the data

    cmd.command = DRM_LOAD_SECURE_CONTENT_KEY;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}

/*
 * Function: BCM_SECUREVMX_LOADCLEARIV
 *   Loads the Clear IV into BCM tsp key table
 *
 * Arguments:
 *   addrIV          : the buffer containing the clear IV
 *   uIVSize         : the length of the IV in the buffer phyAddrIVBuffer
 *   uKeyTableOffset : the key table offset of entry in which the IV will be stored
 *   uAlgo           : the algorithm of the IV used for
 *   uMode           : the mode of the IV used for
 *
 * Returns:
 *   SUCCESS or one of the BCM error code
 */
HRESULT BCM_SECUREVMX_LOADCLEARIV(UINT8* addrIV, UINT32 uIVSize, UINT8 uKeyTableIndex,
                                  UINT32 uAlgo, UINT32 uMode)
{
    return BCM_SECURE_LOADCONTENTKEY(addrIV, EClearIV, uIVSize, uKeyTableIndex, uAlgo, uMode);
}

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
HRESULT BCM_SECUREVMX_GETROMKSTATUS(UINT32* AddrRomkStatus)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8* phyAddrRomkStatus = NULL;

    phyAddrRomkStatus  = (UINT8 *)TEE_VirtToPhys(AddrRomkStatus);
#ifdef BCM_DEBUG
    printf("%s: %d", __func__, __LINE__);
    printf("show the address as below:\n");
    printf("vir: %08x phy: %08x \n", AddrRomkStatus, phyAddrRomkStatus);
#endif

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT)phyAddrRomkStatus;

    cmd.command = DRM_SECUREVMX_GETROMK_STATUS;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}
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


HRESULT BCM_CISCODRM_GETRANDOM(UINT8* phyAddr, UINT32 uPhySize)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.command = DRM_GET_RANDOM;
    cmd.arg0 = (UINT32)phyAddr;
    cmd.arg1 = uPhySize;
    execute_bcm_command(&cmd, &status);
    return status.return_status;
}
/*
 * Function: BCM_RSA_CISCODRM_SIGNKEY
 *   Loads CISCO specific key,sign and unload
 *
 * Arguments:
 *   phyKeyStore          : Phy Address of input buffer
 *   uKeyStoreSize         : Size of buffer
     phyCtx               : Context param
     uMsg                 : Input Massage
     uMsgSize             : Massage Size
     uSig                 : Output buffer
 * Returns:
 *   SUCCESS or one of the BCM error code
 */


HRESULT BCM_RSA_CISCODRM_SIGNKEY(UINT8* phyKeyStore, UINT32 uKeyStoreSize,
        UINT8* phyCtx , UINT8*uMsg ,UINT32 uMsgSize ,UINT8 *uSig)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.command = DRM_CISCO_RSA_SIGN;
    cmd.arg0 = (UINT32)phyKeyStore;
    cmd.arg1 = uKeyStoreSize;
    cmd.arg2 = (UINT32)phyCtx;
    cmd.arg3 = (UINT32)uMsg;
    cmd.arg4 = (UINT32)uMsgSize;
    cmd.arg5 = (UINT32)uSig;
    execute_bcm_command(&cmd, &status);
    return status.return_status;
}
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

HRESULT BCM_GENERATE_SECUREKEY(UINT32 phyAddrUUID,
                        UINT32 phyAddrAppID , UINT32 uAppVariant ,
                        UINT32 phyAddrSecureKey)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT32)phyAddrUUID;
    cmd.arg1 = (UINT32)phyAddrAppID;
    cmd.arg2 = uAppVariant;
    cmd.arg3 = (UINT32)phyAddrSecureKey;

    cmd.command = DRM_GENERATE_SECUREKEY;
    execute_bcm_command(&cmd, &status);
    return status.return_status;
}

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
HRESULT BCM_READ_PUB_OTP(UINT32 uTag, UINT8* AddrOtpBuffer, UINT32 uOtpBufferLen)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8*  phyAddrBuffer = NULL;

    phyAddrBuffer = TEE_VirtToPhys(AddrOtpBuffer);

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = uTag;                 //Tag
    cmd.arg1 = (UINT)phyAddrBuffer;  //AddrOtpBuffer
    cmd.arg2 = uOtpBufferLen;        //size of buffer
    cmd.arg3 = 0;                    // bit mask

    cmd.command = BCM_EXTN_NSK2_READ_OTP;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}

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
HRESULT BCM_READ_OTP_JTAG(UINT8 *AddrJtag)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8* phyAddrJtag = NULL;

    phyAddrJtag = TEE_VirtToPhys(AddrJtag);

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT)phyAddrJtag;

    cmd.command = DRM_OTP_UTILS_GET_JTAG;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}

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
HRESULT BCM_GENX_OEM_COMMAND(UINT8* AddrOEMCommand, UINT32 uOEMCommandSize)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8* phyAddrKey = NULL;

    phyAddrKey  = TEE_VirtToPhys(AddrOEMCommand);
#ifdef BCM_DEBUG
    printf("%s: %d", __func__, __LINE__);
    printf("show the address as below:\n");
    printf("vir:%08x phy:%08x \n", AddrOEMCommand, phyAddrKey);
#endif

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT)phyAddrKey;            //secure bin
    cmd.arg1 = uOEMCommandSize;             //size of bin

    cmd.command = DRM_GENX_OEM_CMD_PI;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}

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
HRESULT BCM_READ_PUBLIC_OTP_ITEMS(UINT8 *AddrPubOTP, UINT32 uPubOTPLen)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8* phyAddrPubOTP = NULL;

    phyAddrPubOTP = TEE_VirtToPhys(AddrPubOTP);

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT)phyAddrPubOTP;
    cmd.arg1 = (UINT)uPubOTPLen;

    cmd.command = DRM_OTP_UTILS_READ_PUBLIC_ITEMS;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}

/*
 * Function: BCM_READ_SOC_UID
 *   PI is called to read SOC UID
 *
 * Arguments:
 * - AddrSOCUID (OUT) - Buffer containing the SOC UID
 * - uSOCUIDLen (IN)  - Length of buffer containing SOC UID
 *
 * Returns:
 *   SUCCESS or one of the BCM error code
 */
HRESULT BCM_READ_SOC_UID(UINT8 *AddrSOCUID, UINT32 uSOCUIDLen)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8* phyAddrSOCUID = NULL;

    phyAddrSOCUID = TEE_VirtToPhys(AddrSOCUID);

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT)phyAddrSOCUID;
    cmd.arg1 = (UINT)uSOCUIDLen;

    cmd.command = DRM_OTP_UTILS_READ_SOC_UID;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}

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
HRESULT BCM_WRITE_RKEKID(UINT8 *AddrRKEKID, UINT32 uRKEKIDLen)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    UINT8 *phyRKEKID = NULL;

    phyRKEKID  = TEE_VirtToPhys(AddrRKEKID);

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT)phyRKEKID;
    cmd.arg1 = uRKEKIDLen;

    cmd.command = DRM_OTP_UTILS_WRITE_RKEKID;
    execute_bcm_command(&cmd, &status);

    return status.return_status;

}

/*
 * Function: HRESULT BCM_OTP_READ(UINT32 type, UINT32 *pdata, UINT32 *pmask)
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
HRESULT BCM_READ_OTP(UINT32 type, UINT32 *pdata, UINT32 *pmask)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = type;
    cmd.arg1 = (UINT)TEE_VirtToPhys(pdata);
    cmd.arg2 = (UINT)TEE_VirtToPhys(pmask);
    cmd.command = DRM_OTP_UTILS_OTP_READ;

    execute_bcm_command(&cmd, &status);

    return status.return_status;
}

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
TEE_Result BCM_ACCESS_SECURITY_SRAM(BCM_SRAM_ACCESS_T access_type, uint32_t idx, uint64_t *pBuf)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (uint32_t)access_type;
    cmd.arg1 = idx;
    cmd.arg2 = (UINT)TEE_VirtToPhys(pBuf);
    cmd.command = DRM_ACCESS_SRAM;

    execute_bcm_command(&cmd, &status);
    return status.return_status;
}
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
HRESULT BCM_WRITE_OTP(UINT32 type, UINT32 data, UINT32 mask)
{
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = type;
    cmd.arg1 = data;
    cmd.arg2 = mask;

    cmd.command = DRM_OTP_UTILS_OTP_WRITE;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}
