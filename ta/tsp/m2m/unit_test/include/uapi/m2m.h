#ifndef __UAPI_SYNA_M2M__
#define __UAPI_SYNA_M2M__

#include <stdint.h>
#include <sys/ioctl.h>

#define M2M_MAX_KEY_LENGTH    32 // 256bit key
#define M2M_MAX_KEY_ID_LENGTH 48

enum m2m_crypto_mode {
	M2M_CRYPTO_MODE_INVALID,
	M2M_CRYPTO_MODE_ENC,
	M2M_CRYPTO_MODE_DEC,
};

enum m2m_crypto_type {
	M2M_CRYPTO_TYPE_INVALID,
	M2M_CRYPTO_TYPE_AES_128_ECB,
	M2M_CRYPTO_TYPE_AES_128_CBC,
	M2M_CRYPTO_TYPE_AES_128_CTR,
	M2M_CRYPTO_TYPE_TDES_128_ECB,
	M2M_CRYPTO_TYPE_TDES_128_CBC,
	M2M_CRYPTO_TYPE_TDES_128_CTR,
	M2M_CRYPTO_TYPE_DVB_CSA_30,
	M2M_CRYPTO_TYPE_DVB_ASA,
	M2M_CRYPTO_TYPE_DES_56_ECB,
	M2M_CRYPTO_TYPE_DVB_CSA_20,
	M2M_CRYPTO_TYPE_AES_128_CTR64,
	M2M_CRYPTO_TYPE_AES_128_CTR128,
};

struct m2m_drm_info {
	uint32_t scheme_type;
	uint32_t session_id;
	uint32_t key_id_length;
	uint8_t key_id[M2M_MAX_KEY_ID_LENGTH];
};

enum m2m_key_option {
	M2M_KEY_INVALID,
	M2M_KEY_CONTENT,
	M2M_KEY_INDEX,
	M2M_KEY_FROM_DRM,
};

struct m2m_key {
	enum m2m_key_option key_option;
	uint32_t key_index;
	uint32_t key_length;
	enum m2m_key_option iv_option;
	uint32_t iv_index;
	uint32_t iv_length;
	struct m2m_drm_info drm_info;
	uint8_t key_data[M2M_MAX_KEY_LENGTH];
	uint8_t iv_data[M2M_MAX_KEY_LENGTH];
};

struct m2m_pattern_mode {
	uint8_t pattern_enc;
	uint8_t pattern_clr;
};

enum m2m_residue_mode {
	M2M_RESIDUE_MODE_DEFAULT,
	M2M_RESIDUE_MODE_PATTERN,
	M2M_RESIDUE_MODE_EXOR,
	M2M_RESIDUE_MODE_RP,
	M2M_RESIDUE_MODE_CTS,
};

enum m2m_iv_policy {
	M2M_IV_POLICY_DEFAULT,
	M2M_IV_POLICY_RESET,
	M2M_IV_POLICY_MAX
};

struct m2m_buf {
	int fd;
	uint32_t offset;
	uint32_t size;
};

struct m2m_mem {
	struct m2m_buf inbuf;
	struct m2m_buf outbuf;
};

#define M2M_IOC_SET_MODE		_IOW('m', 1, enum m2m_crypto_mode)
#define M2M_IOC_SET_SCHEME		_IOW('m', 2, enum m2m_crypto_type)
#define M2M_IOC_SET_KEY			_IOW('m', 3, struct m2m_key)
#define M2M_IOC_SET_PATTERN_MODE	_IOW('m', 4, struct m2m_pattern_mode)
#define M2M_IOC_SET_RESIDUE_MODE	_IOW('m', 5, enum m2m_residue_mode)
#define M2M_IOC_UPDATE			_IOW('m', 6, struct m2m_mem)
#define M2M_IOC_QUERY			_IOR('m', 7, uint32_t)

#endif
