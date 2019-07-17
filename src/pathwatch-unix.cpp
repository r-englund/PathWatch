
#include "pathwatch.h"

#include <sys/inotify.h>
#include <unistd.h>

#include <thread>
#include <iostream>
#include <utility>
#include <unordered_map>

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

namespace pathwatch {

static const uint32_t ALL_FALGS = IN_ACCESS | IN_MODIFY | IN_ATTRIB | IN_CLOSE_WRITE |
                                  IN_CLOSE_NOWRITE | IN_OPEN | IN_MOVED_FROM | IN_MOVED_TO |
                                  IN_CREATE | IN_DELETE | IN_DELETE_SELF;

bool operator!=(struct stat a, struct stat b) {
    return a.st_atime != b.st_atime || a.st_mtime != b.st_mtime || a.st_ctime != b.st_ctime || a.st_size != b.st_size;
}

class PathWatcherUnixInternals : public PathWatcher::PIMPL {
public:
    friend class PathWatcher;
    PathWatcherUnixInternals() : inotifyID(inotify_init()) {
        if (inotifyID == -1) {
            throw Exception("Failed to init PathWatcher");
        }

        thread_ = std::thread([&]() { loop(); });

        std::cout << "IN_ACCESS: " << IN_ACCESS << std::endl;
        std::cout << "IN_MODIFY: " << IN_MODIFY << std::endl;
        std::cout << "IN_ATTRIB: " << IN_ATTRIB << std::endl;
        std::cout << "IN_CLOSE_WRITE: " << IN_CLOSE_WRITE << std::endl;
        std::cout << "IN_CLOSE_NOWRITE: " << IN_CLOSE_NOWRITE << std::endl;
        std::cout << "IN_OPEN: " << IN_OPEN << std::endl;
        std::cout << "IN_MOVED_FROM: " << IN_MOVED_FROM << std::endl;
        std::cout << "IN_MOVED_TO: " << IN_MOVED_TO << std::endl;
        std::cout << "IN_CREATE: " << IN_CREATE << std::endl;
        std::cout << "IN_DELETE: " << IN_DELETE << std::endl;
        std::cout << "IN_DELETE_SELF: " << IN_DELETE_SELF << std::endl;
    }

    // struct inotify_event {
    //        __s32 wd;             /* watch descriptor */
    //        __u32 mask;           /* watch mask */
    //        __u32 cookie;         /* cookie to synchronize two events */
    //        __u32 len;            /* length (including nulls) of name */
    //        char name[0];        /* stub for possible name */
    //};

    void loop() {

        const static size_t EVENT_SIZE = sizeof(struct inotify_event);
        const static size_t BUF_LEN = 1024;
        char buf[BUF_LEN];
        actions::FileRenamed renameAction;

        std::unordered_map<std::wstring, struct stat> lastModificationTimes;

        while (true) {
            int i = 0;
            int len = read(inotifyID, buf, BUF_LEN);

            if (len < 0) {
                if (errno == EINTR) { /* need to reissue system call */
                } else {
                    perror("read");
                }
            } else if (len == 0) {
                /* BUF_LEN too small? */
            } else {
                while (i < len) {
                    struct inotify_event *event;

                    

                    event = (struct inotify_event *)&buf[i];
                    
                    if(false){
                    std::cout  << "Parsing event " << i << "\n";
                    std::cout  << "  wd: " << event->wd << "\n";
                    std::cout  << "  mask: " << event->mask << "\n";
                    std::cout  << "  cookie: " << event->cookie << "\n";
                    if(event->len>0){
                        std::cout  << "  name: " << event->name << "\n";
                    }
                    }


                    auto it = callbacks_.find(event->wd);
                    if (it != callbacks_.end() && event->mask <= ALL_FALGS) {
                        CallbackWrapper &cb = it->second.first;
                        auto path = it->second.second;
                        if (fs::is_directory(path) && event->len > 0) {
                            path /= event->name;
                        }

                        if (event->mask & IN_ATTRIB) {
                        }
                        if (event->mask & IN_MODIFY) {
                            struct stat sb;
                            stat(path.c_str(), &sb);
                            auto it = lastModificationTimes.find(path.wstring());
                            if (it == lastModificationTimes.end() || it->second != sb) {
                                cb(actions::FileModified{path});
                            }
                            lastModificationTimes[path.wstring()] = sb;
                        }
                        if (event->mask & IN_CREATE) {
                            cb(actions::FileAdded{path});
                        }
                        if (event->mask & IN_DELETE || event->mask & IN_DELETE_SELF) {
                            cb(actions::FileRemoved{path});
                        }
                        if (event->mask & IN_MOVED_FROM) {
                            renameAction.oldPath = path;
                        }
                        if (event->mask & IN_MOVED_TO) {
                            renameAction.newPath = path;
                        }

                        if (!renameAction.oldPath.empty() && !renameAction.newPath.empty()) {
                            cb(renameAction);
                            renameAction.oldPath.clear();
                            renameAction.newPath.clear();
                        }
                    }

                    i += EVENT_SIZE + event->len;
                }
            }
        }
    }

    /**
    IN_ACCESS	File was read from.
    IN_MODIFY	File was written to.
    IN_ATTRIB	File's metadata (inode or xattr) was changed.
    IN_CLOSE_WRITE	File was closed (and was open for writing).
    IN_CLOSE_NOWRITE	File was closed (and was not open for writing).
    IN_OPEN	File was opened.
    IN_MOVED_FROM	File was moved away from watch.
    IN_MOVED_TO	File was moved to watch.
    IN_DELETE	File was deleted.
    IN_DELETE_SELF	The watch itself was deleted.
     *
     */

    void addWatch(fs::path path, CallbackWrapper callback) {
        auto id = inotify_add_watch(inotifyID, path.c_str(), IN_ALL_EVENTS);
        if (id < 0) {
            throw Exception("Could not add watch");
        }

        callbacks_.emplace(id, std::make_pair(callback, path));
    }

    std::unordered_map<int, std::pair<CallbackWrapper, fs::path> > callbacks_;

    std::thread thread_;
    int inotifyID;
};

namespace {
PathWatcherUnixInternals &impl(PathWatcher *wathcer) {
    return *static_cast<PathWatcherUnixInternals *>(wathcer->impl_.get());
}
}  // namespace

#define IMPL impl(this)

PathWatcher::PathWatcher() : impl_(std::make_unique<PathWatcherUnixInternals>()) {}

void PathWatcher::watchInternal(fs::path path, CallbackWrapper callback) {
    IMPL.addWatch(path, callback);
}

PathWatcher::~PathWatcher() {}

}  // namespace pathwatch