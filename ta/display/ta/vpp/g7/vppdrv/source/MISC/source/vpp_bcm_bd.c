/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_bcm_bd.c
//! \brief      The BCM dump framework source file
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#include "vpp_bcm_bd.h"

#if defined(VPP_BCM_BUFFER_DUMP)

VPP_BCM_REG_PTR bcm_buff = {0,0,0};
VPP_BCM_REG_PTR cfgq_buff = {0,0,0};

//Internal functions
static int VPP_BCM_BD_get(UINT32 *reg, UINT32 *val, VPP_BCM_REG_PTR *ptr);

/************************Implementation Starts*****************************/
#define VPP_BCM_BD_DEFAULT_ENTRY_PER_LINE   0x6

void VPP_BCM_BD_show_settings(VPP_BCM_REG_PTR *ptr) {
    if(ptr==NULL) {
        ptr = &bcm_buff;
    }
    //Wait until reset is complete-i.e block interrupt context
    VPP_BCM_BD_PRINT("\nSetting for BD : %p(%p) => ", ptr, ptr->buff);
    VPP_BCM_BD_PRINT("start : %x, nIntr : %x, prefix : %x, intr_prefix : %x, cfgq_prefix : %x, hexdump : %x, entryPerLine : %x, flags : %x\n",
        ptr->config.start, ptr->config.nIntr, ptr->config.enablePrefix, ptr->config.prefixPattern_intr,
        ptr->config.prefixPattern_cfgq, ptr->prnStyle.hexdump, ptr->prnStyle.entryPerLine, ptr->prnStyle.flags);
}

static void VPP_BCM_BD_init_ds(VPP_BCM_REG_PTR *ptr) {

    if(ptr==NULL) {
        ptr = &bcm_buff;
    }

#ifdef ENABLE_NON_OS_MUTEX
    //ptr->mutex = 0;
    spin_lock_init(&(ptr->mutex));
#else
    MV_OSAL_Mutex_Create(&(ptr->mutex));
#endif

    ptr->prnStyle.entryPerLine = VPP_BCM_BD_DEFAULT_ENTRY_PER_LINE;
    ptr->prnStyle.hexdump      = 0x00;
    ptr->prnStyle.flags        = VPP_BCM_BD_PRNF_BREAK;

    ptr->config.start = 1;
    ptr->config.nIntr = 0;
    ptr->config.enablePrefix        = 0x1;
    ptr->config.prefixPattern_intr  = 0xFFFFFFFF;
    ptr->config.prefixPattern_cfgq  = 0x0;
}

void VPP_BCM_BD_init(void) {
    static int initialized = 0;

    if(!initialized) {
        VPP_BCM_BD_reset(&bcm_buff);
        VPP_BCM_BD_reset(&cfgq_buff);

        VPP_BCM_BD_init_ds(&bcm_buff);
        VPP_BCM_BD_init_ds(&cfgq_buff);

        initialized = 1;
    }
}

void VPP_BCM_BD_reset(VPP_BCM_REG_PTR *ptr) {

    if(ptr==NULL) {
        ptr = &bcm_buff;
    }
    //Wait until reset is complete-i.e block interrupt context
    VPP_BCM_DB_MUTEX_WAIT_LOCK(ptr);

    ptr->read_ptr = ptr->write_ptr = ptr->count = 0;
    GaloisMemClear((UINT8*)ptr->buff, VPP_BCM_DB_MAX_SIZE*sizeof(VPP_BCM_REG_INFO));

    VPP_BCM_DB_MUTEX_UNLOCK(ptr);
}

