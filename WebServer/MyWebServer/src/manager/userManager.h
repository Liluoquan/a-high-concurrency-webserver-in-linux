#include <fstream>
#include <unordered_map>
#include <string.h>
#include <string>
#include <memory>
#include <iostream>
#include "../base/MutexLock.h"
#include "../memory/MemoryPool.h"

#pragma once

class userManager {
public:
    using userManagerPtr = std::shared_ptr<userManager>;

    userManager(std::string userInfoFile);
    ~userManager();

    static userManagerPtr getUserManger(std::string userInfoFile);
    static bool login(std::string name, std::string psw);

private:
    static std::unordered_map<std::string, std::string> userInfo_;
    static userManagerPtr instance_;
    static MutexLock mutex_;
};




