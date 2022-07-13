// @Author: Lawson
// @CreateDate: 2022/05/13
// @LastModifiedDate: 2022/05/13

#include "fileOperator.h"
#include <assert.h>

// 将传入的目录作为服务器的根目录
fileOperator::fileOperator(std::string dir) {
    this->completePath_.emplace_front(dir);
    // std::cout << "dir is: " << *completePath_.begin() << std::endl;
}

fileOperator::~fileOperator() {
    this->closeWriteFile();       // close files
    this->completePath_.clear();
}

bool fileOperator::changeDir(std::string newPath, bool strict) {
    // std::cout << "dir1 is : " << newPath << std::endl;
    if(strict) {    // strict mode
        getValidDir(newPath);
    }

    // 切换到上一级目录
    if((newPath.compare("..") == 0) || (newPath.compare("../") == 0)) { // 如果目录是.. or ../
        if(this->completePath_.size() <= 1) {
            std::cerr << "Error: Change beyond server root requested (prohibited)!" << std::endl;
            return (EXIT_FAILURE);
        }
        else {
            this->completePath_.pop_back();
            return (EXIT_SUCCESS);
        }
    }
    else if((newPath.compare(".") == 0) || (newPath.compare("./") == 0)) {
        std::cout << "Change to same dir requested (nothing done)!" << std::endl;
        return (EXIT_SUCCESS);
    }

    // std::cout << "currentWorkDir1 is : " << getCurrentWorkingDir(false) << std::endl;
    // std::cout << "dir2 is : " << (getCurrentWorkingDir(false) + newPath) << std::endl;
    
    // 切换到子目录
    if(this->dirCanBeOpened(this->getCurrentWorkingDir(false).append(newPath))) {
        this->completePath_.emplace_back(newPath);
        // std::cout << "currentWorkDir2 is : " << getCurrentWorkingDir() << std::endl;
        return (EXIT_SUCCESS);
    }

    return (EXIT_FAILURE);
}

// 获取合法的目录
// FIXME:好像只能删除斜线
// 'subdir' -> 'subdir/'
void fileOperator::getValidDir(std::string& dirName) {
    //FIXME:
    return;
    std::string slash = "/";
    size_t foundSlash = 0;
    while((foundSlash = dirName.find_first_of(slash)) != std::string::npos) {
        dirName.erase(foundSlash++, 1);
    }
    dirName.append(slash);
}

// 获取文件名
// FIXME:好像只能删除斜线
// filename ../../e.txt -> e.txt
void fileOperator::getValidFile(std::string& fileName) {
    std::string slash = "/";
    size_t foundSlash = 0;
    while((foundSlash = fileName.find_first_of(slash)) != std::string::npos) {
        fileName.erase(foundSlash++, 1);
    }
}

// 获取去除服务器根目录后的字符串
// <root>/data/file -> data/file
void fileOperator::stripServerRootString(std::string& dirOrFileName) {
    size_t foundRootString = 0;
    if(dirOrFileName.find_first_of(SERVERROOTPATHSTRING) == foundRootString) {
        int rootStringLength = ((std::string)SERVERROOTPATHSTRING).size();
        dirOrFileName = dirOrFileName.substr(rootStringLength);
    }
}

