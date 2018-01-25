message("including $$PWD")

QT += network
QT -= gui

CONFIG += C++11
CONFIG += hide_symbols

TEMPLATE = lib
TARGET = shviotqt

isEmpty(SHV_PROJECT_TOP_BUILDDIR) {
	SHV_PROJECT_TOP_BUILDDIR=$$shadowed($$PWD)/..
}
message ( SHV_PROJECT_TOP_BUILDDIR: '$$SHV_PROJECT_TOP_BUILDDIR' )

unix:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib
win32:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

message ( DESTDIR: $$DESTDIR )

DEFINES += SHVIOTQT_BUILD_DLL

INCLUDEPATH += \
	$$PWD/../3rdparty/libshv/libshvcore/include \
	$$PWD/../3rdparty/libshv/libshvcoreqt/include \
	$$PWD/../3rdparty/libshv/libshvchainpack/include \
	../3rdparty/libshv/3rdparty/necrolog \

LIBS += \
    -L$$DESTDIR \
    -lshvchainpack \
    -lshvcore \
    -lshvcoreqt \

include($$PWD/src/src.pri)
