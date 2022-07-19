cmake_minimum_required(VERSION 3.23)
project(shv LANGUAGES CXX)
include(CTest)

add_subdirectory(3rdparty/necrolog)
add_subdirectory(3rdparty/libshv)

find_package(Qt5 REQUIRED COMPONENTS Core)

if(Qt5_FOUND)
    set(CMAKE_AUTOMOC ON)
else()
    message(STATUS "Qt5 not found")
endif()
