project(InferUtil
        VERSION ${LANGPLUGINS_VERSION}
        LANGUAGES C CXX
)

find_package(re2 CONFIG REQUIRED)
file(GLOB_RECURSE _src *.h *.cpp)

LangPlugins_add_library(${PROJECT_NAME} STATIC NO_INSTALL
        SOURCES ${_src}
        FEATURES cxx_std_17
        LINKS LangCore::LangCore LangPlugins::LangPlugins re2::re2
        INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
)