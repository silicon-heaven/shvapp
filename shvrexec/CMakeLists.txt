add_executable(shvrexec
    src/appclioptions.cpp
    src/main.cpp
    src/process.cpp
    src/ptyprocess.cpp
    src/shvrexecapp.cpp
    )

target_link_libraries(shvrexec libshviotqt)

install(TARGETS shvrexec)
