aux_source_directory(. DIR_THREADPOOL_SRCS)

add_library(threadPool ${DIR_THREADPOOL_SRCS})

target_link_libraries(threadPool memory reactor libserver_base package)
