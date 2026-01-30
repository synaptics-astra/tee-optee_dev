#ifndef __DRMSE_CMD_H__
#define __DRMSE_CMD_H__

/* uuid defined for drm secure engine wrapper ta */
#define DRM_SE_UUID {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x03, 0xfa}}

//Command Masks
#define COMMAND_MASK                                0xF000
#define IMAGE_LOADING_MASK                          0x0000
#define KEYTABLE_MGR_MASK                           0x1000
#define RAMTORAM_MASK                               0x2000
#define KEY_MGR_MASK                                0x3000
#define DATA_HASH_MASK                              0xD000
#define OTP_MASK                                    0xE000
#define HDCP_MASK                                   0xF000

//Image loading
#define DRM_SE_WRAPPER_LOADARMIMG                   0x0001
#define DRM_SE_WRAPPER_LOADLIB                      0x0002

//Key Table Management Command
#define DRM_SE_KEYTABLE_ALLOC                       0x1001
#define DRM_SE_KEYTABLE_FREE                        0x1002
#define DRM_SE_KEYRIGHT_CONTENTPROTECT_SET          0x1003
#define DRM_SE_KEYRIGHT_VALIDATE_KEYUSER            0x1005
#define DRM_SE_KEYTABLE_ENTRY_ATTRIBUTE_GET         0x1006
#define DRM_SE_CLIENT_TA_LOADKEYS                   0x1007
#define DRM_SE_KEYRIGHT_COPYPROTECTION_SET          0x1008

//RAM TO RAM functions
#define DRM_SE_WRAPPER_R2R                          0x2001

//Key Manangement Command
//Key management about session
#define DRM_SE_KEYMGR_GETSESSION                    0x3001
#define DRM_SE_KEYMGR_SEARCHSESSION                 0x3002
#define DRM_SE_KEYMGR_CLEARSESSION                  0x3003
//Key management about content key set
#define DRM_SE_KEYMGR_CONTENTKEY_CREATE             0x3004
#define DRM_SE_KEYMGR_CONTENTKEY_SETATTRIBUTE       0x3005
#define DRM_SE_KEYMGR_CONTENTKEY_DELETE             0x3006
//Key management about content key unit
#define DRM_SE_KEYMGR_CONTENTKEYUNIT_CREATE         0x3007
#define DRM_SE_KEYMGR_CONTENTKEYUNIT_SETATTRIBUTE   0x3008
#define DRM_SE_KEYMGR_CONTENTKEYUNIT_DELETE         0x3009
#define DRM_SE_KEYMGR_CONTENTKEYUNIT_GETINFO        0x300A
#define DRM_SE_KEYMGR_CONTENTKEYUNIT_GETHANDLE      0x300B

#define DRM_SE_KEYMGR_CONTENTKEYUNIT_CREATE_EXT     0x300C
#define DRM_SE_KEYMGR_CONTENTKEYUNIT_DELETE_EXT     0x300D
#define DRM_SE_KEYMGR_CONTENTKEYUNIT_COUNT          0x300E
#define DRM_SE_KEYMGR_SET_CONTENTKEYUNIT_ATTRI_EXT  0x300F
#define DRM_SE_KEYMGR_GET_CONTENTKEYUNIT_ATTRI_EXT  0x3010
#define DRM_SE_KEYMGR_CONTENTKEYUNIT_DELETE_ALL     0x3011
#define DRM_SE_KEYMGR_GENERATE_AESK0_KEYS           0x3012
#define DRM_SE_KEYMGR_GET_SECURE_KEYS               0x3013
#define DRM_SE_KEYMGR_GET_RANDOM_DATA               0x3014
#define DRM_SE_KEYMGR_GET_CUSTBLOB_CLEAR_KEY        0x3015

//OTP property get
#define DRM_SE_GET_OTP_PROPERTY   0xE001
#define DRM_SE_GET_OTP_RAWDATA   0xE002

///Load HDCP Key Command
#define DRM_SE_HDCP_LOAD1XKEY                       0xF001
#define DRM_SE_HDCP_LOAD2XLC                        0xF002
#endif
