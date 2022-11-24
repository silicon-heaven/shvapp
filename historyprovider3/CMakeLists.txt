add_library(libhistoryprovider
    src/appclioptions.cpp
    src/leafnode.cpp
    src/getlog.cpp
    src/historyapp.cpp
    src/shvjournalnode.cpp
    src/utils.cpp
    )

target_enable_coroutines(libhistoryprovider)
target_include_directories(libhistoryprovider PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libhistoryprovider PUBLIC QCoro::Coro libshviotqt)

add_executable(historyprovider3
    src/main.cpp
    )

target_enable_coroutines(historyprovider3)
target_link_libraries(historyprovider3 libhistoryprovider)

install(TARGETS historyprovider3)

if(BUILD_TESTING)
    add_library(mockrpcconnection STATIC
        tests/mockrpcconnection.cpp
        tests/utils.cpp
        )
    target_link_libraries(mockrpcconnection libhistoryprovider)
    target_enable_coroutines(mockrpcconnection)

    function(history_test name)
        set(TEST_NAME test_historyprovider_${name})
        add_executable(${TEST_NAME}
            tests/${name}.cpp
            )

        target_enable_coroutines(${TEST_NAME})
        target_link_libraries(${TEST_NAME} libhistoryprovider mockrpcconnection)
        add_test(NAME ${TEST_NAME} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME})
    endfunction()

    history_test(methods)
    history_test(synclog)
    history_test(slavehp)
    history_test(hp_getlog)
    history_test(sites_timeout)
    history_test(connection_failure)
    history_test(getlog)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tests/test_vars.h.in ${CMAKE_CURRENT_BINARY_DIR}/test_vars.h @ONLY)
    target_include_directories(libhistoryprovider PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
endif()
