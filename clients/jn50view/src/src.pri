
HEADERS += \
    $$PWD/appclioptions.h \
    $$PWD/jn50viewapp.h \
	$$PWD/mainwindow.h \
    $$PWD/visuwidget.h \
    $$PWD/settingsdialog.h \
    $$PWD/settings.h \
    $$PWD/visucontroller.h \
    $$PWD/svghandler.h \
    $$PWD/dlgapplog.h \
    $$PWD/thresholdsdialog.h

SOURCES += \
    $$PWD/main.cpp\
    $$PWD/appclioptions.cpp \
    $$PWD/jn50viewapp.cpp \
	$$PWD/mainwindow.cpp \
    $$PWD/visuwidget.cpp \
    $$PWD/settingsdialog.cpp \
    $$PWD/settings.cpp \
    $$PWD/visucontroller.cpp \
    $$PWD/svghandler.cpp \
    $$PWD/dlgapplog.cpp \
    $$PWD/thresholdsdialog.cpp

FORMS += \
	$$PWD/mainwindow.ui \
    $$PWD/settingsdialog.ui \
    $$PWD/dlgapplog.ui \
    $$PWD/thresholdsdialog.ui


include ($$PWD/svgscene/svgscene.pri)




