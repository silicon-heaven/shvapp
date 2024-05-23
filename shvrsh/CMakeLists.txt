add_executable(shvrsh
    src/appclioptions.cpp
    src/main.cpp
    src/shvrshapp.cpp
    )

target_link_libraries(shvrsh libshv::libshviotqt)

install(TARGETS shvrsh)
