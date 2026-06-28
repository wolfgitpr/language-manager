file(GLOB _catch2_src ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

find_package(stdcorelib CONFIG REQUIRED)

add_executable(LangMgrTests ${_catch2_src})
target_include_directories(LangMgrTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
target_link_libraries(LangMgrTests PRIVATE LangCore::LangCore)

add_custom_command(TARGET LangMgrTests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:stdcorelib::stdcorelib>
        $<TARGET_FILE_DIR:LangMgrTests>
        COMMENT "Copying dependency DLLs to test output directory")

add_test(NAME LangMgrTests COMMAND LangMgrTests)