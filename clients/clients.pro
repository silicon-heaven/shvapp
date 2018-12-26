TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    bfsview \

unix {
SUBDIRS += \
    revitestdevice \
}

!noshvsites {
SUBDIRS += \
    shvsites \
}
