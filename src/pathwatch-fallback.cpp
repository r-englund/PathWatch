
#include "pathwatch.h"


namespace pathwatch {


PathWatcher::PathWatcher()  {}

void PathWatcher::watch(fs::path path, OnChangeCallback callback) {
    throw Exception("NOT YET IMPLEMENTED");
    /*bool isFile = fs::is_regular_file(path);
    bool isDir = fs::is_directory(path);
    if (!isFile && !isDir) {
        throw Exception("Given path is not a file nor a directory");
    }*/

    //IMPL.addWatch(path, callback);
}

PathWatcher::~PathWatcher() {}

}