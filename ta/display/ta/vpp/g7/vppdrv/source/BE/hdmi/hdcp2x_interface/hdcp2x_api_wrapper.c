/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */



#include "ta_log.h"
#include "tee_internal_api.h"
#ifndef CONFIG_OPTEE
#include "tee_internal_private_api.h"
#else
#include "tee_api_syna.h"
#endif
#include "tee_ta_osal.h"

#if HDCP2X_ENABLED
#include "hdcp2x_cmd.h"
#include "hdcp2x_api.h"

#include "vpp_module.h"
#include "vpp_com.h"
#include "vpp_be_hdmitx.h"
#include "hdcp2x_api_wrapper.h"

#define AVIO_GLB_SWRESET           (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_GBL_BASE + RA_avioGbl_SWRST_CTRL)

/*--------------------------- STATIC functions --------------------------*/
#if defined (ESM_HDMIRX_RST_DECOUPLED)
static INT HDCP2X_ResetESM(BE_HDMITX_DATA *pHdmiTx);
#endif

static void hdcp2x_esm_clkrst(void)
{
#ifdef BA_avioGbl_SWRST_CTRL_HdmiRefclkRstn
    T32avioGbl_SWRST_CTRL swrst_ctrl;
    GA_REG_WORD32_READ(AVIO_GLB_SWRESET, &swrst_ctrl.u32);
    swrst_ctrl.uSWRST_CTRL_HdmiRefclkRstn = 0;
    GA_REG_WORD32_WRITE(AVIO_GLB_SWRESET, swrst_ctrl.u32);
    swrst_ctrl.uSWRST_CTRL_HdmiRefclkRstn = 1;
    GA_REG_WORD32_WRITE(AVIO_GLB_SWRESET, swrst_ctrl.u32);
#endif
}

static void *HDCP2X_AllocPeerShm(size_t size, uint32_t *shmId)
{
    TEE_Result result = TEE_SUCCESS;
    void *shmData = NULL;
    TEE_UUID shm_user = TA_HDCP2X_UUID;

    result = TEE_AllocateSharedMemory2(size, shmId, &shmData);
    if (result != TEE_SUCCESS){
        VPPHDMI_DBG_ERROR("TEE_AllocateSharedMemory2 fails return value is 0x%08x\n", result);
        return NULL;
    }

    result = TEE_AddSharedMemoryAccess(*shmId, shm_user, TEE_SHM_PEER_READWRITE);
    if (result != TEE_SUCCESS) {
        VPPHDMI_DBG_ERROR( "TEE shared memory grant failed, ret = 0x%x\n", result);
        TEE_UnmapSharedMemory(*shmId);
        shmData = NULL;
    } else {
        memset(shmData, 0, size);
    }

    return shmData;
}

static HDCP2X_RETURN HDCP2X_GetESMState(BE_HDMITX_DATA *pHdmiTx, HDCP2X_ESM_STATE *esmStatus)
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t cancellationTimeout = 0;
    uint32_t returnOrigin;
    uint32_t core_paramTypes;
    TEE_Param core_params[4];

    if ((pHdmiTx == NULL) || (pHdmiTx->hdcp2xSessionHandle == NULL)) {
        VPPHDMI_DBG_ERROR("null param error\n");
        return HDCP2X_ERR_NODEV;
    }

    core_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_OUTPUT,
                                  TEE_PARAM_TYPE_NONE,
                                  TEE_PARAM_TYPE_NONE,
                                  TEE_PARAM_TYPE_NONE);

    core_params[0].value.a = HDCP2X_ERR_GENERIC_FAILURE;

    res = TEE_InvokeTACommand(pHdmiTx->hdcp2xSessionHandle,
                     cancellationTimeout,
                     HDCP2X_CMD_GET_ESM_STAT,
                     core_paramTypes,
                     core_params,
                     &returnOrigin);

    if (res != TEE_SUCCESS) {
        VPPHDMI_DBG_ERROR("error in invoking hdcp2x command : 0x%x\n", res);
        return HDCP2X_ERR_NODEV;
    }

    *esmStatus = core_params[0].value.a;
    VPPHDMI_DBG_INFO("ESM state %d \n", *esmStatus);
    return HDCP2X_ERR_OK;
}

