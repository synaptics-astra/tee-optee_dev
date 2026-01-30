/*
* NDA AND NEED-TO-KNOW REQUIRED
*
* Copyright (C) 2013-2026 Synaptics Incorporated. All rights reserved.
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
*
* INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS", AND
* SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
* INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
* INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
* CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
* OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
* BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
* COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
* DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
* TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
*/

//  Description:    Video Codec Debug Interface
//  Author:         Hongjie Guan
//---------------------------------------------------------------------------
//  WARNING:        This file is not an integral part of the codec API and is
//                  subject to change without notice.
//                  Functions and variables declared here are for debug only
//                  and should not be relied on for normal applications.
//---------------------------------------------------------------------------

#ifndef VPU_DBG_H_
#define VPU_DBG_H_

#include "vpu_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

//---------------------------------------------------------------------------
// Macros
//---------------------------------------------------------------------------
#ifndef TYPEDEF_STRUCT
#define TYPEDEF_STRUCT(x_)  typedef struct x_ x_; struct x_
#endif

//---------------------------------------------------------------------------
// Stream-independent VMeta Internal Settings
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(vpu_icfg_t)
{
    unsigned char clkgt_mode;       // 0=auto; 1=enable clk gating; 2=clk always on;
    unsigned char rsvd[3];
    unsigned int internal_flags;

    // For DDR in/out BW and checksum calculation: set physical address range
    unsigned int axi_raddr_min;
    unsigned int axi_raddr_max;     // [axi_raddr_min, axi_raddr_max]: specifies the AXI read address range; ddr physical address
    unsigned int axi_waddr_min;
    unsigned int axi_waddr_max;     // [axi_waddr_min, axi_waddr_max]: specifies the AXI write address range:ddr physical address
};

typedef vpu_icfg_t vdec_icfg_t;
typedef vpu_icfg_t venc_icfg_t;
typedef vpu_icfg_t vmeta_icfg_t;

//---------------------------------------------------------------------------
// Per-stream Internal Settings
//---------------------------------------------------------------------------
enum
{
    VXG_AHB_IDX_TRIGGER_IDLE = 0,
    VXG_AHB_IDX_TRIGGER_TRI,
    VXG_AHB_IDX_TRIGGER_TRI1,
    VXG_AHB_IDX_TRIGGER_TRIn,
};

TYPEDEF_STRUCT(vpu_strm_icfg_t)
{
    unsigned char parser_mode;      // 0=auto; 1=single-pass; 2=dual-pass;
    unsigned char dblk_mode;        // 0=auto; 1=normal; 2=ref-only; 3=disabled;
    unsigned char bump_mode;        // 0=auto; 1=normal; 2=greedy;
    unsigned char csw_mode;         // 0=auto; 1=per frame; 2=any time;
    unsigned char mvc_mode;         // 0=auto; 1=disabled; 2=enabled;
    unsigned char aso_mode;         // 0=auto; 1=disabled; 2=enabled;
    unsigned char divx_it_mode;     // 0=auto; 1=disabled; 2=enabled;
    unsigned char rsvd[3];
    unsigned char mb_timeout;       // 0=auto; ~0=disabled;
    unsigned short fewd_wmax;       // 0=auto;
    unsigned short fewd_imax;       // 0=auto;
    unsigned short bewd_wmax;       // 0=auto;
    unsigned short bewd_imax;       // 0=auto;
    unsigned short eopwd_wmax;      // 0=auto;
    unsigned short eopwd_imax;      // 0=auto;
    // the following cfgs will only take effect when bit31 is set to 1, and the other bits are used for controlling each threads of MMU
    unsigned int vm_en;             // 0=off; 1=on;
    unsigned int bm_en;             // 0=off; 1=on;
    unsigned int pm_en;             // 0=off; 1=on;
    unsigned int prefetch_dis;      // 0=on; 1=off;
    unsigned int mode;              // 0~4;
    unsigned int cways;             // 0~7;
    unsigned int rp_type;           // 0~1;
    unsigned int pm_h_idx;          // 0~5;
    unsigned int pm_w_idx;          // 0~1;
    unsigned int shuffle;           // 0=off; 1=on;
    unsigned int banks;             // 0~1;
    unsigned int sys_sz;            // 0~3;
    unsigned int tar_sz;            // 0~3;

    unsigned char crc_noreset;      // each bit for each channel: 1: CRC channel NOT reset at frame boundary
    unsigned char crc_dbg_mode;     // 0=auto=2,1=no_crc_calculation;  2=mini_crc_calculation;  3=full_crc_cal_part0;  4=full_crc_cal_part1;

    unsigned char dhub_chan_rd_delay; // RA_dHubReg_flowCtl:  rAlpha
    unsigned char dhub_chan_wd_delay; // RA_dHubReg_flowCtl:  wAlpha

    unsigned char ahb_idx_trigger;  // 0: idle 1: trigger 2: trigger for one frame case
};

