project(LangPlugins
        LANGUAGES CXX
        VERSION 0.0.0.1
        DESCRIPTION "Language Inference Framework"
)

# ----------------------------------
# Build Options
# ----------------------------------
option(LANGPLUGINS_ENABLE_DIRECTML "Enable DirectML provider" ON)
option(LANGPLUGINS_ENABLE_CUDA "Enable CUDA provider" ON)
option(LANGPLUGINS_ENABLE_STATIC_PLUGINS "Enable static plugin linking" OFF) # TODO: Implement this option

# ----------------------------------
# Project Variables
# ----------------------------------
set(LANGPLUGINS_INSTALL ${LANGMGR_INSTALL})
set(LANGPLUGINS_VERSION ${PROJECT_VERSION})
set(LANGPLUGINS_INSTALL_NAME ${PROJECT_NAME})

# ----------------------------------
# Configure BuildAPI
# ----------------------------------
set(LANGPLUGINS_AUTHOR "小狼")
set(LANGPLUGINS_START_YEAR 2025)
set(LANGPLUGINS_INCLUDE_DIR include)
include("${LANGMGR_SOURCE_DIR}/cmake/BuildAPI.cmake")

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

# ----------------------------------
# Add source modules
# ----------------------------------
add_subdirectory(lib)

add_subdirectory(util)

add_subdirectory(plugins)

if (LANGMGR_BUILD_TESTS)
    add_subdirectory(tests)
endif ()

LangPlugins_install()