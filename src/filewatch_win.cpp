#include "filewatch_win.h"

#include <regex>
#include <thread>

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include <iostream>

namespace filewatch {

std::shared_ptr<FileWatcherManager> FileWatcherManager::getManager() {
    return std::make_shared<FileWatcherManagerWin>();
}

FileWatcherWin::FileWatcherWin(std::string path)
    : FileWatcher(std::regex_replace(path, std::regex("\\\\"), "/")) 
    , hFile_(nullptr)
    , lastWriteTime()
{
    hFile_ = CreateFile(filepath_.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, 0, NULL);

    FILETIME creationTime;
    FILETIME lpLastAccessTime;
    GetFileTime(hFile_, &creationTime, &lpLastAccessTime, &lastWriteTime);
}

FileWatcherWin::~FileWatcherWin() { CloseHandle(hFile_); }

FileWatcherManagerWin::FileWatcherManagerWin()
    : newWatchEven_(CreateEvent(NULL, FALSE, FALSE, "New file to watch"))
    , thread_([=]() { mainLoop(); }) {}

std::shared_ptr<FileWatcher> FileWatcherManagerWin::watchFile(std::string filepath) {
    auto fw = std::make_shared<FileWatcherWin>(filepath);

    auto id = fw->getFilename().find_last_of("/") + 1;
    auto dir = fw->getFilename().substr(0, id);
    auto file = fw->getFilename().substr(id);

    LPCTSTR lpDir = dir.c_str();

    TCHAR lpDrive[4];
    TCHAR lpFile[_MAX_FNAME];
    TCHAR lpExt[_MAX_EXT];

    _tsplitpath_s(lpDir, lpDrive, 4, NULL, 0, lpFile, _MAX_FNAME, lpExt, _MAX_EXT);

    lpDrive[2] = (TCHAR)'\\';
    lpDrive[3] = (TCHAR)'\0';

    HANDLE dwChangeHandles =
        FindFirstChangeNotification(lpDrive,                         // directory to watch
                                    TRUE,                            // watch the subtree
                                    FILE_NOTIFY_CHANGE_LAST_WRITE);  // watch dir name changes

    fileWatchers_[dwChangeHandles] = fw;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        newHandles_.push_back(dwChangeHandles);
    }

    SetEvent(newWatchEven_);

    return fw;
}

void FileWatcherManagerWin::mainLoop() {
    std::vector<HANDLE> handles{newWatchEven_};
    while (true) {
        DWORD dwWaitStatus = WaitForMultipleObjects(static_cast<DWORD>(handles.size()),
                                                    handles.data(), FALSE, INFINITE);

        if (dwWaitStatus == WAIT_OBJECT_0) {
            std::lock_guard<std::mutex> lock(mutex_);
            handles.insert(handles.end(), newHandles_.begin(), newHandles_.end());
            newHandles_.clear();
        } else if (dwWaitStatus > WAIT_OBJECT_0 &&
                   dwWaitStatus <= WAIT_OBJECT_0 + handles.size() - 1) {
            auto handle = handles[dwWaitStatus - WAIT_OBJECT_0];
            auto fw = fileWatchers_[handle].lock();
            if (fw) {
                FILETIME creationTime;
                FILETIME lpLastAccessTime;
                FILETIME lastWriteTime;
                GetFileTime(fw->hFile_, &creationTime, &lpLastAccessTime, &lastWriteTime);

                if (CompareFileTime(&fw->lastWriteTime, &lastWriteTime) < 0) {
                    std::cout << fw->lastWriteTime.dwLowDateTime << " " << lastWriteTime.dwLowDateTime << " " << (fw->lastWriteTime.dwLowDateTime - lastWriteTime.dwLowDateTime) << std::endl;
                    fw->lastWriteTime = lastWriteTime;
                    fw->invokeAll();
                }

                if (FindNextChangeNotification(handle) == FALSE) {
                    throw std::exception("FindNextChangeNotification function failed.");
                }
            } else {
                // std::cout << "dead" << std::endl;
            }

        } else {
            throw std::exception("Unhandled dwWaitStatus.");
            break;
        }
    }
}

}  // namespace