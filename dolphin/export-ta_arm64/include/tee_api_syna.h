/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2014, STMicroelectronics International N.V.
 */

/* Based on GP TEE Internal API Specification Version 1.1 */
#ifndef TEE_API_SYNA_H
#define TEE_API_SYNA_H

#include <stddef.h>
#include <compiler.h>
#include <tee_api_defines.h>
#include <tee_api_types.h>
#include <trace.h>
#include "tee_internal_api_extensions.h"
#if 0
TEE_Result TEE_GetMemRegionList(char *region, uint32_t maxNum,
		uint32_t attrMask, uint32_t attrVal);
uint32_t TEE_GetMemRegionCount(uint32_t attrMask, uint32_t attrVal);

#else
#include "mem_region.h"

enum {
	RPMB_QUERY_STATE,
	RPMB_WRITE_KEY,
};

typedef struct {
	unsigned int addr; // physical address
	unsigned int npage; // number of scatter
} scatter_item_t;

typedef struct {
	unsigned int num; // number of element in the table
	scatter_item_t s_item[]; // point to the first element of scatter_item
} scatter_list_t;

#define SIZE_OF_SCATTERLIST(num) (sizeof(unsigned int) + (num * sizeof(scatter_item_t)))

typedef struct {
	scatter_list_t *s_list; // raw pages
	unsigned int a;  // single security attribute
	uint32_t mem_id; // id returned by reserve
} TEE_Reservation;

typedef struct mem_region	TEE_MemRegion;
typedef struct reg_region	TEE_RegRegion;

#define TEE_MR_ATTR(perm, reg, zone, mattr, ctrl)		\
	MR_ATTR(perm, reg, zone, mattr, ctrl)

#define TEE_MR_M_PERM			MR_M_PERM
#define TEE_MR_M_ZONE			MR_M_ZONE
#define TEE_MR_M_TYPE			MR_M_TYPE
#define TEE_MR_M_MEM_ATTR		MR_M_MEM_ATTR
#define TEE_MR_M_CACHEABLE		MR_M_CACHEABLE
#define TEE_MR_M_DATA_ATTR		MR_M_DATA_ATTR
#define TEE_MR_M_PREMAPPED		MR_M_PREMAPPED

#define TEE_MR_PERM(mr)			MR_PERM(mr)
#define TEE_MR_ZONE(mr)			MR_ZONE(mr)
#define TEE_MR_WINDOW(mr)		MR_WINDOW(mr)
#define TEE_MR_MEM_ATTR(mr)		MR_MEM_ATTR(mr)
#define TEE_MR_IS_RESTRICTED(mr)	MR_IS_RESTRICTED(mr)
#define TEE_MR_IS_SECURE(mr)		MR_IS_SECURE(mr)
#define TEE_MR_IS_NONSECURE(mr)		MR_IS_NONSECURE(mr)
#define TEE_MR_IS_PREMAPPED(mr)		MR_IS_PREMAPPED(mr)
#define TEE_MR_IS_CACHEABLE(mr)		MR_IS_CACHEABLE(mr)
#define TEE_MR_IS_DATA_ATTR(mr)		MR_IS_DATA_ATTR(mr)
#define TEE_MR_IS_REGISTER(mr)		MR_IS_REGISTER(mr)
#define TEE_MR_IS_MEMORY(mr)		MR_IS_MEMORY(mr)

/** Get the memory region count
 *
 * @param attrMask	attribute mask to filter memory regions.
 * 			can be the flags in TEE_MR_M_*. 0 to get all.
 * 			only (region.attr & attrMask == attrVal) is filtered.
 * @param attrVal	attribute value to filter memory regions.
 * 			use TEE_MR_ATTR() to generate it. ignored if attrMask=0.
 *
 * @return	region number.
 */
uint32_t TEE_GetMemRegionCount(uint32_t attrMask, uint32_t attrVal);

