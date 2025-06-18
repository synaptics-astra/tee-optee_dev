/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


////////////////////////////////////////////////////////////////////////////////
//! \file OSAL_config.h
//! \brief
//!
//!
//! Purpose:
//!
//!
//!	Version, Date and Author :
//!		V 1.00,	6/18/2007,	Fang Bao
//!		V 1.01,	7/22/2007,	Fang Bao,	Modify the MV_CC_RPC_PARAM_BYTESIZE for RPC TRACE test
//!		V 1.02,	9/03/2007,	Fang Bao,	Add MV_OSAL_PROCESS_SELF and Task Name Strings (such as MV_NAME_RPCServerTask)
//!		V 1.03,	9/09/2007,	Fang Bao,	Add MsgQ_Msg_Flags and MsgQ_Msg_DataLen
//!		V 1.04,	10/22/2008,	Fang Bao,	Add MV_CC_RPC_SHMPARAMETER_BYTESIZE for long parameter feature
//!		V 1.05,	10/24/2008,	Fang Bao,	Bugfix - replace MV_CC_RPC_PARAM_BYTESIZE by MV_CC_RPC_MSGSPACE_BYTESIZE (MV_CC_RPC_MSGSPACE_BYTESIZE is the space of RPC message)
//!									 	and MV_CC_RPC_PARAM_BYTESIZE is set value of 255
//!		V 1.05,	10/24/2008,	Fang Bao,	Bugfix - MV_CC_RPC_PARAM_BYTESIZE is set value of 252 because of Alignment trap error
//!		V 1.06,	2/04/2009,	Fang Bao,	Add MV_OSAL_SEM_VALUE_MAX for parameter checking
//!		V 1.07,	2/18/2009,	Fang Bao,	Add MV_CC_MSGQ_DATA_MSGEXLEN_MAX
//!										use MV_CC_ICCFIFO_FullSize to compute the size of ICCFIFO Size
//!		V 1.08, 10/23/2009,	Fang Bao,	Add CCDSS, CCDSP support
//!		V 1.09, 11/09/2009,	Fang Bao,	Bugfix - High Priority(RR) RPC Server Task maybe hang in Linux because of io operaion or high large overhead in user function
//!									 	set MV_CC_RPC_SERVER_PRIORITY with MV_OSAL_TASK_PRIORITY_DEFAULT in Linux OS, it maybe delay a little response speed
//!		V 1.10,	11/25/2009,	Fang Bao	Add MV_CC_UDP_SEND_TRYTIMES, which is MV_CC_UDP_SendTo try times when CC FIFO is full.
//!		V 1.11,	02/22/2010,	Fang Bao	Add MV_CC_MIDSRV_TASK_PRIORITY and MV_TIMER_TASK_PRIORITY_ANDROID for Android OS
//!     V 1.12, 06/10/2010, Fang Bao,   add CC_DEVICE_CMD_UPDATE, CC_DEVICE_CMD_CREATE_CBUF, CC_DEVICE_CMD_DESTROY_CBUF
//!		V 1.13,	09/15/2010,	Fang Bao,   move CC_RPC_CMD List from SHM_Server.c
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef _OSAL_CONFIG_H_
#define _OSAL_CONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif	// #ifdef __cplusplus


////////////////////////////////////////////////////////////////////////////////
//! \brief Definition: Semphore definition
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_OSAL_SEM_VALUE_MAX				((UINT)INT_MAX)		// The initial count for the semaphore object. This value must be greater than or equal to zero and less than or equal to MV_OSAL_SEM_VALUE_MAX
															// for 32-bit compilers, INT_MAX equals 0x7FFFFFFF (2,147,483,647)

////////////////////////////////////////////////////////////////////////////////
//! \brief Definition: CPU definition
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_OSAL_CPU_NUM						(2)
#define MV_OSAL_CPU_0						(0)
#define MV_OSAL_CPU_1						(1)
#define MV_OSAL_CPU_2						(2)

//------------------------------------------------------------------------------
//! \brief CPU definition when OS = Linux
#if defined( __LINUX__ )

