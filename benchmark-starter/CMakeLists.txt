add_subdirectory(benchmark-1)
add_subdirectory(benchmark-2)

add_executable(benchmark-starter main.c)
target_link_libraries(benchmark-starter PRIVATE benchmark-1 benchmark-2 common)