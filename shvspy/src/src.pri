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
    $$PWD/lastusedparamswidget.h \
    $$PWD/methodparametersdialog.h \
    $$PWD/texteditdialog.h \
    $$PWD/dlgcallshvmethod.h \
    $$PWD/subscriptionswidget.h \
    $$PWD/subscriptionsmodel/subscriptionsmodel.h

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
    $$PWD/lastusedparamswidget.cpp \
    $$PWD/methodparametersdialog.cpp \
    $$PWD/texteditdialog.cpp \
    $$PWD/dlgcallshvmethod.cpp \
    $$PWD/subscriptionswidget.cpp \
    $$PWD/subscriptionsmodel/subscriptionsmodel.cpp

FORMS += \
	$$PWD/mainwindow.ui \
	$$PWD/dlgserverproperties.ui \
	$$PWD/dlgsubscriptionparameters.ui \
    $$PWD/lastusedparamswidget.ui \
    $$PWD/methodparametersdialog.ui \
    $$PWD/texteditdialog.ui \
    $$PWD/subscriptionswidget.ui \
    $$PWD/dlgcallshvmethod.ui

include ( $$PWD/log/log.pri )
