#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>
#include <string>
#include <memory>
#include <iostream>
#include <ostream>
#include <sstream>
#include <set>

#include "shader_compiler.hh"
#include "window.hh"
#include "build_defs.hh"
#include "utils.hh"

const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

int main(void) {
    auto pWindow1 = std::make_shared<Window>("window", 800, 450);

    vk::raii::Context context;
    vk::raii::Instance instance = makeInstance(context);
#ifndef NDEBUG
    vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger = makeDebugMessenger(instance);
#endif
    vk::raii::SurfaceKHR surface = makeSurface(instance, pWindow1);

    vk::raii::PhysicalDevice physicalDevice = selectPhysicalDevice(instance, surface);
    const SwapChainSupportDetails deviceSwapChainSupport =
        getPhysicalDeviceSwapChainSupportDetails(physicalDevice, surface);
    size_t graphicsQueueFamilyIndex = selectGraphicsQueueFamilyIndex(physicalDevice, surface);
    vk::raii::Device device = makeDevice(physicalDevice, graphicsQueueFamilyIndex);
    vk::raii::Queue graphicsQueue = getQueue(device, graphicsQueueFamilyIndex, 0);

    vk::SurfaceFormatKHR surfaceFormat = {vk::Format::eB8G8R8A8Srgb,
                                          vk::ColorSpaceKHR::eSrgbNonlinear};
    vk::Extent2D extent = {pWindow1->width(), pWindow1->height()};
    extent.width =
        std::clamp(extent.width, deviceSwapChainSupport.capabilities.minImageExtent.width,
                   deviceSwapChainSupport.capabilities.maxImageExtent.width);
    extent.height =
        std::clamp(extent.height, deviceSwapChainSupport.capabilities.minImageExtent.height,
                   deviceSwapChainSupport.capabilities.maxImageExtent.height);

    vk::raii::SwapchainKHR swapchain =
        makeSwapchain(device, surface, deviceSwapChainSupport, pWindow1);
    std::vector<vk::raii::ImageView> imageViews = makeImageViews(device, swapchain, surfaceFormat);

    vk::raii::ShaderModule vertShaderModule = makeShaderModule(
        device, std::string(PROJECT_SOURCE_DIR) + "/shaders/basic.vert", SHADER_TYPE::VERT);
    vk::raii::ShaderModule fragShaderModule = makeShaderModule(
        device, std::string(PROJECT_SOURCE_DIR) + "/shaders/basic.frag", SHADER_TYPE::FRAG);

    vk::raii::PipelineLayout pipelineLayout = makePipelineLayout(device);

    vk::raii::RenderPass renderPass = makeRenderPass(device, surfaceFormat.format);

    vk::raii::Pipeline graphicsPipeline = makeGraphicsPipeline(device, pipelineLayout, renderPass,
                                                               vertShaderModule, fragShaderModule);

    std::vector<vk::raii::Framebuffer> framebuffers = makeFramebuffers(device, imageViews, renderPass, extent);

    vk::raii::CommandPool commandPool = makeCommandPool(device, graphicsQueueFamilyIndex);

    vk::raii::CommandBuffer commandBuffer = makeCommandBuffer(device, commandPool);

    vk::raii::Semaphore imageAvailableSemaphore = 0;
    vk::raii::Semaphore renderFinishedSemaphore = 0;
    vk::raii::Fence inFlightFence = 0;

    try {
        vk::SemaphoreCreateInfo semaphoreInfo;
        imageAvailableSemaphore = device.createSemaphore(semaphoreInfo);
        renderFinishedSemaphore = device.createSemaphore(semaphoreInfo);

        vk::FenceCreateInfo fenceInfo;
        fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
        inFlightFence = device.createFence(fenceInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating sync elements : " << e.what() << '\n';
        exit(-1);
    }

    // Main loop
    while (!glfwWindowShouldClose(pWindow1->handle())) {
        glfwPollEvents();
        auto result = device.waitForFences(*inFlightFence, true, UINT_FAST64_MAX);
        device.resetFences(*inFlightFence);
        uint32_t imageIndex =
            swapchain.acquireNextImage(UINT_FAST64_MAX, *imageAvailableSemaphore, nullptr).second;
        commandBuffer.reset();

        vk::CommandBufferBeginInfo beginInfo;
        commandBuffer.begin(beginInfo);
        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = *renderPass;
        renderPassInfo.framebuffer = *framebuffers[imageIndex];
        renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
        renderPassInfo.renderArea.extent = extent;
        vk::ClearValue clearValue = vk::ClearValue{{0.0f, 0.0f, 0.0f, 1.0f}};
        renderPassInfo.setClearValues(clearValue);
        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

        vk::Viewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        commandBuffer.setViewport(0, viewport);

        vk::Rect2D scissor;
        scissor.offset = vk::Offset2D{0, 0};
        scissor.extent = extent;
        commandBuffer.setScissor(0, scissor);

        commandBuffer.draw(3, 1, 0, 0);
        commandBuffer.endRenderPass();
        commandBuffer.end();

        vk::SubmitInfo submitInfo;
        submitInfo.setWaitSemaphores(*imageAvailableSemaphore);
        vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        submitInfo.setWaitDstStageMask(waitStages);
        submitInfo.setCommandBuffers(*commandBuffer);
        submitInfo.setSignalSemaphores(*renderFinishedSemaphore);
        graphicsQueue.submit(submitInfo, *inFlightFence);

        vk::PresentInfoKHR presentInfo;
        presentInfo.setWaitSemaphores(*renderFinishedSemaphore);
        presentInfo.setSwapchains(*swapchain);
        presentInfo.setImageIndices(imageIndex);
        graphicsQueue.presentKHR(presentInfo);
    }
    device.waitIdle();
}