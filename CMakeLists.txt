# CMakeLists paths in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (PathWatch)



option(SHARED_LIBS "Build shared libs, else static libs" ON)
option(FW_BUILD_EXAMPLE "Build FilwWatch Example application" OFF)




if(SHARED_LIBS)
    set(LINK_TYPE SHARED)
    add_definitions(-DFW_SHARED_BUILD)
else()
    set(LINK_TYPE STATIC)
endif()

add_library(pathwatch ${LINK_TYPE} src/pathwatch_win.cpp src/pathwatch.cpp src/pathwatch_win.h include/fw_api.h include/pathwatch.h)

target_include_directories(pathwatch PUBLIC include)


set_property(TARGET pathwatch PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET pathwatch PROPERTY CXX_STANDARD 17)

set_property(TARGET pathwatch APPEND_STRING PROPERTY
    COMPILE_FLAGS "/W4 /D_CRT_SECURE_NO_WARNINGS /wd4005 /wd4996 /nologo /w34189 /w34263 /w34266 /w34289 /w34296 /wd4251")

if(SHARED_LIBS)
    target_compile_definitions(pathwatch PRIVATE -DFW_EXPORTS)
endif()




if(FW_BUILD_EXAMPLE)
    add_executable(example example/example.cpp)
    target_link_libraries(example pathwatch)
endif()
