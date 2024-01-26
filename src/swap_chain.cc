#include "swap_chain.hh"

#include <iostream>

namespace render {

SwapChain::SwapChain(const render::Display& display, const render::Device& device) {
    _extent.width =
        std::clamp(display.width(), device.swapChainSupport().capabilities.minImageExtent.width,
                   device.swapChainSupport().capabilities.maxImageExtent.width);
    _extent.height =
        std::clamp(display.height(), device.swapChainSupport().capabilities.minImageExtent.height,
                   device.swapChainSupport().capabilities.maxImageExtent.height);
    _imageCount = device.swapChainSupport().capabilities.minImageCount + 1;
    createSwapChain(device.device(), display.surface(), device.swapChainSupport());
    createImageViews(device.device());
}

void SwapChain::createSwapChain(const vk::raii::Device& device, const vk::raii::SurfaceKHR& surface,
                                const SwapChainSupport deviceSwapChainSupport) {
    try {
        vk::SwapchainCreateInfoKHR swapChainCreateInfo;
        swapChainCreateInfo.surface = *surface;
        swapChainCreateInfo.minImageCount = _imageCount;
        swapChainCreateInfo.imageFormat = _surfaceFormat.format;
        swapChainCreateInfo.imageColorSpace = _surfaceFormat.colorSpace;
        swapChainCreateInfo.imageExtent = _extent;
        swapChainCreateInfo.imageArrayLayers = 1;
        swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
        swapChainCreateInfo.imageSharingMode =
            vk::SharingMode::eExclusive; // graphics and present queue families
                                         // are the same
        swapChainCreateInfo.preTransform = deviceSwapChainSupport.capabilities.currentTransform;
        swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        swapChainCreateInfo.presentMode = _presentMode;
        swapChainCreateInfo.clipped = VK_TRUE;
        swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
        _swapChain = device.createSwapchainKHR(swapChainCreateInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating swapchain : " << e.what() << '\n';
        exit(-1);
    }
}

void SwapChain::createImageViews(const vk::raii::Device& device) {
    try {
        auto swapChainImages = _swapChain.getImages();
        for (auto& image : swapChainImages) {
            vk::ImageViewCreateInfo imageViewCreateInfo;
            imageViewCreateInfo.image = image;
            imageViewCreateInfo.format = _surfaceFormat.format;
            imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
            imageViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
            imageViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
            imageViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
            imageViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
            imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            imageViewCreateInfo.subresourceRange.layerCount = 1;
            _imageViews.push_back(device.createImageView(imageViewCreateInfo));
        }
    } catch (std::exception& e) {
        std::cerr << "Error while creating swapchain image views : " << e.what() << '\n';
        exit(-1);
    }
}
} // namespace render