/** Retrieve the memory region list.
 *
 * @param region	buffer to store the retrieved regions.
 *			NULL to return region number, same as
 *			TEE_GetMemRegionCount().
 * @param maxNum	max count can be retrieved (count region).
 * @param attrMask	attribute mask to filter memory regions.
 * 			can be the flags in TEE_MR_M_*. 0 to get all.
 * 			only (region.attr & attrMask == attrVal) is filtered.
 * @param attrVal	attribute value to filter memory regions.
 * 			use TEE_MR_ATTR() to generate it. ignored if attrMask=0.
 *
 * @return	retrieved region number. if maxNum < total matched region
 * 		number, then only maxNum would be copied, and return total
 * 		matched region_number.
 */
uint32_t TEE_GetMemRegionList(TEE_MemRegion *region, uint32_t maxNum,
		uint32_t attrMask, uint32_t attrVal);

/** Find memory region based on the input address.
 *
 * @param region	buffer to store the retrieved region.
 * @param buffer	buffer address to check.
 * @param size		size of the buffer, must not cross 2 regions.
 *
 * @retval TEE_SUCCESS			Pass the check.
 * @retval TEE_ERROR_BAD_PARAMETERS	Parameter error.
 */
TEE_Result TEE_FindMemRegion(TEE_MemRegion *region,
		void *buffer, uint32_t size);

/** Find Read register region based on the input address.
 *
 * @param region	buffer to store the register region.
 * @param addr      address to check.
 *
 * @retval TEE_SUCCESS			    Pass the check.
 * @retval TEE_ERROR_BAD_PARAMETERS	Parameter error.
 * @retval TEE_ERROR_ACCESS_DENIED  address is not in the register region
 */
TEE_Result TEE_FindRegRegion(TEE_RegRegion *region,
		void *addr);

/** Get raw data from OTP
 *
 * @param row		row of data in OTP
 * @parm buffer		raw data, should be uint8_t buf[8]
 * @retval TEE_SUCCESS	return TEE_SUCCESS
 */
TEE_Result TEE_GetOtpRawData(uint32_t row, uint8_t *buffer);

/** Verify BCM image.
 *
 * @param in		buffer to keep the enced image.
 * @param inLen		length of the enced image
 * @param out		buffer to keep the decrypted image
 * @param outLen	buffer to keep the return length
 *
 * @retval TEE_SUCCESS			success to get the info.
 * @retval TZ_ERROR_BAD_PARAMETERS	invalid parameters.
 */
TEE_Result TEE_VerifyImage(const void *in, uint32_t inLen, void *out,
		uint32_t *outLen, uint32_t codeType);

/** send command to secure processor and wait until its completion
 *
 * @param cmd_id	command ID
 * @param param		buffer of the input param
 * @param param_len	parameter length
 * @param result	buffer to keep the result
 * @param result_len	result length
 *
 * @retval TEE_SUCCESS	success to get the info.
 */
TEE_Result TEE_ExecCmd(size_t cmd_id, const void *param, size_t param_len,
		const void *result, size_t result_len);

/** send command to secure processor and wait until its completion
 *  can set the delay number
 *
 * @param cmd_id	command ID
 * @param param		buffer of the input param
 * @param param_len	parameter length
 * @param result	buffer to keep the result
 * @param result_len	result length
 * @param delay		delay ms number
 *
 * @retval TEE_SUCCESS	success to get the info.
 */
TEE_Result TEE_ExecCmdEx(size_t cmd_id, const void *param, size_t param_len,
		const void *result, size_t result_len, uint32_t delay);


/** map physical address.
 *
 * currently, it can only be used to map registers.
 *
 * @param pa		Physical Address
 * @param len		bytes to map
 * @param prot		protection type. reserved for future use
 * @param flags		reserved for future use
 * @param va		buffer to return Virtual Address
 *
 * @retval TEE_SUCCESS			success to map.
 * @retval TEE_ERROR_BAD_PARAMETERS	bad parameters.
 * @retval TEE_ERROR_ACCESS_DENIED	not allow to map
 */
