/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef __VPP_BE_SCDC_H__
#define __VPP_BE_SCDC_H__

#include "hdmi_cmn.h"
#include "vpp_module.h"



#define SCDC_PRINTF(...) VPP_log(VPP_LOG_HDMI, VPP_LOG_INFO, "VPPSCDC", __VA_ARGS__)

/*Register Values of SCDC*/


#define SCDC_OFFSET_SINK_VERSION       0x01
#define SCDC_OFFSET_SOURCE_VERSION     0x02
#define SCDC_OFFSET_UPDATE_0_FLAG      0x10
#define SCDC_OFFSET_UPDATE_1_FLAG      0x11
#define SCDC_OFFSET_TMDS_CONFIG        0x20
#define SCDC_OFFSET_SCRAMBLING_STATUS  0x21
#define SCDC_OFFSET_CONFIG_0_REG       0x30
#define SCDC_OFFSET_STATUS_FLAG_0      0x40
#define SCDC_OFFSET_STATUS_FLAG_1      0x41
#define SCDC_OFFSET_CHRCTR_ERR_DET     0x50
#define SCDC_OFFSET_TEST_CONFIG_0      0xC0
#define SCDC_OFFSET_MANFCTUR_DATA      0xD0




/* Macros */


#define TMDS_CHARACTER_ERR_COUNTER_VALID_BITMASK    0x80


#define SET_BIT                          1
#define CLEAR_BIT                        0
#define READ_REG                         0
#define WRITE_REG                        1
#define CHAR_ERROR_COUNT                 7

#define SCDC_I2C_RETCODE_CHK(RetCode) \
    do \
    { \
        if (RetCode == SCDC_ERR_IOFAIL) \
        { \
            VPPHDMI_DBG_ERROR("SCDC: DDC Error \n"); \
            return SCDC_ERR_IOFAIL;\
        } \
    }while(0);

#define NULL_CHECK(param) \
    do \
    { \
        if (param == NULL) \
        {\
            VPPHDMI_DBG_ERROR("The input paramter is NULL\n");\
            return SCDC_ERR_ERRPARAM; \
        } \
    }while(0);


typedef struct tagCRCTR_ERR_CTR {
    UINT16 CrctrErrCtrTMDS0;
    UINT16 CrctrErrCtrTMDS1;
    UINT16 CrctrErrCtrTMDS2;
}CRCTR_ERR_CTR;


enum {
    TMDS_BIT_CLK_RATIO_1_BY_10 = 0,
    TMDS_BIT_CLK_RATIO_1_BY_40 = 1
};


enum {
    SCDC_SOURCE_VERSION_READ  = 0,
    SCDC_SOURCE_VERSION_WRITE = 1
};

typedef struct tagCRCTR_ERR_CTR_CHNL_LCKD_STS {
    UINT8 Ch0_Locked          :1;
    UINT8 Ch1_Locked          :1;
    UINT8 Ch2_Locked          :1;
}CRCTR_ERR_CTR_CHNL_LCKD_STS;

typedef struct tagSCDC_MANUFACTURER_DETAILS {
    UINT8 Manufacturer_OUI_1 ;
    UINT8 Manufacturer_OUI_2 ;
    UINT8 Manufacturer_OUI_3 ;
    unsigned char Device_ID_String[9]; // Device ID string in ASCII format
    UINT8 Hardware_Major_Rev ;
    UINT8 Hardware_Minor_Rev ;
    UINT8 Software_Major_Rev ;
    UINT8 Software_Minor_Rev ;
    UINT8 ManufacturerSpecific[34];
}SCDC_MANUFACTURER_DETAILS;



typedef struct tagSCDC_UPDATE_FLAGS {
    UINT8 Reserved0        :1;
    UINT8 Reserved1        :1;
    UINT8 Reserved2        :1;
    UINT8 Reserved3        :1;
    UINT8 Reserved4        :1;
    UINT8 Reserved5        :1;
    UINT8 RR_Test          :1;
    UINT8 CED_Update       :1;
    UINT8 Status_Update    :1;
}SCDC_UPDATE_FLAGS;


typedef struct tagSCDC_TEST_CONFIG {
    UINT8 TestReadRequest      :1;
    UINT8 TestReadRequestDelay :7;
}SCDC_TEST_CONFIG;

