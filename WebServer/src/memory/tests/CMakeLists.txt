# 将..目录下的所有源文件赋值给DIR_SRCS变量
aux_source_directory(. Dir_source)

add_executable(test_MemoryPool ${Dir_source})

# 将libserver_base与其他库链接（不加后缀）
target_link_libraries(test_MemoryPool memory)


