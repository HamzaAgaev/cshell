add_library(benchmark-1
        source/benchmark-1.c
        source/file-state.c
        source/heap-lib.c
)
target_include_directories(benchmark-1 PUBLIC include)
target_link_libraries(benchmark-1 PRIVATE common)