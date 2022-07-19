add_executable(bfsview
    src/appclioptions.cpp
    src/bfsviewapp.cpp
    src/main.cpp
    src/mainwindow.cpp
    src/settings.cpp
    src/settingsdialog.cpp
    src/visuwidget.cpp
    bfsview.qrc
    bfsview.rc
    )

target_include_directories(bfsview PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bfsview Qt::Xml Qt::Svg libshviotqt)
