#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#define VK_EXT_DEBUG_UTILS_EXTENSION_NAME "VK_EXT_debug_utils"
#define VK_KHR_SURFACE_EXTENSION_NAME "VK_KHR_surface"

#include <vector>

#include <iostream>
// #include <stdexcept>
#include <cstdlib>

#include "utils/fileUtils.hpp"
#include "utils/log.hpp"
#include <algorithm>
#include <map>
#include <optional>
#include <set>

#define VK_KRH_SURFACE_EXTENSION_NAME "VK_KHR_surface"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphicsFamily; // 图形队列族
  std::optional<uint32_t> presentFamily;  // 呈现队列族

  bool isComplete()
  {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;      // 表面能力
  std::vector<VkSurfaceFormatKHR> formats;    // 表面格式
  std::vector<VkPresentModeKHR> presentModes; // 呈现模式
};

class HelloTriangleApplication
{
public:
  void run()
  {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  void initWindow()
  {
    if (!glfwInit())
    {
      throw std::runtime_error("failed to initialize GLFW!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // 禁用OpenGL兼容性
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);   // 禁止窗口调整大小, 暂时禁止窗口调整大小

    if (!(m_window = glfwCreateWindow(m_width, m_height, "Vulkan", nullptr, nullptr)))
    {
      throw std::runtime_error("failed to create window!");
    }
  }

  void initVulkan()
  {
    // 创建Vulkan实例
    // 1. 创建Vulkan实例
    createInstance();

    // 2. 设置调试报告
    setupDebugMessenger();

    // 3. 创建窗口表面
    createSurface();

    // 4. 选择一个合适的物理设备
    pickPhysicalDevice();

    // 5. 创建逻辑设备
    createLogicalDevice();

    // 6. 创建交换链
    createSwapChain();

    // 7. 创建交换链图像视图
    createImageViews();

    // 8. 创建图形渲染管线
    createGraphicsPipeline();
  }

  void mainLoop()
  {
    while (!glfwWindowShouldClose(m_window))
    {
      glfwPollEvents();
    }
  }

  void cleanup()
  {
    // 清理调试报告
    if (enableValidationLayers)
    {
      DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
    }

    // 销毁图形管线
    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);

    // 清理交换链图像视图
    for (auto imageView : m_swapChainImageViews)
    {
      vkDestroyImageView(m_device, imageView, nullptr);
    }

    // 清理交换链
    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);

    // 清理逻辑设备
    vkDestroyDevice(m_device, nullptr);

    // 清理窗口表面
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

    // VkInstance 应该在应用程序退出之前被清理
    vkDestroyInstance(m_instance, nullptr);

    glfwDestroyWindow(m_window);

    glfwTerminate();
  }

public:
  // 创建调试报告
  static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pCallback)
  {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
      return func(instance, pCreateInfo, pAllocator, pCallback);
    }
    else
    {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
  }

  // 销毁调试报告
  static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks *pAllocator)
  {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
      func(instance, callback, pAllocator);
    }
  }

