// SPDX-License-Identifier: Proprietary
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

////////////////////////////////////////////////////////////////////////////////
//! \file vmeta_log.c
//! \brief vmeta log code.
//!
//! Purpose:
//!     control vmeta ta log
//!
//!  Note:
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


#include <vpu_dbg.h>

#include "vmeta_log.h"
#include "vpu_ta_common.h"

#define AMP_LOG_HSZ                 256

#ifdef DYNAMIC_LOG_LEVEL

typedef struct {
    const char *name;
    unsigned int value;
    unsigned int size;
} VMETA_LOG_TYPE;

typedef enum {
    VMETA_PERF = 3,
    VMETA_BUG = 4,
    VMETA_FUNC = 5,
    VMETA_FIGO_PC = 6,
    VMETA_VDEC = 7,
    VMETA_DIS_CRC = 8,
    VMETA_FSM = 9,
    VMETA_RESULT = 10,
    VMETA_SBUF = 11,
    VMETA_SYNC = 12,
    VMETA_FLUSH = 13,
    VMETA_API = 14,
    VMETA_MSG = 15,
    VMETA_VP9 = 16,
    VMETA_SKIP = 17,
    VMETA_RBUF = 18,
    VMETA_DBUF = 19,
    VMETA_HDR = 20,
    VMETA_PIC = 21,
    VMETA_CRC = 22,
    VMETA_CSW = 23,
    VMETA_RST = 24,
    VMETA_VMETA = 25,
    VMETA_INTR = 26,
    VMETA_BUF = 27,
    VMETA_ENTRY = 28,

    VMETA_ALL = 31,
    VMETA_DEFAULT

}ENUM_VEMTA_LOG_E;
#endif

static unsigned int level_firmware = 0;
static unsigned int level_ta = 0;

static const char reset_color[] = "\033[0m";
static const char *lead_color[] = {
    "\033[31;1m",           // Fatal: red
    "\033[35;1m",           // Error: carmine
    "\033[33;1m",           // High:  yellow
    "\033[36;1m",           // Middle:cyan
    "\033[34;1m",           // Low:   blue
    "\033[32;1m",           // Info:  green
    reset_color,            // User1: default
    reset_color,            // User2: default
};

#ifdef DYNAMIC_LOG_LEVEL
static const VMETA_LOG_TYPE VmetaLogType[] = {
    {"@perf",       VMETA_PERF,     5},
    {"@bug",        VMETA_BUG,      4},
    {"@func",       VMETA_FUNC,     5},
    {"@figo_pc",    VMETA_FIGO_PC,  8},
    {"@vdec",       VMETA_VDEC,     5},
    {"@dis_crc",    VMETA_DIS_CRC,  8},
    {"@fsm",        VMETA_FSM,      4},
    {"@result",     VMETA_RESULT,   6},
    {"@sbuf",       VMETA_SBUF,     5},
    {"@sync",       VMETA_SYNC,     5},
    {"@flush",      VMETA_FLUSH,    6},
    {"@api",        VMETA_API,      4},
    {"@msg",        VMETA_MSG,      4},
    {"@vp9",        VMETA_VP9,      4},
    {"@skip",       VMETA_SKIP,     5},
    {"@rbuf",       VMETA_RBUF,     5},
    {"@dbuf",       VMETA_DBUF,     5},
    {"@hdr",        VMETA_HDR,      4},
    {"@pic",        VMETA_PIC,      4},
    {"@crc",        VMETA_CRC,      4},
    {"@csw",        VMETA_CSW,      4},
    {"@rst",        VMETA_RST,      4},
    {"@vmeta",      VMETA_VMETA,    6},
    {"@intr",       VMETA_INTR,     5},
    {"@buf",        VMETA_BUF,      4},
    {"@entry",      VMETA_ENTRY,    6},
};
#endif

