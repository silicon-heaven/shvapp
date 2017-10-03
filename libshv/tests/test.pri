CONFIG += testcase # enable make check
CONFIG += c++11
#QT -= core
QT += testlib # enable test framework

isEmpty(SHV_PROJECT_TOP_BUILDDIR) {
	SHV_PROJECT_TOP_BUILDDIR=$$shadowed($$PWD)/..
}
message ( SHV_PROJECT_TOP_BUILDDIR: '$$SHV_PROJECT_TOP_BUILDDIR' )

DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

INCLUDEPATH += \
	$$PWD/../libshvcore/include

win32:LIB_DIR = $$DESTDIR
else:LIB_DIR = $$SHV_PROJECT_TOP_BUILDDIR/lib

message (LIB_DIR $$LIB_DIR)
message (DESTDIR $$DESTDIR)

LIBS += \
    -L$$LIB_DIR \
    -lshvcore \

unix {
    LIBS += \
        -Wl,-rpath,\'$${LIB_DIR}\'
}

