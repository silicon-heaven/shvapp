#CONFIG += testcase # enable make check
CONFIG += c++11
QT -= core
#QT += testlib # enable test framework

PROJECT_TOP_SRCDIR = $$PWD/../..
PROJECT_TOP_BUILDDIR = $$OUT_PWD/../..

INCLUDEPATH += \
    $$PWD/../libshvcore/include

win32:LIB_DIR = $$PROJECT_TOP_BUILDDIR/bin
else:LIB_DIR = $$PROJECT_TOP_BUILDDIR/lib

LIBS += \
    -L$$LIB_DIR \
    -lshvcore \

unix {
    LIBS += \
        -Wl,-rpath,\'$${LIB_DIR}\'
}
