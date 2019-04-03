#!/bin/bash

if [[ ! -z $DEVICE_NAME ]]; then
	DF_VAL=`df | grep "^${DEVICE_NAME}\s" | sed 's/ \+/ /g; s/%//g' | cut -d ' ' -f 5 `
elif [[ ! -z $MOUNT_POINT ]]; then
	DF_VAL=`df | grep "${MOUNT_POINT}$" | sed 's/ \+/ /g; s/%//g' | cut -d ' ' -f 5 `
fi

# DF_VAL=90
# echo $PROC
if [[ -z $DF_VAL ]]; then
	VAL=ERR
elif [[ $DF_VAL -ge ${ERROR_TRESHOLD:-90} ]]; then
	VAL=ERR
elif [[ $DF_VAL -ge ${WARN_TRESHOLD:-75} ]]; then
	VAL=WARN
else
	VAL=OK
fi

if [[ -z $DF_VAL ]]; then
	MSG="Invalid input parameters."
else
	MSG="Disc fill is ${DF_VAL}%"
fi

echo "{\"val\":\"$VAL\", \"msg\":\"$MSG\"}"
