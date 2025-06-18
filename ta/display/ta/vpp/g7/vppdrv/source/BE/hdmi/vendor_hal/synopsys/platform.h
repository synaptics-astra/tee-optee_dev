// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include "util/types.h"

/*******************************
 * HDMI Tx Support functions   *
 *******************************/

/** Set up oscillator for audio
 * @param baseAddr base address of controller
 * @param value: audio clock [Hz]
 * @return TRUE if successful
 */
uint16_t audio_clock(double value, uint16_t over_sampling_factor);

/** Set up oscillator for video
 * @param baseAddr base address of controller
 * @param value: pixel clock [10KHz]
 * @param cd color depth
 * @return TRUE if successful
 */
uint16_t pixel_clock(double freq);

#endif /* PLATFORM_H_ */
