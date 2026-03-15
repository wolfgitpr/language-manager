file(GLOB_RECURSE _src "*.h" "*.cpp")

LangPlugins_add_library(${PROJECT_NAME} SHARED
        SOURCES ${_src}
        LINKS LangCore::LangCore
        LINKS_PRIVATE
        INCLUDE_PRIVATE ../../include/** **
)