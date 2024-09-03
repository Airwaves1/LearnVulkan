#include "GLFW/glfw3.h"
#include <vector>
#include <vulkan/vulkan.h>

#include <iostream>
// #include <stdexcept>
#include <cstdlib>

#include "utils/log.hpp"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

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
    createInstance();

    setupDebugMessenger();
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
        .apiVersion = VK_API_VERSION_1_0,               // 使用的Vulkan API版本
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

    switch(messageSeverity)
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

  VkInstance m_instance; // Vulkan实例句柄

  VkDebugUtilsMessengerEXT m_debugMessenger; // Vulkan调试报告句柄

  std::vector<VkExtensionProperties> m_extensions; // Vulkan支持的扩展列表

  // 指定的验证层列表
  const std::vector<const char *> m_validationLayers = {
      "VK_LAYER_KHRONOS_validation",
  };
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