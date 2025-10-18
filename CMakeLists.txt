cmake_minimum_required(VERSION 3.19)

project(LangMgr
        VERSION 0.0.0.1
        LANGUAGES CXX
        DESCRIPTION "Language and G2p Manager"
        HOMEPAGE_URL "https://github.com/wolfgitpr/language-manager"
)

set(CMAKE_CXX_STANDARD 17)

# ----------------------------------
# Build Options
# ----------------------------------
option(LANGMGR_BUILD_TESTS "Build test cases" OFF)
option(LANGMGR_BUILD_EXAMPLES "Build examples" OFF)
option(LANGMGR_INSTALL "Install library" ON)
option(LANGMGR_INSTALL_DEPLOY "Install with deploying dependencies" OFF)
option(LANGMGR_INSTALL_MSVC_RUNTIME "Install MSVC runtime" OFF)

# ----------------------------------
# CMake Settings
# ----------------------------------
if (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /manifest:no")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /manifest:no")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /manifest:no")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")

    if (NOT DEFINED CMAKE_DEBUG_POSTFIX)
        set(CMAKE_DEBUG_POSTFIX "d")
    endif ()
elseif (MINGW)
    set(CMAKE_STATIC_LIBRARY_PREFIX "")
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif ()

if (LANGMGR_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
endif ()

# ----------------------------------
# Add platform specific
# ----------------------------------
if (WIN32)
    set(RC_DESCRIPTION "Language and G2p Manager.")
    set(RC_COPYRIGHT "Copyright (C) 2023-2025 wolfgitpr")
endif ()

# ----------------------------------
# Project Variables
# ----------------------------------
set(LANGMGR_VERSION ${PROJECT_VERSION})
set(LANGMGR_INSTALL_NAME ${PROJECT_NAME})
set(LANGMGR_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------
# Find basic dependencies
# ----------------------------------
find_package(qmsetup REQUIRED)

qm_import(Filesystem)
qm_init_directories()

# ----------------------------------
# Add source modules
# ----------------------------------
add_subdirectory(third-party)
add_subdirectory(language-manager)
add_subdirectory(language-plugins)