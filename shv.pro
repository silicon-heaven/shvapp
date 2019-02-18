TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    3rdparty

qtHaveModule(gui) {
SUBDIRS += \
	shvspy \
    clients \
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