#define MV_OSAL_LOGICAL_CPU0				(MV_OSAL_CPU_0)
#define MV_OSAL_LOGICAL_CPU1				(MV_OSAL_CPU_1)
	#if defined( SoC_PCSim )
		#define MV_OSAL_CPU_MASTER			(MV_OSAL_CPU_0)
	#else
		#define MV_OSAL_CPU_MASTER			(MV_OSAL_CPU_1)
	#endif	// #if defined( SoC_PCSim )
#define MV_OSAL_GFX_CPU						(MV_OSAL_CPU_1)
#define MV_CC_DSS_SERVER_CPU_ID				(MV_OSAL_CPU_1)
#define MV_SHM_SERVER_CPU_ID				(MV_OSAL_CPU_1)

//------------------------------------------------------------------------------
//! \brief CPU definition when OS = eCos
#elif defined( __ECOS__ )

#define MV_OSAL_LOGICAL_CPU0				(MV_OSAL_CPU_0)
#define MV_OSAL_LOGICAL_CPU1				(MV_OSAL_CPU_1)
#define MV_OSAL_CPU_MASTER					(MV_OSAL_CPU_1)
#define MV_OSAL_GFX_CPU						(MV_OSAL_CPU_1)
#define MV_CC_DSS_SERVER_CPU_ID				(MV_OSAL_CPU_1)
#define MV_SHM_SERVER_CPU_ID				(MV_OSAL_CPU_1)

//------------------------------------------------------------------------------
//! \brief CPU definition when OS = Windows
#elif defined( WIN32 )

#define MV_OSAL_LOGICAL_CPU0				(MV_OSAL_CPU_0)
#define MV_OSAL_LOGICAL_CPU1				(MV_OSAL_CPU_0)
#define MV_OSAL_CPU_MASTER					(MV_OSAL_CPU_0)
#define MV_OSAL_GFX_CPU						(MV_OSAL_CPU_0)
#define MV_CC_DSS_SERVER_CPU_ID				(MV_OSAL_CPU_0)
#define MV_SHM_SERVER_CPU_ID				(MV_OSAL_CPU_0)

//------------------------------------------------------------------------------
//! \brief CPU definition when OS = Others (Linux Kernel or UBoot)
#else

#define MV_OSAL_LOGICAL_CPU0				(MV_OSAL_CPU_0)
#define MV_OSAL_LOGICAL_CPU1				(MV_OSAL_CPU_1)
#define MV_OSAL_CPU_MASTER					(MV_OSAL_CPU_1)
#define MV_OSAL_GFX_CPU						(MV_OSAL_CPU_1)
#define MV_CC_DSS_SERVER_CPU_ID				(MV_OSAL_CPU_1)
#define MV_SHM_SERVER_CPU_ID				(MV_OSAL_CPU_1)

#endif


////////////////////////////////////////////////////////////////////////////////
//! \brief Process definition
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_OSAL_PROCESS_DEFAULT				(0)
#define MV_OSAL_PROCESS_BASE_IN_LINUX		(2000)

#ifndef MV_OSAL_PROCESS_SELF
	#define MV_OSAL_PROCESS_SELF			(MV_OSAL_PROCESS_DEFAULT)
#else
	#if defined( __ECOS__ )
		#define MV_OSAL_PROCESS_SELF		(MV_OSAL_PROCESS_DEFAULT)
	#endif
#endif

#define MV_CC_PROCESS_ID_START				(0)
#define MV_CC_PROCESS_ID_RESERVED			(0)
#define MV_CC_PROCESS_ID_BROADCAST			(0)

#define MV_CC_PROCESS_ID_MIN				(MV_CC_PROCESS_ID_RESERVED + 1)
#define MV_CC_PROCESS_ID_MAX				(256)


