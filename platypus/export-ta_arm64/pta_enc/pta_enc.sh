#!/bin/bash

if [ $# != "1" ] && [ $# != "2" ];then
  printf "\nUsage: $0 <TA_PATH> [TA_OUTPATH]\n"
  exit 1
fi

if [ $# = 2 ]; then
  TA_OUTPATH=$2
fi

pta_enc_dir=`dirname $0`
security_tools_path="$pta_enc_dir/tools/"
security_keys_path="$pta_enc_dir/keys/"
tacert_path="$pta_enc_dir/tacert_genx/"
tarootcert_path="$pta_enc_dir/tarootcert_genx/"
ta_uuid_path="$pta_enc_dir/ta_uuid/"

ta=$1
source $pta_enc_dir/gen_ta_enc_img.sh $ta

if [ $# = 1 ]; then
	ta_dir=`dirname $ta`
	ta_out=$ta_dir/pta_enc/
	cp $ta_out/*.ta $ta_dir -av
fi
