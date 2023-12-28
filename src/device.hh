#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vector>

#include "instance.hh"

namespace render {
class Device {
public:
    struct SwapChainSupport {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };
    struct QueueFamily {
        uint32_t index;
        vk::QueueFamilyProperties properties;
    };

private:
    vk::raii::PhysicalDevice _physicalDevice = 0;
    vk::raii::Device _device = 0;
    vk::raii::Queue _graphicsQueue = 0;
    QueueFamily _graphicsQueueFamily;
    SwapChainSupport _swapChainSupport;
    vk::raii::CommandPool _graphicsCommandPool = 0;
    vk::raii::CommandBuffer _graphicsCommandBuffer = 0;

    void selectPhysicalDevice(const vk::raii::Instance& instance,
                              const vk::raii::SurfaceKHR& surface);
    void selectGraphicsQueueFamily(const vk::raii::Instance& instance,
                                   const vk::raii::SurfaceKHR& surface);
    void createDevice(const vk::raii::Instance& instance);
    void createGraphicsQueue();
    void createGraphicsCommandPool();
    void createGraphicsCommandBuffer();

public:
    Device(const render::Instance& instance, const vk::raii::SurfaceKHR& surface);
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
    const vk::raii::Device& device() const {
        return _device;
    }
    const vk::raii::Queue& graphicsQueue() const {
        return _graphicsQueue;
    }
    const QueueFamily& graphicsQueueFamily() const {
        return _graphicsQueueFamily;
    }
    const SwapChainSupport& swapChainSupport() const {
        return _swapChainSupport;
    }
    const vk::raii::CommandBuffer& graphicsCommandBuffer() const {
        return _graphicsCommandBuffer;
    }
};

} // namespace render
