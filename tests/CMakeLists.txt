enable_testing()

add_subdirectory(tst_unit)
add_subdirectory(tst_context)
add_subdirectory(tst_dependency)
add_subdirectory(tst_support)
if(LANGMGR_BUILD_PLUGINS)
    add_subdirectory(tst_langCore)
endif()
