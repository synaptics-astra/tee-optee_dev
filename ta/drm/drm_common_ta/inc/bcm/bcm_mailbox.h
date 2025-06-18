/*
 * INTERNAL USE ONLY
 *
 * Unpublished Work Copyright (C) 2013-2017 Synaptics Incorporated.
 * All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 */

#ifndef INCLUDE_BCM_MAILBOX_H
#define INCLUDE_BCM_MAILBOX_H

//#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP || BERLIN_CHIP_VERSION == BERLIN_BG3CD)

#include "drm_tee_types.h"

typedef struct
{
   UINT32 arg0;
   UINT32 arg1;
   UINT32 arg2;
   UINT32 arg3;
   UINT32 arg4;
   UINT32 arg5;
   UINT32 arg6;
   UINT32 arg7;
   UINT32 arg8;
   UINT32 arg9;
   UINT32 arg10;
   UINT32 arg11;
   UINT32 arg12;
   UINT32 arg13;
   UINT32 arg14;
   UINT32 arg15;
   UINT32 command;
} BCM_MAILBOX_COMMAND;

typedef struct
{
   UINT32 return_status;
   UINT32 arg0;
   UINT32 arg1;
   UINT32 arg2;
   UINT32 arg3;
   UINT32 arg4;
   UINT32 arg5;
   UINT32 arg6;
   UINT32 arg7;
   UINT32 arg8;
   UINT32 arg9;
   UINT32 arg10;
   UINT32 arg11;
   UINT32 arg12;
   UINT32 arg13;
   UINT32 arg14;
   UINT32 arg15;
} BCM_MAILBOX_STATUS;

// the command mailbox structure, which appears in the Client's memory space
typedef struct
{
   UINT32 command_parameter0;                  // 0x0 Write Only
   UINT32 command_parameter1;                  // 0x4 Write Only
   UINT32 command_parameter2;                  // 0x8 Write Only
   UINT32 command_parameter3;                  // 0xc Write Only
   UINT32 command_parameter4;                  // 0x10 Write Only
   UINT32 command_parameter5;                  // 0x14 Write Only
   UINT32 command_parameter6;                  // 0x18 Write Only
   UINT32 command_parameter7;                  // 0x1c Write Only
   UINT32 command_parameter8;                  // 0x20 Write Only
   UINT32 command_parameter9;                  // 0x24 Write Only
   UINT32 command_parameter10;                 // 0x28 Write Only
   UINT32 command_parameter11;                 // 0x2c Write Only
   UINT32 command_parameter12;                 // 0x30 Write Only
   UINT32 command_parameter13;                 // 0x34 Write Only
   UINT32 command_parameter14;             // 0x38 Write Only: linked-list dma host-read pointer
   UINT32 command_parameter15;             // 0x3c Write Only: linked-list dma host-write pointer
   UINT32 secure_processor_command;            // 0x40 Write Only
   UINT32 reserved_0x44[15];
   UINT32 command_return_status;               // 0x80 Read Only
   UINT32 command_status0;                     // 0x84 Read Only
   UINT32 command_status1;                     // 0x88 Read Only
   UINT32 command_status2;                     // 0x8c Read Only
   UINT32 command_status3;                     // 0x90 Read Only
   UINT32 command_status4;                     // 0x94 Read Only
   UINT32 command_status5;                     // 0x98 Read Only
   UINT32 command_status6;                     // 0x9c Read Only
   UINT32 command_status7;                     // 0xa0 Read Only
   UINT32 command_status8;                     // 0xa4 Read Only
   UINT32 command_status9;                     // 0xa8 Read Only
   UINT32 command_status10;                    // 0xac Read Only
   UINT32 command_status11;                    // 0xb0 Read Only
   UINT32 command_status12;                    // 0xb4 Read Only
   UINT32 command_status13;                    // 0xb8 Read Only
   UINT32 command_status14;                    // 0xbc Read Only
   UINT32 command_status15;                    // 0xc0 Read Only
   UINT32 command_fifo_status;                 // 0xc4 Read Only
   UINT32 host_interrupt_register;             // 0xc8 Write-to-Clear
   UINT32 host_interrupt_mask;                 // 0xcc Write-to-Clear
   UINT32 host_exception_address;              // 0xd0 Write-to-Clear
   UINT32 sp_trust_register;                   // 0xd4 Read Only
   UINT32 wtm_identification;                  // 0xd8 Read Only
   UINT32 wtm_revision;                        // 0xdc Read Only
} BCM_MAILBOX;