static HDCP2X_RETURN HDCP2X_SetAuthRequest(BE_HDMITX_DATA *pHdmiTx, HDCP2X_AUTH_REQ reqState)
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t cancellationTimeout = 0;
    uint32_t returnOrigin;
    uint32_t core_paramTypes;
    TEE_Param core_params[4];

    if ((pHdmiTx == NULL) || (pHdmiTx->hdcp2xSessionHandle == NULL)) {
        VPPHDMI_DBG_ERROR("null param error\n");
        return HDCP2X_ERR_NODEV;
    }

    core_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                                  TEE_PARAM_TYPE_VALUE_OUTPUT,
                                  TEE_PARAM_TYPE_NONE,
                                  TEE_PARAM_TYPE_NONE);

    core_params[0].value.a = HDCP2X_TX_AUID;
    core_params[0].value.b = reqState;

    core_params[1].value.a = HDCP2X_ERR_GENERIC_FAILURE;

    res = TEE_InvokeTACommand(pHdmiTx->hdcp2xSessionHandle,
                     cancellationTimeout,
                     HDCP2X_CMD_SET_AUTH_REQ,
                     core_paramTypes,
                     core_params,
                     &returnOrigin);

    if (res != TEE_SUCCESS) {
        VPPHDMI_DBG_ERROR("error in invoking hdcp2x command : 0x%x\n", res);
        return HDCP2X_ERR_NODEV;
    }

    if (core_params[1].value.a != HDCP2X_ERR_OK) {
        VPPHDMI_DBG_ERROR("error in triggering authenitcation state:%d : 0x%x\n", reqState, core_params[1].value.a);
        return core_params[1].value.a;
    }

    VPPHDMI_DBG_INFO("Triggered auth state %d for HDCP2.2 Tx \n", reqState);
    return HDCP2X_ERR_OK;
}

static HDCP2X_RETURN HDCP2X_GetAuthStatus(BE_HDMITX_DATA *pHdmiTx, HDCP2X_ESM_MONITOR_RET *esm_monitor_state)
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t cancellationTimeout = 0;
    uint32_t returnOrigin;
    uint32_t core_paramTypes;
    TEE_Param core_params[4];

    if ((pHdmiTx == NULL) || (pHdmiTx->hdcp2xSessionHandle == NULL)) {
        VPPHDMI_DBG_ERROR("null param error\n");
        return HDCP2X_ERR_NODEV;
    }

    core_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                                  TEE_PARAM_TYPE_VALUE_OUTPUT,
                                  TEE_PARAM_TYPE_VALUE_OUTPUT,
                                  TEE_PARAM_TYPE_NONE);

    core_params[0].value.a = HDCP2X_TX_AUID;
    core_params[1].value.a = HDCP2X_ESM_MON_ERR_AUTH_FAILURE;
    core_params[1].value.b = HDCP2X_ESM_CUR_STATE_INVALID;
    core_params[2].value.a = HDCP2X_ERR_GENERIC_FAILURE;

    res = TEE_InvokeTACommand(pHdmiTx->hdcp2xSessionHandle,
                     cancellationTimeout,
                     HDCP2X_CMD_GET_AUTH_STAT,
                     core_paramTypes,
                     core_params,
                     &returnOrigin);

    if (res != TEE_SUCCESS) {
        VPPHDMI_DBG_ERROR("error in invoking hdcp2x command : 0x%x\n", res);
        return HDCP2X_ERR_NODEV;
    }

    if (core_params[2].value.a != HDCP2X_ERR_OK) {
        VPPHDMI_DBG_ERROR("error in reading authenitcation state : 0x%x\n", core_params[2].value.a);
        return HDCP2X_ERR_BADCALL;
    }

    *esm_monitor_state = core_params[1].value.a;
    return HDCP2X_ERR_OK;
}

