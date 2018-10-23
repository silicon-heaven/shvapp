HEADERS += \
	$$PWD/mainwindow.h \
	$$PWD/dlgserverproperties.h \
	$$PWD/theapp.h \
	$$PWD/servertreemodel/servertreemodel.h \
	$$PWD/attributesmodel/attributesmodel.h \
    $$PWD/servertreeview.h \
    $$PWD/dlgsubscriptionparameters.h \
    $$PWD/appclioptions.h \
    $$PWD/servertreemodel/shvnodeitem.h \
    $$PWD/servertreemodel/shvbrokernodeitem.h \
    $$PWD/dlgsubscriptions.h \
    $$PWD/resultview.h \
    $$PWD/lastusedparamswidget.h \
    $$PWD/methodparametersdialog.h

SOURCES += \
	$$PWD/main.cpp\
	$$PWD/mainwindow.cpp \
	$$PWD/dlgserverproperties.cpp \
	$$PWD/theapp.cpp \
	$$PWD/servertreemodel/servertreemodel.cpp \
	$$PWD/attributesmodel/attributesmodel.cpp \
    $$PWD/servertreeview.cpp \
    $$PWD/dlgsubscriptionparameters.cpp \
    $$PWD/appclioptions.cpp \
    $$PWD/servertreemodel/shvnodeitem.cpp \
    $$PWD/servertreemodel/shvbrokernodeitem.cpp \
    $$PWD/dlgsubscriptions.cpp \
    $$PWD/resultview.cpp \
    $$PWD/lastusedparamswidget.cpp \
    $$PWD/methodparametersdialog.cpp

FORMS += \
	$$PWD/mainwindow.ui \
	$$PWD/dlgserverproperties.ui \
	$$PWD/dlgsubscriptionparameters.ui \
	$$PWD/dlgsubscriptions.ui \
	$$PWD/resultview.ui \
    $$PWD/lastusedparamswidget.ui \
    $$PWD/methodparametersdialog.ui

include ( $$PWD/log/log.pri )
