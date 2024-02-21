#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vector>
#include <memory>

#include "instance.hh"
#include "vk_mem_alloc.h"

const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

namespace render {
struct SwapChainSupport {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

struct QueueFamily {
    uint32_t index;
    vk::QueueFamilyProperties properties;
};

class Device {
public:
private:
    std::shared_ptr<const render::Instance> _pInstance;
    vk::raii::PhysicalDevice _physicalDevice = 0;
    vk::raii::Device _device = 0;
    VmaAllocator _allocator;
    QueueFamily _graphicsQueueFamily;
    QueueFamily _transferQueueFamily;
    vk::raii::Queue _graphicsQueue = 0;
    vk::raii::Queue _transferQueue = 0;
    SwapChainSupport _swapChainSupport;
    vk::raii::CommandPool _graphicsCommandPool = 0;
    vk::raii::CommandPool _transferCommandPool = 0;
    vk::raii::CommandBuffer _graphicsCommandBuffer = 0;

    void selectPhysicalDevice(const vk::raii::SurfaceKHR& surface);
    void listPhysicalDeviceQueueFamilies(const vk::raii::SurfaceKHR& surface) const;
    void selectGraphicsQueueFamily(const vk::raii::SurfaceKHR& surface);
    void selectTransferQueueFamily();
    void createDevice();
    void createAllocator();
    void createGraphicsQueue();
    void createTransferQueue();
    void createGraphicsCommandPool();
    void createTransferCommandPool();
    void createGraphicsCommandBuffer();

public:
    Device(std::shared_ptr<const render::Instance> pInstance, const vk::raii::SurfaceKHR& surface);
    ~Device();
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
    const vk::raii::PhysicalDevice& physicalDevice() const {
        return _physicalDevice;
    }
    const vk::raii::Device& device() const {
        return _device;
    }
    const VmaAllocator& allocator() const {
        return _allocator;
    }
    const vk::raii::Queue& graphicsQueue() const {
        return _graphicsQueue;
    }
    const vk::raii::Queue& transferQueue() const {
        return _transferQueue;
    }
    const QueueFamily& graphicsQueueFamily() const {
        return _graphicsQueueFamily;
    }
    const QueueFamily& transferQueueFamily() const {
        return _transferQueueFamily;
    }
    const SwapChainSupport& swapChainSupport() const {
        return _swapChainSupport;
    }
    const vk::raii::CommandPool& transferCommandPool() const {
        return _transferCommandPool;
    }
    const vk::raii::CommandBuffer& graphicsCommandBuffer() const {
        return _graphicsCommandBuffer;
    }
};

} // namespace render
