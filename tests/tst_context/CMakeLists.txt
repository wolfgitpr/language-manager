project(tst_context)

set(_src
    tst_framework.h
    main.cpp
    tst_fqid.cpp
    tst_context_isolation.cpp
    tst_context_dedup.cpp
    tst_context_convert.cpp
    tst_context_version.cpp
)

add_executable(${PROJECT_NAME} ${_src})
target_link_libraries(${PROJECT_NAME} PRIVATE LangCore::LangCore)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
