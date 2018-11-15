
HEADERS += \
    $$PWD/appclioptions.h \
    $$PWD/brokerapp.h \
    $$PWD/brokernode.h \
    $$PWD/subscriptionsnode.h \
    $$PWD/brokerconfigfilenode.h \
    $$PWD/clientconnectionnode.h \
    $$PWD/clientshvnode.h

SOURCES += \
    $$PWD/main.cpp\
    $$PWD/appclioptions.cpp \
    $$PWD/brokerapp.cpp \
    $$PWD/brokernode.cpp \
    $$PWD/subscriptionsnode.cpp \
    $$PWD/brokerconfigfilenode.cpp \
    $$PWD/clientconnectionnode.cpp \
    $$PWD/clientshvnode.cpp

include ($$PWD/sql/sql.pri)
include ($$PWD/rpc/rpc.pri)




