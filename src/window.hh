#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <cstdlib>
#include <stdexcept>

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

class Window {
public:
    Window(const std::string& name, uint width, uint height);

    ~Window() {
        glfwDestroyWindow(_pHandle);
    }

    GLFWwindow* handle() {
        return _pHandle;
    }

    uint width() const {
        return _width;
    }

    uint height() const {
        return _height;
    }

private:
    GLFWContext _glfwContext;
    GLFWwindow* _pHandle;
    uint _width;
    uint _height;
};