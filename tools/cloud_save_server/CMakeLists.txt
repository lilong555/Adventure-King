cmake_minimum_required(VERSION 3.20)

project(ak_cloud_save_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ak_cloud_save_server
    src/main.cpp
)

# 使用项目内 rapidjson（来自 cocos2d-x external/json），避免引入额外 JSON 依赖
target_include_directories(ak_cloud_save_server PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../Adventure-King/cocos2d/external
)

if(UNIX AND NOT APPLE)
    target_link_libraries(ak_cloud_save_server PRIVATE pthread)
endif()

if(WIN32)
    target_link_libraries(ak_cloud_save_server PRIVATE ws2_32)
endif()
