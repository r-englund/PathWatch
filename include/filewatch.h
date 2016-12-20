#ifndef FILEWATCH_H
#define FILEWATCH_H

#include "fw_api.h"

#include <memory>
#include <string>
#include <functional>
#include <vector>

namespace filewatch {

class FW_API FileWatcher {
public:
    using F = std::function<void()>;

    virtual ~FileWatcher();
    std::string getFilename();

    void onChange(F callback);

    void invokeAll();

protected:
    FileWatcher(std::string path);
    std::string filepath_;

    std::vector<F> callbacks_;
};

class FW_API FileWatcherManager {
public:
    virtual ~FileWatcherManager() {}
    virtual std::shared_ptr<FileWatcher> watchFile(std::string file) = 0;
    static std::shared_ptr<FileWatcherManager> getManager();


protected:
    FileWatcherManager() {}
};

}  // namespace
#endif