static HDCP2X_RETURN HDCP2X_GetDownstreamTopology(BE_HDMITX_DATA *pHdmiTx, uint32_t shmId, uint32_t *rxIdListSize)
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t cancellationTimeout = 0;
    uint32_t returnOrigin;
    uint32_t core_paramTypes;
    TEE_Param core_params[4];

    if ((pHdmiTx == NULL) || (pHdmiTx->hdcp2xSessionHandle == NULL)) {
        VPPHDMI_DBG_ERROR("null param error\n");
        return HDCP2X_ERR_NODEV;
    }

    core_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                                  TEE_PARAM_TYPE_VALUE_INOUT,
                                  TEE_PARAM_TYPE_VALUE_INOUT,
                                  TEE_PARAM_TYPE_NONE);

    core_params[0].value.a = shmId;
    core_params[1].value.a = *rxIdListSize;
    core_params[2].value.a = HDCP2X_ERR_GENERIC_FAILURE;

    res = TEE_InvokeTACommand(pHdmiTx->hdcp2xSessionHandle,
                     cancellationTimeout,
                     HDCP2X_CMD_GET_DWNSTRM_TOPOLOGY,
                     core_paramTypes,
                     core_params,
                     &returnOrigin);

    if (res != TEE_SUCCESS) {
        VPPHDMI_DBG_ERROR("error in invoking hdcp2x command : 0x%x\n", res);
        return HDCP2X_ERR_NODEV;
    }

    if (core_params[2].value.a != HDCP2X_ERR_OK) {
        VPPHDMI_DBG_ERROR("error in reading topology info : 0x%x\n", core_params[2].value.a);
        return HDCP2X_ERR_BADCALL;
    }

    *rxIdListSize = core_params[1].value.a;
    VPPHDMI_DBG_INFO("received topology info from downstream. rxIdListSize : %d\n", *rxIdListSize);
    return HDCP2X_ERR_OK;
}

static HDCP2X_RETURN HDCP2X_SetContentStrmType(BE_HDMITX_DATA *pHdmiTx, HDCP2X_STEAM_TYPE hdcp_content_type)
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t cancellationTimeout = 0;
    uint32_t returnOrigin;
    uint32_t core_paramTypes;
    TEE_Param core_params[4];

    if ((pHdmiTx == NULL) || (pHdmiTx->hdcp2xSessionHandle == NULL)) {
        VPPHDMI_DBG_ERROR("null param error\n");
        return HDCP2X_ERR_NODEV;
    }

    core_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                                  TEE_PARAM_TYPE_VALUE_OUTPUT,
                                  TEE_PARAM_TYPE_NONE,
                                  TEE_PARAM_TYPE_NONE);

    core_params[0].value.a = hdcp_content_type;

    core_params[1].value.a = HDCP2X_ERR_GENERIC_FAILURE;

    res = TEE_InvokeTACommand(pHdmiTx->hdcp2xSessionHandle,
                     cancellationTimeout,
                     HDCP2X_CMD_SET_STREAM_TYPE,
                     core_paramTypes,
                     core_params,
                     &returnOrigin);

    if (res != TEE_SUCCESS) {
        VPPHDMI_DBG_ERROR("error in invoking hdcp2x command : 0x%x\n", res);
        return HDCP2X_ERR_NODEV;
    }

    if (core_params[1].value.a != HDCP2X_ERR_OK) {
        VPPHDMI_DBG_ERROR("error in setting content stream type:%d : 0x%x\n", hdcp_content_type, core_params[1].value.a);
        return HDCP2X_ERR_BADCALL;
    }

    return HDCP2X_ERR_OK;
}

