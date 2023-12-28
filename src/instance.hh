#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

namespace render {
class GLFWContext {
public:
    GLFWContext() {
        if (_instancingCount == 0) {
            glfwInit();
        }
        _instancingCount++;
    }
    ~GLFWContext() {
        _instancingCount--;
        if (_instancingCount == 0) {
            glfwTerminate();
        }
    }

private:
    static uint _instancingCount;
};

class Instance {
private:
    GLFWContext _glfwContext;
    vk::raii::Context _context;
    vk::raii::Instance _instance = 0;
    vk::raii::DebugUtilsMessengerEXT _debugUtilsMessenger = 0;

    void createInstance();
    void createDebugUtilsMessenger();
    vk::DebugUtilsMessengerCreateInfoEXT getDebugUtilsMessengerCreateInfoEXT() const;

public:
    Instance();
    operator const vk::raii::Instance&() const {
        return _instance;
    }
};
} // namespace render