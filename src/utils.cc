#include "utils.hh"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <set>
#include <string>
#include <algorithm>

const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static VKAPI_ATTR VkBool32 VKAPI_CALL
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

static vk::DebugUtilsMessengerCreateInfoEXT getDebugUtilsMessengerCreateInfoEXT() {
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

vk::raii::Instance makeInstance(const vk::raii::Context& context) {
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
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // we should check that they are
                                                                 // supported
#endif
        instanceCreateInfo.setPEnabledExtensionNames(extensions);

#ifndef NDEBUG
        // we should check here that the validation layers are supported
        instanceCreateInfo.setPEnabledLayerNames(validationLayers);
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT =
            getDebugUtilsMessengerCreateInfoEXT();
        instanceCreateInfo.pNext = &debugUtilsMessengerCreateInfoEXT;
#endif
        return context.createInstance(instanceCreateInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating instance : " << e.what() << '\n';
        exit(-1);
    }
}

vk::raii::DebugUtilsMessengerEXT makeDebugMessenger(const vk::raii::Instance& instance) {
    try {
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT =
            getDebugUtilsMessengerCreateInfoEXT();
        return instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    } catch (std::exception& e) {
        std::cerr << "Error while creating debug messenger : " << e.what() << '\n';
        exit(-1);
    }
}

vk::raii::SurfaceKHR makeSurface(const vk::raii::Instance& instance,
                                 std::shared_ptr<Window> pWindow) {
    try {
        VkSurfaceKHR surfaceTMP;
        auto result = glfwCreateWindowSurface(*instance, pWindow->handle(), nullptr, &surfaceTMP);
        if (result != VkResult::VK_SUCCESS) {
            std::ostringstream os;
            os << "glfwCreateWindowSurface failed : " << int(result);
            throw std::runtime_error(os.str());
        }
        return vk::raii::SurfaceKHR(instance, surfaceTMP);
    } catch (std::exception& e) {
        std::cerr << "Error while creating surface : " << e.what() << '\n';
        exit(-1);
    }
}

vk::raii::PhysicalDevice selectPhysicalDevice(const vk::raii::Instance& instance,
                                              const vk::raii::SurfaceKHR& surface) {
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
                selectedDeviceScore = score;
            }
        }
        if (selectedDeviceScore == 0) {
            throw std::runtime_error("No physical device could be selected");
        }
        std::cout << physicalDevice.getProperties().deviceName << " selected\n";
    } catch (std::exception& e) {
        std::cerr << "Error while selecting physical device : " << e.what() << '\n';
        exit(-1);
    }
    return physicalDevice;
}

const SwapChainSupportDetails getPhysicalDeviceSwapChainSupportDetails(
    const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::SurfaceKHR& surface) {
    SwapChainSupportDetails deviceSwapChainSupport;
    deviceSwapChainSupport.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    deviceSwapChainSupport.formats = physicalDevice.getSurfaceFormatsKHR(*surface);
    deviceSwapChainSupport.presentModes = physicalDevice.getSurfacePresentModesKHR(*surface);
    return deviceSwapChainSupport;
}

uint32_t selectGraphicsQueueFamilyIndex(const vk::raii::PhysicalDevice& physicalDevice,
                                        const vk::raii::SurfaceKHR& surface) {
    uint32_t graphicsQueueFamilyIndex;
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
    return graphicsQueueFamilyIndex;
}

vk::raii::Device makeDevice(const vk::raii::PhysicalDevice& physicalDevice,
                            uint32_t graphicsQueueFamilyIndex) {
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
        return physicalDevice.createDevice(deviceCreateInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating device : " << e.what() << '\n';
        exit(-1);
    }
}

vk::raii::Queue getQueue(const vk::raii::Device& device, uint32_t queueFamilyIndex,
                         uint32_t queueIndex) {
    try {
        return device.getQueue(queueFamilyIndex, queueIndex);
    } catch (std::exception& e) {
        std::cerr << "Error while retrieving queue : " << e.what() << '\n';
        exit(-1);
    }
}

