cmake_minimum_required(VERSION 3.10.0)

project(myfind)

include(CTest)

add_subdirectory(myfind)

add_subdirectory(mypopen)

add_subdirectory(ring_buffer)
