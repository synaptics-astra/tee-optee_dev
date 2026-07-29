#!/bin/bash
#set -x

function usage()
{
  cat <<-EOF
    usage: ./build.sh <[-m SDK_PATH] [-p PROFILE_NAME]  [-t TEE_CATEGORY] [-n JOB_NUM] [-h]
    -m SDK_PATH   Where is synaptics-sdk, use it to find toolchain path in optee
    -n JOB_NUM    CPU number when compiling, default CPU number is 24.
    -t TEE_CATEGORY    TEE category, can be optee or tzk
    -p PROFILE_NAME   Profile name. vssdk/out/<profile_name>
                      tzk: ${vssdk}/out/<profile_name>
                          see tz_env.mk. can be platypus or dolphin or sl1680_poky_aarch64_rdk.
                      optee: poky pofile will use poky toolchain, other profile will use gnu toolchain.
    -e            Encrypt tzk ta
    -v            TA api version. can be v1, v2, v1v2.
                  AMP API use v1, v4l2 API use v2, v1v2 means support both v1 and v2.
    -d            Enable link debug firmware
    -h            Print help info.

    Example:      ./build.sh -m sdk_path -p dolphin -t optee (-v v1v2, -d)
    Example:      ./build.sh -m sdk_path -p dolphin -t tzk (-v v1v2, -d)
    Example:      ./build.sh -m sdk_path -p vs640_a0_linux_baseline_evk  -t tzk
    Example:      ./build.sh -m sdk_path -p vs640_a0_linux_baseline_evk -t tzk -e
EOF
  exit 0
}


function make_optee()
{
  BUILD_DIR=${MY_DIR}/out
  oe_ver="13.3.0"
  mcpu="cortexa73"

  if [[ "${PROFILE_NAME}" == *"poky"* ]]; then
    use_gnu_toolchain="false"
  else
    use_gnu_toolchain="true"
  fi

  if [[ "${PROFILE_NAME}" == *"680"* || "${PROFILE_NAME}" == *"dolphin"* ]]; then
    CHIP_FULL_NAME="dolphin_a0"
    TA_DEV_KIT_DIR="${SDK_PATH}/tee/optee_dev/dolphin/export-ta_arm64"
  elif [[ "${PROFILE_NAME}" == *"640"* || "${PROFILE_NAME}" == *"platypus"* ]]; then
    CHIP_FULL_NAME="platypus"
    TA_DEV_KIT_DIR="${SDK_PATH}/tee/optee_dev/platypus/export-ta_arm64"
  else
    echo "ERROR: unsupported PROFILE_NAME: ${PROFILE_NAME}"
    exit 1
  fi

  if [ "${use_gnu_toolchain}" == "true" ]; then
    gnu_path="${SDK_PATH}/toolchain/aarch64/gcc-arm-aarch64-linux-gnu-8.3/bin"
    gnu_toolchain="${gnu_path}/aarch64-linux-gnu-"
    CROSS_COMPILE="${gnu_toolchain}"
  else
    poky_path="${SDK_PATH}/toolchain/oe/linux-x64/gcc-${oe_ver}-poky"
    OECORE_TARGET_SYSROOT="${poky_path}/sysroots/${mcpu}-poky-linux"
    OECORE_NATIVE_SYSROOT="${poky_path}/sysroots/x86_64-pokysdk-linux"
    poky_toolchain="${OECORE_NATIVE_SYSROOT}/usr/bin/aarch64-poky-linux"
    CROSS_COMPILE="${poky_toolchain}/aarch64-poky-linux-"
    LIBGCC_LOCATE_CFLAGS="--sysroot=${OECORE_TARGET_SYSROOT}"
  fi
  ARCH="aarch64"

  cmdargs=""
  cmdargs="${cmdargs} TA_DEV_KIT_DIR=${TA_DEV_KIT_DIR}"
  cmdargs="${cmdargs} CHIP_FULL_NAME=${CHIP_FULL_NAME}"
  cmdargs="${cmdargs} CROSS_COMPILE=${CROSS_COMPILE}"
  cmdargs="${cmdargs} ARCH=${ARCH}"
  cmdargs="${cmdargs} O=${BUILD_DIR}"
  cmdargs="${cmdargs} LIBGCC_LOCATE_CFLAGS=${LIBGCC_LOCATE_CFLAGS}"
  if [ "${DEBUG_FW}" == "1" ]; then
    cmdargs="${cmdargs} DEBUG_FW=1"
  fi
  if [ ! -z "${COMPAT_VERS}" ]; then
    cmdargs="${cmdargs} COMPAT_VERS=${COMPAT_VERS}"
  fi

  rm -rf ${BUILD_DIR}
  echo "${cmdargs}"
  eval $cmdargs make -f Makefile.op -j ${THREAD_NUM}
}


