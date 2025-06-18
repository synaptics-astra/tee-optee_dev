#!/bin/bash

echo "VIPTA release script"
source build/header.rc
source build/chipset.rc

mod_dir=$(dirname "$0")
build_dir=${CONFIG_SYNA_SDK_OUT_TARGET_PATH}/${CONFIG_TA_BUILD_PATH}
mkdir -p ${build_dir}

#build all chips
for syna_chip_name in ${syna_chip_name_set[*]}
do
  if [ ${syna_chip_name} = "dvf120" ];then
    continue
  fi
  source build/chipparam.rc $syna_chip_name
  for i in ${!syna_chip_rev_set[*]}
  do
    syna_chip_rev=${syna_chip_rev_set[$i]}

    CONFIG_TEE_DEV_TEEI=${syna_teei[$i]}
    source build/module/toolchain/${CONFIG_TOOLCHAIN_TA}.rc

    make clean -C ${mod_dir} BUILD_DIR="${build_dir}"
    echo "Build VIP TA ${syna_chip_name}_${syna_chip_rev}"
    source ${mod_dir}/common.rc
  done
done
