#include "display.hh"

#include <iostream>
#include <stdexcept>

namespace render {

Display::Display(const render::Instance& instance, const std::string& name, uint width,
                 uint height) {
    createWindow(name, width, height);
    createSurface(instance);
}

void Display::createWindow(const std::string& name, uint width, uint height) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    _pWindow = glfwCreateWindow((int)width, (int)height, name.data(), 0, 0);
    if (!_pWindow) throw std::runtime_error("Failed to create window");
    _width = width;
    _height = height;
    glfwSetWindowUserPointer(_pWindow, this);
}

void Display::createSurface(const vk::raii::Instance& instance) {
    try {
        VkSurfaceKHR surfaceTMP;
        auto result = glfwCreateWindowSurface(*instance, _pWindow, nullptr, &surfaceTMP);
        if (result != VkResult::VK_SUCCESS) {
            std::ostringstream os;
            os << "glfwCreateWindowSurface failed : " << int(result);
            throw std::runtime_error(os.str());
        }
        _surface = vk::raii::SurfaceKHR(instance, surfaceTMP);
    } catch (std::exception& e) {
        std::cerr << "Error while creating surface : " << e.what() << '\n';
        exit(-1);
    }
}
} // namespace render