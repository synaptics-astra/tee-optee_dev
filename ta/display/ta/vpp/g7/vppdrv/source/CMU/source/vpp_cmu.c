/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#define _VPP_CMU_C_

#if defined(VPP_IN_TRUST_ZONE)
#include "com_type.h"
#endif

#include "vpp.h"
#include "vpp_module.h"
#include "vpp_cmu_common.h"
#include "vpp_cmu.h"
#include "vpp_cmu_ace.h"
#include "vpp_cmu_br.h"
#include "vpp_cmu_huesat.h"
#include "vpp_cmu_ftdc.h"
#include "vpp_cmu_ftdccfg.h"
#include "vpp_cmu_ee.h"
#include "vpp_mp.h"
#include "vpp_cmu_tg.h"

/***************************************************************************************
 * Description: reset CMU
 * PARAMS: none
 * RETURN:  1 - succeed
 *                others - failed to load CMU register default values to BCM buffer
 ***************************************************************************************/
INT VPP_CMU_Reset(VPP_OBJ *vpp_obj)
{
#ifdef ENABLE_EECMU
    VPP_CMU_ACE_SetDefaults(vpp_obj, CHAN_MAIN, VPP_CMU_YUV_709);

    VPP_CMU_BR_SetDefaults(vpp_obj, CHAN_MAIN, VPP_CMU_YUV_709);

    VPP_CMU_FTDC_SetDefaults(vpp_obj, VPP_CMU_YUV_709);

    VPP_CMU_HUESAT_SetDefaults(vpp_obj, CHAN_MAIN, VPP_CMU_YUV_709);

    VPP_CMU_SetDefault(vpp_obj);
#endif

    return (MV_VPP_OK);
}

/***************************************************************************************
 * Description: set ACE mode
 * PARAMS: Channel - channel selection
 *               Mode - ACE intensity
 * RETURN:  1 - succeed
 *                others - failed to load ACE mode to BCM buffer
 ***************************************************************************************/
INT VPP_CMU_SetAceLevel(VPP_OBJ *vpp_obj, INT Channel, INT AceMode, INT Range)
{
    PVPP_CMU_ACE_PARAMS pAceParams = NULL;

    if (Range < VPP_CMU_ACE_RANGE_0_255 || Range >= VPP_CMU_ACE_MAX_RANGE)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->cmu.cmu_enabled == 0) {
        VPP_CMU_Enable(vpp_obj,TRUE);
    }

    VPP_CMU_ACE_SetParams(vpp_obj, Channel, pAceParams, Range, AceMode, 1);

    return (MV_VPP_OK);
}

/***************************************************************************************
 * Description: set the level of brightness enhancement
 * PARAMS: Channel - channel selection
 *               BrLevel - brightness enhancement level
 * RETURN:  1 - succeed
 *                others - failed to load brightness enhancement level to BCM buffer
 ***************************************************************************************/
INT VPP_CMU_SetBrightnessLevel(VPP_OBJ *vpp_obj, INT Channel, INT BrLevel)
{

    if (vpp_obj->cmu.cmu_enabled == 0) {
        VPP_CMU_Enable(vpp_obj,TRUE);
    }
    VPP_CMU_BR_SetBrightnessLevel(vpp_obj, Channel, BrLevel);

    return (MV_VPP_OK);
}

/***************************************************************************************
 * Description: set ICR mode
 * PARAMS: Channel - channel selection
 *               Mode - ICR mode
 * RETURN:  1 - succeed
 *                others - failed to load ICR mode to BCM buffer
 ***************************************************************************************/
