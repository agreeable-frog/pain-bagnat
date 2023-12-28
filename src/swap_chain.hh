#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vector>

#include "device.hh"
#include "display.hh"

namespace render {
class SwapChain {
private:
    vk::raii::SwapchainKHR _swapChain = 0;
    uint32_t _imageCount;
    vk::SurfaceFormatKHR _surfaceFormat = {vk::Format::eB8G8R8A8Srgb,
                                           vk::ColorSpaceKHR::eSrgbNonlinear};
    vk::PresentModeKHR _presentMode = vk::PresentModeKHR::eMailbox;
    vk::Extent2D _extent;
    std::vector<vk::raii::ImageView> _imageViews;

    void createSwapChain(const vk::raii::Device& device, const vk::raii::SurfaceKHR& surface,
                         const Device::SwapChainSupport deviceSwapChainSupport);
    void createImageViews(const vk::raii::Device& device);

public:
    SwapChain(const render::Display& display, const render::Device& device);
    vk::SurfaceFormatKHR surfaceFormat() const {
        return _surfaceFormat;
    }
    vk::PresentModeKHR presentMode() const {
        return _presentMode;
    }
    vk::Extent2D extent() const {
        return _extent;
    }
    const vk::raii::SwapchainKHR& swapChain() const {
        return _swapChain;
    }
    uint32_t imageCount() const {
        return _imageCount;
    }
    const std::vector<vk::raii::ImageView>& imageViews() const {
        return _imageViews;
    }
};
} // namespace render