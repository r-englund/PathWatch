#ifndef FILEWATCH_H
#define FILEWATCH_H

#include "fw_api.h"

#include <filesystem>
#include <memory>
#include <string>
#include <functional>
#include <vector>

namespace pathwatch {
namespace fs = std::filesystem;

class Exception : public std::exception {
public:
    Exception(std::string msg) : std::exception(msg.c_str()) {}
};

class FW_API PathWatcher {

public:
    class Impl {};
    using OnChangeCallback = std::function<void()>;

    PathWatcher();
    ~PathWatcher();

    void watch(fs::path path, OnChangeCallback callback);

    std::unique_ptr<Impl> impl_;
};

}  // namespace pathwatch
#endif