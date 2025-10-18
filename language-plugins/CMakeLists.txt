project(LangPlugins
        LANGUAGES CXX
        VERSION 0.0.1.2
        DESCRIPTION "DiffSinger Inference Framework"
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
set(LANGPLUGINS_START_YEAR 2023)
set(LANGPLUGINS_INCLUDE_DIR include)
include("${LANGMGR_SOURCE_DIR}/cmake/BuildAPI.cmake")

# ----------------------------------
# Add source modules
# ----------------------------------
add_subdirectory(lib)

add_subdirectory(plugins)

if (LANGMGR_BUILD_TESTS)
    add_subdirectory(tests)
endif ()

LangPlugins_install()