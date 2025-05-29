/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_bcm_bd.h
//! \brief      The BCM dump framework header file
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#ifndef _VPP_BCM_BD_H_
#define _VPP_BCM_BD_H_

#include "com_type.h"
#include "vpp_osal.h"

#ifdef VPP_DIAGS
//Let us handle interrupt disable/enable for mutex operation at application by commenting the below macro
//#define ENABLE_NON_OS_MUTEX
#endif

#ifdef ENABLE_NON_OS_MUTEX
#include "diag_gic.h"
#endif

//Enable or Disable the BCM BUFFER DUMP feature
//#define VPP_BCM_BUFFER_DUMP

//Enable or Disable debug prints
//#define VPP_BCM_BD_DBG_PRINT_ENABLE

#define VPP_BCM_DB_BUFF_BCM       0
#define VPP_BCM_DB_BUFF_CFGQ      1

//Configuration macros
#define VPP_BCM_DB_MAX_SIZE   30000

//Enable/Disable waiting for mutex in interrupt context
#define VPP_BCM_DB_MUTEX_WAIT_ENABLE
//The waiting for mutex should have following timeout
#define VPP_BCM_DB_MUTEX_TIEMOUT    100


//Interfacing function macros
#if defined(VPP_BCM_BUFFER_DUMP)
#define VPP_BCM_BD_INIT()                      VPP_BCM_BD_init()
#define VPP_BCM_BD_RESET(VPP_BCM_DB_BUFF)      VPP_BCM_BD_reset(VPP_BCM_DB_BUFF==VPP_BCM_DB_BUFF_CFGQ?&cfgq_buff:&bcm_buff)
#define VPP_BCM_BD_DISP(VPP_BCM_DB_BUFF)      VPP_BCM_BD_print(VPP_BCM_DB_BUFF==VPP_BCM_DB_BUFF_CFGQ?&cfgq_buff:&bcm_buff)
#define VPP_BCM_BD_DISP_PTR(VPP_BCM_DB_BUFF, PRINT_BUFF, PRINT_SIZE)      VPP_BCM_BD_PrintPtr(VPP_BCM_DB_BUFF==VPP_BCM_DB_BUFF_CFGQ?&cfgq_buff:&bcm_buff, PRINT_BUFF, PRINT_SIZE)
#define VPP_BCM_BD_SET_PRINTSTYLE(opt)          VPP_BCM_BD_SetPrintStyle(opt.buffer_id==VPP_BCM_DB_BUFF_CFGQ?&cfgq_buff:&bcm_buff, opt)
#define VPP_BCM_BD_SET_CONFIG(opt)              VPP_BCM_BD_SetConfig(opt.buffer_id==VPP_BCM_DB_BUFF_CFGQ?&cfgq_buff:&bcm_buff, opt)
#define VPP_BCM_BD_SET_PREFIX(opt)              VPP_BCM_BD_SetPrefix(opt.buffer_id==VPP_BCM_DB_BUFF_CFGQ?&cfgq_buff:&bcm_buff, opt)
#define VPP_BCM_BD_SHOW_SETTINGS(opt)           VPP_BCM_BD_show_settings(opt.buffer_id==VPP_BCM_DB_BUFF_CFGQ?&cfgq_buff:&bcm_buff)

#define VPP_BCM_BD_INTR_START()     VPP_BCM_BD_intrStart()
#define VPP_BCM_BD_RESET_BCM()      VPP_BCM_BD_reset(&bcm_buff)
#define VPP_BCM_BD_PRINT_BCM()      VPP_BCM_BD_print(&bcm_buff)
#define VPP_BCM_BD_ADD_BCM(reg,val) VPP_BCM_BD_add(reg, val, &bcm_buff)

#define VPP_BCM_BD_RESET_CFGQ()         VPP_BCM_BD_reset(&cfgq_buff)
#define VPP_BCM_BD_PRINT_CFGQ()         VPP_BCM_BD_print(&cfgq_buff)
#define VPP_BCM_BD_ADD_CFGQ(bcmBuff,byteSize)    VPP_BCM_BD_add_buffer(bcmBuff,byteSize)

//Mutex Macros - used inside VPP BCM BD module only
#ifdef ENABLE_NON_OS_MUTEX
//mutex=1 ==> locked, mutex=0 ==> unlocked
#define VPP_BCM_DB_MUTEX_CHECK_RETURN(ptr)    do { if(ptr->mutex) return; } while(0)

