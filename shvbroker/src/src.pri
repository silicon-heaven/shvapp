
HEADERS += \
    $$PWD/appclioptions.h \
    $$PWD/brokerapp.h

SOURCES += \
    $$PWD/main.cpp\
    $$PWD/appclioptions.cpp \
    $$PWD/brokerapp.cpp

include ($$PWD/rpc/rpc.pri)
include ($$PWD/sql/sql.pri)




