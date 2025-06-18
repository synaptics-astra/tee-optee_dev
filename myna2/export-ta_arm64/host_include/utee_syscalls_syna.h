// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2024, Synaptics Incorporated.
 */

#ifndef UTEE_SYSCALLS_SYNA_H
#define UTEE_SYSCALLS_SYNA_H

#include <utee_syscalls.h>
#include "tee_syscall_numbers_syna.h"

inline static TEE_Result _utee_syna_mr_filter(void *region, uint32_t *max_num,
			unsigned long attr_mask, unsigned long attr_val)
{
	return _utee_syna_entry(TEE_SCN_SYNA_MR_FILTER, (uint64_t)region,
			(uint64_t)max_num, (uint64_t)attr_mask,
			(uint64_t)attr_val, 0, 0, 0);
}

inline static TEE_Result _utee_syna_mr_find_region(const void *buffer,
			uint32_t size, void *mr)
{
	return _utee_syna_entry(TEE_SCN_SYNA_MR_FIND, (uint64_t)buffer,
			(uint64_t)size, (uint64_t)mr, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_exec_cmd(uint32_t cmd_id, const void *param,
		uint32_t param_len, const void *result, uint32_t result_len)
{
	return _utee_syna_entry(TEE_SCN_SYNA_EXEC_CMD, (uint64_t)cmd_id,
			(uint64_t)param, (uint64_t)param_len,
			(uint64_t)result, (uint64_t)result_len, 0, 0);
}

inline static TEE_Result _utee_syna_exec_cmd_ex(uint32_t cmd_id, const void *param,
		uint32_t param_len, const void *result,
		uint32_t result_len, uint32_t delay)
{
	return _utee_syna_entry(TEE_SCN_SYNA_EXEC_CMD_EXT, (uint64_t)cmd_id,
			(uint64_t)param, (uint64_t)param_len,
			(uint64_t)result, (uint64_t)result_len,
			(uint64_t)delay, 0);
}

inline static TEE_Result _utee_syna_verify_image(const void *in, uint32_t inLen,
			void *out, uint32_t *outLen, uint32_t codeType)
{
	return _utee_syna_entry(TEE_SCN_SYNA_VERIFY_IMAGE, (uint64_t)in,
			(uint64_t)inLen, (uint64_t)out, (uint64_t)outLen,
			(uint64_t)codeType, 0, 0);
}

inline static TEE_Result _utee_syna_map_register(void **va, size_t len,
			unsigned long prot, unsigned long flags, unsigned long off)
{
	return _utee_syna_entry(TEE_SCN_SYNA_MAP_REGISTER, (uint64_t)va,
			(uint64_t)len, (uint64_t)prot, (uint64_t)flags,
			(uint64_t)off, 0, 0);
}

inline static TEE_Result _utee_syna_unmap_register(void *va, size_t len)
{
	return _utee_syna_entry(TEE_SCN_SYNA_UNMAP_REGISTER, (uint64_t)va,
			(uint64_t)len, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_virt_to_phys(unsigned long *pa, void *va)
{
	return _utee_syna_entry(TEE_SCN_SYNA_VA2PA_CMD, (uint64_t)pa,
			(uint64_t)va, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_get_otp_raw_data(uint32_t row, uint8_t *buffer)
{
	return _utee_syna_entry(TEE_SCN_SYNA_GET_OTP_RAW_CMD, (uint64_t)row,
			(uint64_t)buffer, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_register_memory(paddr_t mem, size_t size,
		uint32_t type, uint32_t *mem_id)
{
	return _utee_syna_entry(TEE_SCN_SYNA_REGISTER_MEMORY, (uint64_t)mem,
			(uint64_t)size, (uint64_t)type, (uint64_t)mem_id,
			0, 0, 0);
}

inline static TEE_Result _utee_syna_unregister_memory(uint32_t mem_id)
{	return _utee_syna_entry(TEE_SCN_SYNA_UNREGISTER_MEMORY, (uint64_t)mem_id,
			0, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_add_memory_access(uint32_t mem_id, TEE_UUID *uuid,
		void *sa, uint32_t permission)
{
	return _utee_syna_entry(TEE_SCN_SYNA_ADD_MEMORY_ACCESS, (uint64_t)mem_id,
			(uint64_t)uuid, (uint64_t)sa, (uint64_t)permission,
			0, 0, 0);
}

inline static TEE_Result _utee_syna_map_memory(uint32_t mem_id, void **va, size_t *size)
{
	return _utee_syna_entry(TEE_SCN_SYNA_MAP_MEMORY, (uint64_t)mem_id,
			(uint64_t)va, (uint64_t)size, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_unmap_memory(uint32_t mem_id)
{
	return _utee_syna_entry(TEE_SCN_SYNA_UNMAP_MEMORY, (uint64_t)mem_id,
			0, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_get_phy_by_memid(uint32_t mem_id, paddr_t *phy_addr)
{
	return _utee_syna_entry(TEE_SCN_SYNA_GET_PHY_BY_MEMID, (uint64_t)mem_id,
			(uint64_t)phy_addr, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_get_memid_by_phys(uint32_t *mem_id,
		paddr_t phy_addr, size_t size)
{
	return _utee_syna_entry(TEE_SCN_SYNA_GET_MEMID_BY_PHYS, (uint64_t)mem_id,
			(uint64_t)phy_addr, (uint64_t)size, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_check_memattr_by_memid(uint32_t mem_id, uint32_t attr)
{
	return _utee_syna_entry(TEE_SCN_SYNA_CHECK_MEMATRR, (uint64_t)mem_id,
			(uint64_t)attr, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_reserve_reservation(void * reservation)
{
	return _utee_syna_entry(TEE_SCN_SYNA_RESERVE_RESERVATION, (uint64_t)reservation,
			0, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_release_reservation(void * reservation)
{
	return _utee_syna_entry(TEE_SCN_SYNA_RELEASE_RESERVATION, (uint64_t)reservation,
			0, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_reservation_get_info(uint32_t mem_id, uint32_t info_type,
		void *buf, uint32_t buf_size)
{
	return _utee_syna_entry(TEE_SCN_SYNA_RESERVATION_GET_INFO, (uint64_t)mem_id,
			(uint64_t)info_type, (uint64_t)buf, (uint64_t)buf_size,
			0, 0, 0);
}

inline static TEE_Result _utee_syna_refcnt_op(uint32_t mem_id, uint32_t type, uint32_t *refc)
{
	return _utee_syna_entry(TEE_SCN_SYNA_REFCNT_OP, (uint64_t)mem_id,
			(uint64_t)type, (uint64_t)refc, 0, 0, 0, 0);
}

/*
 *  * global data
 *   */
inline static TEE_Result _utee_syna_set_globaldata(uint32_t key, uint32_t value)
{
	return _utee_syna_entry(TEE_SCN_SYNA_SET_GLOBALDATA, (uint64_t)key,
			(uint64_t)value, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_get_globaldata(uint32_t key, uint32_t *value)
{
	return _utee_syna_entry(TEE_SCN_SYNA_GET_GLOBALDATA, (uint64_t)key,
			(uint64_t)value, 0, 0, 0, 0, 0);
}

/*
 *  * mutex
 *   */
inline static TEE_Result _utee_syna_mutex_alloc(int *h)
{
	return _utee_syna_entry(TEE_SCN_SYNA_MUTEX_ALLOC, (uint64_t)h,
			0, 0, 0, 0, 0, 0);
}

inline static void _utee_syna_mutex_destroy(int h)
{
	_utee_syna_entry(TEE_SCN_SYNA_MUTEX_DESTORY, (uint64_t)h,
			0, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_mutex_lock(int h)
{
	return _utee_syna_entry(TEE_SCN_SYNA_MUTEX_LOCK, (uint64_t)h,
			0, 0, 0, 0, 0, 0);
}

inline static void _utee_syna_mutex_unlock(int h)
{
	_utee_syna_entry(TEE_SCN_SYNA_MUTEX_UNLOCK, (uint64_t)h,
			0, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_mutex_trylock(int h)
{
	return _utee_syna_entry(TEE_SCN_SYNA_MUTEX_TRYLOCK, (uint64_t)h,
			0, 0, 0, 0, 0, 0);
}

/*
 *  * semaphore
 *   */
inline static TEE_Result _utee_syna_semaphore_alloc(int s)
{
	return _utee_syna_entry(TEE_SCN_SYNA_SEMAPHORE_ALLOC, (uint64_t)s,
			0, 0, 0, 0, 0, 0);
}

inline static void _utee_syna_semaphore_destroy(int s)
{
	_utee_syna_entry(TEE_SCN_SYNA_SEMAPHORE_DESTORY, (uint64_t)s,
			0, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_semaphore_wait(int s)
{
	return _utee_syna_entry(TEE_SCN_SYNA_SEMAPHORE_WAIT, (uint64_t)s,
			0, 0, 0, 0, 0, 0);
}

inline static void _utee_syna_semaphore_post(int s)
{
	_utee_syna_entry(TEE_SCN_SYNA_SEMAPHORE_POST, (uint64_t)s,
			0, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_semaphore_trywait(int s)
{
	return _utee_syna_entry(TEE_SCN_SYNA_SEMAPHORE_TRYWAIT, (uint64_t)s,
			0, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_semaphore_timewait(int s, int t)
{
	return _utee_syna_entry(TEE_SCN_SYNA_SEMAPHORE_TIMEWAIT, (uint64_t)s,
			(uint64_t)t, 0, 0, 0, 0, 0);
}

/*
 *  * secure key
 *   */

inline static TEE_Result _utee_syna_get_securekey(void *sk)
{
	return _utee_syna_entry(TEE_SCN_SYNA_GET_SECUREKEY, (uint64_t)sk,
			0, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_get_securekey2(void *sk)
{
	return _utee_syna_entry(TEE_SCN_SYNA_GET_SECUREKEY2, (uint64_t)sk,
			0, 0, 0, 0, 0, 0);
}

/*
 *  * sa entry
 *   */

inline static TEE_Result _utee_syna_get_sa_entry(void *sessionEntry, uint32_t keySessionId)
{
	return _utee_syna_entry(TEE_SCN_SYNA_GET_SA_ENTRY, (uint64_t)sessionEntry,
			(uint64_t)keySessionId, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_set_sa_entry(void *sessionEntry)
{
	return _utee_syna_entry(TEE_SCN_SYNA_SET_SA_ENTRY, (uint64_t)sessionEntry,
			0, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_get_active_entries(int *sa_num)
{
	return _utee_syna_entry(TEE_SCN_SYNA_GET_ACTIVE_ENTRIES, (uint64_t)sa_num,
			0, 0, 0, 0, 0, 0);
}

inline static TEE_Result _utee_syna_get_active_sa_entry(void *sessionEntry)
{
	return _utee_syna_entry(TEE_SCN_SYNA_GET_ACTIVE_SA_ENTRY, (uint64_t)sessionEntry,
			0, 0, 0, 0, 0, 0);
}

#endif /* UTEE_SYSCALLS_SYNA_H */
