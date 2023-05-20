add_executable(mypopen
    ${YOUR_DIRECTORY}test/mypopen.test.c
    ${YOUR_DIRECTORY}src/mypopen.c
)

target_include_directories(mypopen PRIVATE ${YOUR_DIRECTORY}src)
