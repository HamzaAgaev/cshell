add_library(benchmark-common
        source/benchmark-utils.c
)
target_include_directories(benchmark-common PUBLIC include)
target_link_libraries(benchmark-common PRIVATE cache-library common)