project(DsDict
        VERSION ${LANGPLUGINS_VERSION}
        LANGUAGES CXX
)

file(GLOB_RECURSE _src *.h *.cpp)

LangPlugins_add_plugin(${PROJECT_NAME} dict ${PROJECT_NAME} NO_EXPORT
        SOURCES ${_src}
        FEATURES cxx_std_17
        LINKS LangCore::LangCore
        INCLUDE_PRIVATE *
)