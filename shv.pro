TEMPLATE = subdirs
CONFIG += ordered
QT -= gui

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
	shvcall \
}