int wrap_set_level(uint32_t level)
{
    level_ta = level & 0x7; //the last three bits was used to control TA print
    level_firmware = level; //x111 1111 1111 1111 1VVV
    TA_LOGL("level_ta=0x%08" PRIx32 " level_firmware=0x%08" PRIx32 "\n", level_ta, level_firmware);
    return 0;
}

#ifdef ENABLE_TA_LOG
__attribute__((format(printf, 4, 5)))
void vmeta_ta_log(uint32_t level, const char *func, uint32_t line,
                  const char *format, ...)
{
    static char head[AMP_LOG_HSZ];
    static char log_buf[AMP_LOG_HSZ];
    va_list ap;

    if ((1 << level) & level_ta) {
        va_start(ap, format);
        snprintf(head, AMP_LOG_HSZ, "%sAmp%" PRIu32 "@@[TA_VMETA:%s():%" PRIu32 "]%s ",
                 lead_color[level], level, func, line, reset_color);
        vsnprintf(log_buf, AMP_LOG_HSZ, format, ap);
        TA_LOGL("%s%s\n", head, log_buf);
        va_end(ap);
    }
    return;
}
#endif

/**
 * For performance optimization, firmware only print @err log;
 * Enable other log, need new debug version firmware;
 * So it's meaningless to keep complex log level control here;
 */
#ifdef DYNAMIC_LOG_LEVEL
__attribute__((format(printf, 1, 2)))
void vMetaFwPrint(const char *format, ...)
{
    va_list ap;
    unsigned int i = 0;
    unsigned int type_size = sizeof(VmetaLogType) / sizeof(VMETA_LOG_TYPE);
    bool is_need_print = false;
    is_need_print |= level_firmware >> VMETA_ALL; //all the log of firmware need be open
    is_need_print |= !strncmp(format, "@err", 4); // err was open default

    for (i = 0; i < type_size; i++) {
        if ((1 << VmetaLogType[i].value) & level_firmware) {
           is_need_print |= !strncmp(format, VmetaLogType[i].name,
                                     VmetaLogType[i].size);
        }
    }

    if (is_need_print == true) {
        char log[512] = {0};
        va_start(ap, format);
        vsnprintf(log, 512, format, ap);
        va_end(ap);
        TA_LOGL("%s\n",log);
    }
}
#else  //print all vpu log
__attribute__((format(printf, 1, 2)))
void vMetaFwPrint(const char *format, ...)
{
    char log[512];
    va_list ap;
    va_start(ap, format);
    vsnprintf(log, 512, format, ap);
    va_end(ap);
    TA_LOGL("%s\n",log);
}
#endif //end of DYNAMIC_LOG_LEVEL

// extern vpu_perf_dbg_t g_v2g_perf_obj;
// declared in vpu_dbg.h, leave here for readniess

