project(LangMgrTest)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(${PROJECT_NAME} main.cpp)

find_package(Qt6 COMPONENTS Core REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        LangMgr::LangMgr
)