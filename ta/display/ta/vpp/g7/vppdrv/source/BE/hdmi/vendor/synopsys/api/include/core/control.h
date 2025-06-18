// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

/**
 * @file
 * Core control module:
 * Product information
 * Power management
 * Interrupt handling
 *
 */
#ifndef CONTROL_H_
#define CONTROL_H_

#include "../hdmitx_dev.h"
#include "util/types.h"
/**
 * Initializes PHY and core clocks
 * @param baseAddr base address of controller
 * @param dataEnablePolarity data enable polarity
 * @param pixelClock pixel clock [10KHz]
 * @return TRUE if successful
 */
int control_Initialize(hdmi_tx_dev_t *dev);

/**
 * Go into standby mode: stop all clocks from all modules except for the CEC (refer to CEC for more detail)
 * @param baseAddr base address of controller
 * @return TRUE if successful
 */
int control_Standby(hdmi_tx_dev_t *dev);


///**
// * Mute controller interrupts
// * @param baseAddr base address of controller
// * @param value mask of the register
// * @return TRUE when successful
// */
//int control_InterruptMute(hdmi_tx_dev_t *dev, u8 value);

/** 
 * Clear all controller interrputs (except for hdcp)
 * @param baseAddr base address of controller
 * @return TRUE if successful
 */
int control_InterruptClearAll(hdmi_tx_dev_t *dev);

#endif				/* CONTROL_H_ */
