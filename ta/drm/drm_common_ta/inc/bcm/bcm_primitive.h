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

#ifndef INCLUDE_BCM_PRIMITIVES_H
#define INCLUDE_BCM_PRIMITIVES_H

// Following definitions need to be consistent with BCM f/w
typedef enum _DRM_PRIMITIVE
{
    // Dummy
    DRM_GET_VERSION                 = 0x0000,
    DRM_SUSPEND_START               = 0x0001,
    DRM_SUSPEND_DONE                = 0x0002,
    DRM_RESUME_START                = 0x0003,
    DRM_RESUME_DONE                 = 0x0004,

    DRM_GENX_OEM_CMD_PI             = 0x0021,

    // Secure boot
    EROM_VERIFY_IMAGE               = 0x1000,
    EROM_LOAD_MODULES               = 0x1001,
    USB_BOOT_LOAD_IMAGE             = 0x1002,
    USB_BOOT_TURN_OFF_DEVICE        = 0x1003,
    SET_BG_CHECKING_TABLE           = 0x1004,

    // WideWine
    DRM_WV_INITIALIZE               = 0x2000,
    DRM_WV_TERMINATE                = 0x2001,
    DRM_WV_OPENSESSION              = 0x2002,
    DRM_WV_CLOSESESSION             = 0x2003,
    DRM_WV_GENERATEDERIVEDKEYS      = 0x2004,
    DRM_WV_LOADKEYS_CHECKSIGNATURE  = 0x2005,
    DRM_WV_LOADKEYS_UPDATE          = 0x2007,
    DRM_WV_REFRESHKEYS_CHECKSIGNATURE = 0x2008,
    DRM_WV_REFRESHKEYS              = 0x2009,
    DRM_WV_REFRESHKEYS_UPDATE       = 0x200A,
    DRM_WV_GENERATENONCE            = 0x200B,
    DRM_WV_GENERATESIGNATURE        = 0x200C,
    DRM_WV_REWRAPDEVICERSAKEY       = 0x200D,
    DRM_WV_LOADDEVICERSAKEY         = 0x200E,
    DRM_WV_GENERATERSASIGNATURE     = 0x200F,
    DRM_WV_DERIVEKEYSFROMSESSIONKEY = 0x2010,
    DRM_WV_GETKEYDATA               = 0x2011,
    DRM_WV_GENERIC_ENCRYPT          = 0x2012,
    DRM_WV_GENERIC_DECRYPT          = 0x2013,
    DRM_WV_GENERIC_SIGN             = 0x2014,
    DRM_WV_GENERIC_VERIFY           = 0x2015,
    DRM_WV_GETSESSIONMACKEYS        = 0x2016,
    DRM_WV_SETSESSIONMACKEYS        = 0x2017,
    DRM_WV_LOADENTITLEMENTKEYS      = 0x2018,
    DRM_WV_REFRESHENTITLEMENTKEYS   = 0x201A,
    DRM_WV_LOADCERTPRIVATERSA       = 0x201B,
    DRM_WV_REWRAPDEVICERSAKEY30     = 0x201C,

    // Widevine key loading
    DRM_WV_LOADKEYS                 = 0x2030,
    DRM_WV_LOADENTITLEDCONTENTKEYS  = 0x2031,

    // GTV CA
    DRM_GTV_CA_LOADKEY_ADVANCE      = 0x3000,
    DRM_GTV_CA_SIGN                 = 0x3001,
    DRM_GTV_CA_UNLOADKEY            = 0x3002,

    // Misc
    DRM_HDCP_HDMI_TX_LOADKEYS       = 0x4000,
    DRM_LOAD_CONTENT_KEY            = 0x4001,
    DRM_REMOVE_CONTENT_KEY          = 0x4002,
    DRM_GET_RANDOM                  = 0x4003,
    DRM_LOAD_SECURE_CONTENT_KEY     = 0x4004,
    DRM_DISTURBANCE                 = 0x4005,
    DRM_HDCP_HDMI_RX_LOADKEYS       = 0x4006,
    DRM_UPDATE_CONTENT_KEYMODE      = 0x4007,
    DRM_GET_PRODUCT_LIFE_CYCLE      = 0x4008,
    DRM_EXPORT_SECSTORE             = 0x4009,
    DRM_GENERATE_SECUREKEY          = 0x400A,
    //CISCO
    DRM_CISCO_RSA_SIGN              = 0x400B,
    RPMB_GENERATE_AUTHENKEY         = 0x400C,
    DRM_KM_LOADKEYS                 = 0x400D,
    DRM_PVR_AESK0KDF_KEY            = 0x400F,
    // Misc
    DRM_GENERATE_SECUREKEY_V2       = 0x4010,

    // OTP
    DRM_OTP_PROGRAM_OTP             = 0x5000,
    DRM_OTP_UTILS_CALC_SSTORE_SHA1  = 0x5001,
    DRM_OTP_UTILS_UPDATE_VERSION    = 0x5002,
    DRM_OTP_UTILS_READ_VERSION      = 0x5003,
    DRM_OTP_UTILS_WRITE_SEGID       = 0x5004,
    DRM_OTP_UTILS_READ_SEGID        = 0x5005,
    DRM_OTP_UTILS_SET_JTAG          = 0x5006,
    DRM_OTP_UTILS_GET_JTAG          = 0x5007,
    DRM_OTP_UTILS_SET_RPMB          = 0x5008,
    DRM_OTP_UTILS_GET_RPMB          = 0x5009,
    DRM_OTP_UTILS_READ_PUBLIC_ITEMS = 0x500A,
    DRM_OTP_UTILS_READ_SOC_UID      = 0x500B,
    DRM_OTP_UTILS_WRITE_RKEKID      = 0x500C,
    DRM_OTP_UTILS_OTP_READ          = 0x500D,
    DRM_OTP_UTILS_OTP_WRITE         = 0x500E,

    // NETFLIX
    DRM_NETFLIX_LOAD_SECSTORE       = 0x6000,
    DRM_NETFLIX_BASE64              = 0x6001,
    DRM_NETFLIX_LOAD_KEYS           = 0x6002,

    // ALGORITHMS APIs
    DRM_ALGO_AES                    = 0x7000,
    DRM_ALGO_HMAC_SHA               = 0x7001,
    DRM_ALGO_BASE64                 = 0x7002,
    DRM_ALGO_AES_WRAP_KEY           = 0x7003,
    // ASYMMETRIC CRYPTO ALGORITHMS APIs
    DRM_ALGO_RSA_LOADKEY            = 0x7004,
    DRM_ALGO_RSA_UNLOADKEY          = 0x7005,
    DRM_ALGO_RSA_SIGN               = 0x7006,
    DRM_LOAD_GENKEYSTORE            = 0x7007,

    // USB Device Mode
    EROM_TURN_OFF_USB_DEVICE        = 0x8000,
    EROM_RESET_USB_CONSOLE_BUF      = 0x8001,
    EROM_TURN_ON_USB_DEVICE         = 0x8002,
    EROM_FLUSH_USB_CONSOLE          = 0x8003,

    // PlayReady
    DRM_PR_LOAD_SECSTORE            = 0x9000,
    DRM_PR25_SEND_CMD               = 0x9001,
    DRM_PR32_SEND_CMD               = 0x9002,

    // PlayReady key loading
    DRM_PR_LOAD_SECURE_CONTENT_KEY  = 0x9010,

    // HDCP2.x
    DRM_HDCP2X_LOAD_SECSTORE        = 0xA000,
    DRM_HDCP2X_XOR                  = 0xA001,
    DRM_HDCP2X_DKEY                 = 0xA002,
    DRM_HDCP2X_KEY_DERIVATION       = 0xA003,
    DRM_HDCP2X_RSA_DECRYPT          = 0xA004,
    DRM_HDCP2X_COMPUTE_KD           = 0xA005,
    DRM_HDCP2X_HDMI_TX_LOADKEYS     = 0xA006,

    // Secure Crypto
    DRM_SECURECRYPTO_SEND_CMD       = 0xB000,
    DRM_SECUREVMX_INIT              = 0xB001,
    DRM_SECUREVMX_GETKEYID          = 0xB002,
    DRM_SECUREVMX_KEYLADDER         = 0xB003,
    DRM_SECUREVMX_LOADCW            = 0xB004,
    DRM_SECURER2R_KEYLADDER         = 0xB005,
    DRM_SECUREPVR_GENKEY            = 0xB006,
    DRM_SECUREPVR_LOADKEY           = 0xB007,
    DRM_SECUREVMX_GETROMK_STATUS    = 0xB008,
    DRM_SECURE_TDES_VMX_KEYLADDER   = 0xB009,
    DRM_SECUREVMX_OTP_READ          = 0xB00A,

    // NOCS + COMMON CRYPTO
    DRM_SECURE_ALGO_BLOCK_CIPHER    = 0xC000,
    DRM_SECURE_RSA_GENKEY           = 0xC001,
    DRM_SECURE_RSA_COMPUTE_CRT      = 0xC002,
    DRM_SECURE_RSA                  = 0xC003,
    DRM_SECURE_DH_GENERATE_KEY      = 0xC004,
    DRM_SECURE_DH_COMPUTE_KEY       = 0xC005,
    DRM_SECURE_EC_GENERATE_KEY      = 0xC006,
    DRM_SECURE_ECDSA_SIGN           = 0xC007,
    DRM_SECURE_ECDSA_VERIFY         = 0xC008,
    DRM_SECURE_ECDH_COMPUTE_KEY     = 0xC009,
    DRM_SECURE_ALGO_HASH_INIT       = 0xC00A,
    DRM_SECURE_ALGO_HASH_UPDATE     = 0xC00B,
    DRM_SECURE_ALGO_HASH_FINAL      = 0xC00C,
    DRM_SECURE_ALGO_HASH            = 0xC00D,
    DRM_SECURE_GET_RANDOM           = 0xC00E,
    DRM_SECURE_CHIPSET_INFO         = 0xC00F,
    DRM_SECURE_IMG_AUTHENTICATE     = 0xC010,
    DRM_SECURE_NOCS_RSA_KEY         = 0xC011,
    DRM_SECURE_LOAD_CONTENT_KEY     = 0xC012,
    DRM_SECURE_GET_CRC32            = 0xC013,
    DRM_SECURE_STORE_HMAC           = 0xC014,
    DRM_SECURE_GET_EROMVERSION      = 0xC015,
    DRM_SECURE_GET_PRIVMODE         = 0xC016,
    DRM_SECURE_SET_PRIVMODE         = 0xC017,
    DRM_SECURE_GET_SCSPUBINFO       = 0xC018,
    DRM_SECURE_CREATE_FPRK          = 0xC019,

    //Synamedia PI
    BCM_EXTN_NSK2_RESET              = 0xE001,
    BCM_EXTN_NSK2_BASIC_INIT_COMPL   = 0xE002,
    BCM_EXTN_NSK2_READ_GENIN         = 0xE003,
    BCM_EXTN_NSK2_READ_GENOUT        = 0xE004,
    BCM_EXTN_NSK2_WRITE_GENIN        = 0xE005,
    BCM_EXTN_NSK2_READ_OTP           = 0xE006,
    BCM_EXTN_NSK2_GET_NV_COUNTER     = 0xE007,
    BCM_EXTN_NSK2_INC_NV_COUNTER     = 0xE008,

    // Others
    DRM_ULI_KEY_PROVISION           = 0xD000,
    DRM_CHARACTERIZATION            = 0xD001,
    DRM_GENERATE_SCRAMBLE_KEY       = 0xD002,
    DRM_CONFIGURE_SCRAMBLE_KEY      = 0xD003,
    DRM_ACCESS_SRAM                 = 0xD004,

    DRM_EXPORT_CUSTOM_DATA          = 0xE000,
    //IRDETO PI
    DRM_IRDETO_CALCULATE_CCC        = 0xE020,
    DRM_IRDETO_GET_CSUK4            = 0xE021,

} DRM_PRIMITIVE;


#endif //INCLUDE_BCM_PRIMITIVES_H

