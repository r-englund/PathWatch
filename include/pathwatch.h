#pragma once

#define INC_PATHWATCH_H

#include "pw_api.h"

#include <filesystem>
#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <variant>

namespace pathwatch {

namespace fs = std::filesystem;

class Exception : public std::runtime_error {
public:
    Exception(std::string msg) : std::runtime_error(msg.c_str()) {}
};
//
namespace actions {

struct PW_API FileAdded {
    fs::path path;
    static const char* type;
};
struct PW_API FileRemoved {
    fs::path path;
    static const char* type;
};
struct PW_API FileModified {
    fs::path path;
    static const char* type;
};
struct PW_API FileRenamed {
    fs::path oldPath;
    fs::path newPath;
    static const char* type;
};
}  // namespace actions

struct PW_API CallbackWrapper {

    template <typename Callback>
    CallbackWrapper(Callback callback)
        : onFileAdded{callback}
        , onFileRemoved{callback}
        , onFileModified{callback}
        , onFileRenamed{callback} {}
    std::function<void(actions::FileAdded)> onFileAdded;
    std::function<void(actions::FileRemoved)> onFileRemoved;
    std::function<void(actions::FileModified)> onFileModified;
    std::function<void(actions::FileRenamed)> onFileRenamed;

    void operator()(actions::FileAdded f) { onFileAdded(f); }
    void operator()(actions::FileRemoved f) { onFileRemoved(f); }
    void operator()(actions::FileModified f) { onFileModified(f); }
    void operator()(actions::FileRenamed f) { onFileRenamed(f); }
};

class PW_API PathWatcher {
public:
    using Action = std::variant<actions::FileAdded, actions::FileRemoved, actions::FileModified,
                                actions::FileRenamed>;

    class PIMPL {
    public:
        virtual ~PIMPL() {}
    };

    PathWatcher();
    ~PathWatcher();

    template <typename Callback>
    void watch(fs::path path, Callback callback) {

        if (fs::is_regular_file(path) || fs::is_directory(path)) {
            watchInternal(path, callback);
        } else {
            throw Exception("Given path is not a file nor a directory");
        }
    }

    std::unique_ptr<PIMPL> impl_;

private:
    void watchInternal(fs::path, CallbackWrapper callbacks);
};

}  // namespace pathwatch
template <class Elem, class Traits>
std::basic_ostream<Elem, Traits>& operator<<(std::basic_ostream<Elem, Traits>& os,
                                             pathwatch::actions::FileAdded action) {
    os << action.type << " " << action.path.string();
    return os;
}

template <class Elem, class Traits>
std::basic_ostream<Elem, Traits>& operator<<(std::basic_ostream<Elem, Traits>& os,
                                             pathwatch::actions::FileRemoved action) {
    os << action.type << " " << action.path.string();
    return os;
}

template <class Elem, class Traits>
std::basic_ostream<Elem, Traits>& operator<<(std::basic_ostream<Elem, Traits>& os,
                                             pathwatch::actions::FileModified action) {
    os << action.type << " " << action.path.string();
    return os;
}

template <class Elem, class Traits>
std::basic_ostream<Elem, Traits>& operator<<(std::basic_ostream<Elem, Traits>& os,
                                             pathwatch::actions::FileRenamed action) {
    os << action.type << " " << action.oldPath.string() << " > " << action.newPath.string();
    return os;
}

