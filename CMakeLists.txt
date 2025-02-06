cmake_minimum_required(VERSION 3.26)
project(OS_lab_2 C)

set(CMAKE_C_STANDARD 23)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Using sanitizers for Linux: address, leak, undefined")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address,leak,undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,leak,undefined")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin") # macOS
    message(STATUS "Using sanitizers for macOS: address, undefined")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -gdwarf-4 -fsanitize=address,undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-4 -fsanitize=address,undefined")
endif ()

add_subdirectory(shell)
add_subdirectory(benchmark-starter)
add_subdirectory(common)
