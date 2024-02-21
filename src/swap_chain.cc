#include "swap_chain.hh"

#include <iostream>

namespace render {

SwapChain::SwapChain(std::shared_ptr<const render::Display> pDisplay,
                     std::shared_ptr<const render::Device> pDevice)
    : _pDisplay(pDisplay), _pDevice(pDevice) {
    _extent.width = std::clamp(_pDisplay->width(),
                               _pDevice->swapChainSupport().capabilities.minImageExtent.width,
                               _pDevice->swapChainSupport().capabilities.maxImageExtent.width);
    _extent.height = std::clamp(_pDisplay->height(),
                                _pDevice->swapChainSupport().capabilities.minImageExtent.height,
                                _pDevice->swapChainSupport().capabilities.maxImageExtent.height);
    _imageCount = _pDevice->swapChainSupport().capabilities.minImageCount + 1;
    createSwapChain();
    createImageViews();
}

void SwapChain::createSwapChain() {
    try {
        vk::SwapchainCreateInfoKHR swapChainCreateInfo;
        swapChainCreateInfo.surface = *_pDisplay->surface();
        swapChainCreateInfo.minImageCount = _imageCount;
        swapChainCreateInfo.imageFormat = _surfaceFormat.format;
        swapChainCreateInfo.imageColorSpace = _surfaceFormat.colorSpace;
        swapChainCreateInfo.imageExtent = _extent;
        swapChainCreateInfo.imageArrayLayers = 1;
        swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
        swapChainCreateInfo.imageSharingMode =
            vk::SharingMode::eExclusive; // graphics and present queue families
                                         // are the same
        swapChainCreateInfo.preTransform =
            _pDevice->swapChainSupport().capabilities.currentTransform;
        swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        swapChainCreateInfo.presentMode = _presentMode;
        swapChainCreateInfo.clipped = VK_TRUE;
        swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
        _swapChain = _pDevice->device().createSwapchainKHR(swapChainCreateInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating swapchain : " << e.what() << '\n';
        exit(-1);
    }
}

void SwapChain::createImageViews() {
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
            _imageViews.push_back(_pDevice->device().createImageView(imageViewCreateInfo));
        }
    } catch (std::exception& e) {
        std::cerr << "Error while creating swapchain image views : " << e.what() << '\n';
        exit(-1);
    }
}
} // namespace render