function make_tzk()
{
  SDK_OUT_TARGET="${SDK_PATH}/out/${PROFILE_NAME}/target"
  BUILD_OUT=${SDK_OUT_TARGET}/ta/intermediate/libvmeta.ta

  cmdargs=""
  cmdargs="${cmdargs} VSSDKTOP=${SDK_PATH}"
  cmdargs="${cmdargs} PRODUCT=${PROFILE_NAME}"
  if [ "${DEBUG_FW}" == "1" ]; then
    cmdargs="${cmdargs} DEBUG_FW=1"
  fi
  if [ ! -z "${COMPAT_VERS}" ]; then
    cmdargs="${cmdargs} COMPAT_VERS=${COMPAT_VERS}"
  fi

  rm -rf ${BUILD_OUT}
  echo "${cmdargs}"

  eval $cmdargs make -j ${THREAD_NUM}
  if [ $? -eq 0 ]; then
    eval $cmdargs make install
  fi
  echo "### make && install libvmeta.ta done"
}


function encrypt_tzk()
{
  SDKTOP=${SDK_PATH}
  SDK_OUT_TARGET="${SDK_PATH}/out/${PROFILE_NAME}/target"

  if [[ "${PROFILE_NAME}" == *"680"* || "${PROFILE_NAME}" == *"dolphin"* ]]; then
    rel_dir="dolphin/A0"
  elif [[ "${PROFILE_NAME}" == *"640"* || "${PROFILE_NAME}" == *"platypus"* ]]; then
    rel_dir="platypus/A0"
  else
    echo "ERROR: unsupported PROFILE_NAME: ${PROFILE_NAME}"
    exit 1
  fi

  TA_ENC_RESULT=${SDK_OUT_TARGET}/ta/enc/${rel_dir}/libvmeta.ta
  TA_ENC_BUILD_STAMP=${SDK_PATH}/out/${PROFILE_NAME}/build/TA_ENC
  TA_ENC_MODULE_PATH=${SDK_PATH}/build/module/ta_enc

  rm -rf ${TA_ENC_BUILD_STAMP}
  rm -rf ${TA_ENC_RESULT}

  mv ${TA_ENC_MODULE_PATH}/ta_list.rc ${TA_ENC_MODULE_PATH}/ta_list.rc.bak
  cp build/tzk/ta_list.rc ${TA_ENC_MODULE_PATH}/
  pushd ${SDKTOP}
  make ta_enc
  popd
  mv ${TA_ENC_MODULE_PATH}/ta_list.rc.bak ${TA_ENC_MODULE_PATH}/ta_list.rc

}

function main()
{
  MY_DIR=`dirname $0`

  SDK_PATH=""
  CHIP="dolphin"
  TEE_TYPE="optee"

  PROFILE_NAME=""
  CPU_NUM="48"
  ENCRYPT_TZK="false"
  COMPAT_VERS=""
  DEBUG_FW=0
  while getopts m:p:n:t:v:deh arg
    do case $arg in
      m)  SDK_PATH=$OPTARG;;
      n)  CPU_NUM=$OPTARG;;
      t)  TEE_TYPE=$OPTARG;;
      p)  PROFILE_NAME=$OPTARG;;
      e)  ENCRYPT_TZK=true;;
      d)  DEBUG_FW=1;;
      v)  COMPAT_VERS=$OPTARG;;
      h)  usage
          exit 0;;
      *)  usage
          exit 1;;
    esac
  done

  if [ "is$PROFILE_NAME" == "is" ]; then
    usage
    exit 1
  fi

  if [ "is$TEE_TYPE" == "is" ]; then
    usage
    exit 1
  fi

  if [ "is$SDK_PATH" == "is" ]; then
    usage
    exit 1
  fi

  if [ "${TEE_TYPE}" == "optee" ]; then
    make_optee
  elif [ "${TEE_TYPE}" == "tzk" ]; then
    make_tzk
    if [ "${ENCRYPT_TZK}" == "true" ]; then
      encrypt_tzk
    fi
  fi
}

main $@
