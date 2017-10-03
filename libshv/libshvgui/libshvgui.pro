message("including $$PWD")

QT += widgets

CONFIG += C++11
CONFIG += hide_symbols

TEMPLATE = lib
TARGET = shvgui

isEmpty(SHV_PROJECT_TOP_BUILDDIR) {
    SHV_PROJECT_TOP_BUILDDIR=$$shadowed($$PWD)/..
}
message ( SHV_PROJECT_TOP_BUILDDIR: '$$SHV_PROJECT_TOP_BUILDDIR' )

SHV_TOP_SRCDIR=$$PWD/..
message ( SHV_TOP_SRCDIR: '$$SHV_TOP_SRCDIR' )

unix:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib
win32:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

message ( DESTDIR: $$DESTDIR )

DEFINES += SHVGUI_BUILD_DLL

INCLUDEPATH += \
    #$$QUICKBOX_HOME/libqf/libqfcore/include \
    $$SHV_TOP_SRCDIR/libshvcore/include \

message ( INCLUDEPATH: '$$INCLUDEPATH' )

LIBS += \
    -L$$DESTDIR \
    -lshvcore

include(src/src.pri)

HEADERS += \
    include/shv/graph/graphmodel.h \
    include/shv/graph/view.h \
    include/shv/graph/backgroundstripe.h \
    include/shv/graph/pointofinterest.h \
    include/shv/graph/serie.h \
    include/shv/graph/outsideseriegroup.h