static HDCP2X_RETURN HDCP2X_TAInvokeCmd(BE_HDMITX_DATA *pHdmiTx, int hdcp2xCmd)
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t cancellationTimeout = 0;
    uint32_t returnOrigin;
    uint32_t core_paramTypes;
    TEE_Param core_params[4];

    if ((pHdmiTx == NULL) || (pHdmiTx->hdcp2xSessionHandle == NULL)) {
        VPPHDMI_DBG_ERROR("null param error\n");
        return HDCP2X_ERR_NODEV;
    }

    core_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
                                  TEE_PARAM_TYPE_NONE,
                                  TEE_PARAM_TYPE_NONE,
                                  TEE_PARAM_TYPE_NONE);

    core_params[0].value.a = HDCP2X_ERR_GENERIC_FAILURE;

    res = TEE_InvokeTACommand(pHdmiTx->hdcp2xSessionHandle,
                     cancellationTimeout,
                     hdcp2xCmd,
                     core_paramTypes,
                     core_params,
                     &returnOrigin);

    if (res != TEE_SUCCESS) {
        VPPHDMI_DBG_ERROR("error in invoking hdcp2x command(%d) : 0x%x\n",
                    hdcp2xCmd, res);
        return HDCP2X_ERR_NODEV;
    }

    if (core_params[0].value.a != HDCP2X_ERR_OK) {
        return HDCP2X_ERR_BADCALL;
    }

    return HDCP2X_ERR_OK;
}

static HDCP2X_RETURN HDCP2X_ESMLoadDownstreamCap(BE_HDMITX_DATA *pHdmiTx)
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t cancellationTimeout = 0;
    uint32_t returnOrigin;
    uint32_t core_paramTypes;
    TEE_Param core_params[4];

    if((pHdmiTx == NULL) || (pHdmiTx->hdcp2xSessionHandle == NULL)) {
        VPPHDMI_DBG_ERROR("null param error\n");
        return HDCP2X_ERR_NODEV;
    }

    core_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_OUTPUT,
                                  TEE_PARAM_TYPE_NONE,
                                  TEE_PARAM_TYPE_NONE,
                                  TEE_PARAM_TYPE_NONE);

    core_params[0].value.a = HDCP2X_ERR_GENERIC_FAILURE;
    res = TEE_InvokeTACommand(pHdmiTx->hdcp2xSessionHandle,
                     cancellationTimeout,
                     HDCP2X_CMD_GET_DS_CAP,
                     core_paramTypes,
                     core_params,
                     &returnOrigin);

    if(res != TEE_SUCCESS) {
        VPPHDMI_DBG_ERROR("error in invoking hdcp2x command : 0x%x\n", res);
        return HDCP2X_ERR_NODEV;
    }

    if(core_params[0].value.a != HDCP2X_ERR_OK) {
        VPPHDMI_DBG_ERROR("error in loading ds capability in esm : 0x%x\n", core_params[0].value.a);
        return HDCP2X_ERR_BADCALL;
    }

    return HDCP2X_ERR_OK;
}

static HDCP2X_RETURN HDCP2X_PowerDownESM(BE_HDMITX_DATA *pHdmiTx)
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t cancellationTimeout = 0;
    uint32_t returnOrigin;
    uint32_t core_paramTypes;
    TEE_Param core_params[4];

    core_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
                                  TEE_PARAM_TYPE_VALUE_OUTPUT,
                                  TEE_PARAM_TYPE_NONE,
                                  TEE_PARAM_TYPE_NONE);


    core_params[0].value.a = HDCP2X_ERR_GENERIC_FAILURE;
    res = TEE_InvokeTACommand(pHdmiTx->hdcp2xSessionHandle,
                     cancellationTimeout,
                     HDCP2X_CMD_DEINIT,
                     core_paramTypes,
                     core_params,
                     &returnOrigin);

    if(res != TEE_SUCCESS) {
        VPPHDMI_DBG_ERROR("error in invoking hdcp2x command : 0x%x\n", res);
        return HDCP2X_ERR_NODEV;
    }

    if(core_params[0].value.a != HDCP2X_ERR_OK) {
        VPPHDMI_DBG_ERROR("error in powerdown ESM module: 0x%x\n", core_params[0].value.a);
        return HDCP2X_ERR_BADCALL;
    }
    return HDCP2X_ERR_OK;
}

