/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "vpp_api.h"
#include "vpp_be_hdmitx.h"
#include "hdmi_scdc.h"
#include "hdmi_eddc.h"
#include "galois_io.h"
#ifndef CONFIG_AMP_TEE_ENABLE
#ifdef FIXME_IN_BG4CT
#endif
#endif
#include "vpp_osal.h"

#define DDC_SCDC_SLAVE_ADDR    0xA8
#define BURST_MAX       256
#define EDDC_SCDC_INDEX 2



SCDC_HRETURN SCDC_I2C_regRead(VPP_OBJ *vpp_obj, int regAddr, unsigned char *data, unsigned short count){

    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if(pHdmiTx == NULL)
        return (SCDC_ERR_UNDEFINED);

    /*EDDC Master based I2C*/
    if(EDDC_Read(pHdmiTx, EDDC_SCDC_INDEX , regAddr , count , data)) {
        return SCDC_ERR_IOFAIL;
    }

    return SCDC_SUCCESS;

}


SCDC_HRETURN SCDC_I2C_regWrite(VPP_OBJ *vpp_obj, int regAddr, unsigned char *data, unsigned short count){

    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if(pHdmiTx == NULL)
        return (SCDC_ERR_UNDEFINED);

    /*EDDC Master based I2C*/
    if (EDDC_Write(pHdmiTx, EDDC_SCDC_INDEX , regAddr , count , data)) {
        return SCDC_ERR_IOFAIL;
    }

    return SCDC_SUCCESS;
}


/******************************************************************************
*Function   : VPP_BE_SCDC_ReadCharacterErrorCounters
*Description: Get the error counter values from the SCDC register based on
              the TMDS channel, after verifying the valid bit flag.
              If the valid bit flag is not set, the counter value will not be updated.
*Input      : TMDS_Channel value for which the error count is needed and the
              errorCount Value
*Returns    : Success if successfully read and the value is updated, error otherwise
*******************************************************************************/

SCDC_HRETURN VPP_BE_SCDC_ReadCharacterErrorCounters(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData ) {

    SCDC_HRETURN retVal = SCDC_ERR_UNDEFINED;
    UINT32 Addr = SCDC_OFFSET_CHRCTR_ERR_DET;

    UINT8  DataBuf[CHAR_ERROR_COUNT] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    UINT8 CheckSum = 0x00;
    UINT32 i = 0;
    UINT16 BYTE0 , BYTE1;

    NULL_CHECK(pScdcTxData);
    // Read also clears the error count data
    retVal = SCDC_I2C_regRead(vpp_obj, Addr,DataBuf,CHAR_ERROR_COUNT);
    SCDC_I2C_RETCODE_CHK(retVal);

    //validate the check sum before proceeding
    for(i = 0; i < CHAR_ERROR_COUNT; i++) {
        CheckSum += DataBuf[i];
    }
    if(CheckSum != 0) {
       SCDC_PRINTF("Character Error Counter Check Sum Error");
       retVal = SCDC_ERR_ERRPARAM;
       return retVal;
    }
    //validate  TMDS 0 Error Counter
    if(DataBuf[1] & TMDS_CHARACTER_ERR_COUNTER_VALID_BITMASK) {
        BYTE1 = DataBuf[1] & 0x7F;
        BYTE0 = DataBuf[0];
        pScdcTxData->ChrcterErrCntr.CrctrErrCtrTMDS0  = (BYTE1 << 8 | BYTE0);
    }
    //Validate TMDS 1 Error Counter
    if(DataBuf[3] & TMDS_CHARACTER_ERR_COUNTER_VALID_BITMASK) {
        BYTE1 = DataBuf[3] & 0x7F;
        BYTE0 = DataBuf[2];
        pScdcTxData->ChrcterErrCntr.CrctrErrCtrTMDS1 = BYTE1 << 8 | BYTE0 ;
    }

    //validate and update error count of TMDS3
    if(DataBuf[5] & TMDS_CHARACTER_ERR_COUNTER_VALID_BITMASK) {
         BYTE1 = DataBuf[5] & 0x7F;
         BYTE0 = DataBuf[4];
         pScdcTxData->ChrcterErrCntr.CrctrErrCtrTMDS2 = BYTE1 << 8 | BYTE0;
    }
    return SCDC_SUCCESS;
}


