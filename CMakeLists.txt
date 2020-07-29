cmake_minimum_required(VERSION 3.8)
project(wave_writer)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES main.cpp Wave.h)
add_executable(wave_writer ${SOURCE_FILES})