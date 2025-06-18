/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_FE_PATGEN_H_
#define _VPP_FE_PATGEN_H_


/*-----------------------------------------------------------------------------
 * Enums, Structures and Unions
 *-----------------------------------------------------------------------------
 */
typedef enum {
    PATGEN_MODE_SOLID     = 0,
    PATGEN_MODE_INCREMENT = 1,
    PATGEN_MODE_INVERSE   = 2,
    PATGEN_MODE_RANDOM    = 3,
} VPP_PATGEN_MODE_ENUM;

typedef struct tagVPP_PATGEN_TIMING
{
    int hTot;
    int vTot;
    int hRes;
    int vRes;
    int hSw;
    int vSw;
    int hSt;
    int vSt;

} VPP_PATGEN_TIMING, *PVPP_PATGEN_TIMING;

typedef struct tagVPP_PATGEN_DATA_CTRL
{
    int hPith;
    int vPith;
    int hColPith; // 0x00BBGGRR
    int vColPith; // 0x00BBGGRR
    int ColSeed1; // 0x00BBGGRR
    int ColSeed2; // 0x00BBGGRR

} VPP_PATGEN_DATA_CTRL, *PVPP_PATGEN_DATA_CTRL;


/******* APIs *************/
INT FE_PATGEN_SetDefaults(VPP_OBJ* vpp_obj);
INT FE_PATGEN_SetTiming(VPP_OBJ* vpp_obj, VPP_PATGEN_TIMING *pTiming);
INT FE_PATGEN_SetMode(VPP_OBJ* vpp_obj, int hmodeID, int vmodeID);
INT FE_PATGEN_SetDataCtrl(VPP_OBJ* vpp_obj, VPP_PATGEN_DATA_CTRL *pDataCtrl);
INT FE_PATGEN_SetFrameType(VPP_OBJ* vpp_obj, int type, int cadence, int rff);
INT FE_PATGEN_Enable(VPP_OBJ* vpp_obj, int flag);


#endif // <- _VPP_FE_PATGEN_H_

