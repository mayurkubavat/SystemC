cmake_minimum_required(VERSION 3.14)
project(SystemC_Learning LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SYSTEMC_HOME "" CACHE PATH "Path to SystemC installation")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(SystemC REQUIRED)

add_subdirectory(examples)

if(UVM_SystemC_FOUND)
    add_subdirectory(projects)
else()
    message(STATUS "UVM-SystemC not found — skipping projects/. Set UVM_SYSTEMC_HOME to enable.")
endif()
