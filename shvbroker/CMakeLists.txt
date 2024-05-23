add_executable(shvbroker
    src/main.cpp
    src/shvbrokerapp.cpp
    )

target_link_libraries(shvbroker libshv::libshvbroker)

install(TARGETS shvbroker)
