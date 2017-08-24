message("including $$PWD")

QT += network

CONFIG += C++11
CONFIG += hide_symbols

TEMPLATE = lib
TARGET = shvcoreqt

isEmpty(QF_PROJECT_TOP_BUILDDIR) {
	QF_PROJECT_TOP_BUILDDIR=$$shadowed($$PWD)/../..
}
message ( QF_PROJECT_TOP_BUILDDIR: '$$QF_PROJECT_TOP_BUILDDIR' )

unix:DESTDIR = $$QF_PROJECT_TOP_BUILDDIR/lib
win32:DESTDIR = $$QF_PROJECT_TOP_BUILDDIR/bin

message ( DESTDIR: $$DESTDIR )

DEFINES += SHVCOREQT_BUILD_DLL

INCLUDEPATH += \
	$$PWD/../libshvcore/include \

LIBS += \
    -L$$DESTDIR \
    -lshvcore

include($$PWD/src/src.pri)
