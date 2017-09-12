TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    libshvcore \
    libshvcoreqt \

!beaglebone {
SUBDIRS += \
    libshvgui \
    tests \
}
