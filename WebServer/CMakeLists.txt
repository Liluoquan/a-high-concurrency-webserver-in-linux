project(WebServer LANGUAGES CXX)
cmake_minimum_required(VERSION 3.5.1)

#开关
set(DEBUG OFF)
set(CHECK_MEMORY OFF)
set(OPEN_LOGGING ON)

#项目根目录
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})
#源码目录
set(SRC_DIR ${ROOT_DIR}/src)
#头文件路径
set(INC_DIR ${ROOT_DIR}/include)
#库文件路径
set(LIB_DIR /usr/lib/x86_64-linux-gnu)
#可执行文件安装路径
set(EXEC_INSTALL_DIR ${ROOT_DIR}/bin)
#库安装路径
set(LIB_INSTALL_DIR ${ROOT_DIR}/lib)

#g++ 编译选项 -Wl,-rpath指定运行时库的搜索路径 
#使用-Wl,-Bstatic后面的库都以静态链接，-Wl,-Bdynamic后面的库都以动态链接 
#还可以使用-static来避免动态链接, 此方法会导致对所有的库都以静态链接
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wfatal-errors -Wno-unused-parameter -Wl,-rpath=${LIB_INSTALL_DIR}")
if (${DEBUG} STREQUAL "ON")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    #add_definitions(-DDEBUG)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    add_definitions(-DNDEBUG)
endif()

if (${CHECK_MEMORY} STREQUAL "ON")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
endif()

if (${OPEN_LOGGING} STREQUAL "ON")
    add_definitions(-DOPEN_LOGGING)
endif()

#CPP源文件
file(GLOB UTILITY_SRC_FILE  ${SRC_DIR}/utility/*.cpp)
file(GLOB THREAD_SRC_FILE   ${SRC_DIR}/thread/*.cpp)
file(GLOB TIMER_SRC_FILE    ${SRC_DIR}/timer/*.cpp)
file(GLOB LOG_SRC_FILE      ${SRC_DIR}/log/*.cpp)
file(GLOB EVENT_SRC_FILE    ${SRC_DIR}/event/*.cpp)
file(GLOB HTTP_SRC_FILE     ${SRC_DIR}/http/*.cpp)
file(GLOB SERVER_SRC_FILE   ${SRC_DIR}/server/*.cpp)
set(MAIN_SRC_FILE           ${ROOT_DIR}/main.cpp)

set(LIBEVENT_SOURCES ${UTILITY_SRC_FILE}
                     ${THREAD_SRC_FILE}
                     ${TIMER_SRC_FILE}
                     ${LOG_SRC_FILE} 
                     ${EVENT_SRC_FILE}
                     ${HTTP_SRC_FILE}
                     ${SERVER_SRC_FILE})
#message(${LIBEVENT_SOURCES})

#头文件路径添加
include_directories(${INC_DIR})
#库文件路径添加
link_directories(${LIB_DIR})
#用到的库
set(LINK_LIBRARY pthread)

#生成动态链接库
add_library(event_shared SHARED ${LIBEVENT_SOURCES})
#更改动态链接库输出名字
set_target_properties(event_shared PROPERTIES OUTPUT_NAME event)
#设置动态链接库版本号
set_target_properties(event_shared PROPERTIES VERSION 1.2 SOVERSION 1)
#链接库
target_link_libraries(event_shared ${LINK_LIBRARY})

#生成静态库
add_library(event_static STATIC ${LIBEVENT_SOURCES})
#更改静态库输出名字
set_target_properties(event_static PROPERTIES OUTPUT_NAME event)
#链接库
target_link_libraries(event_static ${LINK_LIBRARY})

#将静态库链接 生成可执行文件
add_executable(web_server ${MAIN_SRC_FILE})
#链接库
target_link_libraries(web_server event_static ${LINK_LIBRARY})

#将所有源文件链接 生成可执行文件
#add_executable(web_server ${LIBEVENT_SOURCES} ${MAIN_SRC_FILE})
#链接库
#target_link_libraries(web_server ${LINK_LIBRARY})

#安装
install(TARGETS event_shared event_static DESTINATION ${LIB_INSTALL_DIR})
install(TARGETS web_server DESTINATION ${EXEC_INSTALL_DIR})
