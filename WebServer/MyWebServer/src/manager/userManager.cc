#include "userManager.h"
#include "../base/Logging.h"

#define DEBUG


std::unordered_map<std::string, std::string> userManager::userInfo_;
userManager::userManagerPtr userManager::instance_ = nullptr;
MutexLock userManager::mutex_;

userManager::userManager(std::string userInfoFile) {
    std::ifstream infile(userInfoFile);
    if(!infile.is_open()) {
        LOG << "open file userInfoFile failed";
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
    LOG << "load userInfoFile successfully";
    #endif
}

userManager::~userManager() {}

userManager::userManagerPtr userManager::getUserManger(std::string userInfoFile) {
    {
        MutexLockGuard lock(mutex_);
        if(instance_ == nullptr) {
            // instance_ = userManagerPtr(newElement<userManager>(userInfoFile), deleteElement<userManager>);
            instance_ = userManagerPtr(new userManager(userInfoFile));
        }
    }
    return instance_;
}

bool userManager::login(std::string name, std::string psw) {
    if(userInfo_.find(name) != userInfo_.end()) {
        return userInfo_[name] == psw;
    }

    return false;
}

