#!/bin/bash

MEM=`free | grep "Mem:" | sed 's/ \+/ /g' `
USED_VAL=`echo $MEM | cut -d ' ' -f 3 `
TOTAL_VAL=`echo $MEM | cut -d ' ' -f 2 `
USED_PERCENT=$(( $USED_VAL * 100 / $TOTAL_VAL ))

if [[ -z $USED_PERCENT ]]; then
    VAL=ERR
elif [[ $USED_PERCENT -ge ${ERROR_THRESHOLD:-90} ]]; then
    VAL=ERR
elif [[ $USED_PERCENT -ge ${WARN_THRESHOLD:-75} ]]; then
    VAL=WARN
else
    VAL=OK
fi

MSG="Used memory ${USED_VAL} of ${TOTAL_VAL} ${USED_PERCENT}%"

echo "{\"val\":\"$VAL\", \"msg\":\"$MSG\"}"


