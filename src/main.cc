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

#ifndef NDEBUG
const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

VKAPI_ATTR VkBool32 VKAPI_CALL
debugMessageFunc(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                 VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                 VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData, void* /*pUserData*/) {
    std::ostringstream message;

    message << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity))
            << " " << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypes))
            << ":\n";
    message << "<" << pCallbackData->pMessage << ">";
    std::cout << message.str() << '\n';
    return false;
}

vk::DebugUtilsMessengerCreateInfoEXT getDebugUtilsMessengerCreateInfoEXT() {
    vk::DebugUtilsMessengerCreateInfoEXT instanceDebugUtilsMessengerCreateInfoEXT;
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    instanceDebugUtilsMessengerCreateInfoEXT.messageSeverity = severityFlags;
    instanceDebugUtilsMessengerCreateInfoEXT.messageType = messageTypeFlags;
    instanceDebugUtilsMessengerCreateInfoEXT.pfnUserCallback = &debugMessageFunc;
    return instanceDebugUtilsMessengerCreateInfoEXT;
}
#endif

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

int main(void) {
    auto pWindow1 = std::make_shared<Window>("window", 800, 450);

    vk::raii::Context context;

    // Instance creation, validation layers enabling and debug messenger
    vk::raii::Instance instance = 0;
    try {
        vk::ApplicationInfo appInfo;
        appInfo.pApplicationName = "pain-bagnat";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        vk::InstanceCreateInfo instanceCreateInfo;
        instanceCreateInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(
            &glfwExtensionCount); // we should check that they are supported
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#ifndef NDEBUG
        extensions.push_back(
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // we should check that they are supported
#endif
        instanceCreateInfo.setPEnabledExtensionNames(extensions);

#ifndef NDEBUG
        // we should check here that the validation layers are supported
        instanceCreateInfo.setPEnabledLayerNames(validationLayers);
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT =
            getDebugUtilsMessengerCreateInfoEXT();
        instanceCreateInfo.pNext = &debugUtilsMessengerCreateInfoEXT;
#endif
        instance = context.createInstance(instanceCreateInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating instance : " << e.what() << '\n';
        exit(-1);
    }
#ifndef NDEBUG
    // Debug messenger creation
    vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger = 0;
    try {
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT =
            getDebugUtilsMessengerCreateInfoEXT();
        debugUtilsMessenger =
            instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    } catch (std::exception& e) {
        std::cerr << "Error while creating debug messenger : " << e.what() << '\n';
        exit(-1);
    }
#endif

    // Surface creation
    vk::raii::SurfaceKHR surface = 0;
    try {
        VkSurfaceKHR surfaceTMP;
        auto result = glfwCreateWindowSurface(*instance, pWindow1->handle(), nullptr, &surfaceTMP);
        if (result != VkResult::VK_SUCCESS) {
            std::ostringstream os;
            os << "glfwCreateWindowSurface failed : " << int(result);
            throw std::runtime_error(os.str());
        }
        surface = vk::raii::SurfaceKHR(instance, surfaceTMP);
    } catch (std::exception& e) {
        std::cerr << "Error while creating surface : " << e.what() << '\n';
        exit(-1);
    }

    // Physical device query
    const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    SwapChainSupportDetails deviceSwapChainSupport;
    vk::raii::PhysicalDevice physicalDevice = 0;
    try {
        vk::raii::PhysicalDevices physicalDevices(instance);
        if (physicalDevices.size() == 0) {
            throw std::runtime_error("No physical device found");
        }
        int selectedDeviceScore = 0;
        for (const auto& aPhysicalDevice : physicalDevices) {
            auto availableExtensions = aPhysicalDevice.enumerateDeviceExtensionProperties();
            std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                                     deviceExtensions.end());
            for (const auto& extension : availableExtensions) {
                requiredExtensions.erase(extension.extensionName);
            }
            if (!requiredExtensions.empty()) continue;

            SwapChainSupportDetails aDeviceSwapChainSupport;
            aDeviceSwapChainSupport.capabilities =
                aPhysicalDevice.getSurfaceCapabilitiesKHR(*surface);
            aDeviceSwapChainSupport.formats = aPhysicalDevice.getSurfaceFormatsKHR(*surface);
            aDeviceSwapChainSupport.presentModes =
                aPhysicalDevice.getSurfacePresentModesKHR(*surface);
            if (aDeviceSwapChainSupport.formats.empty() ||
                aDeviceSwapChainSupport.presentModes.empty())
                continue;

            auto props = aPhysicalDevice.getProperties();
            auto features = aPhysicalDevice.getFeatures();
            int score = 0;
            if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) score += 100;
            if (props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) score += 50;
            if (score >= selectedDeviceScore) {
                physicalDevice = aPhysicalDevice;
                deviceSwapChainSupport = aDeviceSwapChainSupport;
                selectedDeviceScore = score;
            }
        }
        if (selectedDeviceScore == 0) {
            throw std::runtime_error("No physical device could be selected");
        }
        auto props = physicalDevice.getProperties();
        auto features = physicalDevice.getFeatures();
        std::cout << props.deviceName << " selected\n";
    } catch (std::exception& e) {
        std::cerr << "Error while selecting physical device : " << e.what() << '\n';
        exit(-1);
    }

    // Queue family queries
    size_t graphicsQueueFamilyIndex;
    try {
        int selectedGraphicsScore = 0;
        auto queueFamilyProps = physicalDevice.getQueueFamilyProperties();
        for (size_t i = 0; i < queueFamilyProps.size(); i++) {
            const vk::QueueFamilyProperties& queueFamilyProp = queueFamilyProps.at(i);
            int graphicsScore = 0;
            if (queueFamilyProp.queueFlags & vk::QueueFlagBits::eGraphics) {
                graphicsScore += 50; // graphics able queue
            }
            if (!(queueFamilyProp.queueFlags & vk::QueueFlagBits::eTransfer)) {
                graphicsScore += 50;
            }
            if (!physicalDevice.getSurfaceSupportKHR(i, *surface)) {
                graphicsScore = -1; // presentation support
            }
            graphicsScore *= queueFamilyProp.queueCount;
            if (graphicsScore >= selectedGraphicsScore) {
                graphicsQueueFamilyIndex = i;
                selectedGraphicsScore = graphicsScore;
            }
        }
        if (selectedGraphicsScore == 0) {
            throw std::runtime_error("No graphics able queue was found");
        }
    } catch (std::exception& e) {
        std::cerr << "Error while selecting queue families : " << e.what() << '\n';
        exit(-1);
    }

    // Device creation
    vk::raii::Device device = 0;
    try {
        std::vector<vk::DeviceQueueCreateInfo> queuesCreateInfo;

        vk::DeviceQueueCreateInfo graphicsQueueInfo;
        graphicsQueueInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
        std::vector<float> graphicsQueuePriority = {1.0f};
        graphicsQueueInfo.setQueuePriorities(graphicsQueuePriority);
        queuesCreateInfo.push_back(graphicsQueueInfo);

        vk::PhysicalDeviceFeatures physicalDeviceFeatures;

        vk::DeviceCreateInfo deviceCreateInfo;
        deviceCreateInfo.setQueueCreateInfos(queuesCreateInfo);
        deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
        deviceCreateInfo.setPEnabledExtensionNames(deviceExtensions);
#ifndef NDEBUG
        deviceCreateInfo.setPEnabledLayerNames(validationLayers);
#endif
        device = physicalDevice.createDevice(deviceCreateInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating device : " << e.what() << '\n';
        exit(-1);
    }

    vk::raii::Queue graphicsQueue = 0;
    try {
        graphicsQueue = device.getQueue(graphicsQueueFamilyIndex, 0);
    } catch (std::exception& e) {
        std::cerr << "Error while retrieving queues : " << e.what() << '\n';
        exit(-1);
    }

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

    vk::raii::SwapchainKHR swapChain = 0;
    uint32_t imageCount = deviceSwapChainSupport.capabilities.minImageCount + 1;
    try {
        vk::SwapchainCreateInfoKHR swapChainCreateInfo;
        swapChainCreateInfo.surface = *surface;
        swapChainCreateInfo.minImageCount = imageCount;
        swapChainCreateInfo.imageFormat = surfaceFormat.format;
        swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapChainCreateInfo.imageExtent = extent;
        swapChainCreateInfo.imageArrayLayers = 1;
        swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
        swapChainCreateInfo.imageSharingMode =
            vk::SharingMode::eExclusive; // graphics and present queue families are the same
        swapChainCreateInfo.preTransform = deviceSwapChainSupport.capabilities.currentTransform;
        swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        swapChainCreateInfo.presentMode = presentMode;
        swapChainCreateInfo.clipped = VK_TRUE;
        swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
        swapChain = device.createSwapchainKHR(swapChainCreateInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating swapchain : " << e.what() << '\n';
        exit(-1);
    }

    std::vector<vk::raii::ImageView> imageViews;
    try {
        auto swapChainImages = swapChain.getImages();
        for (auto& image : swapChainImages) {
            vk::ImageViewCreateInfo imageViewCreateInfo;
            imageViewCreateInfo.image = image;
            imageViewCreateInfo.format = surfaceFormat.format;
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
            imageViews.push_back(device.createImageView(imageViewCreateInfo));
        }
    } catch (std::exception& e) {
        std::cerr << "Error while creating swapchain image views : " << e.what() << '\n';
        exit(-1);
    }

    auto vertSpv = ShaderCompiler::compileAssembly(
        std::string(PROJECT_SOURCE_DIR) + "/shaders/basic.vert", SHADER_TYPE::VERT);
    auto fragSpv = ShaderCompiler::compileAssembly(
        std::string(PROJECT_SOURCE_DIR) + "/shaders/basic.frag", SHADER_TYPE::FRAG);

    vk::raii::ShaderModule vertShaderModule = 0;
    vk::raii::ShaderModule fragShaderModule = 0;
    try {
        vk::ShaderModuleCreateInfo vertShaderModuleCreateInfo;
        vertShaderModuleCreateInfo.setCode(vertSpv);
        vertShaderModule = device.createShaderModule(vertShaderModuleCreateInfo);

        vk::ShaderModuleCreateInfo fragShaderModuleCreateInfo;
        fragShaderModuleCreateInfo.setCode(fragSpv);
        fragShaderModule = device.createShaderModule(fragShaderModuleCreateInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating shader modules : " << e.what() << '\n';
        exit(-1);
    }

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
            swapChain.acquireNextImage(UINT_FAST64_MAX, *imageAvailableSemaphore, nullptr).second;
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
        presentInfo.setSwapchains(*swapChain);
        presentInfo.setImageIndices(imageIndex);
        graphicsQueue.presentKHR(presentInfo);
    }
    device.waitIdle();
}