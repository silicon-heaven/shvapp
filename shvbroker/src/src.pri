
HEADERS += \
    $$PWD/theapp.h \
    $$PWD/appclioptions.h \
    $$PWD/processor/revitest.h

SOURCES += \
    $$PWD/main.cpp\
    $$PWD/theapp.cpp \
    $$PWD/appclioptions.cpp \
    $$PWD/processor/revitest.cpp

include ($$PWD/rpc/rpc.pri)
include ($$PWD/sql/sql.pri)




