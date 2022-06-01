#include "fileManager.h"

#define DEBUG_FILEMANAGER

std::vector<std::string> fileManager::fileList_;
MutexLock fileManager::mutex_;
fileManager::fileManagerPtr fileManager::instance_ = nullptr;

fileManager::fileManager() {
    std::string filePath(getcwd(NULL, 0));
    if(filePath.empty()) {
        LOG << "get filePath failed";
        exit(0);
    }
    DIR* curDir = opendir(filePath.c_str());
    if(curDir == nullptr) {
        LOG << "get filePath failed";
        exit(0);
    }

    auto dirPtr = readdir(curDir);
    while(dirPtr != nullptr) {
        if(dirPtr->d_type == DT_REG)
            fileList_.emplace_back(dirPtr->d_name);
        dirPtr = readdir(curDir);
    }
    closedir(curDir);

#ifdef DEBUG_FILEMANAGER
    std::cout << "fileList is: " << std::endl;
    for(const std::string& file : fileList_) {
        std::cout<< file << " ";
    }
    std::cout << std::endl;
#endif
}

fileManager::~fileManager() {}



fileManager::fileManagerPtr fileManager::getFileManager() {
    {
        MutexLockGuard lock(mutex_);
        if(instance_ == nullptr)
            instance_ = fileManagerPtr(new fileManager());
    }
    return instance_;
}

