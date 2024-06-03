add_executable(shvbrclabprovider
    src/appclioptions.cpp
    src/brclabfsnode.cpp
    src/brclabnode.cpp
    src/brclabparser.cpp
    src/brclabusersnode.cpp
    src/main.cpp
    src/shvbrclabproviderapp.cpp
    )

target_link_libraries(shvbrclabprovider libshv::libshviotqt)

install(TARGETS shvbrclabprovider)
