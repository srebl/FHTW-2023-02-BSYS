cmake_minimum_required(VERSION 3.10.0)

project(myfind)

find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)

add_subdirectory(myfind)