int VPP_BCM_BD_get(UINT32 *reg, UINT32 *val, VPP_BCM_REG_PTR *ptr) {
    int i;
    int retVal=0;

    if(ptr==NULL) {
        ptr = &bcm_buff;
    }

    //Wait until read is complete-i.e block interrupt context
    VPP_BCM_DB_MUTEX_WAIT_LOCK(ptr);

    i = ptr->read_ptr;
    if(ptr->count) {
        ptr->count--;

        *reg = ptr->buff[i].reg;
        *val = ptr->buff[i].val;

        ptr->read_ptr++;
        if(ptr->read_ptr>=VPP_BCM_DB_MAX_SIZE) {
            ptr->read_ptr=0;
        }
        retVal = 1;
    }

    VPP_BCM_DB_MUTEX_UNLOCK(ptr);

    return retVal;
}

void VPP_BCM_BD_add(UINT32 reg, UINT32 val, VPP_BCM_REG_PTR *ptr) {
    int i;
    if(ptr==NULL) {
        ptr = &bcm_buff;
    }
    if(!ptr->config.start) {
        //Logging is not started - So don't add reg_val pair into buffer
        return;
    }
#ifdef VPP_BCM_DB_MUTEX_WAIT_ENABLE
    //Wait until read is complete-i.e block interrupt context
    VPP_BCM_DB_MUTEX_WAIT_LOCK(ptr);
#else
    //Don't allow addition from interrupt context instead of wait
    VPP_BCM_DB_MUTEX_CHECK_RETURN(ptr);
#endif

    i = ptr->write_ptr;
    //ptr->buff[i].reg = reg&0xFFFF;   //Since MSB is constant-0xF744, ignore it
    ptr->buff[i].reg = reg;
    ptr->buff[i].val = val;
    ptr->write_ptr++;
    if(ptr->count >= VPP_BCM_DB_MAX_SIZE) {
        ptr->read_ptr++;
        if(ptr->read_ptr>=VPP_BCM_DB_MAX_SIZE){
            ptr->read_ptr=0;
        }
    } else {
        ptr->count++;
    }
    if(ptr->write_ptr >= VPP_BCM_DB_MAX_SIZE) {
        ptr->write_ptr = 0;
    }
    VPP_BCM_DB_MUTEX_UNLOCK(ptr);
}

void VPP_BCM_BD_add_buffer(UINT32 *bcmBuffer, UINT32 size) {
    int elements = size/8;
    UINT32 reg;
    UINT32 val;
    VPP_BCM_REG_PTR *ptr=&cfgq_buff;

    int i;
    //VPP_BCM_BD_DBG_PRINT("%s:%d --- %d\n",__FUNCTION__,__LINE__,elements);
    if(ptr->config.enablePrefix & VPP_BCM_BD_PREFIX_MASK_CFGQ) {
       VPP_BCM_BD_add(ptr->config.prefixPattern_cfgq, ptr->config.prefixPattern_cfgq, ptr);
    }
    for(i=0;i<elements;i++) {
       VPP_BCM_BD_add(bcmBuffer[i*2+1], bcmBuffer[i*2+0], ptr);
    }
}

void VPP_BCM_BD_intrStart(void) {
    VPP_BCM_REG_PTR *ptr;
    int i;
    //new VBI interrupt arrived....
    for(i=0;i<2;i++) {
        ptr = i==0?&bcm_buff:&cfgq_buff;

        if(ptr->config.start) {
            //Add the prefix if required
            if(ptr->config.enablePrefix & VPP_BCM_BD_PREFIX_MASK_INTR) {
               VPP_BCM_BD_add(ptr->config.prefixPattern_intr, ptr->config.prefixPattern_intr, ptr);
            }
            //logging enabled - check if it has to be stopped
            if(ptr->config.nIntr) {
                //If loging restricted for n interrupt then stop logging after n interrupts
                if(--ptr->config.nIntr == 0) {
                    //Stop logging
                    ptr->config.start = 0;
                }
            }
        }
    }
}

