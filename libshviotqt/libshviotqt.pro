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
	$$PWD/../libshvcore/include \
	$$PWD/../libshvchainpack/include \
	../3rdparty/libshv/3rdparty/necrolog \

LIBS += \
    -L$$DESTDIR \
    -lshvcore
    -lshvchainpack

include($$PWD/src/src.pri)
