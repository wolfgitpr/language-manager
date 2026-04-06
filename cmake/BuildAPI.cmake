#[[
    BuildAPI.cmake

    Include this file to generate build APIs for your current project.

        Variable                          Default Candidates
    ----------------------------------------------------------------------------------------------------
    Constants:
        proj:                             ${PROJECT_NAME}
        year:                             current year
    
    Variables:
        prefix:                           <proj>_VAR_PREFIX, <proj-upcase>

    Options:
        <prefix>_VERSION:                 ${PROJECT_VERSION}, 0.0.0.0
        <prefix>_DESCRIPTION:             ${PROJECT_DESCRIPTION}, <proj>
        <prefix>_AUTHOR:                  "<proj> Developers"
        <prefix>_START_YEAR:              <NONE>
        <prefix>_COPYRIGHT:               "Copyright (c) ${<proj>_START_YEAR}-<year> ${<proj>_AUTHOR}",
                                          "Copyright (c) <year> ${<proj>_AUTHOR}"
        <prefix>_INSTALL_NAME:            <proj>
        <prefix>_INCLUDE_DIR:             <NONE>
        <prefix>_BUILD_INCLUDE_DIR:       "${CMAKE_CURRENT_BINARY_DIR}/../etc/include"
        <prefix>_GENERATED_INCLUDE_DIR:   "${CMAKE_CURRENT_BINARY_DIR}/../include"
        <prefix>_CONFIG_TEMPLATE:         <proj>Config.cmake.in
        <prefix>_TARGET_PREFIX:           <proj>
        <prefix>_MACRO_PREFIX:            <proj>

        <prefix>_INSTALL:                 FALSE
        <prefix>_BUILD_SHARED:            FALSE
        <prefix>_SYNC_INCLUDE:            FALSE
    ----------------------------------------------------------------------------------------------------
    
    Macros/Functions:
         <name>_add_executable
         <name>_add_library
         <name>_add_plugin
         <name>_install
]]

include_guard(DIRECTORY)

qm_import(Preprocess)

# ----------------------------------
# Project Variables
# ----------------------------------
set(_CUR_VERSION)
set(_CUR_DESCRIPTION)
set(_CUR_AUTHOR)
set(_CUR_START_YEAR)
set(_CUR_COPYRIGHT)
set(_CUR_INSTALL_NAME)

# ----------------------------------
# Project Directories
# ----------------------------------
set(_CUR_INCLUDE_DIR) # Nullable
set(_CUR_BUILD_INCLUDE_DIR)
set(_CUR_GENERATED_INCLUDE_DIR)

# ----------------------------------
# Project Options
# ----------------------------------
set(_CUR_INSTALL FALSE)
set(_CUR_BUILD_SHARED FALSE)
set(_CUR_SYNC_INCLUDE FALSE)

# ----------------------------------
# Other Option Variables
# ----------------------------------
set(_CUR_CONFIG_TEMPLATE)
set(_CUR_TARGET_PREFIX)
set(_CUR_MACRO_PREFIX)

if (NOT PROJECT_NAME)
    message(FATAL_ERROR "PROJECT_NAME not set")
endif ()

