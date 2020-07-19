#!/bin/bash

TSTDIR=`dirname "$(readlink -f "$0")"`
# echo TSTDIR: $TSTDIR
BINDIR=/home/fanda/proj/_build/shv/bin
# SRCDIR=/home/fanda/proj/shv
CFGDIR=$TSTDIR/../etc/broker

BROKER_TOPICS=rpcmsg,subscr,AclResolve #,sigres
SLEEP_SETTLE=0.2

# tmux set option remain-on-exit on

tmux new-window $BINDIR/shvbroker --config-dir $CFGDIR/master/ -v $BROKER_TOPICS

sleep $SLEEP_SETTLE

tmux split-window $BINDIR/shvagent -u iot --password iotpwd --lt plain -m test/agent1 --ts $TSTDIR/tests.cpon -v tester

# tmux select-layout tiled
