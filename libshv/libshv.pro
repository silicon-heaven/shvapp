TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    libshvcore \
    libshvcoreqt \
    tests \

!beaglebone {
SUBDIRS += \
    libshvgui \
}
