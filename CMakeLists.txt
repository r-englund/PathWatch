cmake_minimum_required(VERSION 3.11.4)
cmake_policy(VERSION 3.11.4)

set(PATHWATCH_VERSION "0.0.1")
project (PathWatch VERSION ${PATHWATCH_VERSION})

if(${CMAKE_CURRENT_LIST_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
    set(PW_IS_MAIN_PROJECT TRUE)
    set(PW_FOLDER "" CACHE STRING "Folder prefix used in generated solutions")
    if(NOT ${PW_FOLDER} STREQUAL "")
        set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    endif()
    
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER cmake)
else()
    set(PW_IS_MAIN_PROJECT FALSE)
    set(PW_FOLDER "ext/PathWatch" CACHE  STRING "Folder prefix used in generated solutions")
endif()

option(PW_BUILD_EXAMPLES "Build PathWatch Example application" ${PW_IS_MAIN_PROJECT})
option(PW_BUILD_STATIC "Enable building static version of PathWatch libraries" ON)
option(PW_BUILD_SHARED "Enable building shared version of PathWatch libraries" ON)
option(PW_PREFERE_SHARED "Prefere shared version of PathWatch libraries for alias" OFF)
option(PW_PREFERE_STATIC "Prefere static version of PathWatch libraries for alias" ON)
option(PW_INCLUDE_FALLBACK "Force use fallback soruce instead of os-specifc" ${PW_IS_MAIN_PROJECT})

mark_as_advanced(PW_FORCE_FALLBACK)

if(NOT PW_BUILD_STATIC AND NOT PW_BUILD_SHARED)
    message(FATAL_ERROR "Neither PW_BUILD_STATIC nor PW_BUILD_SHARED is set to true")
endif()



function(pw_set_comp_opts target folder)
    target_include_directories(${target} PUBLIC include)

    set_property(TARGET ${target} PROPERTY CXX_STANDARD_REQUIRED ON)
    set_property(TARGET ${target} PROPERTY CXX_STANDARD 17)
    if(MSVC)
        set_property(TARGET ${target} APPEND_STRING PROPERTY
            COMPILE_FLAGS "/W4 /D_CRT_SECURE_NO_WARNINGS /wd4005 /wd4996 /nologo /w34189 /w34263 /w34266 /w34289 /w34296 /wd4251")
    elseif(UNIX)
        target_link_libraries( ${target}  PUBLIC stdc++fs )
    endif()
    if(NOT ${PW_FOLDER} STREQUAL "")
        
        set_target_properties( ${target} PROPERTIES FOLDER "${PW_FOLDER}/${folder}")
    endif()
endfunction()


set(HEADER_FILES
    include/pw_api.h 
    include/pathwatch.h
)

set(SRC_FILES "")

if(MSVC)
    list(APPEND SRC_FILES src/pathwatch-win.cpp)
else()
    list(APPEND SRC_FILES src/pathwatch-fallback.cpp)
endif()

set(alias "")
if(PW_BUILD_STATIC)
    add_library(pathwatch-static STATIC ${HEADER_FILES} ${SRC_FILES})
    pw_set_comp_opts(pathwatch-static "")
    set(alias "pathwatch-static")
endif()
if(PW_BUILD_SHARED)
    add_library(pathwatch-shared SHARED ${HEADER_FILES} ${SRC_FILES})
    target_compile_definitions(pathwatch-shared PRIVATE PW_EXPORTS)
    target_compile_definitions(pathwatch-shared PUBLIC PW_SHARED_BUILD)
    target_include_directories(pathwatch-shared PUBLIC include)
    pw_set_comp_opts(pathwatch-shared "")
    set(alias "pathwatch-shared")
endif()

if(PW_INCLUDE_FALLBACK)
    if(PW_BUILD_STATIC)
        add_library(pathwatch-fallback-static STATIC ${HEADER_FILES} src/pathwatch-fallback.cpp)
        pw_set_comp_opts(pathwatch-fallback-static "")
    endif()
    if(PW_BUILD_SHARED)
        add_library(pathwatch-fallback-shared SHARED ${HEADER_FILES} src/pathwatch-fallback.cpp)
        target_compile_definitions(pathwatch-fallback-shared PRIVATE PW_EXPORTS)
        target_compile_definitions(pathwatch-fallback-shared PUBLIC PW_SHARED_BUILD)
        target_include_directories(pathwatch-fallback-shared PUBLIC include)
        pw_set_comp_opts(pathwatch-fallback-shared "")
    endif()
    
endif()

if(PW_BUILD_STATIC AND PW_BUILD_SHARED)
    if(PW_PREFERE_STATIC)
        set(alias "pathwatch-static")
    elseif(PW_PREFERE_SHARED)
        set(alias "pathwatch-shared") 
    endif() # else fallback to predefined alias
endif()

add_library(pathwatch ALIAS ${alias})

if(PW_BUILD_EXAMPLES)
    add_executable(example example/example.cpp)
    pw_set_comp_opts(example "/examples")
    target_link_libraries(example PUBLIC pathwatch)
endif()
