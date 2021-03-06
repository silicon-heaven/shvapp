SHV_TOP_BUILDDIR = $$OUT_PWD/../..
SHV_TOP_SRCDIR = $$PWD/../..

message ( SHV_TOP_SRCDIR == '$$SHV_TOP_SRCDIR' )
message ( SHV_TOP_BUILDDIR == '$$SHV_TOP_BUILDDIR' )

QT += core network widgets svg xml
QT -= gui
CONFIG += c++11

TEMPLATE = app
TARGET = bfsview

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

unix {
        LIBS += \
                -Wl,-rpath,\'\$\$ORIGIN/../lib\'
}

bfsview-test {
	DEFINES += TEST
}

INCLUDEPATH += \
        ../../3rdparty/libshv/3rdparty/necrolog/include \
        ../../3rdparty/libshv/libshvchainpack/include \
        ../../3rdparty/libshv/libshvcore/include \
        ../../3rdparty/libshv/libshvcoreqt/include \
        ../../3rdparty/libshv/libshviotqt/include \
        ../../libshviotqt/include \


RESOURCES += \
        bfsview.qrc \

win32 {
RC_ICONS = bfsview.ico
}

TRANSLATIONS += \
#        ../eyassrv.pl_PL.ts \

include (src/src.pri)


