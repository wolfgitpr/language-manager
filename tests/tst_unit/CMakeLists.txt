project(tst_unit)

file(GLOB _src *.h *.cpp)

add_executable(${PROJECT_NAME} ${_src})
target_link_libraries(${PROJECT_NAME} PRIVATE LangCore::LangCore)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
