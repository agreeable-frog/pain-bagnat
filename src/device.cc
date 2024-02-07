#include "device.hh"

#include <iostream>
#include <set>

#include "global_defs.hh"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace render {

void Device::selectPhysicalDevice(const vk::raii::Instance& instance,
                                  const vk::raii::SurfaceKHR& surface) {
    try {
        vk::raii::PhysicalDevices physicalDevices(instance);
        if (physicalDevices.size() == 0) {
            throw std::runtime_error("No physical device found");
        }
        int selectedDeviceScore = 0;
        for (const auto& physicalDevice : physicalDevices) {
            auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
            std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                                     deviceExtensions.end());
            for (const auto& extension : availableExtensions) {
                requiredExtensions.erase(extension.extensionName);
            }
            if (!requiredExtensions.empty()) continue;

            SwapChainSupport deviceSwapChainSupport;
            deviceSwapChainSupport.capabilities =
                physicalDevice.getSurfaceCapabilitiesKHR(*surface);
            deviceSwapChainSupport.formats = physicalDevice.getSurfaceFormatsKHR(*surface);
            deviceSwapChainSupport.presentModes =
                physicalDevice.getSurfacePresentModesKHR(*surface);
            if (deviceSwapChainSupport.formats.empty() ||
                deviceSwapChainSupport.presentModes.empty())
                continue;

            auto props = physicalDevice.getProperties();
            auto features = physicalDevice.getFeatures();
            int score = 0;
            if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) score += 100;
            if (props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) score += 50;
            if (score >= selectedDeviceScore) {
                _physicalDevice = physicalDevice;
                _swapChainSupport = deviceSwapChainSupport;
                selectedDeviceScore = score;
            }
        }
        if (selectedDeviceScore == 0) {
            throw std::runtime_error("No physical device could be selected");
        }
        std::cout << _physicalDevice.getProperties().deviceName << " selected\n";
    } catch (std::exception& e) {
        std::cerr << "Error while selecting physical device : " << e.what() << '\n';
        exit(-1);
    }
}

void Device::listPhysicalDeviceQueueFamilies(const vk::raii::SurfaceKHR& surface) const {
    auto queueFamilyProps = _physicalDevice.getQueueFamilyProperties();
    std::cout << "Available queue families\n";
    for (size_t i = 0; i < queueFamilyProps.size(); i++) {
        const vk::QueueFamilyProperties& queueFamilyProp = queueFamilyProps.at(i);
        std::cout << i << ":";
        std::cout << " count " << queueFamilyProp.queueCount;
        std::cout << " graphics "
                  << (queueFamilyProp.queueFlags & vk::QueueFlagBits::eGraphics ? "yes" : "no");
        std::cout << " transfer "
                  << (queueFamilyProp.queueFlags & vk::QueueFlagBits::eTransfer ? "yes" : "no");
        std::cout << " presentation "
                  << (_physicalDevice.getSurfaceSupportKHR(i, *surface) ? "yes" : "no");
        std::cout << '\n';
    }
}

