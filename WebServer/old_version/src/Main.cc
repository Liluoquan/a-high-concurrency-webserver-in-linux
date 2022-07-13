// @Author: Lawson
// @CreateDate: 2022/04/08
// @ModifiedDate: 2022/04/16

#include <getopt.h>
#include <stdio.h>
#include <string>
#include "base/Logging.h"
#include "server/HTTPServer.h"
#include "server/FTPServer.h"

// #define FTPSERVER_DEBUG



int main(int argc, char* argv[]) {
    int threadNum = 2;
    int port = 8888;
    unsigned short commandOffset = 3;
    std::string rootDir = "./";
    std::string logPath = "./WebServer.log";

    // parse args
    int opt;
    const char* str = "t:l:p:o:r:";
    while((opt = getopt(argc, argv, str)) != -1) {
        switch(opt) {
            case 't' : {  // threadNum
                threadNum = atoi(optarg);
                break;
            }
            case 'l' : {  // logPath
                logPath = optarg;
                if(logPath.size() < 2 || optarg[0] != '/') {
                    printf("logPath should start with \"/\"\n");
                    abort();
                }
                break;
            }
            case 'p' : {  // port
                port = atoi(optarg);
                break;
            }
            case 'v' : {  // version
                printf("WebServer version 3.0 (base on muduo)\n");
                break;
            }
            case 'o' : {  // commandOffset
                commandOffset = atoi(optarg);
                break;
            }
            case 'r' : {  // rootDir
                rootDir = optarg;
                if(rootDir.size() < 2 || optarg[0] != '/') {
                    printf("logPath should start with \"/\"\n");
                    return EXIT_SUCCESS;
                }
                break;
            }
            default:  {  // 参数不存在
                printf("Usage: %s [-t threadNum] [-l logFilePath] [-p port] [-v] \n", argv[0]);
                return 0;
            }
        }
    }

    // STL库在多线程上应用
#ifndef _PTHREADS
    LOG << "_PTHREADS is not defined !";
#endif
    init_MemoryPool();

    Logger::setLogFileName(logPath);


#ifdef FTPSERVER_DEBUG
    // std::cout << "rootDir = " << rootDir << std::endl;
    FTPServer myFTPServer(threadNum, port, rootDir, commandOffset);
    myFTPServer.start();
#else
    HTTPServer myHTTPServer(threadNum, port);
    myHTTPServer.start();
#endif

    return 0;
}

