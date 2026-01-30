*.compile tsp/m2m ta
1.cd tsp/m2m
2.make -f Makefile.op CONFIG_GENX_ENABLE=y CROSS_COMPILE=aarch64-linux-gnu- TA_DEV_KIT_DIR=<path_to_ta_dev_kit> O=out


*buid rdk 32bit m2m_test
1.cd m2m/unit_test
2.make CROSS_COMPILE=arm-linux-gnueabihf  \
CXX=g++ \
TA_CLIENT_KIT=<path_to_ta_client_kit> \
GTEST_HEADER_PATH=<path_to_gtest_include> \
GTEST_SRC_PATH=<path_to_gtest_src> \
TA_CLIENT_LIB_PATH=<path_to_ta_client_lib>


*buid yocto 64bit m2m_test
1.cd m2m/unit_test
2.make CROSS_COMPILE=aarch64-linux-gnu- \
CXX=g++ \
TA_CLIENT_KIT=<path_to_ta_client_kit> \
GTEST_HEADER_PATH=<path_to_gtest_include> \
GTEST_SRC_PATH=<path_to_gtest_src> \
TA_CLIENT_LIB_PATH=<path_to_ta_client_lib64>