
HEADERS += \
    $$PWD/appclioptions.h \
    $$PWD/bfsviewapp.h \
	$$PWD/mainwindow.h \
    $$PWD/visuwidget.h \
    $$PWD/settingsdialog.h \
    $$PWD/settings.h

SOURCES += \
    $$PWD/main.cpp\
    $$PWD/appclioptions.cpp \
    $$PWD/bfsviewapp.cpp \
	$$PWD/mainwindow.cpp \
    $$PWD/visuwidget.cpp \
    $$PWD/settingsdialog.cpp \
    $$PWD/settings.cpp

FORMS += \
	$$PWD/mainwindow.ui \
    $$PWD/settingsdialog.ui


#include ($$PWD/rpc/rpc.pri)




