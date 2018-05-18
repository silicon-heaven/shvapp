TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    3rdparty \
    shvspy \

unix {
SUBDIRS += \
    shvbroker \
    shvagent \
    shvrexec \
    shvrsh \
    revitestdevice \
    bfsview \
}