static HDCP2X_RETURN HDCP2X_SetOperatingMode(BE_HDMITX_DATA *pHdmiTx, INT opMode)
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t cancellationTimeout = 0;
    uint32_t returnOrigin;
    uint32_t core_paramTypes;
    TEE_Param core_params[4];

    if((pHdmiTx == NULL) || (pHdmiTx->hdcp2xSessionHandle == NULL)) {
        VPPHDMI_DBG_ERROR("null param error\n");
        return HDCP2X_ERR_NODEV;
    }

    core_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                                  TEE_PARAM_TYPE_VALUE_OUTPUT,
                                  TEE_PARAM_TYPE_NONE,
                                  TEE_PARAM_TYPE_NONE);

    core_params[0].value.a = opMode;
    core_params[1].value.a = HDCP2X_ERR_GENERIC_FAILURE;

    res = TEE_InvokeTACommand(pHdmiTx->hdcp2xSessionHandle,
                     cancellationTimeout,
                     HDCP2X_CMD_SET_OP_MODE,
                     core_paramTypes,
                     core_params,
                     &returnOrigin);

    if(res != TEE_SUCCESS) {
        VPPHDMI_DBG_ERROR("error in invoking hdcp2x command : 0x%x\n", res);
        return HDCP2X_ERR_NODEV;
    }

    if(core_params[1].value.a != HDCP2X_ERR_OK) {
        VPPHDMI_DBG_ERROR("error in setting hdcp operating mode :%d : 0x%x\n", opMode, core_params[1].value.a);
        return HDCP2X_ERR_BADCALL;
    }

    return HDCP2X_ERR_OK;
}

/*--------------------------- EXPORT functions --------------------------*/

TEE_Result HDCP2X_InitCtx(TEE_TASessionHandle* sessionContext)
{
    TEE_Result res = TEE_SUCCESS;
    const TEE_UUID uuid = TA_HDCP2X_UUID;
    uint32_t        cancellationTimeout = 0;
    uint32_t        core_paramTypes;
    TEE_Param       core_params[4];
    uint32_t        returnOrigin;

    /*Open Session to HDCP2X TA*/
    core_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
                                      TEE_PARAM_TYPE_NONE,
                                      TEE_PARAM_TYPE_NONE,
                                      TEE_PARAM_TYPE_NONE);
    if (*sessionContext == NULL){
        res = TEE_OpenTASession(&uuid,
                                cancellationTimeout,
                                core_paramTypes,
                                core_params,
                                sessionContext,
                                &returnOrigin);

        VPPHDMI_DBG_INFO("open session. result=0x%lx, origin=0x%lx, session=%p\n",
            res, returnOrigin, *sessionContext);
        if (res != TEE_SUCCESS) {
            VPPHDMI_DBG_ERROR("fail to open session. result=0x%lx, origin=%lx, session=%p\n",
                                        res, returnOrigin, *sessionContext);
            return TEE_ERROR_NOT_SUPPORTED;
        }
    }
    return res;
}

void HDCP2X_DeInitCtx(TEE_TASessionHandle sessionContext)
{
    if (sessionContext != NULL){
        TEE_CloseTASession(sessionContext);
        VPPHDMI_DBG_INFO("closed HDCP2X session\n");
    }
}

bool HDCP2X_IsESMAlive(BE_HDMITX_DATA *pHdmiTx)
{
    HDCP2X_RETURN retVal;
    HDCP2X_ESM_STATE esmStatus;

    retVal = HDCP2X_GetESMState(pHdmiTx, &esmStatus);
    if (HDCP2X_ERR_OK != retVal) {
        VPPHDMI_DBG_ERROR("errror in reading HDCP2X esm state:0x%x\n", retVal);
        return false;
    }

    if (esmStatus == HDCP2X_ESM_ACTIVE) {
        VPPHDMI_DBG_ERROR("esm is alive\n");
        return true;
    } else {
        VPPHDMI_DBG_ERROR("esm not active : %d\n", esmStatus);
        return false;
    }
}

