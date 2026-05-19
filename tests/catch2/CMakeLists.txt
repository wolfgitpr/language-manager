file(GLOB _catch2_src ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(LangMgrTests ${_catch2_src})
target_include_directories(LangMgrTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
target_link_libraries(LangMgrTests PRIVATE LangCore::LangCore)

add_test(NAME LangMgrTests COMMAND LangMgrTests)