TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    libshvcore \
    libshvcoreqt \

CONFIG(debug, debug|release) {
SUBDIRS += \
    tests \
}

!beaglebone {
SUBDIRS += \
    libshvgui \
}