TEE_Result TEE_Map_Register(paddr_t pa, size_t len, uint32_t prot,
		uint32_t flags, void **va);
TEE_Result TEE_MemMap(paddr_t pa, size_t len, uint32_t prot,
		uint32_t flags, void **va);

/** unmap memory.
 *
 * @param va		Virtual Address to unmap.
 * @param len		bytes to unmap.
 *
 * @retval TEE_SUCCESS			success to unmap.
 * @retval TEE_ERROR_BAD_PARAMETERS	wrong parameters. !va or len == 0.
 */
TEE_Result TEE_Unmap_Register(void *va, size_t len);

TEE_Result TEE_MemUnmap(void *va, size_t len);

/** register a memory and get a mem_id
 *
 * @param mem		physical start address
 * @param size		size of the memory
 * @param mem_id	id of the memory generated
 *
 * @retval TEE_SUCCESS			register successfully
 * @retval TZ_ERROR_SHORT_BUFFER tzk heap dried up
 * @retval TZ_ERROR_ACCESS_CONFLICT memory already being registered
 * @retval TZ_ERROR_BAD_PARAMETERS memory is not in any ion region
 */
TEE_Result  TEE_Register_Memory(paddr_t mem, size_t size,  uint32_t *mem_id);

/** unregister the memory
 *
 * @param mem_id	id of the memory
 *
 * @retval TEE_SUCCESS		successfully unregistered
 * @retval TZ_ERROR_BUSY	still in using
 * @retval TEE_ERROR_BAD_PARAMETERS  no such memory
 */
TEE_Result TEE_Unregister_Memory(uint32_t mem_id);



/** map the memory into the virtual space of caller TA
 *
 * @param mem_id	id of the memory
 * @param va		the returned virutal address
 * @param size		the returned memory size
 *
 * @retval TEE_SUCCESS			successfully mapped
 * @retval TEE_ERROR_BAD_PARAMETERS  error when map or already mapped
 */
TEE_Result TEE_Map_Memory(uint32_t mem_id, void **va, size_t *size);

/** unmap the memory from the virutal space of caller TA if mapped
 *
 * @param mem_id	id of the memory
 *
 * @retval TEE_SUCCESS			successfully unmapped
 * @retval TEE_ERROR_BAD_PARAMETERS  not mapped
 */
TEE_Result TEE_Unmap_Memory(uint32_t mem_id);

/** get the physical address of contigious memory by memory id.
 *
 * @param mem_id	memory id.
 * @param phy_addr	point of physical address.
 *
 * @retval TEE_SUCCESS			get physical address successfully.
 * @retval TEE_ERROR_BAD_PARAMETERS	wrong parameters.
 *							wrong mem_id or non-contigious memory
 */
TEE_Result TEE_GetPhys_By_Memid(uint32_t mem_id, paddr_t *phy_addr);

/** get the memory id by the physical address of contigious memory.
 *
 * @param mem_id	point of memory id.
 * @param phy_addr	physical address of contigious memory.
 * @param size		size of the contigious memory
 *
 * @retval TEE_SUCCESS				get memory id successfully.
 * @retval TEE_ERROR_ITEM_NOT_FOUND	Contigious memory is not registered
 * @retval TEE_ERROR_BAD_PARAMETERS	wrong parameters.
 *							wrong contigious physical memory
 */
TEE_Result TEE_GetMemid_By_Phys(uint32_t *mem_id, paddr_t phy_addr, uint32_t size);

enum {
	MEMATTR_SECURE = 0x1,
	MEMATTR_NONSECURE = 0x2,

	MEMATTR_CACHEABLE = 0x10,
	MEMATTR_NONCACHEABLE = 0x20,

	MEMATTR_RESTRICTED = 0x100,
	MEMATTR_SYSTEM = 0x200,
	MEMATTR_ION = 0x400,
};

