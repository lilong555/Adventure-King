cmake_minimum_required(VERSION 3.20)

project(ak_cloud_save_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# 说明：cpp-httplib 为 header-only，这里在构建阶段拉取固定版本（避免把大文件直接提交进仓库）。
FetchContent_Declare(
    cpp_httplib
    URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.15.3.zip
)
FetchContent_GetProperties(cpp_httplib)
if(NOT cpp_httplib_POPULATED)
    FetchContent_Populate(cpp_httplib)
endif()

add_executable(ak_cloud_save_server
    src/main.cpp
)

# 使用项目内 rapidjson（来自 cocos2d-x external/json），避免引入额外 JSON 依赖
target_include_directories(ak_cloud_save_server PRIVATE
    ${cpp_httplib_SOURCE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../Adventure-King/cocos2d/external
)

if(UNIX AND NOT APPLE)
    target_link_libraries(ak_cloud_save_server PRIVATE pthread)
endif()

