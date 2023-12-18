#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <string>
#include <memory>
#include <iostream>

#include "shader_compiler.hh"
#include "window.hh"
#include "build_defs.hh"
#include "utils.hh"
#include "shader_defs.hh"

uint32_t findMemoryType(const vk::raii::PhysicalDevice& physicalDevice, uint32_t typeFilter,
                        vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

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

    std::vector<vk::raii::Framebuffer> framebuffers =
        makeFramebuffers(device, imageViews, renderPass, extent);

    vk::raii::CommandPool commandPool = makeCommandPool(device, graphicsQueueFamilyIndex);

    std::vector<VertexBasic> vertices = {{{-0.5, 0.5, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0}},
                                         {{0.5, 0.5, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0}},
                                         {{0.0, -0.5, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0}}

    };

    vk::raii::Buffer vertexBuffer = 0;
    try {
        vk::BufferCreateInfo bufferInfo;
        bufferInfo.setSize(sizeof(vertices[0]) * vertices.size());
        bufferInfo.setUsage(vk::BufferUsageFlagBits::eVertexBuffer);
        bufferInfo.setSharingMode(vk::SharingMode::eExclusive);
        vertexBuffer = device.createBuffer(bufferInfo);
    } catch (std::exception& e) {
        exit(-1);
    }

    vk::MemoryRequirements memRequirements = vertexBuffer.getMemoryRequirements();

    vk::MemoryAllocateInfo allocateInfo;
    allocateInfo.allocationSize = memRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(
        physicalDevice, memRequirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    vk::raii::DeviceMemory vertexBufferMemory = device.allocateMemory(allocateInfo);
    vertexBuffer.bindMemory(*vertexBufferMemory, 0);
    void* data = vertexBufferMemory.mapMemory(0, sizeof(vertices[0]) * vertices.size());
    std::memcpy(data, vertices.data(), (uint32_t)(sizeof(vertices[0]) * vertices.size()));
    vertexBufferMemory.unmapMemory();

    vk::raii::CommandBuffer commandBuffer = makeCommandBuffer(device, commandPool);

    vk::raii::Semaphore imageAvailableSemaphore = 0;
    vk::raii::Semaphore renderFinishedSemaphore = 0;
    vk::raii::Fence inFlightFence = 0;
    {
        auto syncObjects = makeSyncObjects(device);
        imageAvailableSemaphore = std::move(syncObjects.imageAvailableSemaphore);
        renderFinishedSemaphore = std::move(syncObjects.renderFinishedSemaphore);
        inFlightFence = std::move(syncObjects.inFlightFence);
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
        commandBuffer.bindVertexBuffers(0, {*vertexBuffer}, {0});

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

        commandBuffer.draw((uint32_t)vertices.size(), 1, 0, 0);
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