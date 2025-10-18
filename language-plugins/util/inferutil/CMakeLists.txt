project(inferutil
        VERSION ${LANGPLUGINS_VERSION}
        LANGUAGES CXX
)

file(GLOB_RECURSE _src *.h *.cpp)

LangPlugins_add_library(${PROJECT_NAME} STATIC NO_INSTALL
        SOURCES ${_src}
        FEATURES cxx_std_17
        LINKS LangPlugins
        INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
)