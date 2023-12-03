#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "window.hh"
#include "shader_compiler.hh"

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

vk::raii::Instance createInstance(const vk::raii::Context& context);
vk::raii::DebugUtilsMessengerEXT createDebugMessenger(const vk::raii::Instance& instance);
vk::raii::SurfaceKHR createSurface(const vk::raii::Instance& instance,
                                   std::shared_ptr<Window> pWindow);
vk::raii::PhysicalDevice selectPhysicalDevice(const vk::raii::Instance& instance,
                                              const vk::raii::SurfaceKHR& surface);
const SwapChainSupportDetails getPhysicalDeviceSwapChainSupportDetails(
    const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::SurfaceKHR& surface);
uint32_t selectGraphicsQueueFamilyIndex(const vk::raii::PhysicalDevice& physicalDevice,
                                        const vk::raii::SurfaceKHR& surface);
vk::raii::Device createDevice(const vk::raii::PhysicalDevice& physicalDevice,
                              uint32_t graphicsQueueFamilyIndex);
vk::raii::Queue getQueue(const vk::raii::Device& device, uint32_t queueFamilyIndex,
                         uint32_t queueIndex);
vk::raii::SwapchainKHR createSwapchain(const vk::raii::Device& device,
                                       const vk::raii::SurfaceKHR& surface,
                                       const SwapChainSupportDetails deviceSwapChainSupport,
                                       std::shared_ptr<Window> pWindow);
std::vector<vk::raii::ImageView> createImageViews(const vk::raii::Device& device,
                                                  const vk::raii::SwapchainKHR& swapchain,
                                                  vk::SurfaceFormatKHR surfaceFormat);
vk::raii::ShaderModule createShaderModule(const vk::raii::Device& device, const std::string& path,
                                          SHADER_TYPE type);