INT VPP_CMU_SetIcrMode(VPP_OBJ *vpp_obj, INT Channel, INT Mode)
{
    if (vpp_obj->cmu.cmu_enabled == 0 && Mode != VPP_CMU_ICR_MODE_OFF) {
        VPP_CMU_Enable(vpp_obj,TRUE);
    }
    switch (Mode) {
        case VPP_CMU_ICR_MODE_OFF:
            VPP_CMU_HUESAT_SetHSIcrMode(vpp_obj, Channel, VPP_CMU_ICR_MODE1, NULL);
            break;

        case VPP_CMU_ICR_MODE_SKY:
            VPP_CMU_HUESAT_SetHSIcrMode(vpp_obj, Channel, VPP_CMU_ICR_MODE2, NULL);
            break;

        case VPP_CMU_ICR_MODE_GRASS:
            VPP_CMU_HUESAT_SetHSIcrMode(vpp_obj, Channel, VPP_CMU_ICR_MODE3, NULL);
            break;

        case VPP_CMU_ICR_MODE_SKYGRASS:
            VPP_CMU_HUESAT_SetHSIcrMode(vpp_obj, Channel, VPP_CMU_ICR_MODE4, NULL);
            break;

        case VPP_CMU_ICR_MODE_VIVID:
            VPP_CMU_HUESAT_SetHSIcrMode(vpp_obj, Channel, VPP_CMU_ICR_MODE5, NULL);
            break;

        default:
            return (MV_VPP_EBADPARAM);
    }

    return (MV_VPP_OK);
}

/***************************************************************************************
 * Description: enable or disable taper on the flesh tone region
 * PARAMS: Channel - channel selection
 *               enable - 1: enable 0:disable
 * RETURN:  1 - succeed
 *                others - failed to load ICR mode to BCM buffer
 ***************************************************************************************/
INT VPP_CMU_EnableFleshToneTaper(VPP_OBJ *vpp_obj, INT Channel, INT enable)
{

    if (enable != 0) {
        if (vpp_obj->cmu.cmu_enabled == 0) {
            VPP_CMU_Enable(vpp_obj,TRUE);
        }
        VPP_CMU_HUESAT_SetHueSatControls(vpp_obj, Channel, VPP_CMU_FLESH_TAPER_TONE, 1);
    } else
        VPP_CMU_HUESAT_SetHueSatControls(vpp_obj, Channel, VPP_CMU_FLESH_TAPER_TONE, 0);

    return (MV_VPP_OK);
}

/***************************************************************************************
 * Description: set FTDC mode, main and PIP share the same setting
 * PARAMS: Mode - FTDC mode
 * RETURN:  1 - succeed
 *                others - failed to load FTDC mode to BCM buffer
 ***************************************************************************************/
INT VPP_CMU_SetFtdcMode(VPP_OBJ *vpp_obj, INT Channel, INT Mode)
{

    if (vpp_obj->cmu.cmu_enabled == 0) {
        VPP_CMU_Enable(vpp_obj,TRUE);
    }
    if (Mode == FTDC_OFF)
        VPP_CMU_FTDC_Enable(vpp_obj, Channel, FALSE);
    else {
        VPP_CMU_FTDC_Enable(vpp_obj, Channel, TRUE);
        VPP_CMU_FTDC_SetMode(vpp_obj, Mode);/*the same for SD and HD input format*/
    }

    return (MV_VPP_OK);
}

/***************************************************************************************
 * Description: set color level
 * PARAMS: Channel - channel selection
 *               ColorSel - color selection
 *               ColorLev - Color level (0 - 100)
 * RETURN:  1 - succeed
 *                others - failed to load color level to BCM buffer
 ***************************************************************************************/