static void VPP_BCM_BD_PRINT_local(VPP_BCM_REG_PTR *ptr) {
    UINT32 reg;
    UINT32 val;

    if(ptr->prnStyle.count%ptr->prnStyle.entryPerLine == 0) {
        VPP_BCM_BD_PRINT("%04X :  ",ptr->prnStyle.countp*8);
    }
    if(ptr->prnStyle.flags & VPP_BCM_BD_PRNF_SWAP) {
        val = ptr->prnStyle.reg;
        reg = ptr->prnStyle.val;
    } else {
        reg = ptr->prnStyle.reg;
        val = ptr->prnStyle.val;
    }
    if(ptr->prnStyle.hexdump) {
        VPP_BCM_BD_PRINT("%08X  %08X  ", reg, val);
    } else {
        VPP_BCM_BD_PRINT("%08X:%08X  ", reg, val);
    }
    ptr->prnStyle.count++;
    ptr->prnStyle.countp++;
    if((ptr->prnStyle.count%ptr->prnStyle.entryPerLine == 0) ||
        ((ptr->prnStyle.flags & VPP_BCM_BD_PRNF_BREAK) && reg == val && reg == ptr->config.prefixPattern_intr)) {
        VPP_BCM_BD_PRINT("\n");
        //we can backtrack or fast forward count to next line break
        ptr->prnStyle.count-=ptr->prnStyle.count%ptr->prnStyle.entryPerLine;
    }
}

void VPP_BCM_BD_print(VPP_BCM_REG_PTR *ptr) {
    int iter = 0,rd_ptr;
    if(ptr==NULL) {
        ptr = &bcm_buff;
    }

    //Init local variable for printing
    ptr->prnStyle.count = 0;
    ptr->prnStyle.countp = 0;
    rd_ptr = ptr->read_ptr;
    while(VPP_BCM_BD_get(&ptr->prnStyle.reg,&ptr->prnStyle.val,ptr)) {
        if(iter==0 && ptr->prnStyle.count==0) {
            //Printing read_ptr is for debug purpose; please do it only once per print operation
            VPP_BCM_BD_PRINT("read_ptr : %X\n",rd_ptr);
            iter++;
        }
        VPP_BCM_BD_PRINT_local(ptr);
    }

    if(ptr->prnStyle.count%ptr->prnStyle.entryPerLine != 0) {
        VPP_BCM_BD_PRINT("\n");
    }
}

void VPP_BCM_BD_PrintPtr(VPP_BCM_REG_PTR *ptr, int printBuffer, int printSize) {
    int i,n;
    if(ptr==NULL) {
        ptr = &bcm_buff;
    }

    VPP_BCM_BD_PRINT("Buffer : 0x%p\twrite_ptr : %X\t read_ptr : %X\t count : %X\n",
                (void*)&ptr->buff[0], ptr->write_ptr, ptr->read_ptr, ptr->count);

    if(printBuffer) {
        //Init local variable for printing
        ptr->prnStyle.count = 0;
        ptr->prnStyle.countp = 0;
        if(printSize) {
            //Print n elements from start of buffer
            i = 0;
            n = printSize > VPP_BCM_DB_MAX_SIZE? VPP_BCM_DB_MAX_SIZE : printSize;
        } else {
            //Print from read pointer
            i=ptr->read_ptr;
            n=ptr->count;
        }
        for(;i<n;i++) {
            ptr->prnStyle.reg = ptr->buff[i].reg;
            ptr->prnStyle.val = ptr->buff[i].val;
            VPP_BCM_BD_PRINT_local(ptr);
        }
        if(ptr->prnStyle.count%ptr->prnStyle.entryPerLine != 0) {
            VPP_BCM_BD_PRINT("\n");
        }
    }
}

void VPP_BCM_BD_SetPrefix(VPP_BCM_REG_PTR *ptr, VPP_BCM_BD_CMD_OPT opt) {
    if(ptr==NULL) {
        ptr = &bcm_buff;
    }
    ptr->config.enablePrefix        = opt.enablePrefix;
    ptr->config.prefixPattern_intr  = opt.prefixPattern_intr;
    ptr->config.prefixPattern_cfgq  = opt.prefixPattern_cfgq;
    VPP_BCM_BD_show_settings(ptr);
}

