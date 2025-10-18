# ----------------------------------
# Configure BuildAPI
# ----------------------------------
set(LANGMGR_START_YEAR 2023)
set(LANGMGR_INCLUDE_DIR include)
include("${LANGMGR_SOURCE_DIR}/cmake/BuildAPI.cmake")

# ----------------------------------
# Add source modules
# ----------------------------------
add_subdirectory(lib)

if (LANGMGR_BUILD_TESTS)
    add_subdirectory(tests)
endif ()

LangMgr_install()

if (LANGMGR_INSTALL)
    # Install copyright
    install(FILES ../LICENSE
            DESTINATION ${CMAKE_INSTALL_DATADIR}/${LANGMGR_INSTALL_NAME}
    )

    if (LANGMGR_INSTALL_DEPLOY)
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
        if (MSVC AND LANGMGR_INSTALL_MSVC_RUNTIME)
            set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "")
            include(InstallRequiredSystemLibraries)
        endif ()
    endif ()
endif ()