/******************************************************************************
   Function   : VPP_BE_SCDC_EnableScrambling
   Description: Set the scrambling Enable bit in SCDC of the sink to indicate
                the video is scrambled
   Input      : TRUE for enabling, false for disabling
   Returns    : Success on enabling, error otherwise
*******************************************************************************/
SCDC_HRETURN VPP_BE_SCDC_EnableScrambling(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData) {

    SCDC_HRETURN retVal = SCDC_ERR_UNDEFINED;
    UINT32  Addr = SCDC_OFFSET_TMDS_CONFIG;
    UINT16  Length = 1;
    UINT8   DataBuf = 0;


    if(pScdcTxData->Scrambling_Enable == TRUE) {
        retVal = SCDC_I2C_regRead(vpp_obj, Addr,&DataBuf,Length);
        //Mask other bits and set the scrambling enable bit
        DataBuf = (DataBuf | 0x03);
        retVal = SCDC_I2C_regWrite(vpp_obj, Addr,&DataBuf,Length);
        SCDC_I2C_RETCODE_CHK(retVal);
    }
    else {
        retVal = SCDC_I2C_regRead(vpp_obj, Addr,&DataBuf,Length);
        SCDC_I2C_RETCODE_CHK(retVal);
        //Mask other bits and set the scrambling enable bit
        DataBuf = (DataBuf & 0xFC);
        retVal = SCDC_I2C_regWrite(vpp_obj, Addr,&DataBuf,Length);
        SCDC_I2C_RETCODE_CHK(retVal);
    }

    return SCDC_SUCCESS;
}


/******************************************************************************
   Function   : VPP_BE_SCDC_GetScramblingStatus
   Description: Get the status of the scrambling after setting the
                Scrambling_Enable and transmitting scrambled video
   Input      : Pointer which is set to TRUE if set, false other wise
   Returns    : Success on successfully reading, error otherwise
******************************************************************************/

SCDC_HRETURN VPP_BE_SCDC_GetScramblingStatus(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData) {

    SCDC_HRETURN retVal = SCDC_ERR_UNDEFINED;
    UINT32 Addr = SCDC_OFFSET_SCRAMBLING_STATUS;
    UINT16 Length = 1;
    UINT8  DataBuf = 0x00;

    NULL_CHECK(pScdcTxData);
    //Read the scrambling status
    retVal = SCDC_I2C_regRead(vpp_obj, Addr,&DataBuf,Length);
    SCDC_I2C_RETCODE_CHK(retVal);
    //Mask the value to update Scrambling Status bit
    pScdcTxData->Scrambling_Status = DataBuf & 0xFF ;

    return SCDC_SUCCESS;

}


/******************************************************************************
   Function   : VPP_BE_SCDC_ControlTMDSBitPeriodToClockPeriodRatio
   Description: Control the TMDS bit period to TMDS Clock period Ratio. Set it
                to (=1) if the TMDS Bit Rates in range 3.4 to 6 gbps
                Set it to (=0) if the TMDS link operated below 3.4 Gbps
   Input      : TMDS Bit Rates enum value 0 for < 3.4 Gbps and 1 for 3.4-6 Gbps
   Returns    : SUCCESS on success, error otherwise
******************************************************************************/