void VPP_BCM_BD_SetConfig(VPP_BCM_REG_PTR *ptr, VPP_BCM_BD_CMD_OPT opt) {
    if(ptr==NULL) {
        ptr = &bcm_buff;
    }
    ptr->config.nIntr = opt.nIntr;
    ptr->config.start = opt.start;
    VPP_BCM_BD_show_settings(ptr);
}

void VPP_BCM_BD_SetPrintStyle(VPP_BCM_REG_PTR *ptr, VPP_BCM_BD_CMD_OPT opt) {
    if(ptr==NULL) {
        ptr = &bcm_buff;
    }
    if(opt.entryPerLine) {
        ptr->prnStyle.entryPerLine = opt.entryPerLine;
    } else {
        ptr->prnStyle.entryPerLine = VPP_BCM_BD_DEFAULT_ENTRY_PER_LINE;
    }
    ptr->prnStyle.hexdump = opt.hexdump;
    ptr->prnStyle.flags   = opt.flags;
    VPP_BCM_BD_show_settings(ptr);
}

void VPP_BCM_BD_cmdHelp(void) {
    char *cmdHelp[] = {
        "%-12s: VOP BD %d\n",
        "%-12s: VOP BD %d [BUFF_ID]\n",
        "%-12s: VOP BD %d [BUFF_ID] [SHOW_BUFF] [SIZE]\n",
        "%-12s: VOP BD %d [BUFF_ID]\n",
        "%-12s: VOP BD %d [BUFF_ID] [START] [ITER]\n",
        "%-12s: VOP BD %d [BUFF_ID] [HEXDUMP] [ElePerLine] [Flags]\n",
        "%-12s: VOP BD %d [BUFF_ID] [PREFIX] [INTR_PREFIX] [CFGQ_PREFIX]\n",
        "%-12s: VOP BD %d [BUFF_ID]\n",
        "%-12s: VOP BD %d\n"
    };
    char *cmdHelpDesc[] = {
        "-Initialize the BCM BD data structure\n",
        "-Reset the pointer and memory of given buffer - BUFF_ID(0-BCM/1-CFGQ)\n",
        "-Show the pointers and memory of given buffer - BUFF_ID(0-BCM/1-CFGQ), SHOW_BUFF(0-Show only pointer,1-Show Pointer&Memory), Size(0-disp from rd ptr, n-display n bytes from start of buffer\n",
         "-Read the buffer and advance read_pointer of given buffer - BUFF_ID(0-BCM/1-CFGQ) \n",
         "-Start/Stop logging for finate/infinate interrupts on the given buffer - BUFF_ID(0-BCM/1-CFGQ), START(0-Stop,1-Start), ITER(0-Infinate,X-Count) \n",
         "-Set print style of given buffer - BUFF_ID(0-BCM/1-CFGQ), HEXDUMP(0-RegValPair,1-Hexdump), ElePerLine(0-8,X-Count), flags(bit0-swap reg&val,bit1:break)\n",
         "-Set log prefix of given buffer - BUFF_ID(0-BCM/1-CFGQ) PREFIX(0:Disable,1:Enable), INTR_PREFIX(X:PrefixPerInterrupt), CFGQ_PREFIX(X:PrefixPerCFGQ_Operation)\n",
         "-Show the various setting for given buffer - BUFF_ID(0-BCM/1-CFGQ)\n",
         "-Print this help\n",
    };
    char cmdId[] = {
        VPP_BCM_BD_CMD_INIT,
        VPP_BCM_BD_CMD_RESET,
        VPP_BCM_BD_CMD_PEAK,
        VPP_BCM_BD_CMD_READ,
        VPP_BCM_BD_CMD_CONFIG,
        VPP_BCM_BD_CMD_PRINTSTYLE,
        VPP_BCM_BD_CMD_PREFIX,
        VPP_BCM_BD_CMD_SHOW_SETTINGS,
        VPP_BCM_BD_CMD_HELP,
    };
    char *cmdStr[] = {
        "INIT",
        "RESET",
        "PEEP",
        "READ",
        "CONFIG",
        "PRINTSTYLE",
        "PREFIX",
        "SHOW_SETTINGS",
        "HELP",
    };

    int i;
    for(i=0;i<sizeof(cmdId);i++) {
        VPP_BCM_BD_PRINT(cmdHelp[i], cmdStr[i], cmdId[i]);
        VPP_BCM_BD_PRINT("            %s",cmdHelpDesc[i]);
    }
}

