TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    3rdparty \
    clients \

qtHaveModule(gui) {
SUBDIRS += \
	shvspy \
}

unix {
SUBDIRS += \
    shvbroker \
    shvagent \
    shvrexec \
    shvrsh \
    shvsitesprovider \
    shvbrclabprovider \
}

