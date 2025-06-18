/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#define _VPP_CMU_FTDC_CFG_C_

#include "vpp_cmu_ftdccfg.h"

/* Input signal to FTDC block - SD = 0 HD = 1 */
const UCHAR  gCmuFtdcInput = 0;

/* Flseh Tone Mode 0,1,2,3 */
const UCHAR  gCmuFtdcMode = 0;

/*
 * Flesh Tone detection Ranges
 */
/*
    CmuFtdc---[0] = SD
    CmuFtdc---[1] = HD */

const UINT16 gCmuFtdcInLow[2]  = {0x73,0x73};
const UINT16 gCmuFtdcOutLow[2] = {0x6E,0x6E};
const UINT16 gCmuFtdcInHigh[2] = {0x91,0x91};
const UINT16 gCmuFtdcOutHigh[2] = {0x96,0x96};
const UCHAR  gCmuFtdcYthLow[2] = {0x1E,0x1E};
const UCHAR  gCmuFtdcYthHigh[2] = {0xED,0xED};
const UINT16 gCmuFtdcOutChromaLow[2] = {0x64,0x64};
const UINT16 gCmuFtdcInChromaLow[2] = {0xEB,0xEB};
const UINT16 gCmuFtdcOutChromaHigh[2] = {0x663,0x663};
const UINT16 gCmuFtdcInChromaHigh[2] = {0x5DC,0x5DC};
const UINT16 gCmuFtdcHueWidth[2] = {0x333,0x333};
const UINT16 gCmuFtdcChromaWidth[2] = {0xF2,0xF2};

/*
 * Flesh Tone correction Coefficents and Offsets
 */
const UCHAR gCmuFtdcCoeffRegion[4][2][4] = {

    { /* FTDC Option 1 */
        {   /*SD */
            0x35,
            0x87,
            0x3b,
            0x87,
        },
        {   /*HD */
            0x35,
            0x87,
            0x3b,
            0x87,
        }
    },

    { /* FTDC Option 2 */
        {   /*SD */
            0x35,
            0x87,
            0x3b,
            0x87,
        },
        {   /*HD */
            0x35,
            0x87,
            0x3b,
            0x87,
        }
    },

    { /* FTDC Option 3 */
        {   /*SD */
            0x35,
            0x87,
            0x3b,
            0x87,
        },
        {   /*HD */
            0x35,
            0x87,
            0x3b,
            0x87,
        }
    },

    { /* FTDC Option 4 */
        {   /*SD */
            0x35,
            0x87,
            0x3b,
            0x87,
        },
        {   /*HD */
            0x35,
            0x87,
            0x3b,
            0x87,
        }
    }
};

const UCHAR gCmuFtdcOffsetRegion[4][2][2] = {

    { /* FTDC Option 1 */
        {   /*SD */
            0x0,
            0x0,
        },
        {   /*HD */
            0x0,
            0x0,
        }
    },

    { /* FTDC Option 2 */
        {   /*SD */
            0x44,
            0x44,
        },
        {   /*HD */
            0x44,
            0x44,
        }
    },

    { /* FTDC Option 3 */
        {   /*SD */
            0x7F,
            0x7F,
        },
        {   /*HD */
            0x7F,
            0x7F,
        }
    },

    { /* FTDC Option 4 */
        {   /*SD */
            0xFF,
            0xFF,
        },
        {   /*HD */
            0xFF,
            0xFF,
        }
    }
};