enum SCDC_UPDATE_FLAG_REG {
    SCDC_UPDATE_FLAG_RESERVED0,
    SCDC_UPDATE_FLAG_RESERVED1,
    SCDC_UPDATE_FLAG_RESERVED2,
    SCDC_UPDATE_FLAG_RESERVED3,
    SCDC_UPDATE_FLAG_RESERVED4,
    SCDC_UPDATE_FLAG_RESERVED5,
    SCDC_UPDATE_FLAG_RR_TEST,
    SCDC_UPDATE_FLAG_CED_UPDATE,
    SCDC_UPDATE_FLAG_STATUS_UPDATE
};

enum SCDC_CONFIG_0_REG {
    SCDC_CONFIG_0_RESERVED0,
    SCDC_CONFIG_0_RESERVED1,
    SCDC_CONFIG_0_RESERVED2,
    SCDC_CONFIG_0_RESERVED3,
    SCDC_CONFIG_0_RESERVED4,
    SCDC_CONFIG_0_RESERVED5,
    SCDC_CONFIG_0_RESERVED6,
    SCDC_CONFIG_0_RESERVED7,
    SCDC_CONFIG_0_RR_ENABLE
};

typedef struct tagSCDC_TX_DATA {
    CRCTR_ERR_CTR ChrcterErrCntr;
    BOOL  Scrambling_Enable ;
    BOOL  Scrambling_Status ;
    UINT8 TMDS_Bit_Rate ;
    BOOL  Clock_Detected ;
    UINT8 SourceVersion ;
    UINT8 SinkVersion ;
    CRCTR_ERR_CTR_CHNL_LCKD_STS ChnlLckSts;
    SCDC_MANUFACTURER_DETAILS ManfctrDetails;
    SCDC_UPDATE_FLAGS UpdateFlag;
    UINT8 UpdateFlagBit;
    UINT8 ConfigRegBit;
    SCDC_TEST_CONFIG TestConfig;
}SCDC_TX_DATA , *PSCDC_TX_DATA;


typedef enum tagSCDC_HRETURN
{
    SCDC_SUCCESS         = 0,   /**< Success. */
    SCDC_ERR_NOTIMPL     = 1,   /**< Function/feature not implementated. */
    SCDC_ERR_ERRPARAM    = 2,   /**< Function parameter error. */
    SCDC_ERR_NOMEM       = 3,   /**< Not enough memory. */
    SCDC_ERR_TIMEOUT     = 4,   /**< Operation timeout. */
    SCDC_ERR_ERRSYSCALL  = 5,   /**< Syscall error. */
    SCDC_ERR_IOFAIL      = 6,   /**< Perepheral IO fail. */
    SCDC_ERR_HWBUSY      = 7,   /**< Hardware busy. */
    SCDC_ERR_HWFAIL      = 8,   /**< Hardware fail. */
    SCDC_ERR_OSALFAIL    = 9,   /**< OSAL fail. */
    SCDC_ERR_NOSWRSC     = 10,   /**< Not enough software resource. */
    SCDC_ERR_FAIL        = 11,
    SCDC_ERR_UNDEFINED   = 99   /**< The module private error value base. */
}SCDC_HRETURN ;
/*Function Prototypes*/

SCDC_HRETURN SCDC_I2C_regRead(VPP_OBJ *vpp_obj, int regAddr, unsigned char *data, unsigned short count);
SCDC_HRETURN SCDC_I2C_regWrite(VPP_OBJ *vpp_obj, int regAddr, unsigned char *data, unsigned short count);
SCDC_HRETURN VPP_BE_SCDC_ReadCharacterErrorCounters(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData);
SCDC_HRETURN VPP_BE_SCDC_EnableScrambling(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData);
SCDC_HRETURN VPP_BE_SCDC_GetScramblingStatus(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData);
SCDC_HRETURN VPP_BE_SCDC_CtlTMDSBitToClkPeriodRatio(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData);
SCDC_HRETURN VPP_BE_SCDC_GetClockDetectedStatus(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData);
SCDC_HRETURN VPP_BE_SCDC_UpdateSourceVersion(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData,UINT8 UpdateType);
SCDC_HRETURN VPP_BE_SCDC_GetChannelLockStatus(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData);
SCDC_HRETURN VPP_BE_SCDC_GetManufactureDetails(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData);
SCDC_HRETURN VPP_BE_SCDC_ReadUpdateFlag(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData);
SCDC_HRETURN VPP_BE_SCDC_WriteUpdateFlag(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData);
SCDC_HRETURN VPP_BE_SCDC_UpdateTestConfig(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData, BOOL UpdateType);
SCDC_HRETURN VPP_BE_SCDC_GetSinkVersion(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData);

#endif

