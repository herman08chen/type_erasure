cmake_minimum_required(VERSION 3.29)
project(type_erasure)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_FLAGS -O3)

add_executable(type_erasure main.cpp
        smartptr_te.cpp
        smartptr_te.h)
