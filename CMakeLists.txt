cmake_minimum_required(VERSION 3.19)
project(language-manager
        VERSION 0.0.0.1
        LANGUAGES CXX
        DESCRIPTION "Language and G2p Manager"
        HOMEPAGE_URL "https://github.com/wolfgitpr/language-manager"
)

set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "Language Manager project root")
# ----------------------------------
# Build Options
# ----------------------------------
option(LANGMGR_BUILD_PLUGINS "Build plugins" OFF)
option(LANGMGR_BUILD_TESTS "Build test cases" OFF)

option(LANGMGR_INSTALL_VCPKG "Install in vcpkg" OFF)

option(LANGCORE_INSTALL "Install library" ON)
option(LANGCORE_INSTALL_DEPLOY "Install with deploying dependencies" OFF)

option(LANGPLUGINS_INSTALL_ORT "Install ort lib" ON)

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

add_subdirectory(core)

if (LANGMGR_BUILD_PLUGINS)
    add_subdirectory(plugins)
endif ()

if (LANGMGR_BUILD_TESTS)
    add_subdirectory(tests)
endif ()

if (DEFINED LANGMGR_VCPKG_DIR)
    install(
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/res/G2pPackages
            DESTINATION ${LANGMGR_VCPKG_DIR}
    )
endif ()