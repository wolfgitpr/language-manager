project(TemplateG2p
        VERSION ${LANGPLUGINS_VERSION}
        LANGUAGES C CXX
)

find_package(re2 CONFIG REQUIRED)
file(GLOB_RECURSE _src *.h *.cpp)

LangPlugins_add_plugin(${PROJECT_NAME} ${CURRENT_PLUGIN_CATEGORY} ${PROJECT_NAME} NO_EXPORT
        SOURCES ${_src}
        FEATURES cxx_std_17
        LINKS LangCore::LangCore re2::re2
        LINKS_PRIVATE $<BUILD_INTERFACE:InferUtil>
        INCLUDE_PRIVATE *
        DEFINES
)