INT HDCP2X_StopAuthentication(BE_HDMITX_DATA *pHdmiTx)
{
    HDCP2X_RETURN retVal;
    retVal = HDCP2X_SetAuthRequest(pHdmiTx, HDCP2X_REQ_STOP);
    if (retVal != HDCP2X_ERR_OK) {
        VPPHDMI_DBG_ERROR("error in stopping HDCP2.2 authentication:0x%x\n", retVal);
        return MV_VPP_EBADCALL;
    }

    return MV_VPP_OK;
}

INT HDCP2X_StartAuthentication(BE_HDMITX_DATA *pHdmiTx)
{
    HDCP2X_RETURN retVal;
    retVal = HDCP2X_SetAuthRequest(pHdmiTx, HDCP2X_REQ_START);
    if (retVal != HDCP2X_ERR_OK) {
#if defined (ESM_HDMIRX_RST_DECOUPLED)
        if(retVal == HDCP2X_ERR_ESM_TIMEOUT) {
            VPPHDMI_DBG_ERROR("ESM timeout error.. attempting recovery\n");
            HDCP2X_ResetESM(pHdmiTx);
        }
#endif
        VPPHDMI_DBG_ERROR("error in starting HDCP2.2 authentication:0x%x\n", retVal);
        return MV_VPP_EBADCALL;
    }

    return MV_VPP_OK;
}

INT HDCP2X_GetAuthenticationState(BE_HDMITX_DATA *pHdmiTx, HDCP2X_ESM_MONITOR_RET *esm_monitor_state)
{
    HDCP2X_RETURN retVal;
    retVal = HDCP2X_GetAuthStatus(pHdmiTx, esm_monitor_state);
    if (retVal != HDCP2X_ERR_OK) {
        VPPHDMI_DBG_ERROR("error in reading hdcp authentication status : 0x%x \n", retVal);
        return MV_VPP_EBADCALL;
    }

    return MV_VPP_OK;
}

INT HDCP2X_GetTopology(BE_HDMITX_DATA *pHdmiTx)
{
    HDCP2X_RETURN retVal;
    uint32_t shmId;
    uint32_t rxIdListSize;
    void *shmBuffer = NULL;
    HDCP2X_DOWNSTREAM_TOPOLOGY *topology_info = &pHdmiTx->hdcp2xRptrTopology;

    shmBuffer = HDCP2X_AllocPeerShm(sizeof(HDCP2X_DOWNSTREAM_TOPOLOGY), &shmId);
    if (shmBuffer == NULL) {
        VPPHDMI_DBG_ERROR("error in allocating memory to read downstream details\n");
        return MV_VPP_ENOMEM;
    }

    rxIdListSize = DEV_ID_SIZE*MAX_DWNSTRM_DEV_CNT;
    retVal = HDCP2X_GetDownstreamTopology(pHdmiTx, shmId, &rxIdListSize);
    if (retVal != HDCP2X_ERR_OK) {
        VPPHDMI_DBG_ERROR("error in getting downstream topology: 0x%x \n", retVal);
        TEE_UnmapSharedMemory(shmId);
        return MV_VPP_EBADCALL;
    }

    if (rxIdListSize > DEV_ID_SIZE*MAX_DWNSTRM_DEV_CNT) {
        VPPHDMI_DBG_ERROR(" invalid size received for rxIdList: %d\n", rxIdListSize);
        TEE_UnmapSharedMemory(shmId);
        return MV_VPP_EIOFAIL;
    }

    memset(topology_info->devIdList, 0, DEV_ID_SIZE*MAX_DWNSTRM_DEV_CNT);
    memcpy(&topology_info->rxInfo, &((HDCP2X_DOWNSTREAM_TOPOLOGY *)shmBuffer)->rxInfo, sizeof(HDCP2X_RX_INFO));
    memcpy(topology_info->devIdList, ((HDCP2X_DOWNSTREAM_TOPOLOGY *)shmBuffer)->devIdList, rxIdListSize);
    TEE_UnmapSharedMemory(shmId);

    VPPHDMI_DBG_INFO("HDCP2X topology info: depth:%d deviceCount:%d 1x_dwnstrm:%d max_cas:%d max_dev:%d rxIdListSize:%d\n",
                      pHdmiTx->hdcp2xRptrTopology.rxInfo.depth,
                      pHdmiTx->hdcp2xRptrTopology.rxInfo.dev_cnt,
                      pHdmiTx->hdcp2xRptrTopology.rxInfo.downstream_1x_dev,
                      pHdmiTx->hdcp2xRptrTopology.rxInfo.max_depth_exceed,
                      pHdmiTx->hdcp2xRptrTopology.rxInfo.max_dev_exceed,
                      rxIdListSize);

    return MV_VPP_OK;
}