// 在当前工作目录中创建指定名称的目录
// FIXME:返回值有问题
bool fileOperator::createDirectory(std::string& dirName, bool strict) {
    if(strict) {  // strict mode
        getValidDir(dirName);
    }

    // 不允许切换到服务器根目录以外的目录
    if(dirName.compare("../") == 0 && this->completePath_.size() <= 1) {
        dirName = "./";
        std::cerr << "Error: Deletion of dir beyond server root requested (prohibited)!" << std::endl;
        return true;
    }

    umask(0);  // 赋予当前文件操作最大权限
    return (mkdir(this->getCurrentWorkingDir(false).append(dirName).c_str(), 
            S_IRWXU | S_IRWXG | S_IRWXO) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

// 从打read file中读取一个block
char* fileOperator::readFileBlock(unsigned long& sizeInBytes) {
    // 获取写入文件长度
    this->currentOpenReadFile_.seekg(0, std::ios::end);  // 将read file的读取位置设置到末尾
    std::ifstream::pos_type size = this->currentOpenReadFile_.tellg(); // 返回文件写入位置
    sizeInBytes = static_cast<unsigned long> (size);

    this->currentOpenReadFile_.seekg(0, std::ios::beg); // 将read file的读取位置设置到开头

    char* memblock = new char[size];
    this->currentOpenReadFile_.read(memblock, size);

    std::cout << "Reading " << size << " Bytes" << std::endl;
    this->currentOpenReadFile_.close();
    return memblock;
}

// 以文件流形式打开文件
int fileOperator::readFile(std::string fileName) {
    stripServerRootString(fileName);
    this->currentOpenReadFile_.open(fileName.c_str(),
                                    std::ios::in | std::ios::binary);
    if(this->currentOpenReadFile_.fail()) {
        std::cout << "Reading file '" << fileName << "' failed!" << std::endl;
        return (EXIT_FAILURE);
    }
    if(this->currentOpenReadFile_.is_open()) {
        return (EXIT_SUCCESS);
    }

    std::cerr << "Unable to open file '" << fileName << " '" << std::endl;
    return (EXIT_FAILURE);
}

// 将content写入write file
int fileOperator::writeFileBlock(std::string content) {
    if(!this->currentOpenFile_) {
        std::cerr << "Cannot write to output file" << std::endl;
        return (EXIT_FAILURE);
    }
    std::cout << "Appending to file" << std::endl;

    (this->currentOpenFile_) << content;
    return (EXIT_SUCCESS);
}

// 关闭write file
int fileOperator::closeWriteFile() {
    if(this->currentOpenFile_.is_open()) {
        std::cout << "Closing open file" << std::endl;
        this->currentOpenFile_.close();
    }
    return EXIT_SUCCESS;
}

// 将content立即写入fileName
int fileOperator::writeFileAtOnce(std::string fileName, char* content) {
    stripServerRootString(fileName);
    std::ofstream myFile(fileName.c_str(), std::ios::out | std::ios::binary);
    if(!myFile) {
        std::cerr << "Cannot open output file '" << fileName << "'" << std::endl;
        return (EXIT_FAILURE);
    }

    myFile << content;
    myFile.close();
    return (EXIT_SUCCESS);
}

// 类似于linux的touch
// Avoid touch ../file beyond server root!
bool fileOperator::createFile(std::string& fileName, bool strict) {
    if(strict) { // strict mode
        this->getValidFile(fileName);
    }
    try {
        std::ofstream fileOut;
        fileOut.open(this->getCurrentWorkingDir(false).append(fileName).c_str(), 
                    std::ios::out | std::ios::binary | std::ios::app);
        fileOut.close();
    }
    catch (std::exception e) {
        std::cerr << e.what() << std::endl;
        return (EXIT_FAILURE);
    }

    return (EXIT_SUCCESS);
}

// 创建一个文件，如果已存在，则将内容清除
bool fileOperator::createAndCoverFile(std::string& fileName, bool strict) {
    if(strict) { // strict mode
        this->getValidFile(fileName);
    }
    try {
        std::ofstream fileOut;
        std::cout << "filePath is : " << getCurrentWorkingDir(false).append(fileName) << std::endl;
        fileOut.open(this->getCurrentWorkingDir(false).append(fileName).c_str(), 
                    std::ios::out | std::ios::binary | std::ios::trunc);
        fileOut.close();
    }
    catch (std::exception e) {
        std::cerr << e.what() << std::endl;
        return (EXIT_FAILURE);
    }

    return (EXIT_SUCCESS);
}

// 删除文件
// Avoid rm ../file beyond server root!
bool fileOperator::deleteFile(std::string fileName, bool strict) {
    if(strict) {
        this->getValidFile(fileName);
    }

    if(remove(this->getCurrentWorkingDir(false).append(fileName).c_str()) != 0) {
        std::cerr << "Error deleting file '" << fileName << "'" << std::endl;
        return (EXIT_FAILURE);
    }
    else {
        std::cout << "File '" << fileName << "' deleted" << std::endl;
        this->deletedFiles_.push_back(fileName);
        return (EXIT_SUCCESS);
    }
}

// 递归删除目录（包括里面的文件）----慎用！
bool fileOperator::deleteDirectory(std::string dirName, bool cancel, std::string pathToDir) {
    if(cancel)  return true;

    getValidFile(dirName);

    std::vector<std::string>* directories = new std::vector<std::string>();
    std::vector<std::string>::iterator dirIterator;
    std::vector<std::string>* files = new std::vector<std::string>();
    std::vector<std::string>::iterator fileIterator;

    pathToDir.append(dirName);

    this->browse(pathToDir, *directories, *files, false);

    // 删除当前目录中的所有文件
    fileIterator = files->begin();
    while(fileIterator != files->end()) {
        // std::cout << "Removing file " << pathToDir + (*fileIterator) << std::endl;
        cancel = this->deleteFile(pathToDir + *fileIterator, false) || cancel;
        ++fileIterator;
    }

    // 删除当前目录中的所有子目录
    dirIterator = directories->begin();
    while(dirIterator != directories->end()) {
        // If not ./ and ../
        if((*dirIterator).compare(".") == 0 || (*dirIterator).compare("..") == 0) {
            ++dirIterator;
            continue;
        }

        cancel = this->deleteDirectory((*dirIterator).append("/"), cancel, pathToDir) || cancel;
        ++dirIterator;
    }

    if(pathToDir.compare(".") != 0 && pathToDir.compare("..") != 0) {
        if(rmdir(this->getCurrentWorkingDir(false).append(pathToDir).c_str()) < 0) {
            // 0 == success, -1 == error (errno-> EACCES access denies, ENOENT path not found)
            std::cerr << "Failed deleting directory '" << this->getCurrentWorkingDir(false).append(pathToDir) << "'" << std::endl;
        }
        else {
            std::cout << "Directory '" << pathToDir << "' deleted" << std::endl;
            this->deletedDirectories_.emplace_back(pathToDir);
        }
    }

    return cancel; // false = no error, true = error
}

//struct stat {
//  dev_t  st_dev     /* (P) Device, auf dem die Datei liegt */
//  ushort st_ino     /* (P) i-node-Nummer */
//  ushort st_mode    /* (P) Dateityp  */
//  short  st_nlink   /* (P) Anzahl der Links der Datei  */
//  ushort st_uid     /* (P) Eigentuemer-User-ID (uid)  */
//  ushort st_gid     /* (P) Gruppen-ID (gid)  */
//  dev_t  st_rdev    /* Major- und Minornumber, falls Device */
//  off_t  st_size    /* (P) Größe in Byte  */
//  time_t st_atime   /* (P) Zeitpunkt letzter Zugriffs  */
//  time_t st_mtime   /* (P) Zeitpunkt letzte Änderung  */
//  time_t st_ctime   /* (P) Zeitpunkt letzte Statusänderung */
//};
//
//struct tm {
//  int tm_sec; /* seconds */
//  int tm_min; /* minutes */
//  int tm_hour; /* hours */
//  int tm_mday; /* day of the month */
//  int tm_mon; /* month */
//  int tm_year; /* year */
//  int tm_wday; /* day of the week */
//  int tm_yday; /* day in the year */
//  int tm_isdst; /* daylight saving time */
//};

// 获取文件信息
// returns vector<string> result;
// result[0] = AccessRights（user/group/others）
// result[1] = Group
// result[2] = Owner
// result[3] = LastModificationTime
// result[4] = size
std::vector<std::string> fileOperator::getStats(std::string fileName, struct stat Status) {
    std::vector<std::string> result;
    // Check if existent, accessible
    if(stat(this->getCurrentWorkingDir().append(fileName).c_str(), &Status) != 0) {
        std::cerr << "Error when issuing stat() on '" << fileName << "'!" << std::endl;
        return result; // 返回空数组
    }

    struct passwd *pwd;
    struct group *grp;
    std::string tempRes;

    // User flags
    int usr_r = ( Status.st_mode & S_IRUSR ) ? 1 : 0;
    int usr_w = ( Status.st_mode & S_IWUSR ) ? 1 : 0;
    int usr_x = ( Status.st_mode & S_IXUSR ) ? 1 : 0;
    int usr_t = ( usr_r << 2 ) | ( usr_w << 1 ) | usr_x;
    // Group flags
    int grp_r = ( Status.st_mode & S_IRGRP ) ? 1 : 0;
    int grp_w = ( Status.st_mode & S_IWGRP ) ? 1 : 0;
    int grp_x = ( Status.st_mode & S_IXGRP ) ? 1 : 0;
    int grp_t = ( grp_r << 2 ) | ( grp_w << 1 ) | grp_x;
    // Other flags
    int oth_r = ( Status.st_mode & S_IRGRP ) ? 1 : 0;
    int oth_w = ( Status.st_mode & S_IWGRP ) ? 1 : 0;
    int oth_x = ( Status.st_mode & S_IXGRP ) ? 1 : 0;
    int oth_t = ( oth_r << 2 ) | ( oth_w << 1 ) | oth_x;

    IntToString(usr_t*100 + grp_t*10 + oth_t, tempRes);
    result.emplace_back(tempRes);

    if((grp = getgrgid(Status.st_gid)) != nullptr) {
        result.emplace_back((std::string)grp->gr_name); // Group name
    }
    else {
        IntToString(Status.st_gid, tempRes);
        result.emplace_back(tempRes); // Group id
    }

    if((pwd = getpwuid(Status.st_uid)) != nullptr) {
        result.push_back((std::string)pwd->pw_name); // Owner name
    }
    else {
        IntToString(Status.st_uid, tempRes);
        result.push_back(tempRes); // Owner id
    }

    // The time of last modification
    struct tm* date;
    date = localtime(&Status.st_mtime);
    char buffer[20]; // "DD.MM.YYYY HH:mm:SS"正好19个字符
    sprintf (buffer, "%d.%d.%d %d:%d:%d", 
             date->tm_mday, 
             ((date->tm_mon)+1), 
             ((date->tm_year)+1900), 
             date->tm_hour, date->tm_min, date->tm_sec);
    result.emplace_back((std::string)buffer); // LastModificationTime

    // 如果是file，就获取文件大小
    // 如果是dir，就获取文件数
    unsigned long tempSize = ((S_ISDIR(Status.st_mode)) ? this->getDirSize(fileName) : (intmax_t)Status.st_size);
    IntToString(tempSize, tempRes);
    result.push_back(tempRes); // Size (Bytes)
    return result;
}

void fileOperator::IntToString(int i, std::string& res) {
    std::ostringstream temp;
    temp << i;
    res = temp.str();
}

// 返回给定文件所在的目录
std::string fileOperator::getParentDir() {
    std::list<std::string>::reverse_iterator lastDirs;
    unsigned int i = 0;

    for (lastDirs = this->completePath_.rbegin(); ((lastDirs != this->completePath_.rend()) && (++i <= 2)); ++lastDirs) {
        if (i == 2) { // Yes this is the second-last entry (could also be that only one dir exists because we are in the server root dir
            return ((++lastDirs == this->completePath_.rend()) ? SERVERROOTPATHSTRING : (*(--lastDirs))); // Avoid divulging the real server root path
        }
    }
    return SERVERROOTPATHSTRING;
}

// 返回指定目录中的子目录+文件项的计数
unsigned long fileOperator::getDirSize(std::string dirName) {
    getValidFile(dirName);
    std::vector<std::string> directories;
    std::vector<std::string> files;
    this->browse(dirName, directories, files);
    // 去除 "." 和 ".."
    return ((directories.size() - 2) + files.size());
}

// 返回当前工作目录的路径（从服务器根目录开始）
std::string fileOperator::getCurrentWorkingDir(bool showRootPath) {
    std::string fullpath;
    std::list<std::string>::iterator singleDir;
    for (singleDir = this->completePath_.begin(); singleDir != this->completePath_.end(); ++singleDir) {
        if(singleDir == this->completePath_.begin()) {
            if(showRootPath) {
                fullpath.append(SERVERROOTPATHSTRING);
                continue;
            }
        }
        fullpath += (*singleDir);
    }

    // std::cout << "fullPath '" << fullpath << "'" << std::endl;
    return fullpath;
}

// 列出指定目录中的所有文件和目录
void fileOperator::browse(std::string dir, std::vector<std::string>& directories, 
                        std::vector<std::string>& files, bool strict) {
    // std::cout << "Browsing(before) '" << dir << "'" << std::endl;
    if(strict) {
        getValidDir(dir);
        if(dir.compare("../") == 0 && this->completePath_.size() <= 1) {
            dir = "./";
            std::cerr << "Error: Change beyond server root requested (prohibited)!" << std::endl;
        }
        // std::cout << "Browsing(middle) '" << dir << "'" << std::endl;
    }
    if(dir.compare("./") != 0) {
        dir = this->getCurrentWorkingDir(false).append(dir);
    }
    else {
        dir = this->getCurrentWorkingDir(false);
    }

    // std::cout << "Browsing(after) '" << dir << "'" << std::endl;

    DIR* dp;
    struct dirent *dirp;
    if(this->dirCanBeOpened(dir)) {
        try {
            // std::cout << "opendir '" << dir << "' now" << std::endl;
            dp = opendir(dir.c_str());
            while((dirp = readdir(dp)) != nullptr) {
                if(((std::string)dirp->d_name).compare("..") == 0 && this->completePath_.size() <= 1)
                    continue;
                if(dirp->d_type == DT_DIR) {
                    directories.emplace_back(std::string(dirp->d_name));
                }
                else { // Otherwise it must be a file (no special treatment for devs, nodes, etc.)
                    files.emplace_back(std::string(dirp->d_name));
                }
            }
            closedir(dp);

            for(std::string& directory : directories) {
                std::cout << directory << "/ ";
            }
            std::cout << std::endl;
            for(std::string& file : files) {
                std::cout << file << " ";
            }
            std::cout << std::endl;
            
        }
        catch (std::exception e) {
            std::cerr << "Error (" << e.what() << ") opening '" << dir << "'" << std::endl;
        }
    }
    else {
        std::cout << "Error: Directory '" << dir << "' could not be opened!" << std::endl;
    }
}

// 判断目录能否被打开
bool fileOperator::dirCanBeOpened(std::string dir) {
    std::cout << "dir in dirCanBeOpened() : " << dir << std::endl;
    DIR* dp;
    bool canBeOpened = false;
    // canBeOpened = ((dp = opendir(dir.c_str())) != NULL);
    canBeOpened = ((dp = opendir("./page/")) != NULL);
    if(dp == nullptr) {
        std::cerr << "errno is : " << errno << std::endl;
    }
    // assert(dp != nullptr);
    closedir(dp);
    return canBeOpened;
}

void fileOperator::clearListOfDeletedFiles() {
    this->deletedFiles_.clear();
}

void fileOperator::clearListOfDeletedDirectories() {
    this->deletedDirectories_.clear();
}

std::vector<std::string> fileOperator::getListOfDeletedFiles() {
    return this->deletedFiles_;
}

std::vector<std::string> fileOperator::getListOfDeletedDirectories() {
    return this->deletedDirectories_;
}

bool fileOperator::dirIsBelowServerRoot(std::string dirName) {
    this->getValidDir(dirName);
    return ((dirName.compare("../") == 0) && (this->completePath_.size() < 2));
}


