#pragma once

#include "pw_api.h"

#include <filesystem>
#include <memory>
#include <string>
#include <functional>
#include <vector>

namespace pathwatch {
namespace fs = std::filesystem;

class Exception : public std::runtime_error {
public:
    Exception(std::string msg) : std::runtime_error(msg.c_str()) {}
};

class PW_API PathWatcher {

public:
    class PIMPL {};
    using OnChangeCallback = std::function<void()>;

    PathWatcher();
    ~PathWatcher();

    void watch(fs::path path, OnChangeCallback callback);

    std::unique_ptr<PIMPL> impl_;
};

}  // namespace pathwatch