//#define VPP_BCM_DB_MUTEX_WAIT_LOCK(ptr)       do { int t=VPP_BCM_DB_MUTEX_TIEMOUT; while( ptr->mutex && t) { t--; } ptr->mutex = 1;} while(0)
//#define VPP_BCM_DB_MUTEX_LOCK(ptr)            ptr->mutex = 1
//#define VPP_BCM_DB_MUTEX_UNLOCK(ptr)          ptr->mutex = 0;

#define VPP_BCM_DB_MUTEX_WAIT_LOCK(ptr)     VPP_BCM_DB_MUTEX_LOCK(ptr)
#define VPP_BCM_DB_MUTEX_LOCK(ptr)          { ptr->irq_save = DisableIRQ();  ptr->fiq_save = DisableFIQ(); spin_lock(&(ptr->mutex)); }
#define VPP_BCM_DB_MUTEX_UNLOCK(ptr)        { spin_unlock(&(ptr->mutex)); RestoreFIQ(ptr->fiq_save); RestoreIRQ(ptr->irq_save); }

#else
#define VPP_BCM_DB_MUTEX_WAIT_LOCK(ptr)     MV_OSAL_Mutex_Lock(ptr->mutex);
#define VPP_BCM_DB_MUTEX_LOCK(ptr)          MV_OSAL_Mutex_Lock(ptr->mutex);
#define VPP_BCM_DB_MUTEX_UNLOCK(ptr)        MV_OSAL_Mutex_Unlock(ptr->mutex);
#endif

#define VPP_BCM_BD_PRINT(...)           printf(__VA_ARGS__)

#else //!VPP_BCM_BUFFER_DUMP

#define VPP_BCM_BD_INIT()
#define VPP_BCM_BD_RESET(VPP_BCM_DB_BUFF)
#define VPP_BCM_BD_DISP(VPP_BCM_DB_BUFF)
#define VPP_BCM_BD_DISP_PTR(VPP_BCM_DB_BUFF, PRINT_BUFF, PRINT_SIZE)

#define VPP_BCM_BD_INTR_START()

#define VPP_BCM_BD_RESET_BCM()
#define VPP_BCM_BD_PRINT_BCM()
#define VPP_BCM_BD_ADD_BCM(reg,val)

#define VPP_BCM_BD_RESET_CFGQ()
#define VPP_BCM_BD_PRINT_CFGQ()
#define VPP_BCM_BD_ADD_CFGQ(bcmBuff,byteSize)

#define VPP_BCM_BD_PRINT(...)
#endif //!VPP_BCM_BUFFER_DUMP

#ifdef VPP_BCM_BD_DBG_PRINT_ENABLE
#define VPP_BCM_BD_DBG_PRINT(...)       printf(__VA_ARGS__)
#else
#define VPP_BCM_BD_DBG_PRINT(...)
#endif


//Data Structures
typedef struct _tag_vpp_bcm_reg_dump_table {
    UINT32 val;
    UINT32 reg;
} VPP_BCM_REG_INFO;

typedef struct _tag_vpp_bcm_reg_dump_print_style {
    UINT32 hexdump;         //1:hexdump, 0:reg_val pair
    UINT32 entryPerLine;    //0:8 entry perline, n:n entry perline
    UINT32 flags;           //bit0=> 0:no swap, 1: swap reg & val
                            //bit1=> 0:no break, 1: break at interrupt pattern

    //local variables
    UINT32 count;           //for display control purpose-print\n
    UINT32 countp;          //for actual display purpose
    UINT32 reg,val;
}VPP_BCM_BD_PRINT_STYLE;

typedef struct _tag_vpp_bcm_reg_dump_config {
    UINT32 start;       //1:start log, 0:stop log
    UINT32 nIntr;       //0:log infinitly, n:log for n interrupts
    UINT32 enablePrefix; //0:NoPrefix, 0x01:prefix interupt, 0x11:prefix CFGQ
    UINT32 prefixPattern_intr; //pattern to be used for interrupt
    UINT32 prefixPattern_cfgq; //pattern to be used for CFGQ
}VPP_BCM_BD_CONFIG;

typedef struct _tag_vpp_bcm_reg_dump_ptr {
    int write_ptr;
    int read_ptr;
    int count;
#ifdef ENABLE_NON_OS_MUTEX
    int mutex;
    int irq_save, fiq_save;
#else
    MV_OSAL_HANDLE_MUTEX_t mutex;
#endif
    VPP_BCM_BD_CONFIG config;
    VPP_BCM_BD_PRINT_STYLE prnStyle;
    VPP_BCM_REG_INFO buff[VPP_BCM_DB_MAX_SIZE];
}VPP_BCM_REG_PTR;

