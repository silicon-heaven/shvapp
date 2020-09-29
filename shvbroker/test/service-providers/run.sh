#!/bin/bash

TSTDIR=`dirname "$(readlink -f "$0")"`
# echo TSTDIR: $TSTDIR
BINDIR=/home/fanda/proj/_build/shv/bin
# SRCDIR=/home/fanda/proj/shv
CFGDIR=$TSTDIR/../etc/broker

BROKER_TOPICS=rpcmsg,subscr,ServiceProviders,SigRes
SLEEP_SETTLE=0.5

#tmux set remain-on-exit on

tmux new-window $BINDIR/shvbroker --config-dir $CFGDIR/master/ -v $BROKER_TOPICS 

# sleep 1
tmux split-window $BINDIR/shvbroker --config-dir $CFGDIR/slave1/ -v $BROKER_TOPICS

# sleep 1
tmux split-window $BINDIR/shvbroker --config-dir $CFGDIR/slaveA/ -v $BROKER_TOPICS

#tmux split-window -h $BINDIR/revitestdevice -p 3755 -u iot --password iotpwd --lt plain -m test/slave/lub1
# sleep 1
# exit 0

sleep $SLEEP_SETTLE

tmux select-pane -t0
tmux split-window -hf $BINDIR/shvagent -p 3755 -u iot --password iotpwd --lt plain -m test/sites/broker1/brokerA/agentB2 --tester -v tester

sleep $SLEEP_SETTLE

# tmux select-pane -t0
#tmux split-window $BINDIR/shvagent -p 3757 -u iot --password iotpwd --lt plain -m test/agentB1 --tester -v tester

sleep $SLEEP_SETTLE

# tmux split-window $BINDIR/revitestdevice -p 3757 -u iot --password iotpwd --lt plain -m test/slave/lub2 -n 4 -c '[["../../../broker1/slave/lub1/1/status", "get"], [".broker/app", "subscribe", {"method":"chng", "path":"test"}], ["../lub2/3/status", "sim_set", 41]]' -v rpcmsg
tmux split-window $BINDIR/shvagent -p 3757 -u iot --password iotpwd --lt plain -m test/agentB2 --ts $TSTDIR/tests.cpon -v tester,rpcmsg

# tmux select-layout tiled
