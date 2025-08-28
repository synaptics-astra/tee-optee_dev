#!/bin/bash

source build/header.rc
source build/install.rc
source build/module/toolchain/${CONFIG_TOOLCHAIN_APPLICATION}.rc

mod_dir=$(dirname "$0")
sysroot=${CONFIG_SYNA_SDK_OUT_TARGET_PATH}/${CONFIG_SYNA_SDK_OUT_SYSYROOT}
rootfs=${CONFIG_SYNA_SDK_OUT_TARGET_PATH}/${CONFIG_SYNA_SDK_OUT_ROOTFS}
opt_intermediate_dir=${CONFIG_SYNA_SDK_OUT_TARGET_PATH}/otp_program
mkdir -p ${opt_intermediate_dir}
tee_top_dir=${CONFIG_SYNA_SDK_OUT_TARGET_PATH}/${CONFIG_TEE_CLIENT_REL_PATH}

[ ${clean} -eq 1 ] && make clean -C ${mod_dir} BUILD_DIR="${opt_intermediate_dir}" && exit 0

if [ "is${CONFIG_RUNTIME_LINUX}" = "isy" ]; then
  cflags=${CONFIG_TOOLCHAIN_APPLICATION_CFLAG}
else
  cflags="${CONFIG_TOOLCHAIN_APPLICATION_CFLAG} --sysroot=${sysroot}"
fi
ldflags="${CONFIG_TOOLCHAIN_APPLICATION_LDFLAG}"
cc_name="${CONFIG_TOOLCHAIN_APPLICATION}gcc"

if [ "${CONFIG_TOOLCHAIN_APPLICATION_CC}" = "clang" ]; then
    clangcflag=${CONFIG_TOOLCHAIN_CLANG_CFLAG}
    clangldflag=${CONFIG_TOOLCHAIN_CLANG_LDFLAG}
    cflags="${clangcflag} ${cflags}"
    ldflags="${clangcflag} ${clangldflag} ${ldflags}"
    cc_name="clang"
fi

if [ "is${CONFIG_ANDROID_OS}" = "isy" ]; then
  ldflags="${ldflags} --sysroot=${sysroot}"
else
  ldflags="${ldflags} -L${sysroot}/lib -L${sysroot}/usr/lib"
fi

if [ "is${CONFIG_TZK}" = "isy" ]; then
  cflags="${cflags} -DCONFIG_TZK"
fi

make all -C ${mod_dir} CC="${cc_name}" E_CFLAG="${cflags}" E_LDFLAG="${ldflags}" \
     BUILD_DIR="${opt_intermediate_dir}" TEE_TOP_DIR="${tee_top_dir}"

mkdir -p ${rootfs}/usr/bin
make install -C ${mod_dir} APP_DIR="${rootfs}/usr/bin/" BUILD_DIR="${opt_intermediate_dir}"
