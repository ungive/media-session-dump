cmake_minimum_required(VERSION 3.8)
project("msd")

include_directories(.)
add_executable(msd ./main.cpp)

set_property(TARGET msd PROPERTY CXX_STANDARD 20)
set_property(TARGET msd PROPERTY CMAKE_CXX_STANDARD_REQUIRED ON)
