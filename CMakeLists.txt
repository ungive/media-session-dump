cmake_minimum_required(VERSION 3.10)
project("msd")

include_directories(.)
add_executable(msd ./main.cpp)

target_link_libraries(msd PRIVATE RuntimeObject)
target_compile_options(msd PRIVATE /utf-8)

set_property(TARGET msd PROPERTY CXX_STANDARD 20)
set_property(TARGET msd PROPERTY CMAKE_CXX_STANDARD_REQUIRED ON)
