cmake_minimum_required(VERSION 3.13)
project(webbtraders)

set(CMAKE_CXX_STANDARD 17)
set(SRC_LIST src/queue.cpp src/engine.cpp src/serialize.cpp)

add_executable(webbtraders src/main.cpp ${SRC_LIST})
add_executable(webbtraders-test tests/test.cpp ${SRC_LIST})