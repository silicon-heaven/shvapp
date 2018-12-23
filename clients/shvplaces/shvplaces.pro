SHV_TOP_BUILDDIR = $$OUT_PWD/../..
SHV_TOP_SRCDIR = $$PWD/../..

message ( SHV_TOP_SRCDIR == '$$SHV_TOP_SRCDIR' )
message ( SHV_TOP_BUILDDIR == '$$SHV_TOP_BUILDDIR' )

QT += quick qml network positioning location

TARGET = shvplaces
TEMPLATE = app

DESTDIR = $$SHV_TOP_BUILDDIR/bin

LIBDIR = $$DESTDIR
unix: LIBDIR = $$SHV_TOP_BUILDDIR/lib

LIBS += \
        -L$$LIBDIR \

LIBS += \
    -lnecrolog \
    -lshvchainpack \
    -lshvcore \
    -lshvcoreqt \
    -lshviotqt \

unix {
        LIBS += \
                -Wl,-rpath,\'\$\$ORIGIN/../lib\'
}

INCLUDEPATH += \
        ../../3rdparty/libshv/3rdparty/necrolog/include \
        ../../3rdparty/libshv/libshvchainpack/include \
        ../../3rdparty/libshv/libshvcore/include \
        ../../3rdparty/libshv/libshvcoreqt/include \
        ../../3rdparty/libshv/libshviotqt/include \
        ../../libshviotqt/include \

SOURCES = main.cpp \
    poimodel.cpp

HEADERS += \
    poimodel.h

RESOURCES += \
    shvplaces.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/location/$$TARGET
INSTALLS += target

