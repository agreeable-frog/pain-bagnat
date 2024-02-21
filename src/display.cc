#include "display.hh"

#include <iostream>
#include <stdexcept>

namespace render {

Display::Display(std::shared_ptr<const render::Instance> pInstance, const std::string& name, uint width,
                 uint height) : _pInstance(pInstance), _name(name), _width(width), _height(height) {
    createWindow();
    createSurface();
}

void Display::createWindow() {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    _pWindow = glfwCreateWindow((int)_width, (int)_height, _name.data(), 0, 0);
    if (!_pWindow) throw std::runtime_error("Failed to create window");
    glfwSetWindowUserPointer(_pWindow, this);
}

void Display::createSurface() {
    try {
        VkSurfaceKHR surfaceTMP;
        auto result = glfwCreateWindowSurface(*_pInstance->instance(), _pWindow, nullptr, &surfaceTMP);
        if (result != VkResult::VK_SUCCESS) {
            std::ostringstream os;
            os << "glfwCreateWindowSurface failed : " << int(result);
            throw std::runtime_error(os.str());
        }
        _surface = vk::raii::SurfaceKHR(*_pInstance, surfaceTMP);
    } catch (std::exception& e) {
        std::cerr << "Error while creating surface : " << e.what() << '\n';
        exit(-1);
    }
}
} // namespace render