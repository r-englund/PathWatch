#include "pathwatch.h"

#include <regex>
#include <iostream>
#include <unordered_set>

#include <string>
#include <memory>
#include <vector>
#include <map>

#include <windows.h>
#include <mutex>

namespace pathwatch {

namespace win {
void printError() {
    LPTSTR lpMsgBuf;
    DWORD  dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

    // Display the error message and exit the process

    std::cout << lpMsgBuf << std::endl;

    LocalFree(lpMsgBuf);
}
}  // namespace win

class PathWatcherWinInternals : public PathWatcher::Impl {
public:
    PathWatcherWinInternals()
        : newWatchEvent_(CreateEvent(NULL, FALSE, FALSE, "Start to watch file"))
        , removeWatchEvent_(CreateEvent(NULL, FALSE, FALSE, "Stop to watch file"))
        , closeEvent_(CreateEvent(NULL, FALSE, FALSE, "Signal to stop the application")) {
        thread_ = std::thread([&]() { loop(); });
    }

    ~PathWatcherWinInternals() {
        running_ = false;
        SetEvent(closeEvent_);
        thread_.join();
    }

    bool running_ = true;
    void loop() {
        std::vector<HANDLE>      handles;
        std::vector<Directory *> directories_;
        handles.push_back(newWatchEvent_);
        handles.push_back(removeWatchEvent_);
        handles.push_back(closeEvent_);

        directories_.emplace_back();  // dummies
        directories_.emplace_back();  // dummies
        directories_.emplace_back();  // dummies

        while (running_) {
            auto N      = static_cast<DWORD>(handles.size());
            auto status = WaitForMultipleObjects(N, handles.data(), FALSE, INFINITE);
            auto id     = status - WAIT_OBJECT_0;
            if (status == WAIT_FAILED) {
                std::cout << "Wait failed" << std::endl;
                win::printError();
            } else if (id == 0) {
                std::lock_guard<std::mutex> lock(mutex_);
                for (auto w : newWatches_) {
                    bool isFile = fs::is_regular_file(w.first);
                    bool isDir  = fs::is_directory(w.first);
                    if (!isFile && !isDir) {
                        std::cerr << "Given path is not a path to existing file/directory, skipping"
                                  << std::endl;
                        return;
                    }

                    auto dirPath = isFile ? w.first.parent_path() : w.first;

                    auto  res = watchedDirectories_.emplace(dirPath.wstring(), dirPath);
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

    struct Directory {
        Directory(fs::path path)
            : handle(
                  FindFirstChangeNotificationW(path.c_str(), TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE))
            , path_(path) {}
        Directory(const Directory &) = default;
        Directory(Directory &&)      = default;
        Directory &operator=(const Directory &) = default;
        Directory &operator=(Directory &&) = default;

        HANDLE                                                               handle;
        fs::path                                                             path_;
        std::vector<PathWatcher::OnChangeCallback>                           directoryCallbacks;
        std::unordered_multimap<std::wstring, PathWatcher::OnChangeCallback> watchFiles;

        bool notWatching() const { return directoryCallbacks.empty() && watchFiles.empty(); }
        bool isWatching() const { return !notWatching(); }

        void changeDetected() {
            const DWORD FILE_NOTIFY_ALL =
                FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_DIR_NAME |
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE |
                FILE_NOTIFY_CHANGE_SECURITY | FILE_NOTIFY_CHANGE_SIZE;

            FILE_NOTIFY_INFORMATION buffer[1024 * 16];
            DWORD                   bytesReturns;
            auto res = ReadDirectoryChangesW(handle, &buffer, 1024 * 16, TRUE, FILE_NOTIFY_ALL,
                                             &bytesReturns, NULL, NULL);

            if (res == 0) {
                win::printError();
            }

            else if (bytesReturns != 0) {
                auto off = 0;
                do {
                    std::wstring filename(buffer[off].FileName,
                                          buffer[off].FileNameLength / sizeof(WCHAR));
                    if (buffer[off].Action == 3) {}
                    auto fileCallbacks = watchFiles.equal_range(filename);
                    for (auto cb = fileCallbacks.first; cb != fileCallbacks.second; ++cb) {
                        cb->second();
                    }

                    off = buffer[off].NextEntryOffset;

                } while (buffer[off].NextEntryOffset != 0);
            } else {
                std::cout << "Something went wrong? Probably never happens" << std::endl;
            }

            for (auto &dc : directoryCallbacks) { dc(); }

            FindNextChangeNotification(handle);
        }
    };

    void addWatch(fs::path path, PathWatcher::OnChangeCallback callback) {
        newWatches_.emplace_back(path, callback);
        SetEvent(newWatchEvent_);
    }

    std::mutex mutex_;
    HANDLE     newWatchEvent_;
    HANDLE     removeWatchEvent_;
    HANDLE     closeEvent_;

    std::unordered_map<std::wstring, Directory> watchedDirectories_;

    std::vector<std::pair<fs::path, PathWatcher::OnChangeCallback>> newWatches_;

    std::thread thread_;  // init thread last
};

namespace {
PathWatcherWinInternals &impl(PathWatcher *wathcer) {
    return *static_cast<PathWatcherWinInternals *>(wathcer->impl_.get());
}
}  // namespace

#define IMPL impl(this)

PathWatcher::PathWatcher() : impl_(std::make_unique<PathWatcherWinInternals>()) {}

void PathWatcher::watch(fs::path path, OnChangeCallback callback) {
    bool isFile = fs::is_regular_file(path);
    bool isDir  = fs::is_directory(path);
    if (!isFile && !isDir) { throw Exception("Given path is not a file nor a directory"); }

    IMPL.addWatch(path, callback);
}

PathWatcher::~PathWatcher() {}

}  // namespace pathwatch