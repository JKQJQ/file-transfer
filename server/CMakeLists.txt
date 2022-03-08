cmake_minimum_required(VERSION 3.16)
project(file_server)

set(CMAKE_CXX_STANDARD 17)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
add_executable(file_server src/main.cpp src/file_server.h)
target_link_libraries(file_server PRIVATE Threads::Threads)