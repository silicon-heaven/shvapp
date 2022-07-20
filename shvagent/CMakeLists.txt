add_executable(shvagent
    src/appclioptions.cpp
    src/main.cpp
    src/sessionprocess.cpp
    src/shvagentapp.cpp
    src/testernode.cpp
    )

target_link_libraries(shvagent libshviotqt)
