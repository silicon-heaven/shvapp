TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    3rdparty \
!contains(QT_ARCH, arm){
	shvspy \
    clients \
}

unix {
SUBDIRS += \
    shvbroker \
    shvagent \
    shvrexec \
    shvrsh \
!contains(QT_ARCH, arm){
    shvsitesprovider \
    shvfileprovider \
}
}
