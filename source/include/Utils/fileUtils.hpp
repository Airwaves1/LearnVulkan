#pragma once

#include <string>
#include <vector>
#include <fstream>

#include "utils/log.hpp"

#ifdef PROJECT_ROOT_PATH
    #define RESOURCE_PATH PROJECT_ROOT_PATH "/resources/"
    #define SHADER_PATH PROJECT_ROOT_PATH "/resources/shaders/"
    #define TEXTURE_PATH PROJECT_ROOT_PATH "/resources/textures/"
#endif

// 加载二进制文件
static std::vector<char> readFile(const std::string& filename) {

    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        LOG_ERROR("failed to open file: {}", filename);
        throw std::runtime_error("failed to open file: " + filename);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}