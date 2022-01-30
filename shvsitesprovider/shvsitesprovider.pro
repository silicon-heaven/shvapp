QT += core network
QT -= gui
CONFIG += c++14

TEMPLATE = app
TARGET = shvsitesprovider

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
	../3rdparty/necrolog/include \
	../3rdparty/libshv/libshvchainpack/include \
	../3rdparty/libshv/libshvcore/include \
	../3rdparty/libshv/libshvcoreqt/include \
	../3rdparty/libshv/libshviotqt/include \

include (src/src.pri)

DISTFILES +=
