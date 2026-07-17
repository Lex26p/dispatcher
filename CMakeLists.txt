cmake_minimum_required(VERSION 3.25)

project(dispatcher
    VERSION 0.1.0
    DESCRIPTION "Industrial dispatching, monitoring and automation platform"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DISPATCHER_BUILD_TESTS "Build Dispatcher tests" ON)

add_subdirectory(backend)

if(DISPATCHER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()