//////////////////////////////////////////////////////////////////////////////////
////! \brief Hardware definition
////!
//////////////////////////////////////////////////////////////////////////////////
//#define MV_CC_HAL_MEMBOUNDARY				(64)			// Hardware Memroy Boundary Size (DMA operation)
//
////------------------------------------------------------------------------------
////! \brief Memory definition for Shared Memory
//#define MV_NONCACHE_Base					(0x40000000)
//#define MV_NONCACHE_Start					(0x5C000000)
//#define MV_NONCACHE_Size					(0x04000000)				// 64M Size for ICC FIFO, ICC EXTRA and Shared Memory
//#define MV_ICC_FIFO_ADDR_def				(MV_NONCACHE_Start)
//#define MV_ICC_FIFO_SIZE_def				(0x100000)					// 1M data for ICC FIFO
//#define MV_ICC_EXTRA_ADDR_def				(MV_NONCACHE_Start + MV_ICC_FIFO_SIZE_def)
//#define MV_ICC_EXTRA_SIZE_def				(0x1000)					// 4K extra data for ICC
//#define MV_ICC_FIFO_ADDR_def_KernVirt		(0xFA000000)													// Linux kernel Virtual Address of 0x5C000000
//#define MV_ICC_EXTRA_ADDR_def_KernVirt		(MV_ICC_FIFO_ADDR_def_KernVirt + MV_ICC_FIFO_SIZE_def)			// Linux kernel Virtual Address
#define MV_ICC_SHMPOOL_ADDR					(MV_SHM_Start_PhysAddr)
#define MV_ICC_SHMPOOL_SIZE					(MV_SHM_FullSize)
#define MV_ICC_FIFO_ADDR					(MV_ICC_FIFO_ADDR_def)
#define MV_ICC_FIFO_SIZE					(MV_ICC_FIFO_SIZE_def)

//////////////////////////////////////////////////////////////////////////////////
////! \brief Linux CC Device definition
////!
//////////////////////////////////////////////////////////////////////////////////
#define CMD_ICCNET_PID_MAP					(0x1E01)
#define CMD_ICCNET_PID_UNMAP				(0x1E02)
#define CMD_ICCNET_DSS_REG					(0x1E03)
#define CMD_ICCNET_DSS_FREE					(0x1E04)
#define CMD_ICCNET_DSS_INQUIRY				(0x1E05)
#define CMD_ICCNET_DSS_GETLIST				(0x1E06)


////////////////////////////////////////////////////////////////////////////////
//! \brief ICCFIFO and ICCFIFOArray definition
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_CC_ICCFIFO_FRAME_SIZE			(128)				// Fixed Size(Byte) for the every Frame in the ICCFIFO
#define MV_CC_ICCFIFO_NMAX					(63)				// Max number Frame in the ICCFIFO buffer except FIFO head
#define MV_CC_ICCFIFO_FullSize(x,y)			(MV_CC_HAL_MEMBOUNDARY + (x) * (y) )
#define MV_CC_ICCFIFO_FULL_SIZE				(MV_CC_ICCFIFO_FullSize(MV_CC_ICCFIFO_NMAX, MV_CC_ICCFIFO_FRAME_SIZE))  	// Byte Size of the ICCFIFO include the FIFO head
#define MV_CC_ICCFIFO_ARRAY_SIZE			(MV_CC_ICCFIFO_FULL_SIZE * MV_OSAL_CPU_NUM * MV_OSAL_CPU_NUM)				// ICCFIFO Array Memory Size
#define MV_CC_ICCFIFO_FrameNMin				(2)


////////////////////////////////////////////////////////////////////////////////
//! \brief CC - ICCP Message definition
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_CC_ICCP_HEAD_BYTEOFFSET			(0)
#define MV_CC_ICCP_HEAD_BYTESIZE			(12)
#define MV_CC_ICCP_DATA_BYTEOFFSET			(MV_CC_ICCP_HEAD_BYTEOFFSET + MV_CC_ICCP_HEAD_BYTESIZE)


////////////////////////////////////////////////////////////////////////////////
//! \brief CC - UDP definition
//!
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
//! \brief CC - UDP Message
#ifdef BERLIN_SINGLE_CPU

#define MV_CC_CCUDP_HEAD_BYTEOFFSET			(0)
#define MV_CC_CCUDP_HEAD_BYTESIZE			(0)

#else

#define MV_CC_CCUDP_HEAD_BYTEOFFSET			(MV_CC_ICCP_DATA_BYTEOFFSET)
#define MV_CC_CCUDP_HEAD_BYTESIZE			(8)

#endif