SCDC_HRETURN VPP_BE_SCDC_CtlTMDSBitToClkPeriodRatio(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData){
    SCDC_HRETURN retVal = SCDC_ERR_UNDEFINED;
    UINT32 Addr = SCDC_OFFSET_TMDS_CONFIG;
    UINT16 Length = 1;
    UINT8 DataBuf = 0x00;

    if(pScdcTxData->TMDS_Bit_Rate == TMDS_BIT_CLK_RATIO_1_BY_10){

        retVal = SCDC_I2C_regRead(vpp_obj, Addr,&DataBuf,Length);
        //Reset the TMDS clock Ratio Bit
        DataBuf &= 0xFD;
        retVal = SCDC_I2C_regWrite(vpp_obj, Addr,&DataBuf,Length);
        SCDC_I2C_RETCODE_CHK(retVal);
    }
    else if(pScdcTxData->TMDS_Bit_Rate ==  TMDS_BIT_CLK_RATIO_1_BY_40) {

        retVal = SCDC_I2C_regRead(vpp_obj, Addr,&DataBuf,Length);

        //Set the TMDS Clk ratio bit
        DataBuf |= 0x02;
        retVal = SCDC_I2C_regWrite(vpp_obj, Addr,&DataBuf,Length);
        SCDC_I2C_RETCODE_CHK(retVal);
    }

    return SCDC_SUCCESS;
}


/******************************************************************************
   Function   :   VPP_BE_SCDC_GetClockDetectedStatus
   Description: After setting the appropriate TMDS Bit Clock Ratio bit and
                sending the TMDS clock and data,read this status to check if
                the sink detected the clock signal
   Input      : pointer to get the status
   Returns    : SUCCESS on success, error otherwise
******************************************************************************/
SCDC_HRETURN VPP_BE_SCDC_GetClockDetectedStatus(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData){
    SCDC_HRETURN retVal = SCDC_ERR_UNDEFINED;
    UINT32 Addr = SCDC_OFFSET_STATUS_FLAG_0;
    UINT16 Length = 1;
    UINT8 DataBuf = 0x00;

    NULL_CHECK(pScdcTxData);
    retVal = SCDC_I2C_regRead(vpp_obj, Addr,&DataBuf,Length);
    SCDC_I2C_RETCODE_CHK(retVal);
    //Check the clock detected Bit
    pScdcTxData->Clock_Detected = (DataBuf & 0x01);

    return SCDC_SUCCESS;
}


/******************************************************************************
   Function   : VPP_BE_SCDC_GetSinkVersion
   Description: The function will return the version of the sink
   Input      : Pointer to get the version of the sink
   Returns    : Success on reading version and updating, error otherwise
******************************************************************************/

SCDC_HRETURN VPP_BE_SCDC_GetSinkVersion(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData){
    SCDC_HRETURN retVal = SCDC_ERR_UNDEFINED;
    UINT32 Addr = SCDC_OFFSET_SINK_VERSION;
    UINT16 Length = 1;
    UINT8 DataBuf = 0x00;

    NULL_CHECK(pScdcTxData);
    //Read and update the Source Version

    retVal = SCDC_I2C_regRead(vpp_obj, Addr,&DataBuf,Length);
    SCDC_I2C_RETCODE_CHK(retVal);

    pScdcTxData->SinkVersion = DataBuf ;

    return SCDC_SUCCESS;
}


/******************************************************************************
   Function   : VPP_BE_SCDC_UpdateSourceVersion
   Description: The function will update the version of the source
   Input      : The pointer which has source version value
   Returns    : Success on writing the version of source into scdc register,
                error otherwise
******************************************************************************/
SCDC_HRETURN VPP_BE_SCDC_UpdateSourceVersion(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData,UINT8 UpdateType){
    SCDC_HRETURN retVal = SCDC_ERR_UNDEFINED;
    UINT32 Addr = SCDC_OFFSET_SOURCE_VERSION;
    UINT16 Length = 1;
    UINT8 DataBuf = 0x00;

    NULL_CHECK(pScdcTxData);

    //Write source version into scdc register
    if(UpdateType == SCDC_SOURCE_VERSION_WRITE ) {
        DataBuf = pScdcTxData->SourceVersion ;
        SCDC_PRINTF("UpdateSourceVersion Write 0x%x",DataBuf);
        retVal = SCDC_I2C_regWrite(vpp_obj, Addr,&DataBuf,Length);
        SCDC_I2C_RETCODE_CHK(retVal);

    }
    else if (UpdateType == SCDC_SOURCE_VERSION_READ) {
       retVal = SCDC_I2C_regRead(vpp_obj, Addr,&DataBuf,Length);
       SCDC_PRINTF("UpdateSourceVersion Read 0x%x",DataBuf);
       SCDC_I2C_RETCODE_CHK(retVal);
       pScdcTxData->SourceVersion = DataBuf;

    }
    return SCDC_SUCCESS;
}


