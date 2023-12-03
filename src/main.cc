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
    vk::raii::Instance instance = createInstance(context);
#ifndef NDEBUG
    vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger = createDebugMessenger(instance);
#endif
    vk::raii::SurfaceKHR surface = createSurface(instance, pWindow1);
    vk::raii::PhysicalDevice physicalDevice = selectPhysicalDevice(instance, surface);
    const SwapChainSupportDetails deviceSwapChainSupport =
        getPhysicalDeviceSwapChainSupportDetails(physicalDevice, surface);

    // Queue family queries
    size_t graphicsQueueFamilyIndex = selectGraphicsQueueFamilyIndex(physicalDevice, surface);

    // Device creation
    vk::raii::Device device = createDevice(physicalDevice, graphicsQueueFamilyIndex);

    vk::raii::Queue graphicsQueue = getQueue(device, graphicsQueueFamilyIndex, 0);

    // SwapChain creation
    vk::SurfaceFormatKHR surfaceFormat = {vk::Format::eB8G8R8A8Srgb,
                                          vk::ColorSpaceKHR::eSrgbNonlinear};
    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eMailbox;
    // Missing many checks here
    vk::Extent2D extent = {pWindow1->width(), pWindow1->height()};
    extent.width =
        std::clamp(extent.width, deviceSwapChainSupport.capabilities.minImageExtent.width,
                   deviceSwapChainSupport.capabilities.maxImageExtent.width);
    extent.height =
        std::clamp(extent.height, deviceSwapChainSupport.capabilities.minImageExtent.height,
                   deviceSwapChainSupport.capabilities.maxImageExtent.height);

    vk::raii::SwapchainKHR swapchain =
        createSwapchain(device, surface, deviceSwapChainSupport, pWindow1);

    std::vector<vk::raii::ImageView> imageViews =
        createImageViews(device, swapchain, surfaceFormat);

    vk::raii::ShaderModule vertShaderModule = createShaderModule(
        device, std::string(PROJECT_SOURCE_DIR) + "/shaders/basic.vert", SHADER_TYPE::VERT);
    vk::raii::ShaderModule fragShaderModule = createShaderModule(
        device, std::string(PROJECT_SOURCE_DIR) + "/shaders/basic.frag", SHADER_TYPE::FRAG);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = *vertShaderModule;
    vertShaderStageInfo.pName = "main";
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = *fragShaderModule;
    fragShaderStageInfo.pName = "main";

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    inputAssemblyInfo.primitiveRestartEnable = false;
    inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;

    std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport,
                                                   vk::DynamicState::eScissor};

    vk::PipelineDynamicStateCreateInfo dynamicState;
    dynamicState.setDynamicStates(dynamicStates);

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizationInfo;
    rasterizationInfo.depthClampEnable = false;
    rasterizationInfo.rasterizerDiscardEnable = false;
    rasterizationInfo.polygonMode = vk::PolygonMode::eFill;
    rasterizationInfo.lineWidth = 1.0f;
    rasterizationInfo.cullMode = vk::CullModeFlagBits::eBack;
    rasterizationInfo.frontFace = vk::FrontFace::eCounterClockwise;

    vk::PipelineMultisampleStateCreateInfo multisamplingInfo;
    multisamplingInfo.sampleShadingEnable = false;
    multisamplingInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = false;

    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.logicOpEnable = false;
    colorBlending.setAttachments(colorBlendAttachment);

    vk::raii::PipelineLayout pipelineLayout = 0;
    try {
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating pipeline layout : " << e.what() << '\n';
        exit(-1);
    }

    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = surfaceFormat.format;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.setColorAttachments(colorAttachmentRef);

    vk::raii::RenderPass renderPass = 0;
    try {
        vk::RenderPassCreateInfo renderPassInfo;
        renderPassInfo.setAttachments(colorAttachment);
        renderPassInfo.setSubpasses(subpass);
        vk::SubpassDependency dependency;
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.srcAccessMask = vk::AccessFlagBits::eNone;
        dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        renderPassInfo.setDependencies(dependency);
        renderPass = device.createRenderPass(renderPassInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating renderPass : " << e.what() << '\n';
        exit(-1);
    }

    vk::GraphicsPipelineCreateInfo graphicsPipelineInfo;
    std::vector<vk::PipelineShaderStageCreateInfo> stages{vertShaderStageInfo, fragShaderStageInfo};
    graphicsPipelineInfo.setStages(stages);
    graphicsPipelineInfo.setPVertexInputState(&vertexInputInfo);
    graphicsPipelineInfo.setPInputAssemblyState(&inputAssemblyInfo);
    graphicsPipelineInfo.setPViewportState(&viewportState);
    graphicsPipelineInfo.setPDynamicState(&dynamicState);
    graphicsPipelineInfo.setPRasterizationState(&rasterizationInfo);
    graphicsPipelineInfo.setPMultisampleState(&multisamplingInfo);
    graphicsPipelineInfo.setPDepthStencilState(0);
    graphicsPipelineInfo.setPColorBlendState(&colorBlending);
    graphicsPipelineInfo.setLayout(*pipelineLayout);
    graphicsPipelineInfo.setRenderPass(*renderPass);

    vk::raii::Pipeline graphicsPipeline = 0;
    try {
        graphicsPipeline = device.createGraphicsPipeline(nullptr, graphicsPipelineInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating graphics pipeline : " << e.what() << '\n';
        exit(-1);
    }

    std::vector<vk::raii::Framebuffer> framebuffers;
    framebuffers.reserve(imageViews.size());
    for (size_t i = 0; i < imageViews.size(); i++) {
        try {
            vk::ImageView attachments[] = {*imageViews[i]};
            vk::FramebufferCreateInfo framebufferInfo{};
            framebufferInfo.renderPass = *renderPass;
            framebufferInfo.setAttachments(attachments);
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            framebufferInfo.layers = 1;
            framebuffers.push_back(device.createFramebuffer(framebufferInfo));
        } catch (std::exception& e) {
            std::cerr << "Error while creating framebuffer : " << e.what() << '\n';
            exit(-1);
        }
    }

    vk::raii::CommandPool commandPool = 0;
    try {
        vk::CommandPoolCreateInfo commandPoolInfo;
        commandPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        commandPoolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
        commandPool = device.createCommandPool(commandPoolInfo);

    } catch (std::exception& e) {
        std::cerr << "Error while creating command pool : " << e.what() << '\n';
        exit(-1);
    }

    vk::raii::CommandBuffer commandBuffer = 0;
    try {
        vk::CommandBufferAllocateInfo commandBufferAllocInfo;
        commandBufferAllocInfo.commandPool = *commandPool;
        commandBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
        commandBufferAllocInfo.commandBufferCount = 1;
        commandBuffer = std::move(device.allocateCommandBuffers(commandBufferAllocInfo).at(0));
    } catch (std::exception& e) {
        std::cerr << "Error while creating command buffer : " << e.what() << '\n';
        exit(-1);
    }

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