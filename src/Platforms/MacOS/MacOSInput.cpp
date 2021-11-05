#include <GLFW/glfw3.h>

#include "CoreApp.hpp"
#include "MacOSInput.hpp"

namespace Texturia {

Input *Input::s_Instance = new MacOSInput();

bool MacOSInput::IsKeyDownImpl(int keycode) {
  auto window = static_cast<GLFWwindow *>(
      CoreApp::GetCoreApp().GetWindow().GetNativeWindow());
  auto state = glfwGetKey(window, keycode);
  return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool MacOSInput::IsMouseButtonDownImpl(int button) {
  auto window = static_cast<GLFWwindow *>(
      CoreApp::GetCoreApp().GetWindow().GetNativeWindow());
  auto state = glfwGetMouseButton(window, button);
  return state == GLFW_PRESS;
}

std::pair<float, float> MacOSInput::GetMousePosImpl() {
  auto window = static_cast<GLFWwindow *>(
      CoreApp::GetCoreApp().GetWindow().GetNativeWindow());
  double xPos, yPos;
  glfwGetCursorPos(window, &xPos, &yPos);
  return {(float)xPos, (float)yPos};
}

float MacOSInput::GetMouseXImpl() {
  auto [x, y] = GetMousePosImpl();
  return x;
}

float MacOSInput::GetMouseYImpl() {
  auto [x, y] = GetMousePosImpl();
  return y;
}

} // namespace Texturia