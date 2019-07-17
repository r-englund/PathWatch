#include "pathwatch.h"
namespace pathwatch {

namespace actions {
const char *FileAdded::type = "FileAdded";
const char *FileRemoved::type = "FileRemoved";
const char *FileModified::type = "FileModified";
const char *FileRenamed::type = "FileRenamed";
}  // namespace actions
}  // namespace pathwatch