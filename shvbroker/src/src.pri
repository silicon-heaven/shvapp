
HEADERS += \
    $$PWD/appclioptions.h \
    $$PWD/brokerapp.h

SOURCES += \
    $$PWD/main.cpp\
    $$PWD/appclioptions.cpp \
    $$PWD/brokerapp.cpp

include ($$PWD/sql/sql.pri)
include ($$PWD/rpc/rpc.pri)




