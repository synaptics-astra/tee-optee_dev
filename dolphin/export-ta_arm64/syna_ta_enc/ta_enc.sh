#!/bin/bash

if [ $# != "1" ] && [ $# != "2" ];then
  printf "\nUsage: $0 <TA_PATH> [TA_OUTPATH]\n"
  exit 1
fi
ta_enc_dir=`dirname $0`
GENX_TOOL_PATH="$ta_enc_dir/tools/"
if [ "$OPTEE_TA_ENC" == "dev" ]; then
    security_keys_path="$ta_enc_dir/keys/dev"
else
    security_keys_path="$ta_enc_dir/keys/prod"
fi
printf "security_keys_path=$security_keys_path\n"

ta=$1
source $ta_enc_dir/gen_full_ta_img.sh $ta
if [ $# = 1 ]; then
	ta_dir=`dirname $ta`
	ta_out=$ta_dir/ta_enc/
	cp $ta_out/*.ta $ta_dir -av
fi
