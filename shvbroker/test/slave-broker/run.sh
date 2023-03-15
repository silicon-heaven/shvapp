#!/bin/bash

TSTDIR=`dirname "$(readlink -f "$0")"`
# echo TSTDIR: $TSTDIR
# BINDIR=/home/fanda/proj/_build/shv/bin
# SRCDIR=/home/fanda/proj/shv
CFGDIR=$TSTDIR/../etc/broker

BROKER_TOPICS=rpcmsg,subscr,AclResolve,ServiceProviders
SLEEP_SETTLE=0.5

if [[ -z "$BINDIR" ]]
then
	echo 'Please set the BINDIR variable to the directory with SHV binaries on this system'
	echo ''
	echo 'Usage: BINDIR="/path/to/shv/binaries/" ./run.sh'
	echo ''
	exit 1
fi

# tmux set option remain-on-exit on

tmux new-window sh -c "$BINDIR/shvbroker/shvbroker --config-dir $CFGDIR/master/ -v $BROKER_TOPICS || read a"
# sleep 10
tmux split-window sh -c "$BINDIR/shvbroker/shvbroker --config-dir $CFGDIR/slave1/ -v $BROKER_TOPICS -d shvnode || read a"

# sleep 1
tmux split-window sh -c "$BINDIR/shvbroker/shvbroker --config-dir $CFGDIR/slaveA/ -v $BROKER_TOPICS || read a"

sleep $SLEEP_SETTLE

tmux select-pane -t0
tmux split-window -hf sh -c " $BINDIR/shvagent/shvagent -s ssl://localhost:37577 ssl --peer-verify false -u iot --password iotpwd --lt plain -m test/agentA1 --tester -v tester || read a"

# exit 0
# sleep $SLEEP_SETTLE

# tmux select-pane -t0
tmux split-window sh -c " $BINDIR/shvagent/shvagent -s tcp://localhost:3756 -u iot --password iotpwd --lt plain -m test/agent11 --hbi 0 || read a"

sleep $SLEEP_SETTLE

# tmux split-window $BINDIR/revitestdevice -p 3757 -u iot --password iotpwd --lt plain -m test/slave/lub2 --hbi 0 -n 4 -c '[["../../../broker1/slave/lub1/1/status", "get"], [".broker/app", "subscribe", {"method":"chng", "path":"test"}], ["../lub2/3/status", "sim_set", 41]]' -v rpcmsg
tmux split-window sh -c "$BINDIR/shvagent/shvagent -s tcp://localhost:3755 -u iot --password iotpwd --lt plain -m test/agent --ts $TSTDIR/tests.cpon -v tester || read a"

# tmux select-layout tiled