#define MV_CC_CCUDP_DATA_BYTEOFFSET			(MV_CC_CCUDP_HEAD_BYTEOFFSET + MV_CC_CCUDP_HEAD_BYTESIZE)
#define MV_CC_CCUDP_DATA_BYTESIZE			(MV_CC_ICCFIFO_FRAME_SIZE - MV_CC_CCUDP_DATA_BYTEOFFSET)


//------------------------------------------------------------------------------
//! \brief CC - UDP Port
#define MV_CC_UDP_PORT_MAX					(MV_OSAL_PortID_U16_t)(512)
#define MV_CC_UDP_PORT_DEFAULT				(MV_OSAL_PortID_U16_t)(0)		// Default port will no receiver
#define MV_CC_UDP_PORT_SYSTEM_START			(MV_OSAL_PortID_U16_t)(1)
#define MV_CC_UDP_PORT_WELLKNOWN_START		(MV_OSAL_PortID_U16_t)(16)
#define MV_CC_UDP_PORT_EPHEMERAL_START		(MV_OSAL_PortID_U16_t)(128)

//------------------------------------------------------------------------------
//! \brief CC - UDP Send
#define MV_CC_UDP_SEND_TRYTIMES				(100)

////////////////////////////////////////////////////////////////////////////////
//! \brief CC - DSP definition
//!
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
//! \brief CC - DSP Message
#define MV_CC_CCDSP_HEAD_BYTEOFFSET			(MV_CC_ICCP_DATA_BYTEOFFSET)
#define MV_CC_CCDSP_HEAD_BYTESIZE			(4)
#define MV_CC_CCDSP_DATA_BYTEOFFSET			(MV_CC_CCDSP_HEAD_BYTEOFFSET + MV_CC_CCDSP_HEAD_BYTESIZE)
#define MV_CC_CCDSP_DATA_BYTESIZE			(MV_CC_ICCFIFO_FRAME_SIZE - MV_CC_CCDSP_DATA_BYTEOFFSET)

//------------------------------------------------------------------------------
//! \brief CC - DSP Command
#define CMD_DSS_AskUpdate   				(0x01)
#define CMD_DSS_UpdateOK    				(0x02)
#define CMD_DSS_Add         				(0x03)
#define CMD_DSS_Delete     					(0x04)
#define CMD_DSS_UpdateError    				(0x05)

////////////////////////////////////////////////////////////////////////////////
//! \brief CC - RPC Message definition
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_CC_RPC_HEAD_BYTEOFFSET			(MV_CC_CCUDP_DATA_BYTEOFFSET)
#define MV_CC_RPC_HEAD_BYTESIZE				(8)
#define MV_CC_RPC_DATA_BYTEOFFSET			(MV_CC_RPC_HEAD_BYTEOFFSET + MV_CC_RPC_HEAD_BYTESIZE)
#define MV_CC_RPC_DATA_BYTESIZE				(MV_CC_ICCFIFO_FRAME_SIZE - MV_CC_RPC_DATA_BYTEOFFSET)
#define MV_CC_RPC_SHMPARAMETER_BYTESIZE		(8)
#define MV_CC_RPC_PARAM_BYTESIZE			(252)

#ifdef ENABLE_TEST_RPC_TRACE
	#define MV_CC_RPC_MSGSPACE_BYTESIZE			(MV_CC_RPC_DATA_BYTESIZE - LOGTYPE_TEST_RPC_TRACE_Num * LOGTYPE_TEST_RPC_TRACE_PointSize)
#else
	#define MV_CC_RPC_MSGSPACE_BYTESIZE			(MV_CC_RPC_DATA_BYTESIZE)
#endif


//------------------------------------------------------------------------------
//! \brief TaskPool of PRC
#define MV_CC_RPC_TaskPool_TaskNMAX			(16)