#define BCM_MAILBOX_BASE                    (MEMMAP_BCM_REG_BASE)
#define BCM_COMMAND_PARAMETER0              (BCM_MAILBOX_BASE+0x00)  //Write Only
#define BCM_COMMAND_PARAMETER1              (BCM_MAILBOX_BASE+0x04)  //Write Only
#define BCM_COMMAND_PARAMETER2              (BCM_MAILBOX_BASE+0x08)  //Write Only
#define BCM_COMMAND_PARAMETER3              (BCM_MAILBOX_BASE+0x0c)  //Write Only
#define BCM_COMMAND_PARAMETER4              (BCM_MAILBOX_BASE+0x10)  //Write Only
#define BCM_COMMAND_PARAMETER5              (BCM_MAILBOX_BASE+0x14)  //Write Only
#define BCM_COMMAND_PARAMETER6              (BCM_MAILBOX_BASE+0x18)  //Write Only
#define BCM_COMMAND_PARAMETER7              (BCM_MAILBOX_BASE+0x1c)  //Write Only
#define BCM_COMMAND_PARAMETER8              (BCM_MAILBOX_BASE+0x20)  //Write Only
#define BCM_COMMAND_PARAMETER9              (BCM_MAILBOX_BASE+0x24)  //Write Only
#define BCM_COMMAND_PARAMETER10             (BCM_MAILBOX_BASE+0x28)  //Write Only
#define BCM_COMMAND_PARAMETER11             (BCM_MAILBOX_BASE+0x2c)  //Write Only
#define BCM_COMMAND_PARAMETER12             (BCM_MAILBOX_BASE+0x30)  //Write Only
#define BCM_COMMAND_PARAMETER13             (BCM_MAILBOX_BASE+0x34)  //Write Only
//Write Only: linked-list dma host-read pointer
#define BCM_COMMAND_PARAMETER14             (BCM_MAILBOX_BASE+0x38)
//Write Only: linked-list dma host-write pointer
#define BCM_COMMAND_PARAMETER15             (BCM_MAILBOX_BASE+0x3c)
#define BCM_SECURE_PROCESSOR_COMMAND        (BCM_MAILBOX_BASE+0x40)  //Write Only

#define BCM_COMMAND_RETURN_STATUS           (BCM_MAILBOX_BASE+0x80)  //Read Only
#define BCM_COMMAND_STATUS0                 (BCM_MAILBOX_BASE+0x84)  //Read Only
#define BCM_COMMAND_STATUS1                 (BCM_MAILBOX_BASE+0x88)  //Read Only
#define BCM_COMMAND_STATUS2                 (BCM_MAILBOX_BASE+0x8c)  //Read Only
#define BCM_COMMAND_STATUS3                 (BCM_MAILBOX_BASE+0x90)  //Read Only
#define BCM_COMMAND_STATUS4                 (BCM_MAILBOX_BASE+0x94)  //Read Only
#define BCM_COMMAND_STATUS5                 (BCM_MAILBOX_BASE+0x98)  //Read Only
#define BCM_COMMAND_STATUS6                 (BCM_MAILBOX_BASE+0x9c)  //Read Only
#define BCM_COMMAND_STATUS7                 (BCM_MAILBOX_BASE+0xa0)  //Read Only
#define BCM_COMMAND_STATUS8                 (BCM_MAILBOX_BASE+0xa4)  //Read Only
#define BCM_COMMAND_STATUS9                 (BCM_MAILBOX_BASE+0xa8)  //Read Only
#define BCM_COMMAND_STATUS10                (BCM_MAILBOX_BASE+0xac)  //Read Only
#define BCM_COMMAND_STATUS11                (BCM_MAILBOX_BASE+0xb0)  //Read Only
#define BCM_COMMAND_STATUS12                (BCM_MAILBOX_BASE+0xb4)  //Read Only
#define BCM_COMMAND_STATUS13                (BCM_MAILBOX_BASE+0xb8)  //Read Only
#define BCM_COMMAND_STATUS14                (BCM_MAILBOX_BASE+0xbc)  //Read Only
#define BCM_COMMAND_STATUS15                (BCM_MAILBOX_BASE+0xc0)  //Read Only
#define BCM_COMMAND_FIFO_STATUS             (BCM_MAILBOX_BASE+0xc4)  //Read Only

#define BCM_HOST_INTERRUPT_REGISTER         (BCM_MAILBOX_BASE+0xC8)  //Write to clear
#define BCM_HOST_INTERRUPT_MASK             (BCM_MAILBOX_BASE+0xcc)  //Write-to-Clear
#define BCM_HOST_EXCEPTION_ADDRESS          (BCM_MAILBOX_BASE+0xd0)  //Write-to-Clear
#define BCM_SP_TRUST_REGISTER               (BCM_MAILBOX_BASE+0xd4)  //Read Only
#define BCM_WTM_IDENTIFICATION              (BCM_MAILBOX_BASE+0xd8)  //Read Only
#define BCM_WTM_REVISION                    (BCM_MAILBOX_BASE+0xdc)  //Read Only

#define BCM_CLOCK_CTRL                      (MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_bcmClk)

#define BCM_REG(addr)                       (devmem_phy_to_virt((UINT32)(addr)))

#define BCM_Reg_Write32(offset, val)        ((*(volatile UINT32*)(BCM_REG(offset)))=val)
#define BCM_Reg_Read32(offset, holder) \
                            ((*(volatile UINT32*)(holder))=(*(UINT32*)(BCM_REG(offset))))

#define BCM_Reg_Write16(offset, val)        ((*(volatile UINT16*)(BCM_REG(offset)))=val)
#define BCM_Reg_Read16(offset, holder) \
                            ((*(volatile UINT16*)(holder))=(*(UINT16*)(BCM_REG(offset))))

void bcm_memclr(void *p, UINT32 numInt);

void bcm_clear_command(BCM_MAILBOX_COMMAND * pCmd);

void bcm_clear_status(BCM_MAILBOX_STATUS * pStatus);

//#endif //BERLIN_CHIP_VERSION
#endif
