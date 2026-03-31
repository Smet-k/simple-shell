cmake_minimum_required(VERSION 3.14.0)
project(simple-shell LANGUAGES C)

set(CMAKE_BUILD_TYPE debug)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)

file(GLOB shell_src "src/*")
add_executable(simple-shell ${shell_src})
target_link_libraries(simple-shell readline)
target_include_directories(simple-shell PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)