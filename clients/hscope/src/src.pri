
HEADERS += \
		   $$PWD/appclioptions.h \
		   $$PWD/hscopeapp.h \
		   $$PWD/hscopenode.h \
		   $$PWD/lua_utils.h \

SOURCES += \
		  $$PWD/main.cpp\
		  $$PWD/appclioptions.cpp \
		  $$PWD/hscopeapp.cpp \
		  $$PWD/hscopenode.cpp

#include ($$PWD/rpc/rpc.pri)

