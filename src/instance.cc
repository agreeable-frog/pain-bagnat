#include "instance.hh"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>

#include "global_defs.hh"

namespace render {
uint GLFWContext::_instancingCount = 0;

void Instance::createInstance() {
    try {
        vk::ApplicationInfo appInfo;
        appInfo.pApplicationName = "pain-bagnat";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        vk::InstanceCreateInfo instanceCreateInfo;
        instanceCreateInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(
            &glfwExtensionCount); // we should check that they are supported
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#ifndef NDEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // we should check that they are
                                                                 // supported
#endif
        instanceCreateInfo.setPEnabledExtensionNames(extensions);

#ifndef NDEBUG
        // we should check here that the validation layers are supported
        instanceCreateInfo.setPEnabledLayerNames(validationLayers);
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT =
            getDebugUtilsMessengerCreateInfoEXT();
        instanceCreateInfo.pNext = &debugUtilsMessengerCreateInfoEXT;
#endif
        _instance = _context.createInstance(instanceCreateInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating instance : " << e.what() << '\n';
        exit(-1);
    }
}

void Instance::createDebugUtilsMessenger() {
    try {
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT =
            getDebugUtilsMessengerCreateInfoEXT();
        _debugUtilsMessenger =
            _instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    } catch (std::exception& e) {
        std::cerr << "Error while creating debug messenger : " << e.what() << '\n';
        exit(-1);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugMessageFunc(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                 VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                 VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData, void* /*pUserData*/) {
    std::ostringstream message;

    message << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity))
            << " " << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypes))
            << ":\n";
    message << "<" << pCallbackData->pMessage << ">";
    std::cout << message.str() << '\n';
    return false;
}

vk::DebugUtilsMessengerCreateInfoEXT Instance::getDebugUtilsMessengerCreateInfoEXT() const {
    vk::DebugUtilsMessengerCreateInfoEXT instanceDebugUtilsMessengerCreateInfoEXT;
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    instanceDebugUtilsMessengerCreateInfoEXT.messageSeverity = severityFlags;
    instanceDebugUtilsMessengerCreateInfoEXT.messageType = messageTypeFlags;
    instanceDebugUtilsMessengerCreateInfoEXT.pfnUserCallback = &debugMessageFunc;
    return instanceDebugUtilsMessengerCreateInfoEXT;
}

Instance::Instance() {
    createInstance();
    createDebugUtilsMessenger();
}

} // namespace render