isEmpty(SHV_PROJECT_TOP_BUILDDIR) {
	SHV_PROJECT_TOP_BUILDDIR = $$OUT_PWD/../..
}
else {
	message ( SHV_PROJECT_TOP_BUILDDIR is not empty and set to $$SHV_PROJECT_TOP_BUILDDIR )
	message ( This is obviously done in file $$SHV_PROJECT_TOP_SRCDIR/.qmake.conf )
}
message ( SHV_PROJECT_TOP_BUILDDIR == '$$SHV_PROJECT_TOP_BUILDDIR' )

isEmpty(LIBSHV_SRC_DIR) {
    LIBSHV_SRC_DIR=$$SHV_PROJECT_TOP_SRCDIR/3rdparty/libshv
}

QT += core gui widgets
CONFIG += c++11

TEMPLATE = app
TARGET = shvspy
DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

LIBDIR = $$DESTDIR
unix: LIBDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib

LIBS += \
	-L$$LIBDIR \
	-lnecrolog \
	-lshvchainpack \
	-lshvcore \
	-lshvcoreqt \
	-lshviotqt \
	-lshvvisu \
	-lshvbroker \


unix {
	LIBS += \
# 		-lmbedx509 \
# 		-lmbedtls \
# 		-lmbedcrypto \
		-Wl,-rpath,\'\$\$ORIGIN/../lib\'
}

PROJECT_TOP_SRCDIR = $$PWD/../../..
#QUICKBOX_HOME = $$PROJECT_TOP_SRCDIR/3rdparty/quickbox

#include( $$PROJECT_TOP_SRCDIR/common.pri )

INCLUDEPATH += \
    $$LIBSHV_SRC_DIR/3rdparty/necrolog/include \
    $$LIBSHV_SRC_DIR/libshvchainpack/include \
    $$LIBSHV_SRC_DIR/libshvcore/include \
    $$LIBSHV_SRC_DIR/libshvcoreqt/include \
    $$LIBSHV_SRC_DIR/libshviotqt/include \
    $$LIBSHV_SRC_DIR/libshvvisu/include \
    $$LIBSHV_SRC_DIR/libshvbroker/include \

RESOURCES += \
	$${TARGET}.qrc \

include (src/src.pri)

