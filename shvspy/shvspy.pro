isEmpty(QF_PROJECT_TOP_BUILDDIR) {
	QF_PROJECT_TOP_BUILDDIR = $$OUT_PWD/../..
}
else {
	message ( QF_PROJECT_TOP_BUILDDIR is not empty and set to $$QF_PROJECT_TOP_BUILDDIR )
	message ( This is obviously done in file $$QF_PROJECT_TOP_SRCDIR/.qmake.conf )
}
message ( QF_PROJECT_TOP_BUILDDIR == '$$QF_PROJECT_TOP_BUILDDIR' )

QT += core gui widgets
CONFIG += c++11

TEMPLATE = app
TARGET = shvspy
DESTDIR = $$QF_PROJECT_TOP_BUILDDIR/bin

LIBDIR = $$DESTDIR
unix: LIBDIR = $$QF_PROJECT_TOP_BUILDDIR/lib

LIBS += \
	-L$$LIBDIR \
	-lnecrolog \
	-lshvchainpack \
	-lshvcore \
	-lshvcoreqt \
	-lshviotqt \

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
	$$PWD/../3rdparty/libshv/3rdparty/necrolog/include \
	$$PWD/../3rdparty/libshv/libshvchainpack/include \
	$$PWD/../3rdparty/libshv/libshvcore/include \
	$$PWD/../3rdparty/libshv/libshvcoreqt/include \
	$$PWD/../3rdparty/libshv/libshviotqt/include \

RESOURCES += \
	$${TARGET}.qrc \

include (src/src.pri)