typedef vpu_strm_icfg_t vdec_strm_icfg_t;

//---------------------------------------------------------------------------
// Performance Counters
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(vpu_perf_t)
{
    unsigned int mb_cnt;
    unsigned int slc_cnt;
    unsigned int f0_run;
    unsigned int f0_stl;
    unsigned int f1_run;
    unsigned int f1_stl;
    unsigned int f2_run;
    unsigned int f2_stl;
    unsigned int f3_run;
    unsigned int f3_stl;
    unsigned int p0_run;
    unsigned int p0_stl;
    unsigned int p0_bbl;
    unsigned int p0_sem;
    unsigned int p0_ins;
    unsigned int p1_run;
    unsigned int p1_stl;
    unsigned int p1_bbl;
    unsigned int p1_sem;
    unsigned int p1_ins;

    unsigned int axi_rd_cnt;
    unsigned int axi_wr_cnt;
};

typedef vpu_perf_t vdec_perf_t;
typedef vpu_perf_t venc_perf_t;
typedef vpu_perf_t vmeta_perf_t;

//---------------------------------------------------------------------------
// VPU Statistics
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(vpu_stat_t)
{
    unsigned int dummy;

};

typedef vpu_stat_t vmeta_stat_t;

//---------------------------------------------------------------------------
// VPU Stream Debug Object
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(vpu_strm_dbg_t)
{
    void *strm;
    unsigned int strm_id;
};

typedef vpu_strm_dbg_t vmeta_strm_dbg_t;

//---------------------------------------------------------------------------
// VPU Decoder Debug Object
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(vpu_obj_dbg_t)
{
    void *hw_obj;
    vpu_strm_dbg_t strm_dbg[4];
    unsigned int trace_line;
    char *trace_file;
    unsigned int sp_base;
    unsigned int sp_max;
    int dec_func_entered;
    int buf_func_entered;
    vpu_stat_t *stat;
    vpu_perf_t *perf;
    unsigned int agk_en;
    unsigned int agk_delay0;
    unsigned int agk_delay1;
    unsigned int agk_wait;
    unsigned int wdlog[64][6];
    unsigned int wdlog_idx;
};

typedef vpu_obj_dbg_t vmeta_dbg_t;

enum
{
    ICFG_CPU_PARSER     = 1 << 0,
    ICFG_CPU_SYNPRO     = 1 << 1,
    ICFG_CPU_P3CTL      = 1 << 2,
    ICFG_NO_MB          = 1 << 3,
    ICFG_NO_ASP         = 1 << 4,
    ICFG_CABAC_ONLY     = 1 << 5,
    ICFG_CPU_MEECTL     = 1 << 6,
};

//---------------------------------------------------------------------------
// Video Decoder Reserved EVENT/STATE IDs
//---------------------------------------------------------------------------
enum
{
    VDEC_TOTAL_API_EVENTS = VDEC_EVENT_RESERVED,

    // Decoder states

    VDEC_STATE_WAIT_FOR_START,
    VDEC_STATE_WAIT_FOR_SLICE_DONE,
    VDEC_STATE_WAIT_FOR_PICTURE_DONE,
    VDEC_STATE_WAIT_FOR_M3_DONE,

    // End of event definitions

    VDEC_TOTAL_EVENTS
};

