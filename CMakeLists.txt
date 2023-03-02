cmake_minimum_required(VERSION 3.18.4)

set(USE_QT6 ON CACHE BOOL "Qt6 build")

project(shv LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)
set(CMAKE_SHARED_LIBRARY_PREFIX "") # we don't want CMake to prepend "lib" to our libraries, we prefer adding that ourselves
set(CMAKE_STATIC_LIBRARY_PREFIX "") # we don't want CMake to prepend "lib" to our libraries, we prefer adding that ourselves

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(SHV_WARNING_FLAGS "-Wall -Wextra -pedantic -Wcast-align -Wunused -Wpedantic -Wmisleading-indentation -Wdouble-promotion -Wformat=2 -Wimplicit-fallthrough")
    set(SHV_WARNING_FLAGS "${SHV_WARNING_FLAGS} -Wno-sign-conversion")
    set(SHV_WARNING_FLAGS_CXX "-Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual")

	# We switched to qsizetype in Qt6, so let's get rid of this warning in Qt5 for now.
	# TODO: Remove this after dropping support for Qt5
	if(USE_QT6)
		set(SHV_WARNING_FLAGS "${SHV_WARNING_FLAGS} -Wconversion")
	endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(SHV_WARNING_FLAGS "-Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wshadow=local ${SHV_WARNING_FLAGS}")
        set(SHV_WARNING_FLAGS_CXX "-Wuseless-cast ${SHV_WARNING_FLAGS_CXX}")
    else() # Clang
        set(SHV_WARNING_FLAGS "-Wshadow ${SHV_WARNING_FLAGS}")
    endif()

    set(SHV_WARNING_FLAGS_CXX "${SHV_WARNING_FLAGS} ${SHV_WARNING_FLAGS_CXX}")

    set(CMAKE_C_FLAGS "${SHV_WARNING_FLAGS} ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${SHV_WARNING_FLAGS_CXX} ${CMAKE_CXX_FLAGS}")
endif()

if (NOT TARGET libnecrolog)
        add_subdirectory(3rdparty/necrolog)
endif()
if (NOT TARGET libshvchainpack-c)
        add_subdirectory(3rdparty/libshv)
endif()

if(USE_QT6)
	set(SHV_QT_VERSION_MAJOR 6)
else()
	set(SHV_QT_VERSION_MAJOR 5)
endif()

find_package(Qt${SHV_QT_VERSION_MAJOR} COMPONENTS Core Concurrent OPTIONAL_COMPONENTS SerialPort Widgets Svg Xml Quick Mqtt)

set(Qt_FOUND ${Qt${SHV_QT_VERSION_MAJOR}_FOUND})
set(QtWidgets_FOUND ${Qt${SHV_QT_VERSION_MAJOR}Widgets_FOUND})
set(QtSvg_FOUND ${Qt${SHV_QT_VERSION_MAJOR}Svg_FOUND})
set(QtQuick_FOUND ${Qt${SHV_QT_VERSION_MAJOR}Quick_FOUND})
set(QtMqtt_FOUND ${Qt${SHV_QT_VERSION_MAJOR}Mqtt_FOUND})
set(QtXml_FOUND ${Qt${SHV_QT_VERSION_MAJOR}Xml_FOUND})

function(target_enable_coroutines target_name)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${target_name} PRIVATE "-fcoroutines-ts")
    else()
        target_compile_options(${target_name} PRIVATE "-fcoroutines")
    endif()
endfunction()

if(Qt_FOUND)
    set(CMAKE_AUTOMOC ON)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_AUTOMOC_MOC_OPTIONS "-b${CMAKE_CURRENT_SOURCE_DIR}/cmake/qt_moc_disable_warnings.h")
    endif()
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    if (QtWidgets_FOUND AND QtSvg_FOUND AND QtXml_FOUND)
        add_subdirectory(clients/bfsview)
    endif()

    if (QtWidgets_FOUND AND QtXml_FOUND)
        add_subdirectory(clients/jn50view)
    endif()

    if(WITH_SHVSITES)
        if (QtQuick_FOUND)
            add_subdirectory(clients/shvsites)
        else()
            message(FATAL_ERROR "shvsites build requested, but Qt Quick wasn't found")
        endif()
    endif()

    if (NOT WIN32)
        add_subdirectory(shvbroker)
        add_subdirectory(shvagent)
        add_subdirectory(shvrexec)
        add_subdirectory(shvrsh)
        add_subdirectory(shvsitesprovider)
        add_subdirectory(shvbrclabprovider)
        add_subdirectory(shvhistoryprovider)
        add_subdirectory(shvcall)
        if(USE_QT6)
            add_subdirectory(historyprovider3)
        else()
            message(STATUS "historyprovider3 won't be built, because it needs Qt6")
        endif()
        if(QtMqtt_FOUND)
            add_subdirectory(shv2mqtt)
        endif()
        # add_subdirectory(clients/brcgate)
    endif()
else()
    message(STATUS "Qt not found")
endif()
