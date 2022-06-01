aux_source_directory(. DIR_SERVER_SRCS)

add_library(server ${DIR_SERVER_SRCS})

target_link_libraries(server manager memory reactor threadPool libserver_base package)