/** return if the memory has the memory attributes.
 *
 * @param mem_id	memory id.
 * @param attr		memory attribute (can be multiple options)
 *
 * @retval true			memory attributes checked are true
 * @retval false		error
 */
bool TEE_Check_Memory_Attr(uint32_t mem_id, uint32_t attr);


/** create MPT reservation
 *
 * the reservation includes a scatter list and rule index
 *
 * @param sctl_t		contains MPT table and numbers
 * @param rule			MPT rule
 *
 * @retval TEE_SUCCESS				MPT rule is set successfully
 * @retval TEE_ERROR_BAD_PARAMETERS	fail to set MPT rule
 */
TEE_Reservation *TEE_CreateReservation(void *sctl_t, unsigned int rule);

/** destory MPT reservation after it is released
 *
 * @param r		structure of MPT reservation
 *
 * @return
 */
void TEE_DestroyReservation(TEE_Reservation *r);

/** reserve the memory in TEE_Reservation
 *
 * if the rule is secure rule, the MPT rule of memorys in the scatter list
 * will be set to the secure rule
 * if the rule is non-secure rule, the return result shows whether it
 * matchs with the non-secure rule (this is required by PTM TA)
 *
 * @param r		structure of MPT reservation
 *
 * @retval TEE_SUCCESS				secure memory is reserved
 * @retval TEE_ERROR_BAD_PARAMETERS	fail to reserve
 */
TEE_Result TEE_ReserveSecurePages(TEE_Reservation *r);

/** release MPT reservation
 *
 * it means the MPT rule of the memorys in the scatter list will be set
 * to non-secure rule (by default, non-secure rule is 0)
 *
 * @param r		structure of MPT reservation
 *
 * @retval TEE_SUCCESS				secure memory is released
 * @retval TEE_ERROR_BAD_PARAMETERS	fail to release
 */
TEE_Result TEE_ReleaseSecurePages(TEE_Reservation *r);

/** get the number of scatter list associated with the reservation mem_id
 *
 * @param mem_id	memory id returned by TEE_ReserveSecurePages
 * @param num		number of scatter list
 *
 * @retval TEE_SUCCESS			get the number successfully
 */
TEE_Result TEE_Mem_GetSglistNum(uint32_t mem_id, uint32_t *num);

/** get the scatter list associated with the reservation mem_id
 *
 * @param mem_id	memory id returned by TEE_ReserveSecurePages
 * @param sgt		buffer to copy the scatter list
 * @param size		size of the buffer
 *
 * @retval TEE_SUCCESS			get the scatter list
 */
TEE_Result TEE_Mem_GetSglist(uint32_t mem_id, scatter_list_t *sgt, uint32_t size);

/** get the MPT rule associated with the reservation mem_id
 *
 * @param mem_id	memory id returned by TEE_ReserveSecurePages
 * @param rule		mpt rule of the memory
 *
 * @retval TEE_SUCCESS			the rule retrived
 */
TEE_Result TEE_Reservation_GetAttribute(uint32_t mem_id, uint32_t *rule);

/** get the ref count through memory id
 *
 * @param mem_id	id of the memory
 * @param refc		value of ref count
 *
 * @retval TEE_SUCCESS		successfully get the ref count
 */
TEE_Result TEE_Mem_GetRefCnt(uint32_t mem_id, uint32_t *refc);

/** increase the ref count of a memory
 *
 * @param mem_id	id of the memory
 *
 * @retval TEE_SUCCESS		successfully increase the ref count
 */
TEE_Result TEE_MemRef(uint32_t mem_id);

/** decrease the ref count of a memory
 *
 * @param mem_id	id of the memory
 *
 * @retval TEE_SUCCESS		successfully decrease the refcount
 */
TEE_Result TEE_MemUnref(uint32_t mem_id);

/** Set the TA's global data.
 *
 * @param key		the key of the global data.
 * @param value		the value of the global data.
 *
 * @retval TEE_SUCCESS			set success.
 * @retval TEE_ERROR_OUT_OF_MEMORY	out of memory.
 */
