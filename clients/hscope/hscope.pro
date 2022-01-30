QT += core network
QT -= gui
CONFIG += c++11

TEMPLATE = app
TARGET = hscope

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

testing {
	DEFINES += TESTING
}

INCLUDEPATH += \
	$$SHV_PROJECT_TOP_SRCDIR/3rdparty/necrolog/include \
	$$LIBSHV_SRC_DIR/libshvchainpack/include \
	$$LIBSHV_SRC_DIR/libshvcore/include \
	$$LIBSHV_SRC_DIR/libshvcoreqt/include \
	$$LIBSHV_SRC_DIR/libshviotqt/include \

RESOURCES += \
        $$TARGET.qrc \

message ( RESOURCES == '$$RESOURCES' )

include (src/src.pri)


