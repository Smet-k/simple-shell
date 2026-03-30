cmake_minimum_required(VERSION 3.14.0)
project(simple_shell LANGUAGES C)

set(CMAKE_BUILD_TYPE debug)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)

file(GLOB shell_src "src/*")
add_executable(simple_shell ${shell_src})
target_link_libraries(simple_shell readline)
target_include_directories(simple_shell PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)