#if defined (ESM_HDMIRX_RST_DECOUPLED)
static INT HDCP2X_ResetESM(BE_HDMITX_DATA *pHdmiTx)
{
    HDCP2X_RETURN retVal;
    hdcp2x_esm_clkrst();
    retVal = HDCP2X_TAInvokeCmd(pHdmiTx, HDCP2X_CMD_ESM_RESET);
    if (retVal != HDCP2X_ERR_OK) {
        return MV_VPP_EBADCALL;
    }

    return MV_VPP_OK;
}
#endif

INT HDCP2X_Resume(BE_HDMITX_DATA *pHdmiTx)
{
    HDCP2X_RETURN retVal;
    retVal = HDCP2X_TAInvokeCmd(pHdmiTx, HDCP2X_CMD_RESUME);
    if (retVal != HDCP2X_ERR_OK) {
        return MV_VPP_EBADCALL;
    }

    return MV_VPP_OK;
}

INT HDCP2X_Suspend(BE_HDMITX_DATA *pHdmiTx)
{
    HDCP2X_RETURN retVal;
    retVal = HDCP2X_TAInvokeCmd(pHdmiTx, HDCP2X_CMD_SUSPEND);
     if (retVal != HDCP2X_ERR_OK) {
        return MV_VPP_EBADCALL;
    }

    return MV_VPP_OK;
}

INT HDCP2X_UpdateContentStrmInfo(BE_HDMITX_DATA *pHdmiTx, HDCP2X_STEAM_TYPE hdcp_content_type)
{
    HDCP2X_RETURN retVal;
    VPPHDMI_DBG_INFO("updating content stream info to : %d\n", hdcp_content_type);
    retVal = HDCP2X_SetContentStrmType(pHdmiTx, hdcp_content_type);
    if (retVal != HDCP2X_ERR_OK) {
        VPPHDMI_DBG_ERROR("error in setting content stream type : 0x%x \n", retVal);
        return MV_VPP_EBADCALL;
    }

    return MV_VPP_OK;
}

INT HDCP2X_LoadDownstreamCapability(BE_HDMITX_DATA *pHdmiTx)
{
    HDCP2X_RETURN retVal;
    retVal = HDCP2X_ESMLoadDownstreamCap(pHdmiTx);
    if (retVal != HDCP2X_ERR_OK) {
        VPPHDMI_DBG_ERROR("error loading downstream capability in ESM : 0x%x \n", retVal);
        return MV_VPP_EBADCALL;
    }

    return MV_VPP_OK;
}

INT HDCP2X_SetMode(BE_HDMITX_DATA *pHdmiTx, INT opMode)
{
    HDCP2X_RETURN retVal;
    //attempt to keep ESM in suspend mode before changing mode
    HDCP2X_PowerDownESM(pHdmiTx);
    hdcp2x_esm_clkrst();
    retVal = HDCP2X_SetOperatingMode(pHdmiTx, opMode);
    if (retVal != HDCP2X_ERR_OK) {
        VPPHDMI_DBG_ERROR("error(0x%x) in Setting HDCP mode to 0x%x\n", retVal, opMode);
        return ((retVal==HDCP2X_ERR_PRESET)?MV_VPP_EHDCPNOOP:MV_VPP_EBADCALL);

    }

    return MV_VPP_OK;
}

#endif