private:
  // 创建Vulkan实例
  void createInstance()
  {
    // 检查指定的验证层是否被支持
    if (enableValidationLayers && !checkValidationLayerSupport())
    {
      throw std::runtime_error("validation layers requested, but not available!");
    }

    // VkApplicationInfo 结构体---------------------------------------------------------------------------
    // 提供应用程序基本信息，一般用于驱动程序优化和调试
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,    // 结构体类型，必须是 VK_STRUCTURE_TYPE_APPLICATION_INFO
        .pNext = nullptr,                               // 指向扩展信息的指针
        .pApplicationName = "Hello Triangle",           // 应用程序名称字符串的指针
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0), // 应用程序版本号
        .pEngineName = "No Engine",                     // 引擎名称字符串的指针
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),      // 引擎版本号
        .apiVersion = VK_API_VERSION_1_1,               // 使用的Vulkan API版本
    };

    //---------------------------------------------------------------------------------------------------

    // VkInstanceCreateInfo 结构体-----------------------------------------------------------------------
    // 获取Vulkan支持的扩展列表
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    m_extensions.resize(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, m_extensions.data());
    {
      // std::cout << "available extensions:" << std::endl;
      LOG_INFO("available extensions:");
      for (const auto &extension : m_extensions)
      {
        // std::cout << '\t' << extension.extensionName << std::endl;
        LOG_INFO("\t{}", extension.extensionName);
      }
    }
    // 告知Vulkan驱动我们要使用哪些全局的扩展以及验证层。
    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, // 结构体类型，必须是 VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO
        .pNext = nullptr,                                // 指向扩展信息的指针
        .flags = 0,                                      // 保留字段，必须为0
        .pApplicationInfo = &appInfo,                    // 指向应用程序信息的指针
        .enabledLayerCount = 0,                          // 启用的验证层数量
        .ppEnabledLayerNames = nullptr,                  // 启用的验证层名称列表
        .enabledExtensionCount = 0,                      // 启用的扩展数量
        .ppEnabledExtensionNames = nullptr,              // 启用的扩展名称列表
    };
    // 创建与窗口系统相接的扩展
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size()); // 启用的扩展数量
    createInfo.ppEnabledExtensionNames = extensions.data();                      // 启用的扩展名称列表

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers)
    {
      createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size()); // 启用的验证层数量
      createInfo.ppEnabledLayerNames = m_validationLayers.data();                      // 启用的验证层名称列表

      // 启用调试报告
      populateDebugMessengerCreateInfo(debugCreateInfo);
      createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    }
    else
    {
      createInfo.enabledLayerCount = 0;
      createInfo.pNext = nullptr;
    }

    //---------------------------------------------------------------------------------------------------

    // 创建Vulkan实例,一般的Vulkan函数都会返回一个 VkResult 类型的值，用于检查函数是否成功执行
    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create instance!");
    }
  }

  // 创建窗口表面
  void createSurface()
  {
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create window surface!");
    }
  }

  // 选择一个合适的物理设备
  void pickPhysicalDevice()
  {
    // 获取物理设备数量
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDeviceGroups(m_instance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
      LOG_ERROR("failed to find GPUs with Vulkan support!");
      throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    // 获取物理设备列表
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    // 为每个物理设备打分
    std::multimap<int, VkPhysicalDevice> candidates;
    for (const auto &device : devices)
    {
      int score = rateDeviceSuitability(device);
      candidates.insert(std::make_pair(score, device));
    }

    // 打印得分结果
    LOG_INFO("Device score:");
    for (const auto &candidate : candidates)
    {
      LOG_INFO("Device score: {}", candidate.first);
    }

    // 检查最高分的物理设备是否合适
    for (const auto &candidate : candidates)
    {
      if (isDeviceSuitable(candidate.second))
      {
        m_physicalDevice = candidate.second;
        break;
      }
    }

    if (m_physicalDevice == VK_NULL_HANDLE)
    {
      LOG_ERROR("failed to find a suitable GPU!");
      // throw std::runtime_error("failed to find a suitable GPU!");
    }
  }

  // 寻找队列族
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
  {
    QueueFamilyIndices indices;

    // 获取队列族数量
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    // VkQueueFamilyProperties 结构体 包含有关队列族的信息，
    // 例如队列族支持的操作类型以及队列族支持的队列数量，这里我们只关心是否支持图形操作
    int i = 0;
    for (const auto &queueFamily : queueFamilies)
    {
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
        indices.graphicsFamily = i;
      }
      i++;
    }

    // 检查队列族是否支持呈现操作
    VkBool32 presentSupport = false;
    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

      if (presentSupport)
      {
        indices.presentFamily = i;
      }
    }

    return indices;
  }

  // 创建逻辑设备
  void createLogicalDevice()
  {
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;

    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
      VkDeviceQueueCreateInfo queueCreateInfo = {
          .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
          .queueFamilyIndex = queueFamily,
          .queueCount = 1,
          .pQueuePriorities = &queuePriority,
      };
      queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledLayerCount = 0,
        .enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size()),
        .ppEnabledExtensionNames = m_deviceExtensions.data(),
        .pEnabledFeatures = &deviceFeatures,
    };

    if (enableValidationLayers)
    {
      createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
      createInfo.ppEnabledLayerNames = m_validationLayers.data();
    }

    // 创建逻辑设备
    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
    {
      LOG_ERROR("failed to create logical device!");
      return;
      // throw std::runtime_error("failed to create logical device!");
    }

    // 获取图形队列句柄
    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);

    // 获取呈现队列句柄
    vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
  }

  // 创建交换链
  void createSwapChain()
  {
    // 获取交换链支持详情
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    // 交换链图像数量
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1; // 运行时最小图像数量，加1是为了三重缓冲
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
      imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // 填入交换链创建信息
    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    };

    // 交换链队列族索引
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily)
    {
      createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
      createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      createInfo.queueFamilyIndexCount = 0;     // Optional
      createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    // 创建交换链
    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
    {
      LOG_ERROR("failed to create swap chain!");
      throw std::runtime_error("failed to create swap chain!");
    }

    // 获取交换链图像句柄
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

    // 保存交换链图像格式和分辨率
    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
  }

  // 创建交换链图像视图
  void createImageViews()
  {
    // 1. 调整列表的大小以容纳所有图像视图
    m_swapChainImageViews.resize(m_swapChainImages.size());

    // 2. 遍历交换链图像，为每个图像创建视图
    for (size_t i = 0; i < m_swapChainImages.size(); i++)
    {
      // 创建图像视图
      VkImageViewCreateInfo createInfo = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .image = m_swapChainImages[i],     // 指定图像句柄
          .viewType = VK_IMAGE_VIEW_TYPE_2D, // 指定图像视图类型
          .format = m_swapChainImageFormat,  // 指定图像格式
          .components = {
              // 指定颜色通道映射
              .r = VK_COMPONENT_SWIZZLE_IDENTITY,
              .g = VK_COMPONENT_SWIZZLE_IDENTITY,
              .b = VK_COMPONENT_SWIZZLE_IDENTITY,
              .a = VK_COMPONENT_SWIZZLE_IDENTITY,
          },
          .subresourceRange = {
              // 指定图像的哪一部分将被视图访问
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
      };

      if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS)
      {
        LOG_ERROR("failed to create image views!");
        throw std::runtime_error("failed to create image views!");
      }
    }
  }

  // 创建图形渲染管线
  void createGraphicsPipeline()
  {
    // 1.创建着色器阶段
    auto vertShaderCode = readFile(SHADER_PATH "00/triangle.vert.spv");
    auto fragShaderCode = readFile(SHADER_PATH "00/triangle.frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // 2.创建固定功能管线
    // 2.1 动态状态
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH};

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };

    // 2.2 顶点输入
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // 结构体类型
        .vertexBindingDescriptionCount = 0,                                 // 顶点绑定描述数量
        .pVertexBindingDescriptions = nullptr,                              // 顶点绑定描述
        .vertexAttributeDescriptionCount = 0,                               // 顶点属性描述数量
        .pVertexAttributeDescriptions = nullptr,                            // 顶点属性描述
    };

    // 2.3 输入组装
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, // 结构体类型
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,                      // 拓扑图元类型
        .primitiveRestartEnable = VK_FALSE,                                   // 是否启用重启图元
    };

    // 2.4 视口和裁剪
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)m_swapChainExtent.width,
        .height = (float)m_swapChainExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = m_swapChainExtent,
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, // 结构体类型
        .viewportCount = 1,                                             // 视口数量
        .pViewports = &viewport,                                        // 视口
        .scissorCount = 1,                                              // 裁剪矩形数量
        .pScissors = &scissor,                                          // 裁剪矩形
    };

    // 2.5 光栅化
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, // 结构体类型
        .depthClampEnable = VK_FALSE,                                        // 深度截断
        .rasterizerDiscardEnable = VK_FALSE,                                 // 光栅化丢弃
        .polygonMode = VK_POLYGON_MODE_FILL,                                 // 多边形模式
        .cullMode = VK_CULL_MODE_BACK_BIT,                                   // 背面剔除
        .frontFace = VK_FRONT_FACE_CLOCKWISE,                                // 正面顺时针
        .depthBiasEnable = VK_FALSE,                                         // 深度偏移
        .lineWidth = 1.0f,                                                   // 线宽
    };

    // 2.6 多重采样
    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, // 结构体类型
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,                     // 光栅化采样
        .sampleShadingEnable = VK_FALSE,                                   // 采样着色
        .minSampleShading = 1.0f,                                          // 最小采样
        .pSampleMask = nullptr,                                            // 采样掩码
        .alphaToCoverageEnable = VK_FALSE,                                 // Alpha覆盖
        .alphaToOneEnable = VK_FALSE,                                      // Alpha一个
    };

    // 2.7 深度和模板测试
    VkPipelineDepthStencilStateCreateInfo depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, // 结构体类型
        .depthTestEnable = VK_TRUE,                                          // 深度测试
        .depthWriteEnable = VK_TRUE,                                         // 深度写入
        .depthCompareOp = VK_COMPARE_OP_LESS,                                // 深度比较
        .depthBoundsTestEnable = VK_FALSE,                                   // 深度边界测试
        .stencilTestEnable = VK_FALSE,                                       // 模板测试
        .front = {},                                                         // 正面
        .back = {},                                                          // 背面
        .minDepthBounds = 0.0f,                                              // 最小深度边界
        .maxDepthBounds = 1.0f,                                              // 最大深度边界
    };

    // 2.8 颜色混合
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = VK_FALSE,                                                                                                      // 混合启用
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,                                                                                   // 源颜色混合因子
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,                                                                                  // 目标颜色混合因子
        .colorBlendOp = VK_BLEND_OP_ADD,                                                                                              // 颜色混合操作
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,                                                                                   // 源Alpha混合因子
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,                                                                                  // 目标Alpha混合因子
        .alphaBlendOp = VK_BLEND_OP_ADD,                                                                                              // Alpha混合操作                                                                  
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, // 颜色写入掩码
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, // 结构体类型
        .logicOpEnable = VK_FALSE,                                        // 逻辑操作启用
        .logicOp = VK_LOGIC_OP_COPY,                                      // 逻辑操作
        .attachmentCount = 1,                                             // 附件数量
        .pAttachments = &colorBlendAttachment,                            // 附件
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},                      // 混合常数
    };
    
    // 3.创建图形管线
    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, // 结构体类型
        .stageCount = 2,                                         // 着色器阶段数量
        .pStages = shaderStages,                                 // 着色器阶段
        .pVertexInputState = &vertexInputInfo,                   // 顶点输入
        .pInputAssemblyState = &inputAssembly,                   // 输入组装
        .pViewportState = &viewportState,                        // 视口和裁剪
        .pRasterizationState = &rasterizer,                      // 光栅化
        .pMultisampleState = &multisampling,                      // 多重采样
        .pDepthStencilState = &depthStencil,                      // 深度和模板测试
        .pColorBlendState = &colorBlending,                       // 颜色混合
        .pDynamicState = &dynamicState,                          // 动态状态
        .layout = VK_NULL_HANDLE,                                // 管线布局
        .renderPass = VK_NULL_HANDLE,                            // 渲染通道
        .subpass = 0,                                            // 子通道
        .basePipelineHandle = VK_NULL_HANDLE,                    // 基础管线句柄
        .basePipelineIndex = -1,                                 // 基础管线索引
    };

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
    {
      LOG_ERROR("failed to create graphics pipeline!");
      throw std::runtime_error("failed to create graphics pipeline!");
    }

  }

