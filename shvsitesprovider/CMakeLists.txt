add_executable(shvsitesprovider
    src/appclioptions.cpp
    src/dirsynctask.cpp
    src/gitpushtask.cpp
    src/main.cpp
    src/nodes.cpp
    src/sitesproviderapp.cpp
    )

target_link_libraries(shvsitesprovider libshviotqt)

install(TARGETS shvsitesprovider)
