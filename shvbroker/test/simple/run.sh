#!/bin/bash

TSTDIR=`dirname "$(readlink -f "$0")"`
# echo TSTDIR: $TSTDIR
# BINDIR=/home/fanda/proj/_build/shv/bin
# SRCDIR=/home/fanda/proj/shv
CFGDIR=$TSTDIR/../etc/broker

BROKER_TOPICS=rpcmsg,subscr,AclResolve #,sigres
SLEEP_SETTLE=0.2

if [[ -z "$BINDIR" ]]
then
	echo 'Please set the BINDIR variable to the directory with SHV binaries on this system'
	echo ''
	echo 'Usage: BINDIR="/path/to/shv/binaries/" ./run.sh'
	echo ''
	exit 1
fi

# tmux set option remain-on-exit on

# tmux new-window $BINDIR/shvbroker --config-dir $CFGDIR/master/ -v $BROKER_TOPICS
tmux new-window sh -c "$BINDIR/shvbroker/shvbroker --config-dir $CFGDIR/master/ -v $BROKER_TOPICS || read a"

sleep $SLEEP_SETTLE

tmux split-window sh -c "$BINDIR/shvagent/shvagent --sec-type none -u iot --password iotpwd --lt plain -m test/agent1 --ts $TSTDIR/tests.cpon -v tester || read a"

# tmux select-layout tiled
