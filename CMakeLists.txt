cmake_minimum_required(VERSION 3.23)
project(semestralkaplsplspls C)

set(CMAKE_C_STANDARD 99)


#libveci
find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})


# Add both main.c and functions.c to the executable
add_executable(semestralkaplsplspls main.c function.c lzw.c lzw.h files.c files.h embeding.c embeding.h)


target_link_libraries(semestralkaplsplspls  ${PNG_LIBRARY})