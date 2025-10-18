project(tst_langMgr)

file(GLOB _src *.h *.cpp)
add_executable(${PROJECT_NAME} ${_src})
target_link_libraries(${PROJECT_NAME} PRIVATE LangMgr::LangMgr)