HEADERS += \
    $$PWD/dlgaddeditrole.h \
    $$PWD/dlgroleseditor.h \
    $$PWD/accessmodel/accessmodel.h \
    $$PWD/dlgrolesselection.h \
	$$PWD/version.h \
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
    $$PWD/subscriptionsmodel/subscriptionsmodel.h \
    $$PWD/subscriptionsmodel/subscriptionstableitemdelegate.h \
    $$PWD/dlguserseditor.h \
    $$PWD/dlgaddedituser.h \
    $$PWD/rolesmodel/rolesmodel.h


SOURCES += \
    $$PWD/dlgaddeditrole.cpp \
    $$PWD/dlgroleseditor.cpp \
    $$PWD/dlgrolesselection.cpp \
	$$PWD/main.cpp\
	$$PWD/mainwindow.cpp \
	$$PWD/dlgserverproperties.cpp \
    $$PWD/accessmodel/accessmodel.cpp \
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
    $$PWD/subscriptionsmodel/subscriptionsmodel.cpp \
    $$PWD/subscriptionsmodel/subscriptionstableitemdelegate.cpp \
    $$PWD/dlguserseditor.cpp \
    $$PWD/dlgaddedituser.cpp \
    $$PWD/rolesmodel/rolesmodel.cpp

FORMS += \
    $$PWD/dlgaddeditrole.ui \
    $$PWD/dlgroleseditor.ui \
    $$PWD/dlgrolesselection.ui \
	$$PWD/mainwindow.ui \
	$$PWD/dlgserverproperties.ui \
	$$PWD/dlgsubscriptionparameters.ui \
    $$PWD/lastusedparamswidget.ui \
    $$PWD/methodparametersdialog.ui \
    $$PWD/texteditdialog.ui \
    $$PWD/subscriptionswidget.ui \
    $$PWD/dlgcallshvmethod.ui \
    $$PWD/dlgaddedituser.ui \
    $$PWD/dlguserseditor.ui

include ( $$PWD/log/log.pri )