////////////////////////////////////////////////////////////////////////////////
//! \brief CC - SHM definition
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_SHM_DeviceName					"/dev/MV_CC_SHM0"
#define MV_SHM_DeviceName_CACHE				"/dev/MV_CC_SHM1"
#define MV_SHM_DeviceName_ICC				"/dev/MV_CC_ICC"
#define MV_SHM_Start_PhysAddr				(MV_ICC_EXTRA_ADDR_def + MV_ICC_EXTRA_SIZE_def)			// Shared Memory Start Address
#define MV_SHM_Start_PhysAddr_CACHE			(MV_SHM_Start_PhysAddr - MV_NONCACHE_Base) 				// Shared Memory Start Address in Cache memory
#define MV_SHM_FullSize						(MV_NONCACHE_Size - MV_ICC_FIFO_SIZE_def - MV_ICC_EXTRA_SIZE_def)
#define MV_CC_SHM_KEY_DEFAULT				(MV_CC_SHM_Key_t)(0xFFFFFFFF)   //[Bob/5/21/2011]: change default key to invalid both for BG1 and BG2
#define MV_CC_SHM_MAX_ALLOC					(0x400)
#define MV_HW_SHM_Start 					(MV_CC_SHM_Key_t)(3001)			// It is only used in Win32
#define MV_HW_SHM_Start_CPU0 				(MV_CC_SHM_Key_t)(3001)			// It is only used in PC Linux
#define MV_HW_SHM_Start_CPU1 				(MV_CC_SHM_Key_t)(0x01000BB9)	// It is only used in PC Linux


////////////////////////////////////////////////////////////////////////////////
//! \brief CC - Circular Buffer definition
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_CC_CBUF_FLAGS_DEFAULT_SETTING	(MV_CC_CBUF_FLAGS_ALIGNMENT)
#define MV_CC_CBuf_NoticeMsgQ_NMax			(20)


////////////////////////////////////////////////////////////////////////////////
//! \brief CC - Message Queue definition
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_CC_ICC_NMsgsMin					(MV_CC_ICCFIFO_FrameNMin)
#define MV_CC_ITC_NMsgsMin					(MV_CC_ICCFIFO_FrameNMin)

#define MV_CC_MSGQ_HEAD_BYTEOFFSET			(MV_CC_CCUDP_DATA_BYTEOFFSET)
#define MV_CC_MSGQ_HEAD_BYTESIZE			(2)
#define MV_CC_MSGQ_DATA_BYTEOFFSET			(MV_CC_MSGQ_HEAD_BYTEOFFSET + MV_CC_MSGQ_HEAD_BYTESIZE)
#define MV_CC_MSGQ_DATA_BYTESIZE			(MV_CC_ICCFIFO_FRAME_SIZE - MV_CC_MSGQ_DATA_BYTEOFFSET)
#define MV_CC_MSGQ_DATA_BYTESIZE_MAX		(MV_CC_ICCFIFO_FRAME_SIZE - MV_CC_MSGQ_DATA_BYTEOFFSET)
#define MV_CC_MSGQ_DATA_MSGEXLEN_MAX		(MV_CC_MSGQ_DATA_BYTESIZE_MAX - 4)

#define MsgQ_Msg_Flags(x)					(((UINT8 *)(x))[(MV_CC_MSGQ_HEAD_BYTEOFFSET) + 0])
#define MsgQ_Msg_DataLen(x)					(((UINT8 *)(x))[(MV_CC_MSGQ_HEAD_BYTEOFFSET) + 1])

////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL - Task definition
//!
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
//! \brief Task - priority
#ifndef ANDROID
        #define MV_CC_MIDSRV_TASK_PRIORITY			(MV_OSAL_TASK_PRIORITY_15)
#else
        #define MV_CC_MIDSRV_TASK_PRIORITY			(MV_OSAL_TASK_PRIORITY_DEFAULT)
#endif

#if defined( __ECOS__ )
	#define MV_CC_RPC_SERVER_PRIORITY			(MV_OSAL_TASK_PRIORITY_15)
#elif defined( ANDROID )
	#define MV_CC_RPC_SERVER_PRIORITY			(MV_OSAL_TASK_PRIORITY_DEFAULT)
#else
	#define MV_CC_RPC_SERVER_PRIORITY			(MV_OSAL_TASK_PRIORITY_30)
#endif

