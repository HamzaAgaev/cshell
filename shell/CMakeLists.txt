file(GLOB_RECURSE sources CONFIGURE_DEPENDS
    source/*.c
    include/*.h
)

add_executable(shell ${sources})
target_include_directories(shell PRIVATE source include)