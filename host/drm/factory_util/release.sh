#!/bin/bash

echo "FACTORY_UTIL RELEASE"

source build/header.rc
source build/install.rc

mod_dir=$(dirname "$0")
dst_dir=${CONFIG_SYNA_SDK_REL_PATH}/drm/factory_util
INSTALL_D ${mod_dir} ${CONFIG_SYNA_SDK_REL_PATH}/drm

rm ${dst_dir}/release.sh
