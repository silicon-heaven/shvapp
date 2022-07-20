add_executable(jn50view
    src/appclioptions.cpp
    src/dlgapplog.cpp
    src/jn50viewapp.cpp
    src/logview/dlglogview.cpp
    src/logview/logmodel.cpp
    src/main.cpp
    src/mainwindow.cpp
    src/settings.cpp
    src/settingsdialog.cpp
    src/svghandler.cpp
    src/svgscene/graphicsview.cpp
    src/svgscene/saxhandler.cpp
    src/svgscene/simpletextitem.cpp
    src/thresholdsdialog.cpp
    src/visucontroller.cpp
    src/visuwidget.cpp
    jn50view.qrc
    jn50view.rc
    )

target_include_directories(jn50view PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(jn50view Qt::Xml libshvvisu)
