isEmpty(QF_PROJECT_TOP_BUILDDIR) {
        QF_PROJECT_TOP_BUILDDIR = $$OUT_PWD/..
}
else {
        message ( QF_PROJECT_TOP_BUILDDIR is not empty and set to $$QF_PROJECT_TOP_BUILDDIR )
        message ( This is obviously done in file $$QF_PROJECT_TOP_SRCDIR/.qmake.conf )
}
message ( QF_PROJECT_TOP_BUILDDIR == '$$QF_PROJECT_TOP_BUILDDIR' )

QT += core network sql
QT -= gui
CONFIG += c++11

TEMPLATE = app
TARGET = shvbroker

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
#QUICKBOX_HOME = $$PROJECT_TOP_SRCDIR/3rdparty/quickbox

#include( $$PROJECT_TOP_SRCDIR/common.pri )

INCLUDEPATH += \
        ../3rdparty/libshv/3rdparty/necrolog/include \
        ../3rdparty/libshv/libshvchainpack/include \
        ../3rdparty/libshv/libshvcore/include \
        ../3rdparty/libshv/libshvcoreqt/include \
        ../libshviotqt/include \


RESOURCES += \
        #shvbroker.qrc \


TRANSLATIONS += \
#        ../eyassrv.pl_PL.ts \

include (src/src.pri)


