#include "manager/user_manager.h"

#include "log/logging.h"

// #define DEBUG

namespace manager
{

std::unordered_map<std::string, std::string> UserManager::userInfo_;
UserManager::userManagerPtr UserManager::instance_ = nullptr;
locker::MutexLock UserManager::mutex_;

UserManager::UserManager(std::string userInfoFile) {
    std::ifstream infile(userInfoFile);
    if(!infile.is_open()) {
        LOG(WARNING) << "open file userInfoFile failed";
        exit(0);
    }
    char name[255];
    char psw[255];
    while(infile) {
        bzero(name, 255);
        bzero(psw, 255);
        infile.getline(name, 255);
        infile.getline(psw, 255);
        userInfo_[name] = psw;
    }
    #ifdef DEBUG
    LOG(DEBUG) << "load userInfoFile successfully";
    #endif
}

UserManager::~UserManager() {}

UserManager::userManagerPtr UserManager::getUserManger(std::string userInfoFile) {
    {
        locker::LockGuard lock(mutex_);
        if(instance_ == nullptr) {
            // instance_ = userManagerPtr(newElement<UserManager>(userInfoFile), deleteElement<UserManager>);
            instance_ = userManagerPtr(new UserManager(userInfoFile));
        }
    }
    return instance_;
}

bool UserManager::login(std::string name, std::string psw) {
    if(userInfo_.find(name) != userInfo_.end()) {
        return userInfo_[name] == psw;
    }

    return false;
}

} // namespace manager




