aux_source_directory(. DIR_LFUCache_SRCS)

add_library(LFUCache ${DIR_LFUCache_SRCS})

target_link_libraries(LFUCache memory libserver_base)

add_subdirectory(tests)