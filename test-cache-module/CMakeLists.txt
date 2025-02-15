file(GLOB_RECURSE sources CONFIGURE_DEPENDS
        source/*.c
        include/*.h
)

add_executable(test-cache-module ${sources})
target_include_directories(test-cache-module PRIVATE include)
target_link_libraries(test-cache-module PRIVATE cache-library common)