cmake_minimum_required(VERSION 2.8)

set(CMAKE_VERBOSE_MAKEFILE TRUE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(pdf_reader)

link_directories("/usr/local/lib64")
include_directories("inc" "/usr/local/include/poppler")
file(GLOB SOURCES src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE poppler)
