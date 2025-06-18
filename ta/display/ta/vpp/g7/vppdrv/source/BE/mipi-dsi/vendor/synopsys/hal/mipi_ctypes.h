/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef MIPI_CTYPES_H_
#define MIPI_CTYPES_H_

#include "ctypes.h"

//__SIZE_TYPE__ will be defined in toolchain headerfile stddef.h
//4bytes of 32bit and 8bytes for 64bit
#ifndef __SIZE_TYPE__
#ifndef __size_t_defined
typedef unsigned int size_t;
#endif
#endif

/**
 * enum irqreturn
 * @IRQ_NONE     interrupt was not from this device or was not handled
 * @IRQ_HANDLED     interrupt was handled by this device
 * @IRQ_WAKE_THREAD    handler requests to wake the handler thread
 */
enum irqreturn {
    IRQ_NONE        = (0 << 0),
    IRQ_HANDLED     = (1 << 0),
    IRQ_WAKE_THREAD     = (1 << 1),
};
typedef enum irqreturn irqreturn_t;

#endif	/* MIPI_CTYPES_H_ */
