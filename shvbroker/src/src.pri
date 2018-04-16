
HEADERS += \
    $$PWD/appclioptions.h \
    $$PWD/brokerapp.h \
    $$PWD/brokernode.h \
    $$PWD/shvclientnode.h \
    $$PWD/subscriptionsnode.h

SOURCES += \
    $$PWD/main.cpp\
    $$PWD/appclioptions.cpp \
    $$PWD/brokerapp.cpp \
    $$PWD/brokernode.cpp \
    $$PWD/shvclientnode.cpp \
    $$PWD/subscriptionsnode.cpp

include ($$PWD/sql/sql.pri)
include ($$PWD/rpc/rpc.pri)




