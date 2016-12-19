# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (FileWatch)



option(SHARED_LIBS "Build shared libs, else static libs" ON)
option(FW_BUILD_EXAMPLE "Build FilwWatch Example application" ON)


include_directories(include)

if(SHARED_LIBS)
    set(LINK_TYPE SHARED)
    add_definitions(-DFW_SHARED_BUILD)
else()
    set(LINK_TYPE STATIC)
endif()

add_library(filewatch ${LINK_TYPE} src/filewatch_win.cpp)



if(SHARED_LIBS)
    target_compile_definitions(filewatch PRIVATE -DFW_EXPORTS)
endif()

if(FW_BUILD_EXAMPLE)
    add_executable(example example/example.cpp)
    target_link_libraries(example filewatch)
endif()



