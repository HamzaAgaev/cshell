add_library(benchmark-2
        source/benchmark-2.c
)
target_include_directories(benchmark-2 PUBLIC include)
target_link_libraries(benchmark-2 PRIVATE common)