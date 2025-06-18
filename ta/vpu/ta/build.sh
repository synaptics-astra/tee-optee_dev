#!/bin/bash

MY_DIR=`dirname $0`

function usage()
{
cat << _End
################################################################################
Required Envs:
export THREAD_NUM=32
export CHIP_FULL_NAME=platypus
export BUILD_DIR=./out/

CONFIG_TZK=y
    export TEE_DEV_REL_PATH="AMP_SDK/out/profile_name/target/tee_dev/"
    export TEE_DEV_TEEI=GPD11_TZK530
    export TA_BASE_SCRIPTS_PATH="AMP_SDK/ta_app/build/"
    export TA_CROSS_COMPILE=arm-eabi-
    export TA_REL_DIR="./out/release/"
    export TEE_CATEGORY=TZK

CONFIG_OPTEE=y
    export TA_DEV_KIT_DIR="AMP_SDK/tee/optee_dev/export-ta_arm64"
    export CROSS_COMPILE=aarch64-linux-gnu-
    export TEE_CATEGORY=OPTEE
    export ARCH=aarch64
    export O="./out/"
################################################################################
_End
}

if [ -z "${THREAD_NUM}" ] || [ -z "${CHIP_FULL_NAME}" ] || \
   [ -z "${BUILD_DIR}" ]; then
    echo -e "Env THREAD_NUM, CHIP_FULL_NAME and BUILD_DIR must be set!\n"
    usage
    exit -1
fi

if [ -z "${CONFIG_TZK}" ] && [ -z "${CONFIG_OPTEE}" ]; then
    echo -e "Env CONFIG_TZK or CONFIG_OPTEE must be set!\n"
    usage
    exit -1
fi

# export THREAD_NUM=32
# export CHIP_FULL_NAME=platypus
# export BUILD_DIR=${MY_DIR}/out/

if [ ! -d ${BUILD_DIR} ]; then
    mkdir -p ${BUILD_DIR}
fi

if [ ! -z "${CONFIG_TZK}" ]; then
    # export TEE_DEV_REL_PATH="AMP_SDK/out/profile_name/target/tee_dev/"
    # export TEE_DEV_TEEI=GPD11_TZK530
    # export TA_BASE_SCRIPTS_PATH="AMP_SDK/ta_app/build/"
    # export TA_CROSS_COMPILE=arm-eabi-
    # export TA_REL_DIR=${BUILD_DIR}/../release/
    # export TEE_CATEGORY=TZK
    if [ -z "${TEE_DEV_REL_PATH}" ] || [ -z "${TEE_DEV_TEEI}" ] || \
       [ -z "${TA_BASE_SCRIPTS_PATH}" ] || [ -z "${TA_CROSS_COMPILE}" ] || \
       [ -z "${TA_REL_DIR}" ] || [ -z "${TEE_CATEGORY}" ]; then
        echo -e "Env TEE_DEV_REL_PATH, TEE_DEV_TEEI, TA_BASE_SCRIPTS_PATH,"\
                "TA_CROSS_COMPILE, TA_REL_DIR and TEE_CATEGORY must be set!\n"
        usage
        exit -1
    fi

    make clean -j ${THREAD_NUM}
    make all -j ${THREAD_NUM}
    make install
elif [ ! -z "${CONFIG_OPTEE}" ]; then
    # export TA_DEV_KIT_DIR="AMP_SDK/tee/optee_dev/export-ta_arm64"
    # export CROSS_COMPILE=aarch64-linux-gnu-
    # export TEE_CATEGORY=OPTEE
    # export ARCH=aarch64
    # export O=${BUILD_DIR}
    if [ -z "${TA_DEV_KIT_DIR}" ] || [ -z "${CROSS_COMPILE}" ] || \
       [ -z "${TEE_CATEGORY}" ] || [ -z "${ARCH}" ] || [ -z "${O}" ]; then
        echo -e "Env TA_DEV_KIT_DIR, CROSS_COMPILE, TEE_CATEGORY, ARCH, and O "\
                "must be set!\n"
        usage
        exit -1
    fi

    make clean -j ${THREAD_NUM}
    make -j ${THREAD_NUM}
fi