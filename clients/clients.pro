TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    bfsview \
    jn50view \

unix {
SUBDIRS += \
    revitestdevice \
}

with-shvsites {
SUBDIRS += \
    shvsites \
}
