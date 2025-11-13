#!/bin/sh
export WITH_TEEACL=0
make CROSS_COMPILE=arm-linux-gnueabihf- clean
make CROSS_COMPILE=arm-linux-gnueabihf-