void Device::selectGraphicsQueueFamily(const vk::raii::SurfaceKHR& surface) {
    try {
        int selectedGraphicsScore = 0;
        auto queueFamilyProps = _physicalDevice.getQueueFamilyProperties();
        for (size_t i = 0; i < queueFamilyProps.size(); i++) {
            const vk::QueueFamilyProperties& queueFamilyProp = queueFamilyProps.at(i);
            int graphicsScore = 0;
            if (queueFamilyProp.queueFlags & vk::QueueFlagBits::eGraphics) {
                graphicsScore += 50; // graphics able queue
            }
            if (!(queueFamilyProp.queueFlags & vk::QueueFlagBits::eTransfer)) {
                graphicsScore += 50;
            }
            if (!_physicalDevice.getSurfaceSupportKHR(i, *surface)) {
                graphicsScore = -1; // presentation support
            }
            graphicsScore *= queueFamilyProp.queueCount;
            if (graphicsScore >= selectedGraphicsScore) {
                _graphicsQueueFamily.index = i;
                _graphicsQueueFamily.properties = queueFamilyProp;
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
}

void Device::selectTransferQueueFamily() {
    try {
        auto queueFamilyProps = _physicalDevice.getQueueFamilyProperties();
        for (size_t i = 0; i < queueFamilyProps.size(); i++) {
            const vk::QueueFamilyProperties& queueFamilyProp = queueFamilyProps.at(i);
            if ((queueFamilyProp.queueFlags & vk::QueueFlagBits::eTransfer) &&
                !(queueFamilyProp.queueFlags & vk::QueueFlagBits::eGraphics)) {
                _transferQueueFamily.index = i;
                _transferQueueFamily.properties = queueFamilyProp;
                return;
            }
        }
        std::cout << "Transfer specialised family queue was not found, defaulting to the graphics "
                     "family queue\n";
        _transferQueueFamily.index = _graphicsQueueFamily.index;
        _transferQueueFamily.properties = _graphicsQueueFamily.properties;
    } catch (std::exception& e) {
        std::cerr << "Error while selecting queue families : " << e.what() << '\n';
        exit(-1);
    }
}

void Device::createDevice(const vk::raii::Instance& instance) {
    try {
        std::vector<vk::DeviceQueueCreateInfo> queuesCreateInfo;

        vk::DeviceQueueCreateInfo graphicsQueueInfo;
        graphicsQueueInfo.queueFamilyIndex = _graphicsQueueFamily.index;
        std::vector<float> graphicsQueuePriority = {1.0f};
        graphicsQueueInfo.setQueuePriorities(graphicsQueuePriority);
        queuesCreateInfo.push_back(graphicsQueueInfo);

        if (_transferQueueFamily.index != _graphicsQueueFamily.index) {
            vk::DeviceQueueCreateInfo transferQueueInfo;
            transferQueueInfo.queueFamilyIndex = _transferQueueFamily.index;
            std::vector<float> transferQueuePriority = {1.0f};
            transferQueueInfo.setQueuePriorities(transferQueuePriority);
            queuesCreateInfo.push_back(transferQueueInfo);
        }

        vk::PhysicalDeviceFeatures physicalDeviceFeatures;

        vk::DeviceCreateInfo deviceCreateInfo;
        deviceCreateInfo.setQueueCreateInfos(queuesCreateInfo);
        deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
        deviceCreateInfo.setPEnabledExtensionNames(deviceExtensions);
#ifndef NDEBUG
        deviceCreateInfo.setPEnabledLayerNames(validationLayers);
#endif
        _device = _physicalDevice.createDevice(deviceCreateInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating device : " << e.what() << '\n';
        exit(-1);
    }
}

void Device::createAllocator(const vk::raii::Instance& instance) {
    VmaAllocatorCreateInfo allocatorCreateInfo{}; // Don't forget to zero init!!!
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    allocatorCreateInfo.instance = *instance;
    allocatorCreateInfo.physicalDevice = *_physicalDevice;
    allocatorCreateInfo.device = *_device;
    auto result = vmaCreateAllocator(&allocatorCreateInfo, &_allocator);
    if (result != VkResult::VK_SUCCESS) {
        std::cout << "vmaCreateAllocator failed : " << result << "\n";
    };
}

void Device::createGraphicsQueue() {
    try {
        _graphicsQueue = _device.getQueue(_graphicsQueueFamily.index, 0);
    } catch (std::exception& e) {
        std::cerr << "Error while retrieving queue : " << e.what() << '\n';
        exit(-1);
    }
}

void Device::createTransferQueue() {
    try {
        _transferQueue = _device.getQueue(_transferQueueFamily.index, 0);
    } catch (std::exception& e) {
        std::cerr << "Error while retrieving queue : " << e.what() << '\n';
        exit(-1);
    }
}

void Device::createGraphicsCommandPool() {
    try {
        vk::CommandPoolCreateInfo commandPoolInfo;
        commandPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        commandPoolInfo.queueFamilyIndex = _graphicsQueueFamily.index;
        _graphicsCommandPool = _device.createCommandPool(commandPoolInfo);

    } catch (std::exception& e) {
        std::cerr << "Error while creating command pool : " << e.what() << '\n';
        exit(-1);
    }
}

void Device::createTransferCommandPool() {
    try {
        vk::CommandPoolCreateInfo commandPoolInfo;
        commandPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer &
                                vk::CommandPoolCreateFlagBits::eTransient;
        commandPoolInfo.queueFamilyIndex = _transferQueueFamily.index;
        _transferCommandPool = _device.createCommandPool(commandPoolInfo);

    } catch (std::exception& e) {
        std::cerr << "Error while creating command pool : " << e.what() << '\n';
        exit(-1);
    }
}

void Device::createGraphicsCommandBuffer() {
    try {
        vk::CommandBufferAllocateInfo commandBufferAllocInfo;
        commandBufferAllocInfo.commandPool = *_graphicsCommandPool;
        commandBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
        commandBufferAllocInfo.commandBufferCount = 1;
        _graphicsCommandBuffer =
            std::move(_device.allocateCommandBuffers(commandBufferAllocInfo).at(0));
    } catch (std::exception& e) {
        std::cerr << "Error while creating command buffer : " << e.what() << '\n';
        exit(-1);
    }
}

Device::Device(const render::Instance& instance, const vk::raii::SurfaceKHR& surface) {
    selectPhysicalDevice(instance, surface);
    listPhysicalDeviceQueueFamilies(surface);
    selectGraphicsQueueFamily(surface);
    selectTransferQueueFamily();
    createDevice(instance);
    createAllocator(instance);
    createGraphicsQueue();
    createTransferQueue();
    createGraphicsCommandPool();
    createTransferCommandPool();
    createGraphicsCommandBuffer();
}

Device::~Device() {
    vmaDestroyAllocator(_allocator);
}

uint32_t Device::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const {
    vk::PhysicalDeviceMemoryProperties memProperties = _physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

} // namespace render