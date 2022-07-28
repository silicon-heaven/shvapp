include (common.pro)
CONFIG += testcase
TARGET = test_historyprovider
DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/tests

SOURCES += \
		$$PWD/tests/test.cpp \