/******************************************************************************
   Function   :   VPP_BE_SCDC_GetChannelLockStatus
   Description: Get the channel Lock status of the appropriate channel
   Input      : pointer to get the status and also the channel for
                which the status is needed (updated to 1 if the channel is locked 0 otherwise)
   Returns    : SUCCESS on success, error otherwise
******************************************************************************/

SCDC_HRETURN VPP_BE_SCDC_GetChannelLockStatus(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData){
    SCDC_HRETURN retVal = SCDC_ERR_UNDEFINED;
    UINT32 Addr = SCDC_OFFSET_STATUS_FLAG_0;
    UINT16 Length = 1;
    UINT8 DataBuf = 0x00;

    NULL_CHECK(pScdcTxData);


    VPP_BE_SCDC_GetClockDetectedStatus(vpp_obj, pScdcTxData);
    if(!pScdcTxData->Clock_Detected) {
        SCDC_PRINTF("The Sink had not dtetected the clock .");
        return SCDC_ERR_FAIL;
    }

    retVal = SCDC_I2C_regRead(vpp_obj, Addr,&DataBuf,Length);
    SCDC_I2C_RETCODE_CHK(retVal);

    pScdcTxData->ChnlLckSts.Ch0_Locked = DataBuf & 0x02;
    pScdcTxData->ChnlLckSts.Ch1_Locked = DataBuf & 0x04;
    pScdcTxData->ChnlLckSts.Ch2_Locked = DataBuf & 0x08;


    return SCDC_SUCCESS;
}


/******************************************************************************
   Function   :   VPP_BE_SCDC_GetManufactureDetails
   Description: Get the manufacture specific details in the register and update
   Input      : pointer to a structure object for updating the manufactures details
                in SCDC registers
   Returns    : SUCCESS on success, error otherwise
******************************************************************************/

SCDC_HRETURN VPP_BE_SCDC_GetManufactureDetails(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData) {
    SCDC_HRETURN retVal = SCDC_ERR_UNDEFINED;
    UINT32 Addr = SCDC_OFFSET_MANFCTUR_DATA;
    UINT16 Length = sizeof(SCDC_MANUFACTURER_DETAILS);
    UINT8 *pDataBuf = NULL;
    UINT32 i = 0, j = 0, iStrCount;

    NULL_CHECK(pScdcTxData);
    //do a malloc here
    pDataBuf = (UINT8 *)VPP_Malloc(sizeof(SCDC_MANUFACTURER_DETAILS));
    if(pDataBuf == NULL) {
        SCDC_PRINTF("The Malloc function Failed");
        return SCDC_ERR_NOMEM;
    }
    VPP_Memset(pDataBuf , 0 , sizeof(SCDC_MANUFACTURER_DETAILS));

    retVal = SCDC_I2C_regRead(vpp_obj, Addr,pDataBuf,Length);
    SCDC_I2C_RETCODE_CHK(retVal);

    pScdcTxData->ManfctrDetails.Manufacturer_OUI_1 = pDataBuf[0];
    pScdcTxData->ManfctrDetails.Manufacturer_OUI_2 = pDataBuf[1];
    pScdcTxData->ManfctrDetails.Manufacturer_OUI_3 = pDataBuf[2];

    iStrCount = 0;
    for(i = 3; i < 11; i ++) {
        if((pDataBuf[i] != 0x00)) {
            pScdcTxData->ManfctrDetails.Device_ID_String[iStrCount] = pDataBuf[i];
            iStrCount++;
        }
        else {
            pScdcTxData->ManfctrDetails.Device_ID_String[iStrCount] = '\0';
        }
    }
    pScdcTxData->ManfctrDetails.Device_ID_String[8] = '\0';

    pScdcTxData->ManfctrDetails.Hardware_Major_Rev = pDataBuf[11] & 0xF0 >> 4;
    pScdcTxData->ManfctrDetails.Hardware_Minor_Rev = pDataBuf[11] & 0x0F;
    pScdcTxData->ManfctrDetails.Software_Major_Rev = pDataBuf[12];
    pScdcTxData->ManfctrDetails.Software_Minor_Rev = pDataBuf[13];

    //manufacturer specific
    for(j = 0, i = 14; i < sizeof(SCDC_MANUFACTURER_DETAILS) && j < 34; i++, j++)
        pScdcTxData->ManfctrDetails.ManufacturerSpecific[j] = pDataBuf[i];

    VPP_Free(pDataBuf);

    return SCDC_SUCCESS;
}

