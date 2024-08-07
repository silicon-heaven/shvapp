add_executable(shvsitesprovider
    src/appclioptions.cpp
    src/appclioptions.h
    src/gitpushtask.cpp
    src/gitpushtask.h
    src/main.cpp
    src/nodes.cpp
    src/nodes.h
    src/sitesproviderapp.cpp
    src/sitesproviderapp.h
    )

target_link_libraries(shvsitesprovider libshv::libshviotqt)

install(TARGETS shvsitesprovider)
