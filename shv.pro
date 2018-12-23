TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    3rdparty \
    shvspy \
    clients \

unix {
SUBDIRS += \
    shvbroker \
    shvagent \
    shvrexec \
    shvrsh \
    shvsitesprovider \
    shvfileprovider \
}
