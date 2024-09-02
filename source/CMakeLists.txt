file(GLOB_RECURSE source CONFIGURE_DEPENDS src/*.cpp)

add_executable(LearnVulkan ${source})

# Include directories
target_include_directories(LearnVulkan PUBLIC include)
target_include_directories(LearnVulkan PUBLIC thirdparty/stb)

# compile options
# GLM NDC 坐标的深度范围是【-1，1】，而 Vulkan 是【0，1】
target_compile_definitions(LearnVulkan PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)

target_link_libraries(LearnVulkan PUBLIC glfw glm)

# Vulkan
find_package(Vulkan REQUIRED)
if( Vulkan_FOUND )
    message("++ Found Vulkan SDK : ${Vulkan_INCLUDE_DIRS}")
endif()

target_include_directories(LearnVulkan PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(LearnVulkan PUBLIC ${Vulkan_LIBRARIES})
