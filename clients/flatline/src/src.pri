
HEADERS += \
    $$PWD/mainwindow.h\
    $$PWD/appclioptions.h \
    $$PWD/graphframe.h \
    $$PWD/datasample.h \
    $$PWD/flatlineapp.h

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/mainwindow.cpp \
    $$PWD/appclioptions.cpp \
    $$PWD/graphframe.cpp \
    $$PWD/datasample.cpp \
    $$PWD/flatlineapp.cpp

FORMS += \
    $$PWD/mainwindow.ui\

include ($$PWD/timeline/timeline.pri)



