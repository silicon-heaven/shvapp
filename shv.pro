TEMPLATE = subdirs
CONFIG += ordered

!debpkg {
SUBDIRS += \
    3rdparty \
    clients \

}

!no-gui:qtHaveModule(gui) {
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
    shvhistoryprovider \
    #eyassrvctl \
}