#define MV_TIMER_TASK_PRIORITY_LINUX		(MV_OSAL_TASK_PRIORITY_29)			// It is higher than the default task priority
#define MV_TIMER_TASK_PRIORITY_ECOS			(MV_OSAL_TASK_PRIORITY_29)			// It is higher than the default task priority
#define MV_TIMER_TASK_PRIORITY_ANDROID		(MV_OSAL_TASK_PRIORITY_DEFAULT)			// It is a common task in Android
#define	MV_GFXI_TASK_PRIORITY_CPU1			(MV_OSAL_TASK_PRIORITY_4)
#define	MV_GFXM_TASK_PRIORITY_CPU1			(MV_OSAL_TASK_PRIORITY_20)
#define	MV_CC_DSS_UPDATE_TASK_PRIORITY		(MV_OSAL_TASK_PRIORITY_15)


#define MV_NAME_RPCServerTask				"RPCSrv Task"
#define MV_NAME_RPCNetServerTask				"NET RPCSrv Task"
#define MV_NAME_TimerTask					"Timer Task"
#define MV_NAME_MidSrvTask					"MidSrv Task"
#define MV_NAME_GFXITask					"GFXI Task"
#define MV_NAME_DSSUPDATETASK				"DSS Update Task"

//#define	MV_CC_DSS_UPDATE_TASK_MSGQDEPTH		(10)

// the CC Status Register is the Data Register of CPU1->CPU0
#define MV_CC_STATUS_SrcCPU						MV_OSAL_CPU_1
#define MV_CC_STATUS_DstCPU						MV_OSAL_CPU_0

//------------------------------------------------------------------------------
//! \brief Task - message queue
#define MV_OSAL_Task_NMsgsMin				(1)


//------------------------------------------------------------------------------
//! \brief CC
#define NETLINK_GALOIS_CC_SINGLECPU         (29)        /* Galois CC module transports by netlink*/
#define NETLINK_GALOIS_CC_GROUP_SINGLECPU 	(0)         /* Galois CC module transports by netlink*/
#define DSS_DEVICE_PATH                     "/dev/galois_cc"

#define CC_DEVICE_NAME                      "galois_cc"
#define CC_DEVICE_TAG                       "[Galois][cc_driver] "
#define CC_DEVICE_PATH                      ("/dev/" CC_DEVICE_NAME)

#define CC_DEVICE_PROCFILE_STATUS           "status"
#define CC_DEVICE_PROCFILE_DETAIL           "detail"

#define CC_DEVICE_LIST_NUM                  (100)

// CC DSS IO Command List
#define CC_DEVICE_CMD_REG                  (0x1E01)
#define CC_DEVICE_CMD_FREE                 (0x1E02)
#define CC_DEVICE_CMD_INQUIRY              (0x1E03)
#define CC_DEVICE_CMD_GET_LIST             (0x1E04)
#define CC_DEVICE_CMD_GET_STATUS           (0x1E05)
#define CC_DEVICE_CMD_TEST_MSG             (0x1E06)
#define CC_DEVICE_CMD_UPDATE               (0x1E07)
#define CC_DEVICE_CMD_CREATE_CBUF          (0x1E08)
#define CC_DEVICE_CMD_DESTROY_CBUF         (0x1E09)

//////////////////////////////////////////////////////////////////////////////////
////! \brief  SHM RPC Command definition
////!
//////////////////////////////////////////////////////////////////////////////////
#define CC_RPC_CMD_SHM_MALLOC              (0x0001)
#define CC_RPC_CMD_SHM_GET                 (0x0002)
#define CC_RPC_CMD_SHM_FREE                (0x0003)
#define CC_RPC_CMD_SHM_FREEKEY             (0x0004)
#define CC_RPC_CMD_SHM_GETINFO             (0x0005)
#define CC_RPC_CMD_SHM_MALLOC_CACHE        (0x0006)
#define CC_RPC_CMD_SHM_GET_CACHE           (0x0007)
#define CC_RPC_CMD_SHM_FREE_CACHE          (0x0008)
#define CC_RPC_CMD_SHM_FREEKEY_CACHE       (0x0009)
#define CC_RPC_CMD_END                     (0xFFFF)

#ifdef __cplusplus
}		// extern "C"
#endif	// #ifdef __cplusplus

#endif	// #ifdef _OSAL_CONFIG_H_
