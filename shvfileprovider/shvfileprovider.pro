 isEmpty(QF_PROJECT_TOP_BUILDDIR) {
        QF_PROJECT_TOP_BUILDDIR = $$OUT_PWD/..
}
else {
        message ( QF_PROJECT_TOP_BUILDDIR is not empty and set to $$QF_PROJECT_TOP_BUILDDIR )
        message ( This is obviously done in file $$QF_PROJECT_TOP_SRCDIR/.qmake.conf )
}
message ( QF_PROJECT_TOP_BUILDDIR == '$$QF_PROJECT_TOP_BUILDDIR' )

QT += core network
QT -= gui
CONFIG += c++11

TEMPLATE = app
TARGET = shvfileprovider

DESTDIR = $$QF_PROJECT_TOP_BUILDDIR/bin

LIBDIR = $$DESTDIR
unix: LIBDIR = $$QF_PROJECT_TOP_BUILDDIR/lib

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

SHV_TOP_SRCDIR = $$PWD/..

INCLUDEPATH += \
        ../3rdparty/libshv/3rdparty/necrolog/include \
        ../3rdparty/libshv/libshvchainpack/include \
        ../3rdparty/libshv/libshvcore/include \
        ../3rdparty/libshv/libshvcoreqt/include \
        ../3rdparty/libshv/libshviotqt/include \
        ../libshviotqt/include \

include (src/src.pri)

DISTFILES +=