INT VPP_CMU_SetColorLevel(VPP_OBJ *vpp_obj, INT Channel, INT ColorSel, INT16 ColorLev)
{
    float SaturVal;

    if (vpp_obj->cmu.cmu_enabled == 0) {
        VPP_CMU_Enable(vpp_obj,TRUE);
    }
    /*map [0, 100] to register value [0, 128], saturation value [0. 2] */
    SaturVal = 64 + 32 * (ColorLev - 50) / 25;

    /*set icr mode to default, recover the axes positions to default*/
    if (ColorSel != VPP_CMU_COLOR_ALL)
        VPP_CMU_HUESAT_SetHSIcrMode(vpp_obj, Channel, VPP_CMU_ICR_RECOVER_AXES, NULL);

    switch (ColorSel) {
        case VPP_CMU_COLOR_RED:
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_SATCHANGE, 4, (UINT16)SaturVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_SATCHANGE, 5, (UINT16)SaturVal);
            break;

        case VPP_CMU_COLOR_GREEN:
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_SATCHANGE, 8, (UINT16)SaturVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_SATCHANGE, 9, (UINT16)SaturVal);
            break;

        case VPP_CMU_COLOR_BLUE:
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_SATCHANGE, 0, (UINT16)SaturVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_SATCHANGE, 1, (UINT16)SaturVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_SATCHANGE, 12, (UINT16)SaturVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_SATCHANGE, 13, (UINT16)SaturVal);
            break;

        case VPP_CMU_COLOR_CYAN:
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_SATCHANGE, 10, (UINT16)SaturVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_SATCHANGE, 11, (UINT16)SaturVal);
            break;

        case VPP_CMU_COLOR_YELLOW:
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_SATCHANGE, 6, (UINT16)SaturVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_SATCHANGE, 7, (UINT16)SaturVal);
            break;

        case VPP_CMU_COLOR_MAGENTA:
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_SATCHANGE, 2, (UINT16)SaturVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_SATCHANGE, 3, (UINT16)SaturVal);
            break;

        case VPP_CMU_COLOR_ALL:
            /*set globle saturation change*/
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_GLOBALSAT_CHANGE, 0, (UINT16)SaturVal);
            break;
    }

    return (MV_VPP_OK);
}

/***************************************************************************************
 * Description: set color hue
 * PARAMS: Channel - channel selection
 *               ColorSel - color selection
 *               ColorHue - Color hue (0 - 100)
 * RETURN:  1 - succeed
 *                others - failed to load color level to BCM buffer
 ***************************************************************************************/
INT VPP_CMU_SetColorHue(VPP_OBJ *vpp_obj, INT Channel, INT ColorSel, INT16 ColorHue)
{
    float HueVal;

    if (vpp_obj->cmu.cmu_enabled == 0) {
        VPP_CMU_Enable(vpp_obj,TRUE);
    }

    /*map [0, 100] to register value [0, 30] */
    HueVal = ColorHue * 30 /100;

    /*set icr mode to default, recover the axes positions to default*/
    VPP_CMU_HUESAT_SetHSIcrMode(vpp_obj, Channel, VPP_CMU_ICR_RECOVER_AXES, NULL);

    switch (ColorSel) {
        case VPP_CMU_COLOR_RED:
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 4, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 5, (UINT16)HueVal);
            break;

        case VPP_CMU_COLOR_GREEN:
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 8, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 9, (UINT16)HueVal);
            break;

        case VPP_CMU_COLOR_BLUE:
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 0, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 1, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 12, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 13, (UINT16)HueVal);
            break;

        case VPP_CMU_COLOR_CYAN:
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 10, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 11, (UINT16)HueVal);
            break;

        case VPP_CMU_COLOR_YELLOW:
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 6, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 7, (UINT16)HueVal);
            break;

        case VPP_CMU_COLOR_MAGENTA:
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 2, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 3, (UINT16)HueVal);
            break;

        case VPP_CMU_COLOR_ALL:
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 0, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 1, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 2, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 3, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 4, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 5, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 6, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 7, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 8, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 9, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 10, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 11, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 12, (UINT16)HueVal);
            VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel, VPP_CMU_HUESAT_HUECHANGE, 13, (UINT16)HueVal);
            break;
    }

    return (MV_VPP_OK);
}


INT VPP_CMU_Enable(VPP_OBJ *vpp_obj,INT enable)
{
    return CMU_Enable(vpp_obj,enable);
}

INT VPP_CMU_TG_Config(VPP_OBJ *vpp_obj, int width, int height)
{
    int ret;

    ret = CMU_ConfigTG(vpp_obj, width,  height);

    return ret;
}

INT VPP_CMU_TG_Start(VPP_OBJ *vpp_obj)
{
    return  CMU_TG_Start(vpp_obj);
}

INT VPP_CMU_TG_Clear(VPP_OBJ *vpp_obj)
{
    return  CMU_TG_Clear(vpp_obj);
}
