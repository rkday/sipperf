cmake_minimum_required(VERSION 3.4)
include_directories(include/ libre/include/)
link_directories(libre/)

INCLUDE(ExternalProject)

ExternalProject_Add(
    libre
    SOURCE_DIR libre
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    BUILD_COMMAND make libre.a
    INSTALL_COMMAND ""
    )

set(CMAKE_CXX_FLAGS "-std=c++11 -ggdb3 -Wall")

add_executable(sipperf src/sipperf.cpp src/stack.cpp src/sipua.cpp)
target_link_libraries(sipperf re pthread crypto z resolv)
