#pragma once

#include <string.h>

#include <fstream>
#include <unordered_map>
#include <string>
#include <memory>
#include <iostream>

#include "locker/mutex_lock.h"
#include "memory/memory_pool.h"


namespace manager
{

class UserManager {
 public:
    using userManagerPtr = std::shared_ptr<UserManager>;
    
 public:
    UserManager(std::string userInfoFile);
    ~UserManager();

    static userManagerPtr getUserManger(std::string userInfoFile);
    static bool login(std::string name, std::string psw);

 private:
    static std::unordered_map<std::string, std::string> userInfo_;
    static userManagerPtr instance_;
    static locker::MutexLock mutex_;
};

} // namespace manager

