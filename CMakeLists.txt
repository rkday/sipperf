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
    BUILD_ALWAYS 1
    INSTALL_COMMAND ""
    )

add_custom_command(OUTPUT include/docopt.h
    COMMAND docopt.c/docopt_c.py options.docopt > include/docopt.h
    DEPENDS options.docopt)
add_custom_target(Docopt DEPENDS include/docopt.h)

set(CMAKE_CXX_FLAGS "-std=c++11 -ggdb3 -Wall -fsanitize=undefined -fno-omit-frame-pointer")

add_executable(sipperf src/sipperf.cpp src/stack.cpp src/sipua.cpp src/timer.cpp src/uamanager.cpp)
add_dependencies(sipperf Docopt)
add_dependencies(sipperf libre)
target_link_libraries(sipperf re pthread crypto z resolv)