private:
  // 一些辅助函数

  // 创建VkShaderModule
  VkShaderModule createShaderModule(const std::vector<char> &code)
  {
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t *>(code.data()),
    };

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
      LOG_ERROR("failed to create shader module!");
      throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
  }

  // 检查物理设备是否合适
  bool isDeviceSuitable(VkPhysicalDevice device)
  {
    QueueFamilyIndices indices = findQueueFamilies(device); // 寻找队列族

    bool extensionsSupported = checkDeviceExtensionSupport(device); // 检查设备扩展是否支持

    bool swapChainAdequate = false; // 交换链是否支持
    if (extensionsSupported)
    {
      SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
      swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
  }

  // 枚举扩展并检查其中是否包含所有必需的扩展
  bool checkDeviceExtensionSupport(VkPhysicalDevice device)
  {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());

    for (const auto &extension : availableExtensions)
    {
      requiredExtensions.erase(extension.extensionName);
    }

    if (requiredExtensions.empty())
    {
      LOG_DEBUG("Remaining required extensions:");
      for (const auto &extension : requiredExtensions)
      {
        LOG_DEBUG("\t{}", extension);
      }
    }

    return requiredExtensions.empty();
  }

  // 给物理设备打分
  int rateDeviceSuitability(VkPhysicalDevice device)
  {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int score = 0;

    // 为离散GPU打分
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
      score += 1000;
    }

    // 为支持的图形族打分
    if (!deviceFeatures.geometryShader)
    {
      return 0;
    }

    return score;
  }

  // 获取交换链支持的详细信息
  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
  {
    SwapChainSupportDetails details;

    // 获取基本表面能力
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    // 获取表面格式
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
      details.formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    // 获取呈现模式
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
      details.presentModes.resize(presentModeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }

    return details;
  }

  // 表面格式,每个VkSurfaceFormatKHR条目包含一个format和一个colorSpace成员
  // format成员指定颜色通道和存储类型，colorSpace成员指定颜色空间
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
  {
    for (const auto &availableFormat : availableFormats)
    {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
        return availableFormat;
      }
    }

    return availableFormats[0];
  }

  // 交换链呈现模式
  // VK_PRESENT_MODE_IMMEDIATE_KHR：应用程序提交图像后立即显示，可能会造成撕裂
  // VK_PRESENT_MODE_FIFO_KHR：交换链以队列的方式显示图像，当队列满时应用程序会被阻塞
  // VK_PRESENT_MODE_FIFO_RELAXED_KHR：交换链以队列的方式显示图像，当队列满时会显示新的图像，可能会造成撕裂
  // VK_PRESENT_MODE_MAILBOX_KHR：交换链以队列的方式显示图像，当队列满时会显示新的图像，旧的图像会被丢弃
  VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
  {
    for (const auto &availablePresentMode : availablePresentModes)
    {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
      {
        return availablePresentMode;
      }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
  }

  // 交换链分辨率
  // 如果当前分辨率不受限制，则返回当前分辨率
  // 否则返回最大分辨率
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
  {
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
      return capabilities.currentExtent;
    }
    else
    {
      int width, height;
      glfwGetFramebufferSize(m_window, &width, &height);

      VkExtent2D actualExtent = {
          static_cast<uint32_t>(width),
          static_cast<uint32_t>(height),
      };

      actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(actualExtent.width, capabilities.maxImageExtent.width));
      actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(actualExtent.height, capabilities.maxImageExtent.height));

      return actualExtent;
    }
  }

  // 填充调试报告信息
  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
  {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
  }

  // 检查指定的验证层是否被支持
  bool checkValidationLayerSupport()
  {
    // 获取支持的验证层数量
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    // 获取支持的验证层列表
    std::vector<VkLayerProperties> availableLayers(layerCount); // 获取支持的验证层列表
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // 检查所有的指定的层是否都被支持
    for (auto layerName : m_validationLayers)
    {
      bool layerFound = false;

      for (const auto &layerProperties : availableLayers)
      {
        if (strcmp(layerName, layerProperties.layerName) == 0)
        {
          layerFound = true;
          break;
        }
      }

      if (!layerFound)
      {
        return false;
      }
    }

    return true;
  }

  // 根据启用的验证层返回我们需要的插件列表
  std::vector<const char *> getRequiredExtensions()
  {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // 如果启用了验证层，添加相应的调试报告插件
    if (enableValidationLayers)
    {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
  }

  // 信息回调函数, 返回一个布尔值指示当验证层消息被Vulkan函数调用触发时是否应该退出程序
  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,    // 消息的严重性
      VkDebugUtilsMessageTypeFlagsEXT messageType,               // 消息的类型
      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, // 指向包含消息的结构体
      void *pUserData)
  {
    std::string validationLayer = "validation layer: " + std::string(pCallbackData->pMessage);

    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      LOG_TRACE("{}", validationLayer);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      LOG_INFO("{}", validationLayer);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      LOG_WARN("{}", validationLayer);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      LOG_ERROR("{}", validationLayer);
      break;
    default:
      break;
    }

    return VK_FALSE;
  }

  void setupDebugMessenger()
  {
    if (!enableValidationLayers)
      return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
        .pUserData = nullptr,
    };

    if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to set up debug messenger!");
    }
  }

