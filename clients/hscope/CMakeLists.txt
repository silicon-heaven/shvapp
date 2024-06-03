add_executable(hscope
    src/appclioptions.cpp
    src/hscopeapp.cpp
    src/hscopenode.cpp
    src/lua_utils.cpp
    src/main.cpp
)

target_include_directories(hscope PRIVATE "${LUA_INCLUDE_DIR}")
target_link_libraries(hscope PRIVATE libshv::libshviotqt "${LUA_LIBRARY}")

install(TARGETS hscope)
