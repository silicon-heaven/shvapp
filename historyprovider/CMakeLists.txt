add_library(libhistoryprovider
    src/appclioptions.cpp
    src/leafnode.cpp
    src/historyapp.cpp
    src/shvjournalnode.cpp
    src/utils.cpp
    )

target_enable_coroutines(libhistoryprovider)
target_include_directories(libhistoryprovider PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libhistoryprovider PUBLIC QCoro::Coro libshviotqt)

add_executable(historyprovider
    src/main.cpp
    )

target_enable_coroutines(historyprovider)
target_link_libraries(historyprovider libhistoryprovider)

install(TARGETS historyprovider)

if(BUILD_TESTING)
    add_executable(test_historyprovider
        tests/test.cpp
        )

    target_enable_coroutines(test_historyprovider)
    target_link_libraries(test_historyprovider libhistoryprovider)
    add_test(NAME historyprovider COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_historyprovider)
endif()
