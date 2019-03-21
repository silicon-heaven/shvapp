 isEmpty(QF_PROJECT_TOP_BUILDDIR) {
        QF_PROJECT_TOP_BUILDDIR = $$OUT_PWD/..
}
else {
        message ( QF_PROJECT_TOP_BUILDDIR is not empty and set to $$QF_PROJECT_TOP_BUILDDIR )
        message ( This is obviously done in file $$QF_PROJECT_TOP_SRCDIR/.qmake.conf )
}
message ( QF_PROJECT_TOP_BUILDDIR == '$$QF_PROJECT_TOP_BUILDDIR' )

isEmpty(LIBSHV_SRC_DIR) {
    LIBSHV_SRC_DIR=$$SHV_PROJECT_TOP_SRCDIR/3rdparty/libshv
}

QT += core network
QT -= gui
CONFIG += c++14

TEMPLATE = app
TARGET = shvsitesprovider

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
    $$LIBSHV_SRC_DIR/3rdparty/necrolog/include \
    $$LIBSHV_SRC_DIR/libshvchainpack/include \
    $$LIBSHV_SRC_DIR/libshvcore/include \
    $$LIBSHV_SRC_DIR/libshvcoreqt/include \
    $$LIBSHV_SRC_DIR/libshviotqt/include \

include (src/src.pri)

DISTFILES +=
