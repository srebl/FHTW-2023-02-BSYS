add_executable(myfind
    ${YOUR_DIRECTORY}src/myfind.c
)

target_include_directories(myfind PRIVATE ${YOUR_DIRECTORY}include)


add_library(myfind_lib src/myfind.c)
target_include_directories(myfind PUBLIC include)

add_executable(myfind_test 
test/main.cpp 
test/myfind_test.cpp 
)

target_link_libraries(myfind_test myfind_lib)

catch_discover_tests(myfind_test)