add_executable(shvbroker
    src/main.cpp
    src/shvbrokerapp.cpp
    )

target_link_libraries(shvbroker libshvbroker)

install(TARGETS shvbroker)
