#ifndef FILEWATCH_WIN_H
#define FILEWATCH_WIN_H

#include "fw_api.h"

#include <filewatch.h>

#include <string>
#include <memory>
#include <vector>
#include <map>

#include <windows.h>
#include <mutex>

namespace filewatch {

class FW_API FileWatcherWin : public FileWatcher {
public:
    FileWatcherWin(std::string path);
    virtual ~FileWatcherWin();

private:
    HANDLE hFile_;
    FILETIME lastWriteTime;

    friend class FileWatcherManagerWin;
};



class FW_API FileWatcherManagerWin : public FileWatcherManager {
public:
    FileWatcherManagerWin();
    virtual ~FileWatcherManagerWin() {}

    virtual std::shared_ptr<FileWatcher> watchFile(std::string file) override;

    void mainLoop();


    HANDLE newWatchEven_;
    std::vector<HANDLE> newHandles_;
    std::map<HANDLE, std::weak_ptr<FileWatcherWin>> fileWatchers_;

    std::mutex mutex_;



    std::thread thread_;
};


}  // namespace
#endif