cmake_minimum_required(VERSION 3.13)
project(webbtraders)

set(CMAKE_CXX_STANDARD 11)

add_executable(webbtraders src/main.cpp src/engine.cpp src/serialize.cpp)

add_executable(webbtraders-test tests/test.cpp src/engine.cpp src/serialize.cpp)