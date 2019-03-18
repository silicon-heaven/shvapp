HEADERS += \
    $$PWD/tcpserver.h \
    $$PWD/masterbrokerconnection.h \
    $$PWD/commonrpcclienthandle.h \
    $$PWD/clientbrokerconnection.h

SOURCES += \
    $$PWD/tcpserver.cpp \
    $$PWD/masterbrokerconnection.cpp \
    $$PWD/commonrpcclienthandle.cpp \
    $$PWD/clientbrokerconnection.cpp

with-shvwebsockets {
HEADERS += \
    $$PWD/websocketserver.h \
    $$PWD/websocket.h

SOURCES += \
    $$PWD/websocketserver.cpp \
    $$PWD/websocket.cpp
}
