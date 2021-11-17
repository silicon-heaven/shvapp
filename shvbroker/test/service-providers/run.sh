#!/bin/bash

TSTDIR=`dirname "$(readlink -f "$0")"`
# echo TSTDIR: $TSTDIR
# BINDIR=/home/fanda/proj/_build/shv/bin
# SRCDIR=/home/fanda/proj/shv
CFGDIR=$TSTDIR/../etc/broker

BROKER_TOPICS=rpcmsg,subscr,ServiceProviders,SigRes #,AclResolve
SLEEP_SETTLE=0.5

if [[ -z "$BINDIR" ]]
then
	echo 'Please set the BINDIR variable to the directory with SHV binaries on this system'
	echo ''
	echo 'Usage: BINDIR="/path/to/shv/binaries/" ./run.sh'
	echo ''
	exit 1
fi

# tmux set remain-on-exit on
tmux new-window sh -c "$BINDIR/shvbroker --config-dir $CFGDIR/master/ -v $BROKER_TOPICS || read a"
#tmux new-window sh -c "$BINDIR/shvbroker --config-dir $CFGDIR/master/ -v $BROKER_TOPICS 2>&1 | tee ~/t/bmaster.log"

# sleep 1
tmux split-window sh -c "$BINDIR/shvbroker --config-dir $CFGDIR/slave1/ -v $BROKER_TOPICS || read a"

# sleep 1
tmux split-window sh -c "$BINDIR/shvbroker --config-dir $CFGDIR/slaveA/ -v $BROKER_TOPICS || read a"

sleep $SLEEP_SETTLE

tmux select-pane -t0
tmux split-window -hf sh -c " $BINDIR/shvagent -p 37555 --sec-type ssl --peer-verify false -u iot --password iotpwd --lt plain -m test/sites/broker1/brokerA/agentB2 --tester -v tester,rpcmsg || read a"

sleep $SLEEP_SETTLE

tmux split-window sh -c "$BINDIR/shvagent -p 3757 --sec-type none -u iot --password iotpwd --lt plain -m test/agentB2 --ts $TSTDIR/tests.cpon -v tester,rpcmsg || read a"

# tmux select-layout tiled