TEE_Result set_globaldata(uint32_t key, uint32_t value);

/** Get the TA's global data.
 *
 * @param key		the key of the global data.
 * @param value		address of the value.
 *
 * @retval TEE_SUCCESS			get success.
 * @retval TEE_ERROR_BAD_PARAMETERS	the address of value is error.
 * @retval TEE_ERROR_ITEM_NOT_FOUND	the key is not found.
 */
TEE_Result get_globaldata(uint32_t key, uint32_t *value);

/*
 * Physical/Virtual Address Conversion API
 */
paddr_t TEE_VirtToPhys(void *va);

void TEE_GetSystemTimeNS(uint64_t *ns);

#define TEE_CleanCache TEE_CacheClean
#define TEE_FlushCache TEE_CacheFlush
#define TEE_InvalidateCache TEE_CacheInvalidate

enum TEE_MallocHint {
	TEE_MALLOC_ZEROS		= 0x00000000,
	TEE_MALLOC_FLAG_DEFAULT		= TEE_MALLOC_ZEROS,
	TEE_MALLOC_FLAG_SYS_MAX		= 0x7fffffff,
	TEE_MALLOC_FLAG_USER_START	= 0x80000000,
	TEE_MALLOC_FLAG_USER_MAX	= 0xffffffff
};

/* Memory Management Constant */
//#define TEE_MALLOC_FILL_ZERO               TEE_MALLOC_ZEROS
#define TEE_MALLOC_NO_FILL                 0x00000001
#define TEE_MALLOC_NO_SHARE                0x00000002
#define TEE_ERROR_TIMEOUT				0xFFFF3001
#define TEE_ERROR_OVERFLOW				0xFFFF300F


/* Memory Transfer Operation */
typedef enum {
	TEE_MT_COPY,
	TEE_MT_ENCRYPT,
	TEE_MT_DECRYPT
} TEE_MemoryOperation;

/** check memory transfer rights.
 *
 * @param dst		destination address.
 * @param dstLen	destination data length
 * @param src		source address.
 * @param srcLen	source data length
 * @param op		Operation, see TEE_MemoryOperation.
 *
 * @retval TEE_SUCCESS			Pass the check.
 * @retval TEE_ERROR_ACCESS_CONFLICT	Can't pass the check, master must not
 *					issue the transfer.
 *
 * Examples:
 * - memory to memory copy:
 *	res = TEE_CheckMemoryTransferRights(dst, dstLen, src, srcLen,
 *			TEE_MT_COPY);
 * - memory to memory decrypt:
 *	res = TEE_CheckMemoryTransferRights(dst, dstLen, src, srcLen,
 *			TEE_MT_DECRYPT);
 */
TEE_Result TEE_CheckMemoryTransferRights(
		const void*		dst,
		uint32_t		dstLen,
		const void*		src,
		uint32_t		srcLen,
		TEE_MemoryOperation	op);

/* memory access permision, see TZ_Sxx_Nxx in tz_perm.h */
#define TEE_NONSECURE				TZ_SRW_NRW
#define TEE_SECURE				TZ_SRW_NNA
#define TEE_RESTRICTED				TZ_SNA_NNA

/** check memory input rights.
 *
 * @param src		source address.
 * @param srcLen	source data length
 * @param dstPerm	desitination memory access permission, can be
 *			TEE_NONSECURE, TEE_SECURE.
 * @param op		Operation, see TEE_MemoryOperation.
 *
 * @retval TEE_SUCCESS			Pass the check.
 * @retval TEE_ERROR_ACCESS_CONFLICT	Can't pass the check, master must not
 *					issue the transfer.
 *
 * Examples:
 * - memory to memory copy:
 *	res = TEE_CheckMemoryInputRights(src, srcLen,
 *			TEE_NONSECURE, TEE_MT_COPY);
 * - memory to memory decrypt:
 *	res = TEE_CheckMemoryTransferRights(src, srcLen,
 *			TEE_SECURE, TEE_MT_DECRYPT);
 */
