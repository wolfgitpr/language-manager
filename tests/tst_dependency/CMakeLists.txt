project(tst_dependency)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)

add_executable(${PROJECT_NAME}
    main.cpp
    tst_dependency_graph.cpp
    tst_dependency_resolver.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE LangCore::LangCore)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})