include_directories(${PROJECT_SOURCE_DIR}/src/base)
include_directories(${PROJECT_SOURCE_DIR}/src/server)

add_executable(WebServer Main.cpp)

target_link_libraries(WebServer
    server
    libserver_base
)


# 该命令告诉CMake去子目录中查看可用的CMakeLists.txt文件
add_subdirectory(base)
add_subdirectory(LFUCache)
add_subdirectory(client)
add_subdirectory(connection)
add_subdirectory(memory)
add_subdirectory(package)
add_subdirectory(reactor)
add_subdirectory(server)
add_subdirectory(threadPool)
add_subdirectory(manager)
add_subdirectory(FTPClient)



