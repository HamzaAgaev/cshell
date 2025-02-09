add_library(cache-library SHARED
        source/lab2.c
        source/cache.c
)
target_include_directories(cache-library PUBLIC include)
target_link_libraries(cache-library PRIVATE common)