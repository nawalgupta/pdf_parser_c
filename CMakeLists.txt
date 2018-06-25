cmake_minimum_required(VERSION 2.8)

project(pdf_reader)

set(CMAKE_VERBOSE_MAKEFILE TRUE)
set(POPPLER_SOURCE_DIRECTORY "${CMAKE_SOURCE_DIR}/poppler")

include_directories(${POPPLER_SOURCE_DIRECTORY} ${POPPLER_SOURCE_DIRECTORY}/poppler poppler-header)

add_executable(${PROJECT_NAME} "main.cpp" "parseargs.cc" "printencodings.cc" "Win32Console.cc")
target_link_libraries(${PROJECT_NAME} poppler)
