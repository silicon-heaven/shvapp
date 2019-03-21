isEmpty(LIBSHV_SRC_DIR) {
    LIBSHV_SRC_DIR=$$SHV_PROJECT_TOP_SRCDIR/3rdparty/libshv
}
message ( SHV_PROJECT_TOP_SRCDIR == '$$SHV_PROJECT_TOP_SRCDIR' )

isEmpty(SHV_PROJECT_TOP_BUILDDIR) {
        SHV_PROJECT_TOP_BUILDDIR = $$OUT_PWD/..
}
else {
        message ( SHV_PROJECT_TOP_BUILDDIR is not empty and set to $$SHV_PROJECT_TOP_BUILDDIR )
        message ( This is obviously done in file $$SHV_PROJECT_TOP_SRCDIR/.qmake.conf )
}
message ( SHV_PROJECT_TOP_BUILDDIR == '$$SHV_PROJECT_TOP_BUILDDIR' )

QT -= gui
QT += core network

with-shvwebsockets {
	QT += websockets
	DEFINES += WITH_SHV_WEBSOCKETS
}

CONFIG += c++11

TEMPLATE = app
TARGET = shvbroker

DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

LIBDIR = $$DESTDIR
unix: LIBDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib

LIBS += \
        -L$$LIBDIR \

LIBS += \
    -lnecrolog \
    -lshvchainpack \
    -lshvcore \
    -lshvcoreqt \
    -lshviotqt \

unix {
        LIBS += \
                -Wl,-rpath,\'\$\$ORIGIN/../lib\'
}

INCLUDEPATH += \
    $$LIBSHV_SRC_DIR/3rdparty/necrolog/include \
    $$LIBSHV_SRC_DIR/libshvchainpack/include \
    $$LIBSHV_SRC_DIR/libshvcore/include \
    $$LIBSHV_SRC_DIR/libshvcoreqt/include \
    $$LIBSHV_SRC_DIR/libshviotqt/include \

RESOURCES += \
        #shvbroker.qrc \


TRANSLATIONS += \

include (src/src.pri)


