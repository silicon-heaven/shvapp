TEMPLATE = subdirs
CONFIG += ordered

!debpkg {
SUBDIRS += \
    3rdparty \
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
	shvhistoryprovider \
}

