#include <GLFW/glfw3.h>

class VkGlfwWindow {
public:
  VkGlfwWindow() = delete;
  VkGlfwWindow(const VkGlfwWindow &) = delete;
  VkGlfwWindow &operator=(const VkGlfwWindow &) = delete;

  VkGlfwWindow(uint32_t width, uint32_t height, const char *title);

  ~VkGlfwWindow();

  bool ShouldClose() const;
  void PollEvents() const;
  void SwapBuffers() const;

public:
  void TitleFPS();

  uint32_t GetWidth() { return m_width; }
  uint32_t GetHeight() { return m_height; }

private:
  GLFWwindow *m_window;
  uint32_t m_width;
  uint32_t m_height;
};