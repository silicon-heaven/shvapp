#!/bin/bash

BINDIR=/home/fanda/proj/_build/shv/bin
SRCDIR=/home/fanda/proj/shv
TSTDIR=$SRCDIR/shvbroker/test/relative-paths
CFGDIR=$TSTDIR/etc/broker

BROKER_TOPICS=rpcmsg,subscr,AclResolve #,sigres
SLEEP_SETTLE=0.2

# tmux set option remain-on-exit on

tmux new-window $BINDIR/shvbroker --config-dir $CFGDIR/master/ -v $BROKER_TOPICS

# sleep 1
tmux split-window $BINDIR/shvbroker --config-dir $CFGDIR/slave1/ -v $BROKER_TOPICS

# sleep 1
tmux split-window -h $BINDIR/shvbroker --config-dir $CFGDIR/slave2/ -v $BROKER_TOPICS

#tmux split-window -h $BINDIR/revitestdevice -p 3755 -u iot --password lub42DUB --lt plain -m test/slave/lub1 --hbi 0
# sleep 1

sleep $SLEEP_SETTLE
tmux select-pane -t0
tmux split-window -h $BINDIR/revitestdevice -p 3756 -u iot --password lub42DUB --lt plain -m test/slave/lub1 --hbi 0 -n 3

sleep $SLEEP_SETTLE

# tmux split-window $BINDIR/revitestdevice -p 3757 -u iot --password lub42DUB --lt plain -m test/slave/lub2 --hbi 0 -n 4 -c '[["../../../broker1/slave/lub1/1/status", "get"], [".broker/app", "subscribe", {"method":"chng", "path":"test"}], ["../lub2/3/status", "sim_set", 41]]' -v rpcmsg
tmux split-window $BINDIR/revitestdevice -p 3757 -u iot --password lub42DUB --lt plain -m test/slave/lub2 --hbi 0 -n 4 -f $TSTDIR/test-calls.cpon -v testcalls

# tmux select-layout tiled
