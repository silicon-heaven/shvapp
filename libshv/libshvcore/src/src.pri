HEADERS += \
	$$PWD/shvcoreglobal.h \
    $$PWD/shvexception.h \
    $$PWD/string.h \
    $$PWD/shvlog.h \
    $$PWD/log.h

SOURCES += \
    $$PWD/string.cpp \
    $$PWD/shvlog.cpp

include ( $$PWD/chainpack/chainpack.pri )

