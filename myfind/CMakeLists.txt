add_executable(myfind
    ${YOUR_DIRECTORY}src/myfind.c
)

target_include_directories(myfind PRIVATE ${YOUR_DIRECTORY}include)
