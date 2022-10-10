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
    function(history_test name)
        set(TEST_NAME test_historyprovider_${name})
        add_executable(${TEST_NAME}
            tests/${name}.cpp
            )

        target_enable_coroutines(${TEST_NAME})
        target_link_libraries(${TEST_NAME} libhistoryprovider)
        add_test(NAME ${TEST_NAME} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME})
    endfunction()

    history_test(test)
endif()
