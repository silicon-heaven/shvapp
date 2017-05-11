message("including $$PWD")

QT -= core

CONFIG += C++11
CONFIG += hide_symbols

TEMPLATE = lib
TARGET = shvcore

isEmpty(QF_PROJECT_TOP_BUILDDIR) {
	QF_PROJECT_TOP_BUILDDIR=$$shadowed($$PWD)/../..
}
message ( QF_PROJECT_TOP_BUILDDIR: '$$QF_PROJECT_TOP_BUILDDIR' )

unix:DESTDIR = $$QF_PROJECT_TOP_BUILDDIR/lib
win32:DESTDIR = $$QF_PROJECT_TOP_BUILDDIR/bin

message ( DESTDIR: $$DESTDIR )

DEFINES += SHVCORE_BUILD_DLL

INCLUDEPATH += \
	#$$QUICKBOX_HOME/libqf/libqfcore/include \
	#$$PROJECT_TOP_SRCDIR/qfopcua/libqfopcua/include \

LIBS += \
    -L$$DESTDIR \
    #-lqfcore

include($$PWD/src/src.pri)
