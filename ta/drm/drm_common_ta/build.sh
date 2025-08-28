#!/bin/bash

source build/header.rc
source build/chip.rc
source build/module/toolchain/${CONFIG_TOOLCHAIN_TA}.rc

mod_dir=$(dirname "$0")
build_dir=${CONFIG_SYNA_SDK_OUT_TARGET_PATH}/${CONFIG_TA_BUILD_PATH}
mkdir -p ${build_dir}

[ ${clean} -eq 1 ] && make clean -C ${mod_dir} BUILD_DIR="${build_dir}" && exit 0

source ${mod_dir}/common.rc