private:
  uint32_t m_width = 1200;
  uint32_t m_height = 900;
  GLFWwindow *m_window = nullptr;

  VkInstance m_instance; // Vulkan 实例句柄

  VkSurfaceKHR m_surface; // Vulkan 窗口表面句柄

  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE; // Vulkan 物理设备

  VkDevice m_device = VK_NULL_HANDLE; // Vulkan 逻辑设备

  VkQueue m_graphicsQueue; // Vulkan 图形队列

  VkQueue m_presentQueue; // Vulkan 呈现队列

  VkSwapchainKHR m_swapChain; // Vulkan 交换链

  std::vector<VkImage> m_swapChainImages; // 交换链图像

  VkFormat m_swapChainImageFormat; // 交换链图像格式

  VkExtent2D m_swapChainExtent; // 交换链图像分辨率

  std::vector<VkImageView> m_swapChainImageViews; // 交换链图像视图

  VkPipeline m_graphicsPipeline; // 图形管线

  VkDebugUtilsMessengerEXT m_debugMessenger; // Vulkan调试报告

  std::vector<VkExtensionProperties> m_extensions; // Vulkan支持的扩展列表

  // 指定的验证层列表
  const std::vector<const char *> m_validationLayers = {
      "VK_LAYER_KHRONOS_validation",
  };

  // 指定的设备扩展列表
  const std::vector<const char *> m_deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};

int main()
{
  Log::Init();

  LOG_INFO("Hello, Vulkan!");
  HelloTriangleApplication app;

  try
  {
    app.run();
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}