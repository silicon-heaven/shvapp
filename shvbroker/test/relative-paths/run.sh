#!/bin/bash

export BINDIR=/home/fanda/proj/_build/shv/bin
export SRCDIR=/home/fanda/proj/shv

# tmux setw remain-on-exit on
# tmux read
tmux new-window $BINDIR/shvbroker --config-dir $SRCDIR/shvbroker/etc/ -v rpcmsg,subscr,sigres

# sleep 1
tmux split-window $BINDIR/shvbroker --config-dir $SRCDIR/shvbroker/test/relative-paths/etc1/ -v rpcmsg,subscr,sigres

# sleep 1
tmux split-window -h $BINDIR/shvbroker --config-dir $SRCDIR/shvbroker/test/relative-paths/etc2/ -v rpcmsg,subscr,sigres

#tmux split-window -h $BINDIR/revitestdevice -p 3755 -u iot --password lub42DUB --lt plain -m test/slave/lub1 --hbi 0
# sleep 1

tmux select-pane -t0
tmux split-window -h $BINDIR/revitestdevice -p 3756 -u iot --password lub42DUB --lt plain -m test/slave/lub1 --hbi 0 -n 3

sleep 1

# tmux split-window $BINDIR/revitestdevice -p 3757 -u iot --password lub42DUB --lt plain -m test/slave/lub2 --hbi 0 -n 4 -c '[["../../../broker1/slave/lub1/1/status", "get"], [".broker/app", "subscribe", {"method":"chng", "path":"test"}], ["../lub2/3/status", "sim_set", 41]]' -v rpcmsg
tmux split-window $BINDIR/revitestdevice -p 3757 -u iot --password lub42DUB --lt plain -m test/slave/lub2 --hbi 0 -n 4 -c '[[".broker/app", "subscribe", {"method":"chng", "path":"../../../broker1/slave/lub1/1/status"}], ["../../../broker1/slave/lub1/1/status", "sim_set", 42]]' -v rpcmsg
