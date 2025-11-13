#!/bin/sh

make HOST_CROSS_COMPILE=arm-linux-gnueabihf- TEEC_EXPORT=`pwd`/../optee_client/out/export/usr/ TA_DEV_KIT_DIR=`pwd`/../export-ta_arm64/   TA_CROSS_COMPILE=aarch64-linux-gnu- clean
make HOST_CROSS_COMPILE=arm-linux-gnueabihf- TEEC_EXPORT=`pwd`/../optee_client/out/export/usr/ TA_DEV_KIT_DIR=`pwd`/../export-ta_arm64/   TA_CROSS_COMPILE=aarch64-linux-gnu- 
