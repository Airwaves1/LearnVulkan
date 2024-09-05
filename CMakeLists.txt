cmake_minimum_required(VERSION 3.7)

project(
    LearnVulkan
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置项目路径宏
set(PROJECT_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
add_definitions(-DPROJECT_ROOT_PATH="${PROJECT_ROOT_PATH}")

# set(CMAKE_BUILD_TYPE Release)
include_directories(thirdparty/spdlog/include)


add_subdirectory(thirdparty/glfw)
add_subdirectory(thirdparty/glm)
add_subdirectory(thirdparty/spdlog)

add_subdirectory(source)