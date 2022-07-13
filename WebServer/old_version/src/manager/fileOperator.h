// @Author: Lawson
// @CreateDate: 2022/05/13
// @LastModifiedDate: 2022/05/13

#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <list>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <grp.h>
#include <pwd.h>
#include <dirent.h>

// 缓冲区大小
#define BUFFER_SIZE 4096

// 用于指示服务器根目录
#define SERVERROOTPATHSTRING "<root>/"


// bool strict 代表对其他目录的访问权
// strict = true: 只能访问当前工作目录
// strict = false: 可以访问其他目录
class fileOperator {
public:
    fileOperator(std::string dir);
    virtual ~fileOperator();
    int readFile(std::string fileName);
    char* readFileBlock(unsigned long& sizeInBytes);
    int writeFileAtOnce(std::string fileName, char* content);
    int beginWriteFile(std::string fileName);
    int writeFileBlock(std::string content);
    int closeWriteFile();
    bool changeDir(std::string newPath, bool strict = true);
    std::string getCurrentWorkingDir(bool showRootPath = true);
    bool createFile(std::string& fileName, bool strict = true);
    bool createAndCoverFile(std::string& fileName, bool strict = true);
    bool createDirectory(std::string& dirName, bool strict = true);
    bool deleteDirectory(std::string dirName, bool cancel = false, std::string pathToDir = "");
    bool deleteFile(std::string fileName, bool strict = true);
    void browse(std::string dir, std::vector<std::string> &directories, std::vector<std::string>& files, bool strict = true);
    bool dirCanBeOpened(std::string dir);
    std::string getParentDir();
    unsigned long getDirSize(std::string dirName);
    std::vector<std::string> getStats(std::string fileName, struct stat Status);
    void clearListOfDeletedFiles();
    void clearListOfDeletedDirectories();
    std::vector<std::string> getListOfDeletedFiles();
    std::vector<std::string> getListOfDeletedDirectories();
    bool dirIsBelowServerRoot(std::string dirName);
    std::ofstream currentOpenFile_;
    std::ifstream currentOpenReadFile_;

private:
    std::vector<std::string> deletedDirectories_;
    std::vector<std::string> deletedFiles_;
    void getValidDir(std::string& dirName);
    void getValidFile(std::string& fileName);
    void stripServerRootString(std::string& dirOrFileName);
    
    std::list<std::string> completePath_; //从服务器根目录向上到当前工作目录的路径，每个列表元素包含一个目录
    static void IntToString(int i, std::string& res);
};

