set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

find_package(Threads REQUIRED)

add_executable(sender
    ${YOUR_DIRECTORY}src/common.c
    ${YOUR_DIRECTORY}src/sender.c
)

target_include_directories(sender PRIVATE ${YOUR_DIRECTORY}include)

target_link_libraries(sender Threads::Threads rt)


add_executable(receiver
    ${YOUR_DIRECTORY}src/common.c
    ${YOUR_DIRECTORY}src/receiver.c
)

target_include_directories(receiver PRIVATE ${YOUR_DIRECTORY}include)

target_link_libraries(receiver Threads::Threads rt)

