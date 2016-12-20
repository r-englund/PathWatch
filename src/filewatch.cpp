#include "filewatch_win.h"

#include <regex>

namespace filewatch {

FileWatcher::FileWatcher(std::string path) : filepath_(path) {}

FileWatcher::~FileWatcher() {}

std::string FileWatcher::getFilename() { return filepath_; }

void FileWatcher::onChange(F callback) { callbacks_.push_back(callback); }

void FileWatcher::invokeAll() {
    std::for_each(callbacks_.begin(), callbacks_.end(), [](auto c) { c(); });
}

}  // namespace