TYPEDEF_STRUCT(vpu_perf_dbg_t)
{
    unsigned int hw_version;        // x.x.x.x
    unsigned int hw_build;          // usually BCD coding of RTL release date/time
    unsigned int fw_version;        // usually SVN revision number
    unsigned int fw_build;          // usually BCD coding of f/w build date
    unsigned int pic_num;           // picture number
    unsigned int frm_num;           // frame number
    unsigned short width;           // pic width
    unsigned short height;          // pic height
    unsigned char profile;          // profile id
    unsigned char level;            // level id
    unsigned char pic_type;         // 'I', 'P', 'B'
    unsigned char pic_struct;       // 'T', 'B', 'F'
    unsigned int num_ref : 5;       // number of reference frames, 0=16
    unsigned int dpb_size: 5;       // dpb size (in frames), 0=16
    unsigned int luma_bpp : 4;      // luma bits per pixel (8, 9, 10), 0=16
    unsigned int chroma_bpp : 4;    // chroma bits per pixel (8, 9, 10) , 0=16
    unsigned int field_idx : 4;     // field index within frame (usually 0 or 1)
    unsigned int is_idr : 1;        // is IDR picture
    unsigned int is_ref : 1;        // is reference picture
    unsigned int is_intl : 1;       // is interlaced stream
    unsigned int is_mbaff : 1;      // is MBAFF frame (H.264 only)
    unsigned int is_cabac : 1;      // is CABAC picture (H.264 only)
    unsigned int show_flag : 1;     // for vp8/vp9/av1 only
    unsigned int repeat_flag : 1;   // for av1

    unsigned int repeat_frame_idx;  // if repeat flag, repeat_frame_idx = existing_frame_idx.
    unsigned int size;              // current EOF bit counter - previous EOF bit counter
    unsigned int bits;              // current frame bits in MB level
    unsigned int mbs;               // number of MBs
    unsigned short slcs;            // number of slices
    unsigned short ints;            // number of interrupts

    unsigned int hw_cycles;         // sum of h/w cycles
    unsigned int int_lat;           // sum of interrupt latency
    unsigned int fw0_cycles;        // sum of f/w cycles before h/w is kicked off
    unsigned int fw1_cycles;        // sum of f/w cycles after h/w is kicked off
    unsigned int total_cycles;      // current EOF timestamp - previous EOF timestamp, ~= hw_cycles+fw0_cycles+int_lat

    unsigned int pc_scu;            // SCU performance counter
    unsigned int pc_spu;            // SPU performance counter
    unsigned int pc_mvu;            // MVU performance counter
    unsigned int pc_qtu;            // QTU performance counter
    unsigned int pc_vcu;            // VCU performance counter (total)
    unsigned int pc_vcu1;           // VCU performance counter (processing)
    unsigned int pc_ppu;            // PPU performance counter
    unsigned int pc_fcu;            // FCU performance counter

    unsigned int pc_fcu1;
    unsigned int pc_pfu;
    unsigned int pc_pfu1;

    unsigned int pc_spu1;
    unsigned int pc_spu2;
    unsigned int pc_spu3;

    unsigned int pc_qtu1;
    unsigned int pc_vcu2;
    unsigned int pc_ppu1;

    unsigned int read_bw;           // number of AXI read transactions (128b for v2g/g2, 64b for g1/h1/vmeta)
    unsigned int write_bw;          // number of AXI write transactions (128b for v2g/g2, 64b for g1/h1/vmeta)

    unsigned int rbuf_hold;
    unsigned int rbuf_release;
    unsigned int dbuf_hold;
    unsigned int dbuf_release;

    unsigned int sbuf_alloc;        // counter number of s/w allocating stream buffer
    unsigned int rbuf_alloc;        // counter number of s/w allocating reference buffer
    unsigned int dbuf_alloc;        // counter number of s/w allocating display buffer
    unsigned int sbuf_free;         // counter number of s/w freeing stream buffer
    unsigned int rbuf_free;         // counter number of s/w freeing reference buffer
    unsigned int dbuf_free;         // counter number of s/w freeing display buffer

    unsigned int sw_syncword;       // counter number of s/w handling event VDEC_EVENT_SYNCWORD
    unsigned int sw_sof;            // counter number of s/w handling event VDEC_EVENT_START_OF_FRAME
    unsigned int sw_sop;            // counter number of s/w handling event VDEC_EVENT_START_OF_PICTURE
    unsigned int sw_eop;            // counter number of s/w handling event VDEC_EVENT_END_OF_PICTURE
    unsigned int sw_eof;            // counter number of s/w handling event VDEC_EVENT_END_OF_FRAME
    unsigned int sw_switch;         // counter number of s/w handling event VDEC_EVENT_SWITCH_POINT

    unsigned int output_crc[8][10];
    unsigned int output_cnt[8];

    // end of data fields
    unsigned int _rsvd_;

    unsigned long long total_hw_cycles; // sum of h/w cycles of all frames
    unsigned long long total_fw_cycles; // sum of f/w cycles of all frames
    unsigned long long total_dec_cycles;// sum of decoding cycles of all frames
    unsigned long long total_int_lat;   // sum of interrupt latency cycles of all frames
    unsigned int decoded_frames;    // decoded frames
    unsigned int valid_frames;      // valid frames

    unsigned int pc_m3;
};

extern vpu_perf_dbg_t g_v2g_perf_obj;

extern vpu_perf_dbg_t *g_v2g_perf;

int vdec_debug(void *strm_cntxt, void *x);

#ifdef __cplusplus
}
#endif

#endif
