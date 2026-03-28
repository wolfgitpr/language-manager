project(MandarinG2p
        VERSION ${LANGPLUGINS_VERSION}
        LANGUAGES C CXX
)

find_package(re2 CONFIG REQUIRED)
find_package(cpp-pinyin CONFIG REQUIRED)

file(GLOB_RECURSE _src *.h *.cpp)

LangPlugins_add_plugin(${PROJECT_NAME} ${CURRENT_PLUGIN_CATEGORY} ${PROJECT_NAME} NO_EXPORT
        SOURCES ${_src}
        FEATURES cxx_std_17
        LINKS LangCore::LangCore cpp-pinyin::cpp-pinyin re2::re2
        LINKS_PRIVATE $<BUILD_INTERFACE:InferUtil>
        INCLUDE_PRIVATE *
        DEFINES
)