TEE_Result TEE_CheckMemoryInputRights(
		const void*		src,
		uint32_t		srcLen,
		uint32_t		dstPerm,
		TEE_MemoryOperation	op);

/** check memory output rights.
 *
 * @param dst		destination address.
 * @param dstLen	destination data length
 * @param srcPerm	source memory access permission, can be
 *			TEE_NONSECURE, TEE_SECURE.
 * @param op		Operation, see TEE_MemoryOperation.
 *
 * @retval TEE_SUCCESS			Pass the check.
 * @retval TEE_ERROR_ACCESS_CONFLICT	Can't pass the check, master must not
 *					issue the transfer.
 *
 * Examples:
 * - memory to memory copy:
 *	res = TEE_CheckMemoryOutputRights(dst, dstLen,
 *			TEE_NONSECURE, TEE_MT_COPY);
 * - memory to memory decrypt:
 *	res = TEE_CheckMemoryTransferRights(dst, dstLen,
 *			TEE_SECURE, TEE_MT_DECRYPT);
 */
TEE_Result TEE_CheckMemoryOutputRights(
		const void*		dst,
		uint32_t		dstLen,
		uint32_t		srcPerm,
		TEE_MemoryOperation	op);



enum mutex_value {
	MUTEX_VALUE_UNLOCKED,
	MUTEX_VALUE_LOCKED,
};

typedef int TEE_MutexHandle;

TEE_Result TEE_MutexCreateExt(TEE_MutexHandle *);
TEE_Result TEE_MutexLockExt(TEE_MutexHandle);
TEE_Result TEE_MutexUnlockExt(TEE_MutexHandle);
TEE_Result TEE_MutexTryLockExt(TEE_MutexHandle);
TEE_Result TEE_MutexDestroyExt(TEE_MutexHandle);

/** create mutex.
 *
 * @param lock		return the lock handle.
 * @param name		name of the mutex.
 * 			if different modules use same mutex name,
 * 			they will share same mutex handle.
 * 			if name==NULL, then it would create a new
 * 			anonymous mutex. to avoid duplicate name,
 *			it is suggested to use UUID.
 *
 * @retval TEE_SUCCESS	Success to create the mutex.
 * @retval TEE_ERROR_*	Error code if fail to create the mutex.
 */
TEE_Result TEE_MutexCreate(void **lock, const char *name);

/** destroy mutex.
 *
 * @param lock		mutex handle.
 *
 * @retval TEE_SUCCESS	success to destroy the lock.
 * @retval TEE_ERROR_*	Error code if fail to destroy the mutex.
 */
TEE_Result TEE_MutexDestroy(void *lock);

/** lock mutex.
 *
 * @param lock		mutex handle.
 *
 * @retval TEE_SUCCESS	success to lock the mutex.
 * @retval TEE_ERROR_*	Error code if fail to lock the mutex.
 */
TEE_Result TEE_MutexLock(void *lock);

/** try to lock mutex.
 *
 * @param lock		mutex handle.
 *
 * @retval TEE_SUCCESS	success to trylock the mutex.
 * @retval TEE_ERROR_BAD_STATE	lock is owned by others.
 * @retval TEE_ERROR_*	Error code if fail to trylock the mutex.
 */
TEE_Result TEE_MutexTryLock(void *lock);

/** unlock mutex.
 *
 * @param lock		mutex handle.
 *
 * @retval TEE_SUCCESS	success to unlock the mutex.
 * @retval TEE_ERROR_*	Error code if fail to unlock the mutex.
 */
TEE_Result TEE_MutexUnlock(void *lock);


