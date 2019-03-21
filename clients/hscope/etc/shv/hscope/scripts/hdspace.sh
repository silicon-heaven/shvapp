#!/bin/bash

# MOUNT={{mount,/home}}
MOUNT=/home
# WARN_TRESH={{warningTreshold,75}}
WARN_TRESH=75
# ERROR_TRESH={{errorTreshold,90}}
ERROR_TRESH=90

VAL=`df | grep $MOUNT | sed 's/ \+/ /g; s/%//g' | cut -d ' ' -f 5 `
# VAL=90
# echo $PROC
if [[ $VAL -ge $ERROR_TRESH ]]; then
	STATUS=ERR
elif [[ $VAL -ge $WARN_TRESH ]]; then
	STATUS=WARN
else
	STATUS=OK
fi

MSG="Disc fill is ${VAL}%"
echo "{\"staus\":\"$STATUS\", \"msg\":\"$MSG\"}"
