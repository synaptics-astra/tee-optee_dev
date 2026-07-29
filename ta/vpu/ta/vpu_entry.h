#ifndef __VMETA_V1_H__
#define __VMETA_V1_H__

#include <stdint.h>

#ifdef CONFIG_OPTEE
#include "tee_api_types.h"
#else // Default TZK
#include "tee_common.h"
#endif // CONFIG_OPTEE


#ifdef VMETA_V1_EN
TEE_Result InitSessionV1(VPU_SESSION_CTX *sessionContext);
void DeinitSessionV1(VPU_SESSION_CTX *sessionContext);
TEE_Result InvokeCommandV1(VPU_SESSION_CTX *session, uint32_t command_id,
                           uint32_t parameters_type, TEE_Param params[4]);
#else
static inline TEE_Result InitSessionV1(VPU_SESSION_CTX *sessionContext) {
    return TEE_SUCCESS;
}
static inline void DeinitSessionV1(VPU_SESSION_CTX *sessionContext) {
    return;
}
static inline TEE_Result InvokeCommandV1(VPU_SESSION_CTX *session,
                                         uint32_t command_id,
                                         uint32_t parameters_type,
                                         TEE_Param params[4]) {
    return TEE_SUCCESS;
}
#endif

#ifdef VMETA_V2_EN
TEE_Result InitSessionV2(VPU_SESSION_CTX *sessionContext);
void DeinitSessionV2(VPU_SESSION_CTX *sessionContext);
TEE_Result InvokeCommandV2(VPU_SESSION_CTX *session, uint32_t command_id,
                           uint32_t parameters_type, TEE_Param params[4]);
#else
static inline TEE_Result InitSessionV2(VPU_SESSION_CTX *sessionContext) {
    return TEE_SUCCESS;
}
static inline void DeinitSessionV2(VPU_SESSION_CTX *sessionContext) {
    return;
}
static inline TEE_Result InvokeCommandV2(VPU_SESSION_CTX *session,
                                         uint32_t command_id,
                                         uint32_t parameters_type,
                                         TEE_Param params[4]) {
    return TEE_SUCCESS;
}
#endif

#endif //__VPU_ENTREY_H__