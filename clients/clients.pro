TEMPLATE = subdirs
CONFIG += ordered

qtHaveModule(gui) {
SUBDIRS += \
    bfsview \
    jn50view \

    with-shvsites {
    SUBDIRS += \
        shvsites \
    }
}


unix {
SUBDIRS += \
    revitestdevice \
    hscope \
}

