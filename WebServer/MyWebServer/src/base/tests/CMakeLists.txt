# 使用指定的源文件生成目标可执行文件
add_executable(LoggingTest LoggingTest.cpp)

# 将libserver_base库链接到该可执行文件
target_link_libraries(LoggingTest libserver_base)
