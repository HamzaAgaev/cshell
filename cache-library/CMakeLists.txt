add_library(cache-library SHARED
        source/cio.c
        source/cache.c
)
target_include_directories(cache-library PUBLIC include)
target_link_libraries(cache-library PRIVATE common)