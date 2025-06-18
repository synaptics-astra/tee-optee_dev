#!/bin/bash

if [ $# != "1" ];then
  printf "\nUsage: $0 <TA>\n"
  exit 1
fi

source $pta_enc_dir/common.rc $1

printf "\nOutDIR=$OutputData\n"
if [ ! -d $OutputData ];then
  mkdir $OutputData
fi

[ ! -f ${tacert_path}/${TA_BASENAME}.cert ]
[ ! -f ${ta_uuid_path}/common.uuid.cfg ]

cp -adv $1 $OutputData/$TA_RAW_BIN || exit 0

source ${ta_uuid_path}/common.uuid.cfg

enc_x_ta $OutputData/${TA_RAW_BIN} ${OutputData}/${TA_BASENAME}.ta 0
