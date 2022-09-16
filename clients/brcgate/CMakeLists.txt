add_executable(brcgate
    src/appclioptions.cpp
    src/main.cpp
    src/application.cpp
    )

target_link_libraries(brcgate libshviotqt)

install(TARGETS brcgate)
