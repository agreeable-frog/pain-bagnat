#include "window.hh"

uint GLFWContext::_instancingCount = 0;

Window::Window(const std::string& name, uint width, uint height) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    _pHandle = glfwCreateWindow((int)width, (int)height, name.data(), 0, 0);
    if (!_pHandle) throw std::runtime_error("Failed to create window");
    _width = width;
    _height = height;
    glfwSetWindowUserPointer(_pHandle, this);
}