#ifndef  __VPU_REG_MEMMAP_H
#define __VPU_REG_MEMMAP_H

#ifdef __cplusplus
  extern "C" {
#endif

#define BERLIN_DOLPHIN              0x3130
#define BERLIN_PLATYPUS             0x3140
#define BERLIN_DOLPHIN_A0           0x3150

#if (BERLIN_CHIP_VERSION == BERLIN_DOLPHIN || BERLIN_CHIP_VERSION == BERLIN_DOLPHIN_A0)

#define MEMMAP_CHIP_CTRL_REG_BASE       0xF7EA0000
#define MEMMAP_CHIP_CTRL_REG_SIZE       0x10000

#define MEMMAP_V4G_REG_BASE             0xF7600000
#define MEMMAP_V4G_REG_SIZE             0x200000
#define MEMMAP_H1_REG_BASE              0xF7000000
#define MEMMAP_H1_REG_SIZE              0x400000

#define RA_Gbl_decoderClk               0x074C
#define RA_Gbl_encoderClk               0x0754
#endif

#if (BERLIN_CHIP_VERSION == BERLIN_PLATYPUS)
#define MEMMAP_CHIP_CTRL_REG_BASE       0xF7EA0000
#define MEMMAP_CHIP_CTRL_REG_SIZE       0x10000

#define MEMMAP_V4G_REG_BASE             0xF7600000
#define MEMMAP_V4G_REG_SIZE             0x100000
#define MEMMAP_H1_REG_BASE              0xF7000000
#define MEMMAP_H1_REG_SIZE              0x1000

#define RA_Gbl_decoderClk               0x074C
#define RA_Gbl_encoderClk               0x0754
#endif

#ifdef __cplusplus
  }
#endif
#endif /* __VPU_REG_MEMMAP_H */
