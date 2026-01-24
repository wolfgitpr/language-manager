project(ChineseG2p
        VERSION ${LANGPLUGINS_VERSION}
        LANGUAGES C CXX
)

find_package(cpp-pinyin CONFIG REQUIRED)

file(GLOB_RECURSE _src *.h *.cpp)

LangPlugins_add_plugin(${PROJECT_NAME} ${CURRENT_PLUGIN_CATEGORY} ${PROJECT_NAME} NO_EXPORT
        SOURCES ${_src}
        FEATURES cxx_std_17
        LINKS LangPlugins cpp-pinyin::cpp-pinyin
        LINKS_PRIVATE $<BUILD_INTERFACE:inferutil>
        INCLUDE_PRIVATE *
        DEFINES
)
