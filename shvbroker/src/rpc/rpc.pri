HEADERS += \
    $$PWD/serverconnection.h \
    $$PWD/tcpserver.h \
    $$PWD/masterbrokerconnection.h \
    $$PWD/commonrpcclienthandle.h \

SOURCES += \
    $$PWD/serverconnection.cpp \
    $$PWD/tcpserver.cpp \
    $$PWD/masterbrokerconnection.cpp \
    $$PWD/commonrpcclienthandle.cpp \

with-shvwebsockets {
HEADERS += \
    $$PWD/websocketserver.h \
    $$PWD/websocket.h

SOURCES += \
    $$PWD/websocketserver.cpp \
    $$PWD/websocket.cpp
}
