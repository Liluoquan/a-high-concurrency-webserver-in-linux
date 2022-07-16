#pragma once


#include <unistd.h>
#include <dirent.h>

#include <vector>
#include <string>
#include <iostream>
#include <memory>

#include "locker/mutex_lock.h"
#include "log/logging.h"


namespace manager
{
    
class FileManager {
    using fileManagerPtr = std::shared_ptr<FileManager>;
    
 public:
    FileManager();
    ~FileManager();

    static fileManagerPtr getFileManager();
    static std::vector<std::string> getFileList() { return fileList_; }
    static std::string getFileName();


 private:
    static std::vector<std::string> fileList_;
    static locker::MutexLock mutex_;
    static fileManagerPtr instance_;
};


} // namespace manager



