#include "pathwatch.h"

#include <regex>
#include <iostream>
#include <unordered_set>

#include <windows.h>

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <array>
#include <mutex>

const static DWORD FILE_NOTIFY_ALL = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                                     FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
                                     FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS |
                                     FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY;

namespace pathwatch {

    
struct Directory {
    Directory(fs::path path);
    Directory(const Directory &) = default;
    Directory(Directory &&) = default;
    Directory &operator=(const Directory &) = default;
    Directory &operator=(Directory &&) = default;

    HANDLE handle;
    fs::path path_;
    std::vector<CallbackWrapper> directoryCallbacks;
    std::unordered_multimap<std::wstring, CallbackWrapper> watchFiles;

    void changeDetected();

    bool notWatching() const { return directoryCallbacks.empty() && watchFiles.empty(); }
    bool isWatching() const { return !notWatching(); }
};

class PathWatcherWinInternals : public PathWatcher::PIMPL {
public:
    friend class PathWatcher;
    PathWatcherWinInternals();
    ~PathWatcherWinInternals();
    void addWatch(fs::path path, CallbackWrapper callback);

private:
    void loop();
    bool running_ = true;

    std::mutex mutex_;
    HANDLE newWatchEvent_;
    HANDLE removeWatchEvent_;
    HANDLE closeEvent_;

    std::unordered_map<std::wstring, Directory> watchedDirectories_;

    std::vector<std::pair<fs::path, CallbackWrapper>> newWatches_;

    std::thread thread_;  // init thread last
};

namespace {
PathWatcherWinInternals &impl(PathWatcher *wathcer) {
    return *static_cast<PathWatcherWinInternals *>(wathcer->impl_.get());
}
}  // namespace

#define IMPL impl(this)




namespace winutil {

void printError() {
    LPTSTR lpMsgBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

    // Display the error message and exit the process

    std::cout << lpMsgBuf << std::endl;

    LocalFree(lpMsgBuf);
}
}  // namespace winutil

Directory::Directory(fs::path path)
    : handle(CreateFileW(path.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL)), path_(path) {}

void Directory::changeDetected() {
    std::vector<FILE_NOTIFY_INFORMATION> buffer(1024 * 16);
    DWORD bytesReturns;
    auto res = ReadDirectoryChangesW(handle, buffer.data(), 1024 * 16, TRUE, FILE_NOTIFY_ALL,
                                     &bytesReturns, NULL, NULL);


    
    auto invoke = [&](auto action) {
        for (auto &dc : directoryCallbacks) {
            dc(action);
        }
    };

    if (res == 0) {
        winutil::printError();
    }


    else if (bytesReturns != 0) {
        auto off = 0;
        actions::FileRenamed renameAction;
        while (true) {

            std::wstring filename(buffer[off].FileName, buffer[off].FileNameLength / sizeof(WCHAR));
            fs::path filepath = this->path_ / filename;
            switch (buffer[off].Action) {
                case FILE_ACTION_ADDED:
                    invoke(actions::FileAdded{filepath});
                    break;
                case FILE_ACTION_REMOVED:
                    invoke(actions::FileRemoved{filepath});
                    break;
                case FILE_ACTION_MODIFIED: {
                    actions::FileModified action{filepath};
                    auto fileCallbacks = watchFiles.equal_range(filename);
                    for (auto cb = fileCallbacks.first; cb != fileCallbacks.second; ++cb) {
                        cb->second(action);
                    }
                    invoke(action);
                } break;
                case FILE_ACTION_RENAMED_OLD_NAME:
                    renameAction.oldPath = filepath;
                    break;
                case FILE_ACTION_RENAMED_NEW_NAME:
                    renameAction.newPath = filepath;
                    break;
                default:
                    break;
            }
            if (!renameAction.oldPath.empty() && !renameAction.newPath.empty()) {
                invoke(renameAction);
                renameAction.oldPath.clear();
                renameAction.newPath.clear();
            }




            if (buffer[off].Action == 3) {
            }

            if (!buffer[off].NextEntryOffset) break;
            off = buffer[off].NextEntryOffset;
        }
    }
    else {
        std::cout << "Something went wrong? Probably never happens" << std::endl;
    }

    

    FindNextChangeNotification(handle);
}

PathWatcherWinInternals::PathWatcherWinInternals()
    : newWatchEvent_(CreateEvent(NULL, FALSE, FALSE, "Start to watch file"))
    , removeWatchEvent_(CreateEvent(NULL, FALSE, FALSE, "Stop to watch file"))
    , closeEvent_(CreateEvent(NULL, FALSE, FALSE, "Signal to stop the application")) {
    thread_ = std::thread([&]() { loop(); });
}

PathWatcherWinInternals::~PathWatcherWinInternals() {
    running_ = false;
    SetEvent(closeEvent_);
    thread_.join();
}

void PathWatcherWinInternals::loop() {
    std::vector<HANDLE> handles;
    std::vector<Directory *> directories_;
    handles.push_back(newWatchEvent_);
    handles.push_back(removeWatchEvent_);
    handles.push_back(closeEvent_);

    directories_.emplace_back();  // dummies
    directories_.emplace_back();  // dummies
    directories_.emplace_back();  // dummies

    while (running_) {
        auto N = static_cast<DWORD>(handles.size());
        auto status = WaitForMultipleObjects(N, handles.data(), FALSE, INFINITE);
        auto id = status - WAIT_OBJECT_0;
        if (status == WAIT_FAILED) {
            std::cout << "Wait failed" << std::endl;
            winutil::printError();
        } else if (id == 0) {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto w : newWatches_) {
                bool isFile = fs::is_regular_file(w.first);
                bool isDir = fs::is_directory(w.first);
                if (!isFile && !isDir) {
                    std::cerr << "Given path is not a path to existing file/directory, skipping"
                              << std::endl;
                    return;
                }

                auto dirPath = isFile ? w.first.parent_path() : w.first;

                auto res = watchedDirectories_.emplace(dirPath.wstring(), dirPath);
                auto &dir = res.first->second;

                if (res.second) {  // new directory, start watching
                    handles.push_back(dir.handle);
                    directories_.push_back(&dir);
                }

                if (isFile) {
                    dir.watchFiles.emplace(w.first.filename().wstring(), w.second);
                } else {
                    dir.directoryCallbacks.emplace_back(w.second);
                }
            }
        } else if (id == 1) {
            // removed watches
        } else if (id == 2) {
            // exit signaled
            break;
        } else if (id < handles.size()) {
            directories_[id]->changeDetected();
        }
    }
}

void PathWatcherWinInternals::addWatch(fs::path path, CallbackWrapper callback) {
    newWatches_.emplace_back(path, callback);
    SetEvent(newWatchEvent_);
}

PathWatcher::PathWatcher() : impl_(std::make_unique<PathWatcherWinInternals>()) {}

 void PathWatcher::watchInternal(fs::path path, CallbackWrapper callback) {
    bool isFile = fs::is_regular_file(path);
    bool isDir = fs::is_directory(path);
    if (!isFile && !isDir) {
        throw Exception("Given path is not a file nor a directory");
    }
    IMPL.addWatch(path, callback);
}

PathWatcher::~PathWatcher() {}

}  // namespace pathwatch