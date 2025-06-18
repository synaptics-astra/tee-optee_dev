/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _FASTLOGO_DEFS_H_
#define _FASTLOGO_DEFS_H_

//Introduce configuration macro needed for fastlogo.ta
//#define CONFIG_VPP_ENABLE_INTERRUPT_ON_PUSHFRAME
#define CONFIG_VPP_DISABLE_OFFLINE_SCALER_BUFFER
#define CONFIG_VPP_ENABLE_AVPLL_IN_CA
#define CONFIG_VPP_ENABLE_INTERRUPTS_IN_CA

#define CONFIG_VPP_DISABLE_CMU
#define CONFIG_VPP_DISABLE_GDETEE

#define CONFIG_VPP_DISALE_LOG
#define CONFIG_VPP_ENABLE_DELAY_BY_LOOP

#define VPP_HDMI_FORCE_CONFIGURE_WITHOUT_SINK
/* Currently recyle is not implemented by user of fastlogo.ta,
 * So enable recycle immediately, instead of defering until user recycle */
#define CONFIG_VPP_RECYLE_LOCAL_VBUF_IMMEDIATE


//un-define configuration macro not-needed for fastlogo.ta
#undef HDMI_SRV_SUPPORT
#undef CONFIG_AMP_HDMI_HDCP_2X
#undef CONFIG_HDMI_CEC_SUPPORT
#undef VPP_VMX_ENABLE


#undef VPP_ENABLE_FRAME_CAPTURE

#undef CONFIG_AMP_VPP_NGPTV
#undef CONFIG_AMP_VPP_DOLBY_VISION
#undef VPP_VMX_NOTIFY_FRAMEWORK
#undef CONFIG_HDR_SUPPORT
#undef BT2020_SUPPORT
#undef CONFIG_BT2020_SUPPORT


#undef CONFIG_AMP_HDMI_SERVICE
#undef CONFIG_MV_AMP_HDMI_SPD_SUPPORT
#undef CONFIG_VPP_ENV_SA_SUPPORT
#undef CONFIG_DEINT_ENABLE

#undef CONFIG_AMP_HDMI_AUDIO_RETURN
#undef EARC_RX_ENABLED
#undef CONFIG_VPP_PTM_REF_UNREF_ENABLE

#endif //_FASTLOGO_DEFS_H_