# ----------------------------------
# Variables
# ----------------------------------
set(_CUR_NAME ${PROJECT_NAME})
set(_CUR_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
string(TIMESTAMP _CUR_BUILD_YEAR "%Y" UTC)

# Set default values
qm_set_value(_CUR_PREFIX ${_CUR_NAME}_VAR_PREFIX ${_CUR_NAME})
string(TOUPPER ${_CUR_PREFIX} _CUR_PREFIX_UPPER)

qm_set_value(_CUR_VERSION ${_CUR_PREFIX_UPPER}_VERSION PROJECT_VERSION "0.0.0.0")
qm_set_value(_CUR_DESCRIPTION ${_CUR_PREFIX_UPPER}_DESCRIPTION PROJECT_DESCRIPTION "${_CUR_NAME}")
qm_set_value(_CUR_AUTHOR ${_CUR_PREFIX_UPPER}_AUTHOR "${_CUR_NAME} Developers")

if (${_CUR_PREFIX_UPPER}_START_YEAR)
    set(_CUR_START_YEAR ${${_CUR_PREFIX_UPPER}_START_YEAR})
endif ()

if (_CUR_START_YEAR)
    set(_CUR_COPYRIGHT "Copyright (c) ${_CUR_START_YEAR}-${_CUR_BUILD_YEAR} ${_CUR_AUTHOR}")
else ()
    set(_CUR_COPYRIGHT "Copyright (c) ${_CUR_BUILD_YEAR} ${_CUR_AUTHOR}")
endif ()

qm_set_value(_CUR_COPYRIGHT ${_CUR_PREFIX_UPPER}_COPYRIGHT "${_CUR_COPYRIGHT}")
qm_set_value(_CUR_INSTALL_NAME ${_CUR_PREFIX_UPPER}_INSTALL_NAME "${_CUR_NAME}")

if (${_CUR_PREFIX_UPPER}_INCLUDE_DIR)
    set(_CUR_INCLUDE_DIR ${${_CUR_PREFIX_UPPER}_INCLUDE_DIR})
endif ()

qm_set_value(_CUR_BUILD_INCLUDE_DIR ${_CUR_PREFIX_UPPER}_BUILD_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/../etc/include")
qm_set_value(_CUR_GENERATED_INCLUDE_DIR ${_CUR_PREFIX_UPPER}_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/../include")

if (${_CUR_PREFIX_UPPER}_INSTALL)
    set(_CUR_INSTALL TRUE)
endif ()

if (${_CUR_PREFIX_UPPER}_BUILD_SHARED)
    set(_CUR_BUILD_SHARED TRUE)
endif ()

if (${_CUR_PREFIX_UPPER}_SYNC_INCLUDE)
    set(_CUR_SYNC_INCLUDE TRUE)
endif ()

qm_set_value(_CUR_CONFIG_TEMPLATE ${_CUR_PREFIX_UPPER}_CONFIG_TEMPLATE "${_CUR_NAME}Config.cmake.in")
qm_set_value(_CUR_TARGET_PREFIX ${_CUR_PREFIX_UPPER}_TARGET_PREFIX "${_CUR_NAME}")
qm_set_value(_CUR_MACRO_PREFIX ${_CUR_PREFIX_UPPER}_MACRO_PREFIX "${_CUR_PREFIX}")

# ----------------------------------
# Prepare
# ----------------------------------
if (_CUR_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
endif ()

# ----------------------------------
# Declare macros
# ----------------------------------

#[[
    Add library target.

    <name>_add_library(<target>
        [SHARED | STATIC | INTERFACE]
        [NO_EXPORT]
        [NO_INSTALL]
        <configure_options...>
    )
]]
#
macro(${_CUR_MACRO_PREFIX}_add_library _target)
    set(options SHARED STATIC INTERFACE)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(FUNC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (FUNC_SHARED)
        set(_type SHARED)
    elseif (FUNC_STATIC)
        set(_type STATIC)
    elseif (FUNC_INTERFACE)
        set(_type INTERFACE)
    elseif (_CUR_BUILD_SHARED)
        set(_type SHARED)
    elseif (BUILD_SHARED_LIBS)
        set(_type SHARED)
    else ()
        set(_type STATIC)
    endif ()

    _cur_add_library_internal(${_target} ${_type} ${FUNC_UNPARSED_ARGUMENTS})
endmacro()

#[[
    Add plugin target with optional description file generation.

    <name>_add_plugin(<target> <category>
        [NO_EXPORT]
        [NO_INSTALL]
        [GEN_DESC]                    # Generate description file
        [VERSION <version>]           # Plugin version (default: ${_CUR_VERSION})
        [TEMPLATE_NAME <template_name>] # Template file name (default: "plugin_desc.json.in")
        [OUTPUT_NAME <output_name>]   # Output JSON file name (default: "plugin.json")
        [NO_INSTALL_DESC]             # Don't install description file
        [EXTRA_VARS <var=value>...]   # Extra variables for template
        <configure_options...>
    )
]]
#
macro(${_CUR_MACRO_PREFIX}_add_plugin _target _category _plugin_folder)
    set(_plugin_dir plugins/${_CUR_INSTALL_NAME}/${_category}/${_plugin_folder})
    set(_IS_PLUGIN TRUE)
    _cur_add_library_internal(${_target} SHARED
            BUILD_RUNTIME_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${_plugin_dir}"
            BUILD_LIBRARY_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${_plugin_dir}"
            BUILD_ARCHIVE_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${_plugin_dir}"
            INSTALL_RUNTIME_DIR "${CMAKE_INSTALL_LIBDIR}/${_plugin_dir}"
            INSTALL_LIBRARY_DIR "${CMAKE_INSTALL_LIBDIR}/${_plugin_dir}"
            INSTALL_ARCHIVE_DIR "${CMAKE_INSTALL_LIBDIR}/${_plugin_dir}"
            ${ARGN}
    )
    unset(_IS_PLUGIN)
    _cur_add_desc_internal(${_target} ${_plugin_dir} ${ARGN})
endmacro()

#[[
    Install targets, CMake configuration files and include files.

    <name>_install(
        [NO_EXPORT]
        [NO_INCLUDE]
    )
]]
#
function(${_CUR_MACRO_PREFIX}_install)
    set(options NO_EXPORT NO_INCLUDE)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(FUNC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT _CUR_INSTALL)
        return()
    endif ()

    if (NOT FUNC_NO_EXPORT)
        qm_basic_install(
                NAME ${_CUR_INSTALL_NAME}
                VERSION ${_CUR_VERSION}
                INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${_CUR_INSTALL_NAME}
                CONFIG_TEMPLATE "${_CUR_CONFIG_TEMPLATE}"
                NAMESPACE ${_CUR_INSTALL_NAME}::
                EXPORT ${_CUR_INSTALL_NAME}Targets
                WRITE_CONFIG_OPTIONS NO_CHECK_REQUIRED_COMPONENTS_MACRO
        )
    endif ()

    if (NOT FUNC_NO_INCLUDE AND _CUR_INCLUDE_DIR)
        get_filename_component(_dir ${_CUR_INCLUDE_DIR} ABSOLUTE BASE_DIR ${_CUR_SOURCE_DIR})
        install(DIRECTORY ${_dir}/
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
                FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.hxx"
        )
    endif ()
