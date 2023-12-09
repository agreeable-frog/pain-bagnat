#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <tuple>

#include "window.hh"
#include "shader_compiler.hh"

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

vk::raii::Instance makeInstance(const vk::raii::Context& context);
vk::raii::DebugUtilsMessengerEXT makeDebugMessenger(const vk::raii::Instance& instance);
vk::raii::SurfaceKHR makeSurface(const vk::raii::Instance& instance,
                                 std::shared_ptr<Window> pWindow);
vk::raii::PhysicalDevice selectPhysicalDevice(const vk::raii::Instance& instance,
                                              const vk::raii::SurfaceKHR& surface);
const SwapChainSupportDetails getPhysicalDeviceSwapChainSupportDetails(
    const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::SurfaceKHR& surface);
uint32_t selectGraphicsQueueFamilyIndex(const vk::raii::PhysicalDevice& physicalDevice,
                                        const vk::raii::SurfaceKHR& surface);
vk::raii::Device makeDevice(const vk::raii::PhysicalDevice& physicalDevice,
                            uint32_t graphicsQueueFamilyIndex);
vk::raii::Queue getQueue(const vk::raii::Device& device, uint32_t queueFamilyIndex,
                         uint32_t queueIndex);
vk::raii::SwapchainKHR makeSwapchain(const vk::raii::Device& device,
                                     const vk::raii::SurfaceKHR& surface,
                                     const SwapChainSupportDetails deviceSwapChainSupport,
                                     std::shared_ptr<Window> pWindow);
std::vector<vk::raii::ImageView> makeImageViews(const vk::raii::Device& device,
                                                const vk::raii::SwapchainKHR& swapchain,
                                                vk::SurfaceFormatKHR surfaceFormat);
vk::raii::ShaderModule makeShaderModule(const vk::raii::Device& device, const std::string& path,
                                        SHADER_TYPE type);
vk::raii::PipelineLayout makePipelineLayout(const vk::raii::Device& device);
vk::raii::RenderPass makeRenderPass(const vk::raii::Device& device, vk::Format format);
vk::raii::Pipeline makeGraphicsPipeline(const vk::raii::Device& device,
                                        const vk::raii::PipelineLayout& pipelineLayout,
                                        const vk::raii::RenderPass& renderPass,
                                        const vk::raii::ShaderModule& vertShaderModule,
                                        const vk::raii::ShaderModule& fragShaderModule);
std::vector<vk::raii::Framebuffer> makeFramebuffers(
    const vk::raii::Device& device, const std::vector<vk::raii::ImageView>& imageViews,
    const vk::raii::RenderPass& renderPass, vk::Extent2D extent);
vk::raii::CommandPool makeCommandPool(const vk::raii::Device& device, uint32_t queueFamilyIndex);
vk::raii::CommandBuffer makeCommandBuffer(const vk::raii::Device& device,
                                          const vk::raii::CommandPool& commandPool);
struct SyncObjects {
    vk::raii::Semaphore imageAvailableSemaphore = 0;
    vk::raii::Semaphore renderFinishedSemaphore = 0;
    vk::raii::Fence inFlightFence = 0;
};
SyncObjects makeSyncObjects(const vk::raii::Device& device);