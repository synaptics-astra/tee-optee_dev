#!/bin/bash
#set -x

function usage()
{
  cat <<-EOF
    usage: ./build.sh <-p PRODUCT> [-m SDK_PATH] [-n JOB_NUM] [-t TEE_CATEGORY] [-c PROFILE_NAME] [-h]
    -p PRODUCT    Select chip name, can be platypus or dolphin
    -m SDK_PATH   Where is synaptics-sdk
    -n JOB_NUM    CPU number when compiling, default CPU number is 24.
    -t TEE_CATEGORY    TEE category, can be optee or tzk
    -c PROFILE_NAME    Profile name, for tzk use, can be platypus or dolphin or sl1680_poky_aarch64_rdk
    -h            Print help info.

    Example:      ./build.sh -p dolphin -m sdk_path -t optee
    Example:      ./build.sh -p dolphin -m sdk_path -t tzk -c sl1680_poky_aarch64_rdk
EOF
  exit 0
}


function make_optee()
{
  BUILD_DIR=${MY_DIR}/out

  if [ "${use_gnu_toolchain}" == "true" ]; then
    gnu_path="${SDK_PATH}/toolchain/aarch64/gcc-arm-aarch64-linux-gnu-8.3/bin"
    gnu_toolchain="${gnu_path}/aarch64-linux-gnu-"
    CROSS_COMPILE="${gnu_toolchain}"
  else
    poky_path="${SDK_PATH}/toolchain/oe/linux-x64/gcc-11.3.0-poky"
    OECORE_TARGET_SYSROOT="${poky_path}/sysroots/armv8a-poky-linux"
    OECORE_NATIVE_SYSROOT="${poky_path}/sysroots/x86_64-pokysdk-linux"
    poky_toolchain="${OECORE_NATIVE_SYSROOT}/usr/bin/aarch64-poky-linux"
    CROSS_COMPILE="${poky_toolchain}/aarch64-poky-linux-"
    LIBGCC_LOCATE_CFLAGS="--sysroot=${OECORE_TARGET_SYSROOT}"
  fi
  ARCH="aarch64"
  TEE_CATEGORY="OPTEE"

  if [ "${PRODUCT}" == "dolphin" ]; then
    CHIP_FULL_NAME="dolphin_a0"
    TA_DEV_KIT_DIR="${SDK_PATH}/tee/optee_dev/${PRODUCT}/export-ta_arm64"
  elif [ "${PRODUCT}" == "platypus" ]; then
    CHIP_FULL_NAME="platypus"
    TA_DEV_KIT_DIR="${SDK_PATH}/tee/optee_dev/${PRODUCT}/export-ta_arm64"
  fi

  cmdargs=""
  cmdargs="${cmdargs} TA_DEV_KIT_DIR=${TA_DEV_KIT_DIR}"
  cmdargs="${cmdargs} CHIP_FULL_NAME=${CHIP_FULL_NAME}"
  cmdargs="${cmdargs} CROSS_COMPILE=${CROSS_COMPILE}"
  cmdargs="${cmdargs} TEE_CATEGORY=${TEE_CATEGORY}"
  cmdargs="${cmdargs} ARCH=${ARCH}"
  cmdargs="${cmdargs} O=${BUILD_DIR}"
  cmdargs="${cmdargs} LIBGCC_LOCATE_CFLAGS=${LIBGCC_LOCATE_CFLAGS}"

  rm -rf ${BUILD_DIR}
  echo "${cmdargs}"
  eval $cmdargs make -j ${THREAD_NUM}
}

function make_tzk()
{
  BUILD_DIR=${MY_DIR}/out
  if [ "is$PROFILE_NAME" == "is" ]; then
    PROFILE_NAME=${PRODUCT}
  fi

  toolchain="${SDK_PATH}/toolchain/aarch32/gcc-linaro-arm-eabi-6.2.1/bin"
  TA_CROSS_COMPILE="${toolchain}/arm-eabi-"

  TEE_DEV_REL_PATH="${SDK_PATH}/out/${PROFILE_NAME}/target/tee_dev/"
  TEE_DEV_TEEI="GPD11_TZK530"
  TA_BASE_SCRIPTS_PATH="${SDK_PATH}/ta_app/build/"
  TA_REL_DIR=${BUILD_DIR}/release/
  TEE_CATEGORY="TZK"

  cmdargs=""
  cmdargs="${cmdargs} TEE_DEV_REL_PATH=${TEE_DEV_REL_PATH}"
  cmdargs="${cmdargs} TEE_DEV_TEEI=${TEE_DEV_TEEI}"
  cmdargs="${cmdargs} TA_BASE_SCRIPTS_PATH=${TA_BASE_SCRIPTS_PATH}"
  cmdargs="${cmdargs} TA_CROSS_COMPILE=${TA_CROSS_COMPILE}"
  cmdargs="${cmdargs} TA_REL_DIR=${TA_REL_DIR}"
  cmdargs="${cmdargs} TEE_CATEGORY=${TEE_CATEGORY}"
  cmdargs="${cmdargs} O=${BUILD_DIR}"
  echo "${cmdargs}"
  rm -rf ${BUILD_DIR}
  eval $cmdargs make -j ${THREAD_NUM}
}

function main()
{
  MY_DIR=`dirname $0`

  SDK_PATH=""
  PRODUCT="dolphin"
  TEE_TYPE="optee"

  PROFILE_NAME=""
  CPU_NUM="48"
  while getopts m:p:n:t:c:h arg
    do case $arg in
      m)  SDK_PATH=$OPTARG;;
      p)  PRODUCT=$OPTARG;;
      n)  CPU_NUM=$OPTARG;;
      t)  TEE_TYPE=$OPTARG;;
      c)  PROFILE_NAME=$OPTARG;;
      h)  usage
          exit 0;;
      *)  usage
          exit 1;;
    esac
  done

  if [ "is$PRODUCT" == "is" ]; then
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
  fi
}

main $@
