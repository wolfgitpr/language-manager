project(g2pProvider
    VERSION ${LANGPLUGINS_VERSION}
    LANGUAGES CXX
)

file(GLOB_RECURSE _src *.h *.cpp)

LangPlugins_add_plugin(${PROJECT_NAME} ${CURRENT_PLUGIN_CATEGORY} ${PROJECT_NAME} NO_EXPORT
    SOURCES ${_src}
    FEATURES cxx_std_17
    LINKS LangPlugins
    LINKS_PRIVATE
    INCLUDE_PRIVATE *
)