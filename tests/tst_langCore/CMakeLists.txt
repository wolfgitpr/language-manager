project(tst_langCore)

find_package(re2 CONFIG REQUIRED)

file(GLOB _src *.h *.cpp)
add_executable(${PROJECT_NAME} ${_src})
target_link_libraries(${PROJECT_NAME} PRIVATE LangCore::LangCore re2::re2)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