typedef enum _tag_VPP_BCM_BD_PREFIX_MASK_ {
    VPP_BCM_BD_PREFIX_MASK_INTR = 0x01,
    VPP_BCM_BD_PREFIX_MASK_CFGQ = 0x10
}VPP_BCM_BD_PREFIX_MASK;

typedef enum _tag_VPP_BCM_BD_CMD_ {
    VPP_BCM_BD_CMD_INIT = 0,
    VPP_BCM_BD_CMD_RESET,
    VPP_BCM_BD_CMD_PEAK,
    VPP_BCM_BD_CMD_READ,
    VPP_BCM_BD_CMD_CONFIG,
    VPP_BCM_BD_CMD_PRINTSTYLE,
    VPP_BCM_BD_CMD_PREFIX,
    VPP_BCM_BD_CMD_SHOW_SETTINGS,
    VPP_BCM_BD_CMD_HELP,
}VPP_BCM_BD_CMD;

typedef enum _tag_VPP_BCM_BUFF_ID_ {
    VPP_BCM_BUFF_ID_BCM = 0,
    VPP_BCM_BUFF_ID_CFGQ,
}VPP_BCM_BUFF_ID;

typedef enum _tag_VPP_BCM_BD_PRNF_ {
    VPP_BCM_BD_PRNF_SWAP  = 0x01,
    VPP_BCM_BD_PRNF_BREAK = 0x02,
}VPP_BCM_BD_PRNF;

typedef union _tag_VPP_BCM_BD_CMD_OPT_ {
    struct {
        UINT32 opt1;
        UINT32 opt2;
        UINT32 opt3;
        UINT32 opt4;
    };

    struct {
        VPP_BCM_BUFF_ID buffer_id;      //0:BCM,1:CFGQ
        union {
            //VPP_BCM_BD_CMD_RESET = 0,
            //VPP_BCM_BD_CMD_PEAK, VPP_BCM_BD_CMD_READ
            struct {
                UINT32 buffer;      //1:print buffer,0:print pointer only
                //VPP_BCM_BD_CMD_READ,
                UINT32 size;        //0:print from rd pointer, n:print n entry fom start of buffer
            };
            //VPP_BCM_BD_CMD_CONFIG,
            struct {
                UINT32 start;       //1:start log, 0:stop log
                UINT32 nIntr;       //0:log infinitly, n:log for n interrupts
            };
            //VPP_BCM_BD_CMD_PRINTSTYLE,
            struct {
                UINT32 hexdump;        //1:hexdump, 0:reg_val pair
                UINT32 entryPerLine;   //0:8 entry perline, n:n entry perline
                UINT32 flags;           //bit0=> 0:no swap, 1: swap reg & val
                                        //bit1=> 0:no break, 1: break at interrupt pattern
            };
            //VPP_BCM_BD_CMD_PREFIX
            struct {
                UINT32 enablePrefix; //0:NoPrefix, 0x01:prefix interupt, 0x11:prefix CFGQ
                UINT32 prefixPattern_intr; //pattern to be used for interrupt
                UINT32 prefixPattern_cfgq; //pattern to be used for CFGQ
            };
        };
    };

}VPP_BCM_BD_CMD_OPT;

//Interfacing functions
void VPP_BCM_BD_init(void);
void VPP_BCM_BD_reset(VPP_BCM_REG_PTR *ptr);
void VPP_BCM_BD_add(UINT32 reg, UINT32 val, VPP_BCM_REG_PTR *ptr);
void VPP_BCM_BD_add_buffer(UINT32 *bcmBuffer, UINT32 size);
void VPP_BCM_BD_print(VPP_BCM_REG_PTR *ptr);
void VPP_BCM_BD_SetPrintStyle(VPP_BCM_REG_PTR *ptr, VPP_BCM_BD_CMD_OPT opt);
void VPP_BCM_BD_SetConfig(VPP_BCM_REG_PTR *ptr, VPP_BCM_BD_CMD_OPT opt);
void VPP_BCM_BD_SetPrefix(VPP_BCM_REG_PTR *ptr, VPP_BCM_BD_CMD_OPT opt);
void VPP_BCM_BD_show_settings(VPP_BCM_REG_PTR *ptr);
void VPP_BCM_BD_intrStart(void);

void VPP_BCM_BD_cmd(VPP_BCM_BD_CMD cmd, VPP_BCM_BD_CMD_OPT opt);

//Debug functions
void VPP_BCM_BD_PrintPtr(VPP_BCM_REG_PTR *ptr, int printBuffer, int printSize);

extern VPP_BCM_REG_PTR bcm_buff;
extern VPP_BCM_REG_PTR cfgq_buff;

#endif //_VPP_BCM_BD_H_
