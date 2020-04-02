cmake_minimum_required(VERSION 3.10)
# -DCMAKE_TOOLCHAIN_FILE=/mnt/f/vcpkg/scripts/buildsystems/vcpkg.cmake
project(Rtp1078)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-pthread")

#SET(Boost_USE_STATIC_LIBS ON)
#SET(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.72.0 REQUIRED COMPONENTS system log log_setup)
# Note: 11 target(s) were omitted.

include_directories(${Boost_INCLUDE_DIRS})

add_executable(Rtp1078 main.cpp tcp_server.h tcp_session.cpp tcp_session.h jtt1078_matcher.h)
target_link_libraries(Rtp1078 ${Boost_LIBRARIES})
