QT -= gui
QT += core network sql

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
    -lshvbroker \

unix {
        LIBS += \
                -Wl,-rpath,\'\$\$ORIGIN/../lib\'
}

INCLUDEPATH += \
	$$SHV_PROJECT_TOP_SRCDIR/3rdparty/necrolog/include \
    $$LIBSHV_SRC_DIR/libshvchainpack/include \
    $$LIBSHV_SRC_DIR/libshvcore/include \
    $$LIBSHV_SRC_DIR/libshvcoreqt/include \
    $$LIBSHV_SRC_DIR/libshviotqt/include \
    $$LIBSHV_SRC_DIR/libshvbroker/include \

RESOURCES += \
        #shvbroker.qrc \


TRANSLATIONS += \

include (src/src.pri)


