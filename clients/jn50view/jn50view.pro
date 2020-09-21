SHV_TOP_BUILDDIR = $$OUT_PWD/../..
SHV_TOP_SRCDIR = $$PWD/../..

message ( SHV_TOP_SRCDIR == '$$SHV_TOP_SRCDIR' )
message ( SHV_TOP_BUILDDIR == '$$SHV_TOP_BUILDDIR' )

QT += core network widgets xml
QT -= gui
CONFIG += c++11

TEMPLATE = app
TARGET = jn50view

DESTDIR = $$SHV_TOP_BUILDDIR/bin

LIBDIR = $$DESTDIR
unix: LIBDIR = $$SHV_TOP_BUILDDIR/lib

LIBS += \
        -L$$LIBDIR \

LIBS += \
    -lnecrolog \
    -lshvchainpack \
    -lshvcore \
    -lshvcoreqt \
    -lshviotqt \
    -lshvvisu \

unix {
        LIBS += \
                -Wl,-rpath,\'\$\$ORIGIN/../lib\'
}

testing {
	DEFINES += TESTING
}

INCLUDEPATH += \
        ../../3rdparty/libshv/3rdparty/necrolog/include \
        ../../3rdparty/libshv/libshvchainpack/include \
        ../../3rdparty/libshv/libshvcore/include \
        ../../3rdparty/libshv/libshvcoreqt/include \
        ../../3rdparty/libshv/libshviotqt/include \
        ../../3rdparty/libshv/libshvvisu/include \

RESOURCES += \
        jn50view.qrc \

message ( RESOURCES == '$$RESOURCES' )

win32 {
RC_ICONS = $$TARGET.ico
}

TRANSLATIONS += \
    jn50view.cs_CZ.ts \

include (src/src.pri)


