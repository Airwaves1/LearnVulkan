#include "VkGlfwWindow.hpp"
#include "GLFW/glfw3.h"
#include <stdexcept>

VkGlfwWindow::VkGlfwWindow(uint32_t width, uint32_t height, const char *title)
    : m_width(width), m_height(height), m_window(nullptr) {

  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // 禁用OpenGL兼容性

  m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!m_window) {
    throw std::runtime_error("Failed to create GLFW window");
  }
}

VkGlfwWindow::~VkGlfwWindow() {
  if (m_window) {
    glfwDestroyWindow(m_window);
  }
  glfwTerminate();
}

bool VkGlfwWindow::ShouldClose() const {
  return glfwWindowShouldClose(m_window);
}

void VkGlfwWindow::PollEvents() const { glfwPollEvents(); }

void VkGlfwWindow::SwapBuffers() const { glfwSwapBuffers(m_window); }

void VkGlfwWindow::TitleFPS() {
  static double lastTime = glfwGetTime(); // 记录上次更新时间
  static int frameCount = 0;              // 记录当前秒内渲染的帧数

  double currentTime = glfwGetTime();    // 获取当前时间
  double delta = currentTime - lastTime; // 计算时间间隔
  frameCount++;                          // 增加帧计数

  // 每秒钟更新一次标题
  if (delta >= 1.0) {
    double fps = frameCount / delta; // 计算 FPS
    char buffer[256];
    std::snprintf(buffer, sizeof(buffer), "Vulkan %s [FPS: %.2f]", "Triangle",
                  fps);                   // 格式化标题字符串
    glfwSetWindowTitle(m_window, buffer); // 设置窗口标题
    frameCount = 0;                       // 重置帧计数
    lastTime = currentTime;               // 更新最后更新时间
  }
}
