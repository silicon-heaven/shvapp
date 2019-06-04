#SHV_TOP_BUILDDIR = $$OUT_PWD/../..
#SHV_TOP_SRCDIR = $$PWD/../..

message ( SHV_PROJECT_TOP_SRCDIR == '$$SHV_PROJECT_TOP_SRCDIR' )
message ( SHV_PROJECT_TOP_BUILDDIR == '$$SHV_PROJECT_TOP_BUILDDIR' )

QT += core network widgets gui svg
# QT -= gui
CONFIG += c++11

TEMPLATE = app
TARGET = flatline

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
    -lshvvisu \

unix {
	LIBS += \
		-Wl,-rpath,\'\$\$ORIGIN/../lib\'
}

#SHV_TOP_SRCDIR = $$PWD/..

INCLUDEPATH += \
	$$SHV_PROJECT_TOP_SRCDIR/3rdparty/libshv/3rdparty/necrolog/include \
	$$SHV_PROJECT_TOP_SRCDIR/3rdparty/libshv/libshvchainpack/include \
	$$SHV_PROJECT_TOP_SRCDIR/3rdparty/libshv/libshvcore/include \
	$$SHV_PROJECT_TOP_SRCDIR/3rdparty/libshv/libshvcoreqt/include \
    $$SHV_PROJECT_TOP_SRCDIR/3rdparty/libshv/libshviotqt/include \
    $$SHV_PROJECT_TOP_SRCDIR/3rdparty/libshv/libshvvisu/include \

RESOURCES += \
	images/images.qrc


TRANSLATIONS += \

include (src/src.pri)

win32 {
RC_ICONS = flatline.ico
}


