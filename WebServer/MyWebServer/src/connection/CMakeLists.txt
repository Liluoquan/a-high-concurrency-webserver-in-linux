aux_source_directory(. DIR_CONNECTION_SRCS)

add_library(connection ${DIR_CONNECTION_SRCS})

target_link_libraries(connection manager reactor LFUCache package libserver_base)

