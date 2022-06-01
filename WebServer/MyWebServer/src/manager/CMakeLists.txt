aux_source_directory(. DIR_manager_SRCS)

add_library(manager ${DIR_manager_SRCS})

target_link_libraries(manager memory libserver_base)