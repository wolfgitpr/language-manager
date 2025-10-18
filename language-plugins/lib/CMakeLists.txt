file(GLOB_RECURSE _src "*.cpp")

LangPlugins_add_library(${PROJECT_NAME} SHARED
        SOURCES ${_src}
        LINKS LangMgr
        LINKS_PRIVATE
        INCLUDE_PRIVATE ../include/** **
)