void VPP_BCM_BD_cmd(VPP_BCM_BD_CMD cmd, VPP_BCM_BD_CMD_OPT opt) {
    //VPP_BCM_BD_PRINT("BCM BD : %X -> %X %X %X %X\n", cmd, opt.opt1, opt.opt2, opt.opt3, opt.opt4);
    switch(cmd) {
        case VPP_BCM_BD_CMD_INIT:
            VPP_BCM_BD_INIT();
        break;
        case VPP_BCM_BD_CMD_RESET:
            VPP_BCM_BD_RESET(opt.buffer_id);
        break;
        case VPP_BCM_BD_CMD_PEAK:
            VPP_BCM_BD_DISP_PTR(opt.buffer_id, opt.buffer, opt.size);
        break;
        case VPP_BCM_BD_CMD_READ:
            VPP_BCM_BD_DISP(opt.buffer_id);
        break;
        case VPP_BCM_BD_CMD_CONFIG:
            VPP_BCM_BD_SET_CONFIG(opt);
        break;
        case VPP_BCM_BD_CMD_PRINTSTYLE:
            VPP_BCM_BD_SET_PRINTSTYLE(opt);
        break;
        case VPP_BCM_BD_CMD_PREFIX:
            VPP_BCM_BD_SET_PREFIX(opt);
        break;
        case VPP_BCM_BD_CMD_SHOW_SETTINGS:
            VPP_BCM_BD_SHOW_SETTINGS(opt);
        break;
        default:
        case VPP_BCM_BD_CMD_HELP:
            VPP_BCM_BD_cmdHelp();
        break;
    }
}

#else //!VPP_BCM_BUFFER_DUMP

//Interfacing functions
void VPP_BCM_BD_init(void){}
void VPP_BCM_BD_reset(VPP_BCM_REG_PTR *ptr){}
void VPP_BCM_BD_add(UINT32 reg, UINT32 val, VPP_BCM_REG_PTR *ptr){}
void VPP_BCM_BD_add_buffer(UINT32 *bcmBuffer, UINT32 size){}
void VPP_BCM_BD_intrStart(void) {}
void VPP_BCM_BD_print(VPP_BCM_REG_PTR *ptr){}
void VPP_BCM_BD_cmd(VPP_BCM_BD_CMD cmd, VPP_BCM_BD_CMD_OPT opt){}
void VPP_BCM_BD_SetPrintStyle(VPP_BCM_REG_PTR *ptr, VPP_BCM_BD_CMD_OPT opt){}
void VPP_BCM_BD_SetConfig(VPP_BCM_REG_PTR *ptr, VPP_BCM_BD_CMD_OPT opt){}
void VPP_BCM_BD_SetPrefix(VPP_BCM_REG_PTR *ptr, VPP_BCM_BD_CMD_OPT opt){}
void VPP_BCM_BD_show_settings(VPP_BCM_REG_PTR *ptr){}

//Debug functions
void VPP_BCM_BD_PrintPtr(VPP_BCM_REG_PTR *ptr, int printBuffer, int printSize){}

#endif //VPP_BCM_BUFFER_DUMP

