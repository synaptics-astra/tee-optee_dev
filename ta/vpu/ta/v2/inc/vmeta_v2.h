#ifndef __VMETA_V2_H__
#define __VMETA_V2_H__

#include <stdint.h>

#ifdef CONFIG_OPTEE
#include "tee_api_types.h"
#else // Default TZK
#include "tee_common.h"
#endif // CONFIG_OPTEE

TEE_Result InitEntryPointV2(void);
void DeinitEntryPointV2(void);
TEE_Result InitSessionV2(void *sessionContext);
void DeinitSessionV2(void *sessionContext);
TEE_Result InvokeCommandV2(void *session, uint32_t command_id,
                           uint32_t parameters_type, TEE_Param params[4]);

#endif // __VMETA_V2_H__