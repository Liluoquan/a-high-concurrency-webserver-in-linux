#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include <dirent.h>
#include <memory>

#include "../base/MutexLock.h"
#include "../base/Logging.h"

#pragma once

class fileManager {
public:
    using fileManagerPtr = std::shared_ptr<fileManager>;

    fileManager();
    ~fileManager();

    static fileManagerPtr getFileManager();
    static std::vector<std::string> getFileList() { return fileList_; }
    static std::string getFileName();


private:
    static std::vector<std::string> fileList_;
    static MutexLock mutex_;
    static fileManagerPtr instance_;
};
