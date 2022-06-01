# 将多个源文件复制给LIB_SRC变量
set(LIB_SRC
    AsyncLogging.cpp
    CountDownLatch.cpp
    FileUtil.cpp
    LogFile.cpp
    Logging.cpp
    LogStream.cpp
    Thread.cpp
)

# 将LIB_SRC编译成名为libserver_base的静态库（动态库需要加SHARED关键字）
add_library(libserver_base STATIC ${LIB_SRC})

# 将libserver_base与其他库链接（不加后缀）
target_link_libraries(libserver_base pthread rt)

# 设置libserver_base的输出名为"server_base"
set_target_properties(libserver_base PROPERTIES OUTPUT_NAME "server_base")

# 该命令告诉CMake去子目录中查看可用的CMakeLists.txt文件
add_subdirectory(tests)