#if defined(ENABLE_VPU_PROFILING)
void vpu_perf_log(void* inst)
{
#if defined(ENABLE_VPU_DETAIL_PROFILING)
    TA_LOGL("@perf>> pic_num: %d  type: %c  width: %d  height: %d  mbs: %d  ints: %d"
            "  bits: %d  slcs: %d\n",
            g_v2g_perf_obj.pic_num, g_v2g_perf_obj.pic_type,
            g_v2g_perf_obj.width, g_v2g_perf_obj.height,
            g_v2g_perf_obj.mbs, g_v2g_perf_obj.ints, g_v2g_perf_obj.bits, g_v2g_perf_obj.slcs);

    TA_LOGL("@perf_bw>> rd_bd: 0x%08x  wr_bd: 0x%08x\n",
            g_v2g_perf_obj.read_bw, g_v2g_perf_obj.write_bw);

    TA_LOGL("@perf>> hw_cycle: 0x%08x  sw_cycle: 0x%08x  sw1_cycle: 0x%08x total: 0x%08x"
            "  show_flag: 0x%08x int_lat: 0x%08x\n",
            g_v2g_perf_obj.hw_cycles, g_v2g_perf_obj.fw0_cycles, g_v2g_perf_obj.fw1_cycles,
            g_v2g_perf_obj.total_cycles, g_v2g_perf_obj.show_flag, g_v2g_perf_obj.int_lat);

    TA_LOGL("@perf>> scu: 0x%08x spu: 0x%08x  mvu: 0x%08x  qtu: 0x%08x  vcu: 0x%08x"
            "  vcu1: 0x%08x  ppu: 0x%08x fcu: 0x%08x\n",
            g_v2g_perf_obj.pc_scu, g_v2g_perf_obj.pc_spu, g_v2g_perf_obj.pc_mvu,
            g_v2g_perf_obj.pc_qtu, g_v2g_perf_obj.pc_vcu, g_v2g_perf_obj.pc_vcu1,
            g_v2g_perf_obj.pc_ppu, g_v2g_perf_obj.pc_fcu);

    TA_LOGL("@perf>> spu1: 0x%08x  spu2: 0x%08x  spu3: 0x%08x  qtu1: 0x%08x"
            "  vcu2: 0x%08x  ppu1: 0x%08x  fcu1: 0x%08x  pfu: 0x%08x  pfu1: 0x%08x\n",
            g_v2g_perf_obj.pc_spu1,  g_v2g_perf_obj.pc_spu2,  g_v2g_perf_obj.pc_spu3,
            g_v2g_perf_obj.pc_qtu1,  g_v2g_perf_obj.pc_vcu2,  g_v2g_perf_obj.pc_ppu1,
            g_v2g_perf_obj.pc_fcu1,  g_v2g_perf_obj.pc_pfu,   g_v2g_perf_obj.pc_pfu1);
#endif
    TA_LOGL("@perf>> Ins_num: %p hw_cycle: 0x%08x fw_cycle: 0x%08x vdec: 0x%08x total: 0x%08x"
            " int_lat: 0x%08x\n",
            inst,
            g_v2g_perf_obj.hw_cycles,
            g_v2g_perf_obj.fw0_cycles,
            g_v2g_perf_obj.total_cycles - g_v2g_perf_obj.hw_cycles
            - g_v2g_perf_obj.fw0_cycles,
            g_v2g_perf_obj.total_cycles, g_v2g_perf_obj.int_lat);
}

uint32_t vpu_profiling(void *inst)
{
    vpu_perf_log(inst);
    return (uint32_t)(g_v2g_perf_obj.hw_cycles + g_v2g_perf_obj.fw0_cycles);
}

#else  //no profileing
uint32_t vpu_profiling(void *inst)
{
    return 0;
}
#endif //end of enable_vpu_profiling

void vpu_perf_summary(void)
{
    double    vxg_clk          = 1000000000.0; // 1000M as default
    uint32_t decoded_frames   = g_v2g_perf_obj.decoded_frames;
    uint32_t valid_frames     = g_v2g_perf_obj.valid_frames;
    uint64_t total_hw_cycles  = g_v2g_perf_obj.total_hw_cycles;
    uint64_t total_fw_cycles  = g_v2g_perf_obj.total_fw_cycles;
    uint64_t total_dec_cycles = g_v2g_perf_obj.total_dec_cycles;

    // vxg clock has different value for different chip,
    // please copy exact value defined in boot/common/chip/xxx/diag_clock.c
#if (BERLIN_CHIP_VERSION == BERLIN_PLATYPUS)
    vxg_clk = 900000000.0; // 900M
#elif (BERLIN_CHIP_VERSION == BERLIN_DOLPHIN_A0)
    // 1000M, same as default
#endif

    if (valid_frames) {
        uint32_t avg_hw    = (uint32_t)(total_hw_cycles/valid_frames);
        uint32_t avg_fw    = (uint32_t)((total_hw_cycles+total_fw_cycles)/valid_frames);
        uint32_t avg_total =(uint32_t)(total_dec_cycles/valid_frames);
        VMETA_LOGI("@perf>> total %" PRIu32 "/%" PRIu32 " frames decoded, "
            "avg_hw_fps: %.2f(0x%08" PRIx32 ")"
            "avg_fw_fps: %.2f(0x%08" PRIx32 ")"
            "avg_fps: %.2f(0x%08" PRIx32 ")\n",
            valid_frames, decoded_frames, vxg_clk/avg_hw, avg_hw,
            vxg_clk/avg_fw, avg_fw, vxg_clk/avg_total, avg_total);
    }
}


