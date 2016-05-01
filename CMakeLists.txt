cmake_minimum_required(VERSION 2.8)
include_directories(include/ libre/include/)
link_directories(libre_build/)

add_subdirectory(libre/mk libre_build)

add_custom_command(OUTPUT include/docopt.h
    COMMAND docopt.c/docopt_c.py options.docopt > include/docopt.h
    DEPENDS options.docopt)
add_custom_target(Docopt DEPENDS include/docopt.h)

set(CMAKE_CXX_FLAGS "-std=gnu++11 -ggdb3 -Wall -fno-omit-frame-pointer")

add_executable(sipperf src/sipperf.cpp src/stack.cpp src/useragent.cpp src/timer.cpp src/uamanager.cpp)
add_dependencies(sipperf Docopt)
add_dependencies(sipperf re)
target_link_libraries(sipperf re pthread crypto z resolv)
