project(LangCore
        VERSION 0.0.0.1
        LANGUAGES CXX
        DESCRIPTION "Language and G2p Manager Core"
)

set(CMAKE_CXX_STANDARD 17)

# ----------------------------------
# Configure BuildAPI
# ----------------------------------
set(LANGCORE_START_YEAR 2023)
set(LANGCORE_INCLUDE_DIR include)
include("${PROJECT_ROOT_DIR}/cmake/BuildAPI.cmake")

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

if (LANGCORE_INSTALL)
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
set(LANGCORE_VERSION ${PROJECT_VERSION})
set(LANGCORE_INSTALL_NAME ${PROJECT_NAME})

# ----------------------------------
# Add source modules
# ----------------------------------
add_subdirectory(lib)

LangCore_install()

if (LANGCORE_INSTALL)
    # Install copyright
    install(FILES ../LICENSE
            DESTINATION ${CMAKE_INSTALL_DATADIR}/${LANGCORE_INSTALL_NAME}
    )

    if (LANGCORE_INSTALL_DEPLOY)
        if (WIN32)
            set(_lib_dir bin)
        else ()
            set(_lib_dir lib)
        endif ()

        if (WIN32)
            set(_extra_search_path ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        else ()
            set(_extra_search_path ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
        endif ()

        qm_import(Deploy)
        qm_deploy_directory(${CMAKE_INSTALL_PREFIX}
                LIBRARY_DIR ${_lib_dir}
                EXTRA_SEARCHING_PATHS ${_extra_search_path}
                VERBOSE
        )

        # Install vcruntime
        if (MSVC AND LANGCORE_INSTALL_MSVC_RUNTIME)
            set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "")
            include(InstallRequiredSystemLibraries)
        endif ()
    endif ()
endif ()