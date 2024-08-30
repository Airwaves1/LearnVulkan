

file(GLOB_RECURSE source CONFIGURE_DEPENDS src/*.cpp)

add_executable(LearnVulkan ${source})