/** create semaphore.
 *
 * @param sem		return the sem handle.
 * @param value		initial value of the semaphore.
 * @param name		name of the semaphore.
 * 			if different modules use same semaphore name,
 * 			they will share same semaphore handle. and the initial
 * 			value is ignored. to avoid duplicate name, it is suggested
 *			to use UUID.
 * 			if name==NULL, then it would create a new
 * 			anonymous semaphore.
 *
 * @retval TEE_SUCCESS	Success to create the semaphore.
 * @retval TEE_ERROR_*	Error code if fail to create the semaphore.
 */
TEE_Result TEE_SemaphoreCreate(void **sem, int value, const char *name);

/** destroy semaphore.
 *
 * @param sem		semaphore handle.
 *
 * @retval TEE_SUCCESS	success to destroy the sem.
 * @retval TEE_ERROR_*	Error code if fail to destroy the semaphore.
 */
TEE_Result TEE_SemaphoreDestroy(void *sem);

/** wait semaphore, and decrease the count
 *
 * @param sem		semaphore handle.
 *
 * @retval TEE_SUCCESS	success to wait the semaphore.
 * @retval TEE_ERROR_*	Error code if fail to wait the semaphore.
 */
TEE_Result TEE_SemaphoreWait(void *sem);

/** wait semaphore until timeout.
 *
 * @param sem		semaphore handle.
 * @param timeout	timeout in milliseconds.
 *
 * @retval TEE_SUCCESS	success to wait the semaphore.
 * @retval TEE_ERROR_TIMEOUT	timeout reached.
 * @retval TEE_ERROR_*	Error code if fail to wait the semaphore.
 */
TEE_Result TEE_SemaphoreTimedWait(void *sem, uint32_t timeout);

/** try to wait semaphore.
 *
 * it's similar as TEE_SemaphoreTimedWait(sem, 0).
 *
 * @param sem		semaphore handle.
 *
 * @retval TEE_SUCCESS	success to trywait the semaphore.
 * @retval TEE_ERROR_BAD_STATE	sem is owned by others.
 * @retval TEE_ERROR_*	Error code if fail to trywait the semaphore.
 */
TEE_Result TEE_SemaphoreTryWait(void *sem);

/** post semaphore, and increase the count.
 *
 * @param sem		semaphore handle.
 *
 * @retval TEE_SUCCESS	success to post the semaphore.
 * @retval TEE_ERROR_*	Error code if fail to post the semaphore.
 */
TEE_Result TEE_SemaphorePost(void *sem);



#define APP_UNIQUE_KEY_SIZE		(16)
#define UUID_SIZE			(16)
#define APPID_SIZE			(16)

struct SecureKeyParam {
	uint8_t uuID[UUID_SIZE];
	uint8_t appID[APPID_SIZE];
	uint32_t appVariant;
	uint8_t secureKey[APP_UNIQUE_KEY_SIZE];
};

TEE_Result getSecureKey(struct SecureKeyParam* sk);
TEE_Result getSecureKey2(struct SecureKeyParam* sk);


#define MAX_ENTRY_NUM		14

struct SecurityAttribute {
	uint32_t	index;
	int32_t		sessionId;
	TEE_UUID	uuid;
	int8_t		receivedUsageRules;
	int8_t		hdcpVersion;
	int8_t		ContentStreamType;
	uint16_t	hdcpCappingResolutionWidth;
	uint16_t	hdcpCappingResolutionHeight;
	uint16_t	deviceCappingResolutionWidth;
	uint16_t	deviceCappingResolutionHeight;
	int8_t		resolutionCappingRotationAllowed:1,
			resolutionCappingAspectAllowed:1,
			deviceCappingRotationAllowed:1,
			deviceCappingAspectAllowed:1,
			reserved:4;
};

TEE_Result getSecurityAttributeEntry(struct SecurityAttribute* sessionEntry,
				uint32_t keySessionId);

TEE_Result setSecurityAttritbuteEntry(struct SecurityAttribute* sessionEntry);

uint32_t getActiveEntries(void);

TEE_Result getActiveSecurityAttributeEntry(struct SecurityAttribute*
				sessionEntry);

#endif
#endif /* TEE_API_SYNA_H */
