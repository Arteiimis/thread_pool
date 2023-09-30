cmake_minimum_required(VERSION 3.15)
project(thread_pool_server)

# set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE Debug)
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -march=native -O3 -pthread" )
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(${project_name} SHARED
    ${PROJECT_SOURCE_DIR}/src/epool_server.cpp
    ${PROJECT_SOURCE_DIR}/src/HTTP_server.cpp
    ${PROJECT_SOURCE_DIR}/src/test_main.cpp
)
add_executable(thread_pool_server
    ${PROJECT_SOURCE_DIR}/src/test_main.cpp
)

target_link_libraries(thread_pool_server PRIVATE pthread)