/******************************************************************************
   Function    : VPP_BE_SCDC_ReadUpdateFlag
   Description : Read the Update Flag status
   Input       : pointer to a structure object for updating the SCDC Update registers
   Returns     : SUCCESS on success, error otherwise


******************************************************************************/


SCDC_HRETURN VPP_BE_SCDC_ReadUpdateFlag(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData ){
    SCDC_HRETURN retVal = SCDC_ERR_UNDEFINED;
    UINT32 Addr = SCDC_OFFSET_UPDATE_0_FLAG;
    UINT16 Length = 1;
    UINT8 DataBuf = 0x00;

    NULL_CHECK(pScdcTxData);


    retVal = SCDC_I2C_regRead(vpp_obj, Addr,&DataBuf,Length);
    SCDC_I2C_RETCODE_CHK(retVal);

    pScdcTxData->UpdateFlag.RR_Test      = (DataBuf & 0x04);
    pScdcTxData->UpdateFlag.CED_Update   = (DataBuf & 0x02);
    pScdcTxData->UpdateFlag.Status_Update = (DataBuf & 0x01);

    return SCDC_SUCCESS;
}

/******************************************************************************
   Function    : VPP_BE_SCDC_WriteUpdateFlag
   Description : Write the Update Flag status
   Input       : Enum for updating the corresponding bit of the Register
   Returns     : SUCCESS on success, error otherwise


******************************************************************************/


SCDC_HRETURN VPP_BE_SCDC_WriteUpdateFlag(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData ){
    SCDC_HRETURN retVal = SCDC_ERR_UNDEFINED;
    UINT32 Addr = SCDC_OFFSET_UPDATE_0_FLAG;
    UINT16 Length = 1;
    UINT8 DataBuf = 0x00;

    NULL_CHECK(pScdcTxData);

    retVal = SCDC_I2C_regRead(vpp_obj, Addr,&DataBuf,Length);
    SCDC_I2C_RETCODE_CHK(retVal);

    switch(pScdcTxData->UpdateFlagBit) {

        case SCDC_UPDATE_FLAG_RR_TEST:
            DataBuf = DataBuf | 0x04;
            retVal = SCDC_I2C_regWrite(vpp_obj, Addr,&DataBuf,Length);
            SCDC_I2C_RETCODE_CHK(retVal);
        break;

        case SCDC_UPDATE_FLAG_CED_UPDATE:
            DataBuf = DataBuf | 0x02;
            retVal = SCDC_I2C_regWrite(vpp_obj, Addr,&DataBuf,Length);
            SCDC_I2C_RETCODE_CHK(retVal);
        break;

        case SCDC_UPDATE_FLAG_STATUS_UPDATE:
            DataBuf = DataBuf | 0x01;
            retVal = SCDC_I2C_regWrite(vpp_obj, Addr,&DataBuf,Length);
            SCDC_I2C_RETCODE_CHK(retVal);
        break;

        default:
        break;


    }

    return SCDC_SUCCESS;
}

