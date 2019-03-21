TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    bfsview \
    jn50view \

unix {
SUBDIRS += \
    revitestdevice \
    hscope \
}

with-shvsites {
SUBDIRS += \
    shvsites \
}
