cmake_minimum_required(VERSION 3.10)
# -DCMAKE_TOOLCHAIN_FILE=/mnt/f/vcpkg/scripts/buildsystems/vcpkg.cmake
project(Rtp1078)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED)
# Note: 11 target(s) were omitted.

include_directories(${Boost_INCLUDE_DIRS})

add_executable(Rtp1078 main.cpp)
target_link_libraries(Rtp1078 LINK_PUBLIC ${Boost_LIBRARIES})
