# SPDX-License-Identifier: GPL-2.0
# Copyright (C) 2025 Synaptics Incorporated

export TA_DEV_KIT_DIR=../../../tee/optee_dev/dolphin/export-ta_arm64/
export CROSS_COMPILE=../../../toolchain/aarch64/gcc-arm-aarch64-linux-gnu-8.3/bin/aarch64-linux-gnu-

make -f Makefile.op O=out
