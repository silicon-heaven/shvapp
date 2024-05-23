add_executable(shvsites
    appclioptions.cpp
    main.cpp
    shvsites.qrc
    shvsitesapp.cpp
    sitesmodel.cpp
    )

target_link_libraries(shvsites Qt::Quick libshv::libshviotqt)

install(TARGETS shvsites)
