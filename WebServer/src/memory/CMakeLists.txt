aux_source_directory(. DIR_MEMORY_SRCS)

add_library(memory ${DIR_MEMORY_SRCS})

target_link_libraries(memory libserver_base)

add_subdirectory(tests)