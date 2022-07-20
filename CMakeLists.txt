cmake_minimum_required(VERSION 3.23)
project(shv LANGUAGES CXX)
include(CTest)

add_subdirectory(3rdparty/necrolog)
add_subdirectory(3rdparty/libshv)

find_package(Qt5 REQUIRED COMPONENTS Core OPTIONAL_COMPONENTS Widgets Svg Xml)

if(Qt5_FOUND)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    if (Qt5Widgets_FOUND AND Qt5Svg_FOUND AND Qt5Xml_FOUND)
        add_subdirectory(clients/bfsview)
    endif()

    if (Qt5Widgets_FOUND AND Qt5Xml_FOUND)
        add_subdirectory(clients/jn50view)
    endif()

else()
    message(STATUS "Qt5 not found")
endif()
