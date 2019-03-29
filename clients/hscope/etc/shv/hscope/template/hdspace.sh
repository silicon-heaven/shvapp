#!/bin/bash

DF_VAL=`df | grep $MOUNT_POINT | sed 's/ \+/ /g; s/%//g' | cut -d ' ' -f 5 `
# DF_VAL=90
# echo $PROC
if [[ $DF_VAL -ge $ERROR_TRESHOLD ]]; then
	VAL=ERR
elif [[ $DF_VAL -ge $WARN_TRESHOLD ]]; then
	VAL=WARN
else
	VAL=OK
fi

MSG="Disc fill is ${DF_VAL}%"
echo "{\"val\":\"$VAL\", \"msg\":\"$MSG\"}"
