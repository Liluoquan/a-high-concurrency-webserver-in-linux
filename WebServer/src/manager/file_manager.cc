#include "manager/file_manager.h"
#include "log/logging.h"

#define DEBUG_FILEMANAGER

namespace manager
{

std::vector<std::string> FileManager::fileList_;
locker::MutexLock FileManager::mutex_;
FileManager::fileManagerPtr FileManager::instance_ = nullptr;

FileManager::FileManager() {
    std::string filePath(getcwd(NULL, 0));
    if(filePath.empty()) {
        LOG(DEBUG) << "get filePath failed";
        exit(0);
    }
    DIR* curDir = opendir(filePath.c_str());
    if(curDir == nullptr) {
        LOG(DEBUG) << "get filePath failed";
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

FileManager::~FileManager() {}

FileManager::fileManagerPtr FileManager::getFileManager() {
    {
        locker::LockGuard lock(mutex_);
        if(instance_ == nullptr)
            instance_ = fileManagerPtr(new FileManager());
    }
    return instance_;
}

} // namespace manager


