/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef __VPP_DV_OCSC_H__
#define __VPP_DV_OCSC_H__


#define VPP_MAX_OCSC_COEFF             9
#define VPP_MAX_OCSC_DENORM_MODES      1

#define    w32OCSC_R2YCOEF                {\
           UNSG32 sOCSC_R2YCOEF            : 16;\
           UNSG32 RSV_b16                  : 16;\
    }
#define     w32OCSC_R2YOFFSET_0          {\
            UNSG32 uOCSC_R2YOFF0         : 12;\
            UNSG32 uOCSC_R2YOFF1         : 12;\
            UNSG32 RSV_b24               :  8;\
          }
#define     w32OCSC_R2YOFFSET_1          {\
            UNSG32 uOCSC_R2YOFF2         : 12;\
            UNSG32 RSV_b12               : 20;\
          }
#define     w32OCSC_CTRL                              {\
            UNSG32 uOCSC_IFORMAT                          :  1;\
            UNSG32 uOCSC_OFORMAT                          :  3;\
            UNSG32 uOCSC_IBT1886                          :  1;\
            UNSG32 uOCSC_RESERVED                         :  3;\
            UNSG32 uOCSC_HACTIVE                          : 16;\
            UNSG32 uOCSC_INPSWAPSEL_Cb                    :  2;\
            UNSG32 uOCSC_INPSWAPSEL_Cr                    :  2;\
            UNSG32 uOCSC_INPSWAPSEL_Y                     :  2;\
            UNSG32 uOCSC_OUT_UVSWAP                       :  1;\
            UNSG32 RSV_b31                                :  1;\
          }
#define     w32OCSC_DENORM_RANGE_OVERONE              {\
            UNSG32 uOCSC_DENORM_RANGE_OVERONE             : 17;\
            UNSG32 RSV_b17                                : 15;\
          }

#define     w32OCSC_OCSC_SCALE2P                      {\
            UNSG32 uOCSC_SCALE2P_IPT2LMS_SCALE2P          :  5;\
            UNSG32 uOCSC_SCALE2P_LMS2RGB_SCALE2P          :  5;\
            UNSG32 uOCSC_SCALE2P_RGB2YUV_SCALE2P          :  5;\
            UNSG32 RSVDx70_b15                            : 17;\
        }

typedef union  T32OCSC_OCSC_SCALE2P
{
    UNSG32 u32;
    struct w32OCSC_OCSC_SCALE2P;
} T32OCSC_OCSC_SCALE2P;

typedef union T32OCSC_DENORM_RANGE_OVERONE
{
   UNSG32 u32;
   struct w32OCSC_DENORM_RANGE_OVERONE;
} T32OCSC_DENORM_RANGE_OVERONE;

typedef union  T32OCSC_R2YCOEF
{
    UNSG32 u32;
    struct w32OCSC_R2YCOEF;
}T32OCSC_R2YCOEF;

typedef union  T32OCSC_R2YOFFSET_0
{
    UNSG32 u32;
    struct w32OCSC_R2YOFFSET_0;
}T32OCSC_R2YOFFSET_0;

typedef union  T32OCSC_R2YOFFSET_1
{
    UNSG32 u32;
    struct w32OCSC_R2YOFFSET_1;
}T32OCSC_R2YOFFSET_1;

typedef union  T32OCSC_CTRL
{
    UNSG32 u32;
    struct w32OCSC_CTRL;
}T32OCSC_CTRL;

typedef enum VPP_DV_OCSC_MODES_T
{
    VPP_DV_OCSC_MODE_MIN = 0,
    VPP_DV_OCSC_MODE_BYPASS,
    VPP_DV_OCSC_MODE_RGB444_TO_YUV444,
    VPP_DV_OCSC_MODE_RGB444_TO_YUV422,
    VPP_DV_OCSC_MODE_RGB444_TO_YUV444_R2020,
    VPP_DV_OCSC_MODE_RGB444_TO_YUV422_R2020,
    VPP_DV_OCSC_MODE_MAX
}VPP_DV_OCSC_MODE;



typedef enum VPP_DV_OCSC_DENORM_MODE_T
{
    VPP_DV_OCSC_DENORM
}VPP_DV_OCSC_DENORM_MODE;

typedef enum VPP_DV_OCSC_IFORMAT_T
{
    VPP_DV_OCSC_IFORMAT_RGB444 = 1
}VPP_DV_OCSC_IFORMAT_TYPE;

typedef enum VPP_DV_OCSC_OFORMAT_T
{
    VPP_DV_OCSC_OFORMAT_YUV444  = 0,
    VPP_DV_OCSC_OFORMAT_YUV422  = 1,
    VPP_DV_OCSC_OFORMAT_RGB444  = 2,
    VPP_DV_OCSC_OFORMAT_RESERVE = 3,
}VPP_DV_OCSC_OFORMAT_TYPE;

INT VPP_BE_VOP_LoadCscCoeff(VPP_OBJ *vpp_obj, INT ColorFmt, INT ActWidth);

#endif