vk::raii::SwapchainKHR makeSwapchain(const vk::raii::Device& device,
                                     const vk::raii::SurfaceKHR& surface,
                                     const SwapChainSupportDetails deviceSwapChainSupport,
                                     std::shared_ptr<Window> pWindow) {
    vk::SurfaceFormatKHR surfaceFormat = {vk::Format::eB8G8R8A8Srgb,
                                          vk::ColorSpaceKHR::eSrgbNonlinear};
    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eMailbox;
    // Missing many checks here
    vk::Extent2D extent = {pWindow->width(), pWindow->height()};
    extent.width =
        std::clamp(extent.width, deviceSwapChainSupport.capabilities.minImageExtent.width,
                   deviceSwapChainSupport.capabilities.maxImageExtent.width);
    extent.height =
        std::clamp(extent.height, deviceSwapChainSupport.capabilities.minImageExtent.height,
                   deviceSwapChainSupport.capabilities.maxImageExtent.height);

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
            vk::SharingMode::eExclusive; // graphics and present queue families
                                         // are the same
        swapChainCreateInfo.preTransform = deviceSwapChainSupport.capabilities.currentTransform;
        swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        swapChainCreateInfo.presentMode = presentMode;
        swapChainCreateInfo.clipped = VK_TRUE;
        swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
        return device.createSwapchainKHR(swapChainCreateInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating swapchain : " << e.what() << '\n';
        exit(-1);
    }
}

std::vector<vk::raii::ImageView> makeImageViews(const vk::raii::Device& device,
                                                const vk::raii::SwapchainKHR& swapchain,
                                                vk::SurfaceFormatKHR surfaceFormat) {
    std::vector<vk::raii::ImageView> imageViews;
    try {
        auto swapChainImages = swapchain.getImages();
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
    return std::move(imageViews);
}

vk::raii::ShaderModule makeShaderModule(const vk::raii::Device& device, const std::string& path,
                                        SHADER_TYPE type) {
    try {
        auto spv = ShaderCompiler::compileAssembly(path, type);

        vk::ShaderModuleCreateInfo shaderModuleCreateInfo;
        shaderModuleCreateInfo.setCode(spv);
        return device.createShaderModule(shaderModuleCreateInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating shader module from " << path << " : " << e.what()
                  << '\n';
        exit(-1);
    }
}

vk::raii::PipelineLayout makePipelineLayout(const vk::raii::Device& device) {
    try {
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        return device.createPipelineLayout(pipelineLayoutInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating pipeline layout : " << e.what() << '\n';
        exit(-1);
    }
}

vk::raii::RenderPass makeRenderPass(const vk::raii::Device& device, vk::Format format) {
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = format;
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
        return device.createRenderPass(renderPassInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating renderPass : " << e.what() << '\n';
        exit(-1);
    }
}

vk::raii::Pipeline makeGraphicsPipeline(const vk::raii::Device& device,
                                        const vk::raii::PipelineLayout& pipelineLayout,
                                        const vk::raii::RenderPass& renderPass,
                                        const vk::raii::ShaderModule& vertShaderModule,
                                        const vk::raii::ShaderModule& fragShaderModule) {
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = *vertShaderModule;
    vertShaderStageInfo.pName = "main";
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = *fragShaderModule;
    fragShaderStageInfo.pName = "main";
    std::vector<vk::PipelineShaderStageCreateInfo> stages{vertShaderStageInfo, fragShaderStageInfo};

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

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::PipelineDynamicStateCreateInfo dynamicState;
    dynamicState.setDynamicStates(dynamicStates);

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

    vk::GraphicsPipelineCreateInfo graphicsPipelineInfo;
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

    try {
        return device.createGraphicsPipeline(nullptr, graphicsPipelineInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating graphics pipeline : " << e.what() << '\n';
        exit(-1);
    }
}

std::vector<vk::raii::Framebuffer> makeFramebuffers(
    const vk::raii::Device& device, const std::vector<vk::raii::ImageView>& imageViews,
    const vk::raii::RenderPass& renderPass, vk::Extent2D extent) {
    std::vector<vk::raii::Framebuffer> framebuffers;
    framebuffers.reserve(imageViews.size());
    for (size_t i = 0; i < imageViews.size(); i++) {
        try {
            vk::ImageView attachments[] = {*imageViews.at(i)};
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
    return framebuffers;
}

vk::raii::CommandPool makeCommandPool(const vk::raii::Device& device, uint32_t queueFamilyIndex) {
    try {
        vk::CommandPoolCreateInfo commandPoolInfo;
        commandPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        commandPoolInfo.queueFamilyIndex = queueFamilyIndex;
        return device.createCommandPool(commandPoolInfo);

    } catch (std::exception& e) {
        std::cerr << "Error while creating command pool : " << e.what() << '\n';
        exit(-1);
    }
}

vk::raii::CommandBuffer makeCommandBuffer(const vk::raii::Device& device, const vk::raii::CommandPool& commandPool) {
    try {
        vk::CommandBufferAllocateInfo commandBufferAllocInfo;
        commandBufferAllocInfo.commandPool = *commandPool;
        commandBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
        commandBufferAllocInfo.commandBufferCount = 1;
        return std::move(device.allocateCommandBuffers(commandBufferAllocInfo).at(0));
    } catch (std::exception& e) {
        std::cerr << "Error while creating command buffer : " << e.what() << '\n';
        exit(-1);
    }
}