endfunction()

# ----------------------------------
# Private
# ----------------------------------
macro(_cur_add_library_internal _target _type)
    set(options NO_EXPORT NO_INSTALL)
    set(oneValueArgs
            BUILD_RUNTIME_DIR BUILD_LIBRARY_DIR BUILD_ARCHIVE_DIR
            INSTALL_RUNTIME_DIR INSTALL_LIBRARY_DIR INSTALL_ARCHIVE_DIR
    )
    set(multiValueArgs)
    cmake_parse_arguments(FUNC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_library(${_target} ${_type})

    qm_set_value(_rc_name FUNC_RC_NAME ${_CUR_INSTALL_NAME})
    qm_set_value(_rc_description FUNC_RC_DESCRIPTION ${_CUR_DESCRIPTION})
    qm_set_value(_rc_copyright FUNC_RC_COPYRIGHT ${_CUR_COPYRIGHT})

    if (WIN32)
        qm_add_win_rc(${_target}
                NAME ${_rc_name}
                DESCRIPTION ${_rc_description}
                COPYRIGHT ${_rc_copyright}
        )
    endif ()

    # Set global definitions
    qm_export_defines(${_target})

    # Configure target
    qm_configure_target(${_target} ${FUNC_UNPARSED_ARGUMENTS})

    set(_include_scope "PUBLIC")

    if (_type STREQUAL "INTERFACE")
        set(_include_scope INTERFACE)
    endif ()

    # Add include directories
    if (_CUR_INCLUDE_DIR)
        target_include_directories(${_target} ${_include_scope}
                $<BUILD_INTERFACE:${_CUR_SOURCE_DIR}/${_CUR_INCLUDE_DIR}>
        )
    endif ()

    if (NOT _type STREQUAL "INTERFACE")
        target_include_directories(${_target} PRIVATE ${_CUR_BUILD_INCLUDE_DIR})
        target_include_directories(${_target} PRIVATE .)
    endif ()

    # Library name
    if (_target MATCHES "^${_CUR_NAME}(.+)")
        set(_name ${CMAKE_MATCH_1})
        set_target_properties(${_target} PROPERTIES EXPORT_NAME ${_name})
    else ()
        set(_name ${_target})
    endif ()

    add_library(${_CUR_INSTALL_NAME}::${_name} ALIAS ${_target})

    # Build output directories
    set(_build_output_dir_options)

    if (FUNC_BUILD_RUNTIME_DIR)
        list(APPEND _build_output_dir_options RUNTIME_OUTPUT_DIRECTORY ${FUNC_BUILD_RUNTIME_DIR})
    endif ()

    if (FUNC_BUILD_LIBRARY_DIR)
        list(APPEND _build_output_dir_options LIBRARY_OUTPUT_DIRECTORY ${FUNC_BUILD_LIBRARY_DIR})
    endif ()

    if (FUNC_BUILD_ARCHIVE_DIR)
        list(APPEND _build_output_dir_options ARCHIVE_OUTPUT_DIRECTORY ${FUNC_BUILD_ARCHIVE_DIR})
    endif ()

    if (_build_output_dir_options)
        set_target_properties(${_target} PROPERTIES ${_build_output_dir_options})
    endif ()

    if (FUNC_SYNC_INCLUDE_PREFIX)
        set(_inc_name ${FUNC_SYNC_INCLUDE_PREFIX})
    else ()
        set(_inc_name ${_target})
    endif ()

    if (_IS_PLUGIN)
        set(_all_direct_deps "")
        get_target_property(_link_libs ${_target} LINK_LIBRARIES)
        if (_link_libs AND NOT _link_libs STREQUAL "_link_libs-NOTFOUND")
            list(APPEND _all_direct_deps ${_link_libs})
        endif ()
        get_target_property(_interface_link_libs ${_target} INTERFACE_LINK_LIBRARIES)
        if (_interface_link_libs AND NOT _interface_link_libs STREQUAL "_interface_link_libs-NOTFOUND")
            list(APPEND _all_direct_deps ${_interface_link_libs})
        endif ()

        set(_queue ${_all_direct_deps})
        set(_processed_targets "")
        set(_deps_to_copy "")

        while (_queue)
            list(POP_FRONT _queue _current)
            if (_current MATCHES "^\\$<")
                continue()
            endif ()
            if (NOT TARGET ${_current})
                continue()
            endif ()
            get_target_property(_aliased ${_current} ALIASED_TARGET)
            if (_aliased)
                set(_current ${_aliased})
            endif ()
            if (_current IN_LIST _processed_targets)
                continue()
            endif ()
            list(APPEND _processed_targets ${_current})

            get_target_property(_type ${_current} TYPE)
            if (_type STREQUAL "SHARED_LIBRARY" OR _type STREQUAL "MODULE_LIBRARY")
                list(APPEND _deps_to_copy $<TARGET_FILE:${_current}>)
            endif ()

            set(_sub_libs "")
            get_target_property(_sub_link_libs ${_current} LINK_LIBRARIES)
            if (_sub_link_libs AND NOT _sub_link_libs STREQUAL "_sub_link_libs-NOTFOUND")
                list(APPEND _sub_libs ${_sub_link_libs})
            endif ()
            get_target_property(_sub_interface_libs ${_current} INTERFACE_LINK_LIBRARIES)
            if (_sub_interface_libs AND NOT _sub_interface_libs STREQUAL "_sub_interface_libs-NOTFOUND")
                list(APPEND _sub_libs ${_sub_interface_libs})
            endif ()
            if (_sub_libs)
                list(APPEND _queue ${_sub_libs})
            endif ()
        endwhile ()

        if (_deps_to_copy)
            list(REMOVE_DUPLICATES _deps_to_copy)
            if (_deps_to_copy)
                message(STATUS "Dependencies to copy: ${_deps_to_copy}")
            endif ()
            add_custom_command(TARGET ${_target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${_deps_to_copy}
                    $<TARGET_FILE_DIR:${_target}>
                    COMMENT "Copying dependent shared libraries to plugin directory")
        endif ()

        # Copy assets directory if it exists
        set(_assets_src_dir "${CMAKE_CURRENT_SOURCE_DIR}/assets")
        if (EXISTS "${_assets_src_dir}")
            set(_assets_dst_dir "$<TARGET_FILE_DIR:${_target}>/assets")
            add_custom_command(TARGET ${_target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${_assets_src_dir}"
                    "${_assets_dst_dir}"
                    COMMENT "Copying assets directory to plugin directory")
            message(STATUS "Assets directory will be copied for ${_target}")
        endif ()
    endif ()

    set(_install_options)

    if (_CUR_INSTALL AND NOT FUNC_NO_INSTALL)
        # Install directories
        qm_set_value(_install_runtime_dir FUNC_INSTALL_RUNTIME_DIR "${CMAKE_INSTALL_BINDIR}")
        qm_set_value(_install_library_dir FUNC_INSTALL_LIBRARY_DIR "${CMAKE_INSTALL_LIBDIR}")
        qm_set_value(_install_archive_dir FUNC_INSTALL_ARCHIVE_DIR "${CMAKE_INSTALL_LIBDIR}")

        if (FUNC_NO_EXPORT)
            set(_export)
        else ()
            set(_export
                    EXPORT ${_CUR_INSTALL_NAME}Targets
                    ARCHIVE DESTINATION "${_install_archive_dir}" OPTIONAL
            )
        endif ()

        install(TARGETS ${_target}
                ${_export}
                RUNTIME DESTINATION "${_install_runtime_dir}" OPTIONAL
                LIBRARY DESTINATION "${_install_library_dir}" OPTIONAL
        )

        # Install assets directory if it exists
        set(_assets_src_dir "${CMAKE_CURRENT_SOURCE_DIR}/assets")
        if (EXISTS "${_assets_src_dir}")
            install(DIRECTORY ${_assets_src_dir}/
                    DESTINATION "${_install_library_dir}/assets"
                    FILES_MATCHING PATTERN "*")
            message(STATUS "Assets directory will be installed for ${_target}")
        endif ()

        target_include_directories(${_target} INTERFACE
                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        )

        if (_CUR_INCLUDE_DIR)
            target_include_directories(${_target} INTERFACE
                    "$<INSTALL_INTERFACE:${_CUR_INCLUDE_DIR}>"
            )
        endif ()

        set(_install_options
                INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}/${_CUR_INSTALL_NAME}/${_inc_name}"
        )

        install(CODE "
                set(target_file \"$<TARGET_FILE:${_target}>\")
                get_filename_component(target_name \"\${target_file}\" NAME)
                set(src_dir \"$<TARGET_FILE_DIR:${_target}>\")
                set(dst_dir \"\${CMAKE_INSTALL_PREFIX}/${_install_library_dir}\")
                file(GLOB dynamic_libs
                    \"\${src_dir}/*.so\"
                    \"\${src_dir}/*.dll\"
                    \"\${src_dir}/*.dylib\"
                )
                foreach(lib \${dynamic_libs})
                    get_filename_component(lib_name \${lib} NAME)
                    if(NOT \${lib_name} STREQUAL \${target_name})
                        message(STATUS \"Installing dependency: \${lib}\")
                        file(INSTALL \${lib} DESTINATION \${dst_dir})
                    endif()
                endforeach()
            ")

    endif ()

    if (FUNC_SYNC_INCLUDE OR (_CUR_SYNC_INCLUDE AND NOT FUNC_NO_SYNC_INCLUDE))
        # Generate a standard include directory in build directory
        qm_sync_include(. "${_CUR_GENERATED_INCLUDE_DIR}/${_inc_name}" ${_install_options}
                ${FUNC_SYNC_INCLUDE_OPTIONS} FORCE
        )
        target_include_directories(${_target} ${_include_scope}
                "$<BUILD_INTERFACE:${_CUR_GENERATED_INCLUDE_DIR}>"
        )
        target_include_directories(${_target} INTERFACE
                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${_CUR_INSTALL_NAME}>"
        )
    endif ()
endmacro()

macro(_cur_add_desc_internal _target _plugin_dir)
    # Set default values
    qm_set_value(_version FUNC_VERSION ${_CUR_VERSION})
    qm_set_value(_output_name FUNC_OUTPUT_NAME "plugin.json")
    qm_set_value(_template_name FUNC_TEMPLATE_NAME "plugin_desc.json.in")

    # Template file path - look in project root's cmake directory
    set(_template_file "${PROJECT_ROOT_DIR}/cmake/${_template_name}")

    # Fallback: look in current directory
    if (NOT EXISTS "${_template_file}")
        set(_template_file "${CMAKE_CURRENT_SOURCE_DIR}/${_template_name}")
    endif ()

    # Check if template exists
    if (NOT EXISTS "${_template_file}")
        message(WARNING "Template file not found: ${_template_name}. Looking in: ${PROJECT_SOURCE_DIR}/cmake/ and ${CMAKE_CURRENT_SOURCE_DIR}/")
    else ()
        # Target variables
        get_target_property(_target_prefix ${_target} PREFIX)
        if (_target_prefix STREQUAL "_target_prefix-NOTFOUND")
            set(_target_prefix "${CMAKE_SHARED_LIBRARY_PREFIX}")
        endif ()
        get_target_property(_target_output_name ${_target} OUTPUT_NAME)
        if (_target_output_name STREQUAL "_target_output_name-NOTFOUND")
            set(_target_output_name "${_target}")
        endif ()
        set(_target_name "${_target_prefix}${_target_output_name}${CMAKE_SHARED_LIBRARY_SUFFIX}")

        if (WIN32 AND MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(_target_name "${_target_prefix}${_target_output_name}d${CMAKE_SHARED_LIBRARY_SUFFIX}")
        endif ()

        set(_plugin_name ${_CUR_INSTALL_NAME})
        string(TIMESTAMP _timestamp "%Y-%m-%dT%H:%M:%SZ")

        # Set template variables
        set(PROJECT_VERSION "${_version}")
        set(PLUGIN_NAME "${_plugin_name}")
        set(TARGET_NAME "${_target_name}")
        set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
        set(TIMESTAMP "${_timestamp}")
        set(PROJECT_DESCRIPTION "${_CUR_DESCRIPTION}")
        set(PROJECT_AUTHOR "${_CUR_AUTHOR}")
        set(PROJECT_COPYRIGHT "${_CUR_COPYRIGHT}")

        # Additional project variables
        set(PROJECT_NAME "${_CUR_NAME}")
        set(BUILD_YEAR "${_CUR_BUILD_YEAR}")
        set(PROJECT_START_YEAR "${_CUR_START_YEAR}")

        # Process extra variables
        foreach (_var_pair ${FUNC_EXTRA_VARS})
            string(REPLACE "=" ";" _pair_list ${_var_pair})
            list(GET _pair_list 0 _var_name)
            list(GET _pair_list 1 _var_value)
            set(${_var_name} "${_var_value}")
        endforeach ()

        # Generate JSON file
        set(_generated_file "${_plugin_dir}/${_output_name}")

        configure_file(
                "${_template_file}"
                "${_generated_file}"
                @ONLY
        )

        message(STATUS "Generated description file: ${_generated_file} from template: ${_template_file}")

        # Copy to build output directory (same as plugin)
        add_custom_command(
                TARGET ${_target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${_generated_file}"
                "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${_plugin_dir}/${_output_name}"
                COMMENT "Generating ${_output_name} for ${_target}"
        )

        # Install description file if requested
        if (_CUR_INSTALL AND NOT FUNC_NO_INSTALL AND NOT FUNC_NO_INSTALL_DESC)
            # Install to same directory as plugin
            install(
                    FILES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${_plugin_dir}/${_output_name}
                    DESTINATION "${CMAKE_INSTALL_LIBDIR}/${_plugin_dir}"
                    RENAME ${_output_name}
            )

            message(STATUS "Will install ${_output_name} to ${CMAKE_INSTALL_LIBDIR}/${_plugin_dir}")
        endif ()
    endif ()
endmacro()