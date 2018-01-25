
HEADERS += \
    $$PWD/theapp.h \
    $$PWD/appclioptions.h \

SOURCES += \
    $$PWD/main.cpp\
    $$PWD/theapp.cpp \
    $$PWD/appclioptions.cpp \

include ($$PWD/rpc/rpc.pri)
include ($$PWD/sql/sql.pri)




