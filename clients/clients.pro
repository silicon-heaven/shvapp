TEMPLATE = subdirs
CONFIG += ordered

!no-gui:qtHaveModule(gui) {
SUBDIRS += \
    bfsview \
    jn50view \

    with-shvsites {
    SUBDIRS += \
        shvsites \
    }
    with-flatline {
    SUBDIRS += \
        flatline \
    }
}


unix {
SUBDIRS += \
	#revitestdevice \
	hscope
}

