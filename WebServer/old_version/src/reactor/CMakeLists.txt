aux_source_directory(. DIR_REACTOR_SRCS)

add_library(reactor ${DIR_REACTOR_SRCS} ${DIR_CONNECTION_SRCS})

target_link_libraries(reactor connection memory LFUCache libserver_base package)
