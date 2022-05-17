QT += core network widgets xml
QT -= gui
CONFIG += c++11

TEMPLATE = app
TARGET = jn50view

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

testing {
	DEFINES += TESTING
}

INCLUDEPATH += \
	$$SHV_PROJECT_TOP_SRCDIR/3rdparty/necrolog/include \
	$$LIBSHV_SRC_DIR/libshvchainpack/include \
	$$LIBSHV_SRC_DIR/libshvcore/include \
	$$LIBSHV_SRC_DIR/libshvcoreqt/include \
	$$LIBSHV_SRC_DIR/libshviotqt/include \
	$$LIBSHV_SRC_DIR/libshvvisu/include \

RESOURCES += \
        jn50view.qrc \

message ( RESOURCES == '$$RESOURCES' )

win32 {
RC_ICONS = $$TARGET.ico
}

TRANSLATIONS += \
    translations/jn50view.cs_CZ.ts \

include (src/src.pri)

DISTFILES += \
    translations/jn50view.cs_CZ.ts


