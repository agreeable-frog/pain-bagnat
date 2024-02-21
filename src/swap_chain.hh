#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vector>
#include <memory>

#include "device.hh"
#include "display.hh"

namespace render {
class SwapChain {
private:
    std::shared_ptr<const render::Display> _pDisplay;
    std::shared_ptr<const render::Device> _pDevice;
    uint32_t _imageCount;
    vk::SurfaceFormatKHR _surfaceFormat = {vk::Format::eB8G8R8A8Srgb,
                                           vk::ColorSpaceKHR::eSrgbNonlinear};
    vk::PresentModeKHR _presentMode = vk::PresentModeKHR::eMailbox;
    vk::Extent2D _extent;
    vk::raii::SwapchainKHR _swapChain = 0;
    std::vector<vk::raii::ImageView> _imageViews;

    void createSwapChain();
    void createImageViews();

public:
    SwapChain(std::shared_ptr<const render::Display> pDisplay,
              std::shared_ptr<const render::Device> pDevice);
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