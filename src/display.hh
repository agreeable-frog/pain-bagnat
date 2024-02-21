#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>
#include <string>
#include <memory>

#include "instance.hh"

namespace render {

class Display {
private:
    std::shared_ptr<const render::Instance> _pInstance;
    uint _width;
    uint _height;
    std::string _name;
    GLFWwindow* _pWindow;
    vk::raii::SurfaceKHR _surface = 0;

    void createWindow();
    void createSurface();

public:
    Display(std::shared_ptr<const render::Instance> instance, const std::string& name, uint width, uint height);
    ~Display() {
        glfwDestroyWindow(_pWindow);
    }

    GLFWwindow* pWindow() {
        return _pWindow;
    }

    uint width() const {
        return _width;
    }

    uint height() const {
        return _height;
    }
    const vk::raii::SurfaceKHR& surface() const {
        return _surface;
    }
};
} // namespace render