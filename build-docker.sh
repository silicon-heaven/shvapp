#!/bin/bash

CPU_NR="`(cat /proc/cpuinfo 2> /dev/null || echo 'processor') | grep "^processor" | wc -l`"

/opt/qt/5.8/gcc_64/bin/qmake -r CONFIG+=no-libshv-gui /src/shv.pro DEFINES+=GIT_COMMIT=`git -C /src rev-parse HEAD`
make -j $CPU_NR
cp -avr lib/ /opt/shv/
cp -avr bin/ /opt/shv/

