#!/bin/bash

echo "OTP_PROGRAM RELEASE"

source build/header.rc
source build/install.rc

mod_dir=$(dirname "$0")
dst_dir=${CONFIG_SYNA_SDK_REL_PATH}/drm/otp_program
INSTALL_D ${mod_dir} ${CONFIG_SYNA_SDK_REL_PATH}/drm

rm ${dst_dir}/release.sh