//legacy code, keep it for readniess
#if defined(PROFILING_VIDEO_PERFORMANCE)
unsigned int sw_clk0  = 0;
unsigned int sw_clk1  = 0;
unsigned int hw_clk0  = 0;
unsigned int hw_clk1  = 0;
unsigned int sw_total = 0;
unsigned int hw_total = 0;
unsigned int fr_total  = 0;
unsigned int fr_clk0  = 0;
unsigned int fr_clk1  = 0;
#ifdef MEMMAP_V4G_REG_BASE
unsigned int g_io_base = MEMMAP_V4G_REG_BASE;
#else
unsigned int g_io_base = MEMMAP_VPRO_REG_BASE;
#endif
unsigned int g_intr_flag = 0;
struct timeval g_start;
struct timeval g_end;
unsigned int  g_second = 0;
extern vmeta_dbg_t *g_vmeta_dbg;
unsigned int g_strmID = 0;

void ta_profile_start() {
    sw_clk0 = hw_clk1 =  *(volatile int *)(g_io_base + 0x10);
    gettimeofday(&g_start, 0);
    if (g_intr_flag) {
        hw_total += (hw_clk1 - hw_clk0);
        g_intr_flag = 0;
    }
}

void ta_profile_end(vdec_strm_status_t *strm_status) {
    FILE *fp = 0;
    int wait_for_int = strm_status->wait_for_int;
    int event_id = strm_status->event_id;
    static unsigned int frame_counter = 0;
    static unsigned int intr_counter = 0;
    fr_clk1 = sw_clk1 = (*(volatile int *)(g_io_base + 0x10));
    sw_total += (sw_clk1 - sw_clk0);
    gettimeofday(&g_end, 0);
    g_second += (1000000 * ( g_end.tv_sec - g_start.tv_sec ) + g_end.tv_usec - g_start.tv_usec);

    if (wait_for_int) {
        hw_clk0 = sw_clk1;
        g_intr_flag = 1;
        intr_counter++;
    }
    if (event_id == VDEC_EVENT_END_OF_PICTURE) {
        vdec_pic_desc_t *pic = strm_status->pic_desc;
        const vmeta_perf_t *perf = g_vmeta_dbg->perf;
        fr_total = fr_clk1 - fr_clk0;
        frame_counter++;
        if (/*pic->is_valid*/1) {
            VMETA_LOGI("idx:<%6d>: id:[%2d] type:[%c] sw(ms): %10d  sw: %10d  hw: %10d total: %10d "
                       "f1: %10d  %10d  f0: %10d  %10d  f2: %10d  %10d "
                       "p0: %10d %10d (sem: %10d stl: %10d bbl: %10d) "
                       "p1: %10d %10d (sem: %10d stl: %10d  bbl: %10d intr_counter: %10d check:0x%08x\n",
                       frame_counter, g_strmID, pic->pic_type, g_second, sw_total, hw_total,
                       fr_total,
                       perf->f1_run, (perf->f1_run - perf->f1_stl),
                       perf->f0_run, (perf->f0_run - perf->f0_stl),
                       perf->f2_run, (perf->f2_run - perf->f2_stl),
                       perf->p0_run, perf->p0_ins, perf->p0_sem, perf->p0_stl, perf->p0_bbl,
                       perf->p1_run, perf->p1_ins, perf->p1_sem, perf->p1_stl, perf->p1_bbl,
                       intr_counter,  pic->chksum_data[0]);

            sw_total = 0;
            hw_total = 0;
            fr_total = 0;
            g_second = 0;
            fr_clk0  = fr_clk1;
            intr_counter = 0;
        }
    }
}
#endif
