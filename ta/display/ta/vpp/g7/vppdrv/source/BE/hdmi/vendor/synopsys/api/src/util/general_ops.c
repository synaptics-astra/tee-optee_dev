// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */
/*
 * math_operations.c
 *
 *  Created on: Jan 28, 2015
 *      Author: arcxplorer
 */

#include "general_ops.h"

u8 double_is_equal(double a, double b){
	return (fabs(a-b) < 0.005);
}
