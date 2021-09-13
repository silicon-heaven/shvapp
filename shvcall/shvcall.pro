QT += core network
QT -= gui
CONFIG += c++14
CONFIG += console

TEMPLATE = app
TARGET = shvcall

isEmpty(SHV_PROJECT_TOP_BUILDDIR) {
    SHV_PROJECT_TOP_BUILDDIR = $$OUT_PWD/..
}
else {
    message ( SHV_PROJECT_TOP_BUILDDIR is not empty and set to $$SHV_PROJECT_TOP_BUILDDIR )
    message ( This is obviously done in file $$SHV_PROJECT_TOP_SRCDIR/.qmake.conf )
}
message (SHV_PROJECT_TOP_BUILDDIR == '$$SHV_PROJECT_TOP_BUILDDIR')

DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

LIBDIR = $$DESTDIR
unix: LIBDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib

LIBS += -L$$LIBDIR \

LIBS += \
    -lnecrolog \
    -lshvchainpack \
    -lshvcore \
    -lshvcoreqt \
    -lshviotqt \


unix {
    LIBS += -Wl,-rpath,\'\$\$ORIGIN/../lib\'
}

INCLUDEPATH += \
        ../3rdparty/necrolog/include \
        ../3rdparty/libshv/libshvchainpack/include \
        ../3rdparty/libshv/libshvcore/include \
        ../3rdparty/libshv/libshvcoreqt/include \
        ../3rdparty/libshv/libshviotqt/include \
        ./src \

include (src/src.pri)