/******************************************************************************
   Function    : VPP_BE_SCDC_WriteConfigReg
   Description : Write the Config Register
   Input       : Enum for updating the corresponding bit of the Register and
                 enum to SET and RESET bit
   Returns     : SUCCESS on success, error otherwise


******************************************************************************/

SCDC_HRETURN VPP_BE_SCDC_WriteConfigReg(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData , BOOL WriteType) {

    SCDC_HRETURN retVal = SCDC_ERR_UNDEFINED;
    UINT32 Addr = SCDC_OFFSET_CONFIG_0_REG;
    UINT16 Length = 1;
    UINT8 DataBuf = 0x00;

    NULL_CHECK(pScdcTxData);

    switch(pScdcTxData->ConfigRegBit) {
        case SCDC_CONFIG_0_RR_ENABLE:
            if(WriteType ==  SET_BIT) {
                DataBuf = DataBuf | 0x01;
                retVal = SCDC_I2C_regWrite(vpp_obj, Addr,&DataBuf,Length);
                SCDC_I2C_RETCODE_CHK(retVal);
            }
            else if(WriteType == CLEAR_BIT){
                DataBuf = DataBuf & 0xFE;
                retVal = SCDC_I2C_regWrite(vpp_obj, Addr,&DataBuf,Length);
                SCDC_I2C_RETCODE_CHK(retVal);

            }
        break;

        default:
        break;
    }
    return retVal;
}

/******************************************************************************
   Function    : VPP_BE_SCDC_UpdateTestConfig
   Description : Write/Read the TestConfig Register
   Input       : Enum for Read/Write , pointer to strcuture Test Config
   Returns     : SCDC_SUCCESS on success, error otherwise


******************************************************************************/


SCDC_HRETURN VPP_BE_SCDC_UpdateTestConfig(VPP_OBJ *vpp_obj, PSCDC_TX_DATA pScdcTxData, BOOL UpdateType){

    SCDC_HRETURN retVal = SCDC_ERR_UNDEFINED;
    UINT32 Addr    = SCDC_OFFSET_TEST_CONFIG_0;
    UINT16 Length  = 1;
    UINT8 DataBuf  = 0x00;


   NULL_CHECK(pScdcTxData);

   //set the Test Read Request bit and test read request delay

   if(UpdateType == READ_REG){
       retVal = SCDC_I2C_regRead(vpp_obj, Addr,&DataBuf,Length);
       SCDC_I2C_RETCODE_CHK(retVal);
       pScdcTxData->TestConfig.TestReadRequest = (DataBuf & 0x80) >> 7; //FIXME: werror TestReadRequest is only 1 bit
       pScdcTxData->TestConfig.TestReadRequestDelay = DataBuf & 0x7F;

   }
   else if (UpdateType == WRITE_REG) {

       //Update the RR_Enable Flag First
       pScdcTxData->ConfigRegBit = SCDC_CONFIG_0_RR_ENABLE;
       VPP_BE_SCDC_WriteConfigReg(vpp_obj, pScdcTxData,SET_BIT);
       retVal = SCDC_I2C_regRead(vpp_obj, Addr,&DataBuf,Length);
       SCDC_I2C_RETCODE_CHK(retVal);
       DataBuf = ( pScdcTxData->TestConfig.TestReadRequest << 7 | pScdcTxData->TestConfig.TestReadRequestDelay);
       retVal = SCDC_I2C_regWrite(vpp_obj, Addr,&DataBuf,Length);
       SCDC_I2C_RETCODE_CHK(retVal);
   }


    return SCDC_SUCCESS;
}

