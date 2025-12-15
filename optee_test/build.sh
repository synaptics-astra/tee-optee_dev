#!/bin/sh

#make TA_DEV_KIT_DIR=`pwd`/../export-ta_arm64/  CROSS_COMPILE_TA=aarch64-linux-gnu- CROSS_COMPILE_HOST=arm-linux-gnueabihf- OPTEE_CLIENT_EXPORT=`pwd`/../optee_client/out/export/usr/ clean
make TA_DEV_KIT_DIR=`pwd`/../export-ta_arm64/  CROSS_COMPILE_TA=aarch64-linux-gnu- CROSS_COMPILE_HOST=arm-linux-gnueabihf- OPTEE_CLIENT_EXPORT=`pwd`/../optee_client/out/export/usr/ CFG_PKCS11_TA=y
