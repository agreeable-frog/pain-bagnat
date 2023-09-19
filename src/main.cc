#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>
#include <string>
#include <memory>
#include <iostream>
#include <ostream>
#include <sstream>

#include "window.hh"

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"}; // we should check that it is supported

VKAPI_ATTR VkBool32 VKAPI_CALL
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

vk::DebugUtilsMessengerCreateInfoEXT getDebugUtilsMessengerCreateInfoEXT() {
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

int main(void) {
    auto pWindow1 = std::make_shared<Window>("window", 800, 450);

    vk::raii::Context context;

    // Instance creation, validation layers enabling and instance debug
    // messenger creation
    vk::raii::Instance instance(nullptr);
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
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        instanceCreateInfo.enabledExtensionCount = extensions.size();
        instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

#ifndef NDEBUG
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT =
            getDebugUtilsMessengerCreateInfoEXT();
        instanceCreateInfo.pNext = &debugUtilsMessengerCreateInfoEXT;
#endif
        instance = context.createInstance(instanceCreateInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating instance : " << e.what() << '\n';
        exit(-1);
    }
#ifndef NDEBUG
    // Debug messenger creation
    vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger(nullptr);
    try {
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT =
            getDebugUtilsMessengerCreateInfoEXT();
        debugUtilsMessenger =
            instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    } catch (std::exception& e) {
        std::cerr << "Error while creating debug messenger : " << e.what() << '\n';
        exit(-1);
    }
#endif

    vk::raii::SurfaceKHR surface(nullptr);
    try {
        VkSurfaceKHR surfaceTMP;
        auto result = glfwCreateWindowSurface(*instance, pWindow1->handle(), nullptr, &surfaceTMP);
        if (result != VkResult::VK_SUCCESS) {
            std::ostringstream os;
            os << "glfwCreateWindowSurface failed : " << int(result);
            throw std::runtime_error(os.str());
        }
        surface = vk::raii::SurfaceKHR(instance, surfaceTMP);
    } catch (std::exception& e) {
        std::cerr << "Error while creating surface : " << e.what() << '\n';
        exit(-1);
    }

    // Physical device query
    vk::raii::PhysicalDevice physicalDevice(nullptr);
    try {
        vk::raii::PhysicalDevices physicalDevices(instance);
        if (physicalDevices.size() == 0) {
            throw std::runtime_error("No physical device found");
        }
        int selectedDeviceScore = 0;
        physicalDevice = physicalDevices[0];
        for (const auto& aPhysicalDevice : physicalDevices) {
            auto props = aPhysicalDevice.getProperties();
            auto features = aPhysicalDevice.getFeatures();
            int score = 0;
            if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) score += 100;
            if (props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) score += 50;
            if (score >= selectedDeviceScore) {
                physicalDevice = aPhysicalDevice;
                selectedDeviceScore = score;
            }
        }
        auto props = physicalDevice.getProperties();
        auto features = physicalDevice.getFeatures();
        std::cout << props.deviceName << " selected\n";
    } catch (std::exception& e) {
        std::cerr << "Error while selecting physical device : " << e.what() << '\n';
        exit(-1);
    }

    size_t graphicsQueueFamilyIndex;
    try {
        int selectedGraphicsScore = 0;
        auto queueFamilyProps = physicalDevice.getQueueFamilyProperties();
        for (size_t i = 0; i < queueFamilyProps.size(); i++) {
            const vk::QueueFamilyProperties& queueFamilyProp = queueFamilyProps.at(i);
            int graphicsScore = 0;
            if (queueFamilyProp.queueFlags & vk::QueueFlagBits::eGraphics) {
                graphicsScore += 50; // graphics able queue
            }
            if (!(queueFamilyProp.queueFlags & vk::QueueFlagBits::eTransfer)) {
                graphicsScore += 50;
            }
            if (!physicalDevice.getSurfaceSupportKHR(i, *surface)) {
                graphicsScore = -1; // presentation support
            }
            graphicsScore *= queueFamilyProp.queueCount;
            if (graphicsScore >= selectedGraphicsScore) {
                graphicsQueueFamilyIndex = i;
                selectedGraphicsScore = graphicsScore;
            }
        }
        if (selectedGraphicsScore == 0) {
            throw std::runtime_error("No graphics able queue was found");
        }
    } catch (std::exception& e) {
        std::cerr << "Error while selecting queue families : " << e.what() << '\n';
        exit(-1);
    }

    vk::raii::Device device(nullptr);
    try {
        std::vector<vk::DeviceQueueCreateInfo> queuesCreateInfo;

        vk::DeviceQueueCreateInfo graphicsQueueInfo;
        graphicsQueueInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
        graphicsQueueInfo.queueCount = 1;
        std::vector<float> graphicsQueuePriority = {1.0f};
        graphicsQueueInfo.pQueuePriorities = graphicsQueuePriority.data();
        queuesCreateInfo.push_back(graphicsQueueInfo);

        vk::PhysicalDeviceFeatures physicalDeviceFeatures;

        vk::DeviceCreateInfo deviceCreateInfo;
        deviceCreateInfo.pQueueCreateInfos = queuesCreateInfo.data();
        deviceCreateInfo.queueCreateInfoCount = queuesCreateInfo.size();
        deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
        deviceCreateInfo.enabledExtensionCount = 0;
        deviceCreateInfo.enabledLayerCount = 0;
#ifndef NDEBUG
        deviceCreateInfo.enabledLayerCount = (uint32_t)validationLayers.size();
        deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
#endif
        device = physicalDevice.createDevice(deviceCreateInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating device : " << e.what() << '\n';
        exit(-1);
    }

    vk::raii::Queue graphicsQueue = vk::raii::Queue(nullptr);
    try {
        graphicsQueue = device.getQueue(graphicsQueueFamilyIndex, 0);
    } catch (std::exception& e) {
        std::cerr << "Error while retrieving queues : " << e.what() << '\n';
        exit(-1);
    }

    // Main loop
    while (!glfwWindowShouldClose(pWindow1->handle())) {
        glfwPollEvents();
    }
}