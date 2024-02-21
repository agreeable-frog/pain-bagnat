#include "pipeline.hh"

#include <iostream>

#include "build_defs.hh"

namespace render {
void Pipeline::createVertShaderModule() {
    _vertShaderModule = createShaderModule(std::string(PROJECT_SOURCE_DIR) + "/shaders/basic.vert",
                                           SHADER_TYPE::VERT);
}

void Pipeline::createFragShaderModule() {
    _fragShaderModule = createShaderModule(std::string(PROJECT_SOURCE_DIR) + "/shaders/basic.frag",
                                           SHADER_TYPE::FRAG);
}

vk::raii::ShaderModule Pipeline::createShaderModule(const std::string& path, SHADER_TYPE type) {
    try {
        auto spv = ShaderCompiler::compileAssembly(path, type);

        vk::ShaderModuleCreateInfo shaderModuleCreateInfo;
        shaderModuleCreateInfo.setCode(spv);
        return _pDevice->device().createShaderModule(shaderModuleCreateInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating shader module from " << path << " : " << e.what()
                  << '\n';
        exit(-1);
    }
}

void Pipeline::createDescriptorSetLayout() {
    try {
        vk::DescriptorSetLayoutBinding uboLayoutBinding;
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

        vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo;
        descriptorSetLayoutInfo.setBindings(uboLayoutBinding);
        _descriptorSetLayout =
            _pDevice->device().createDescriptorSetLayout(descriptorSetLayoutInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating descriptor set layout : " << e.what() << '\n';
        exit(-1);
    }
}

void Pipeline::createPipelineLayout() {
    try {
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        // pipelineLayoutInfo.setSetLayouts(*_descriptorSetLayout);
        _layout = _pDevice->device().createPipelineLayout(pipelineLayoutInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating pipeline layout : " << e.what() << '\n';
        exit(-1);
    }
}

void Pipeline::createRenderPass() {
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = _pSwapChain->surfaceFormat().format;
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
        _renderPass = _pDevice->device().createRenderPass(renderPassInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating renderPass : " << e.what() << '\n';
        exit(-1);
    }
}

void Pipeline::createGraphicsPipeline() {
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = *_vertShaderModule;
    vertShaderStageInfo.pName = "main";
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = *_fragShaderModule;
    fragShaderStageInfo.pName = "main";
    std::vector<vk::PipelineShaderStageCreateInfo> stages{vertShaderStageInfo, fragShaderStageInfo};

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vk::VertexInputBindingDescription vertexBasicBindingDescription =
        VertexBasic::bindingDescription();
    vertexInputInfo.setVertexBindingDescriptions(vertexBasicBindingDescription);
    std::array<vk::VertexInputAttributeDescription, 3> vertexBasicAttributeDescriptions =
        VertexBasic::attributeDescriptions();
    vertexInputInfo.setVertexAttributeDescriptions(vertexBasicAttributeDescriptions);

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
    graphicsPipelineInfo.setLayout(*_layout);
    graphicsPipelineInfo.setRenderPass(*_renderPass);

    try {
        _pipeline = _pDevice->device().createGraphicsPipeline(nullptr, graphicsPipelineInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating graphics pipeline : " << e.what() << '\n';
        exit(-1);
    }
}

void Pipeline::createFramebuffers() {
    _framebuffers.reserve(_pSwapChain->imageViews().size());
    for (size_t i = 0; i < _pSwapChain->imageViews().size(); i++) {
        try {
            vk::ImageView attachments[] = {*(_pSwapChain->imageViews()).at(i)};
            vk::FramebufferCreateInfo framebufferInfo{};
            framebufferInfo.renderPass = *_renderPass;
            framebufferInfo.setAttachments(attachments);
            framebufferInfo.width = _pSwapChain->extent().width;
            framebufferInfo.height = _pSwapChain->extent().height;
            framebufferInfo.layers = 1;
            _framebuffers.push_back(_pDevice->device().createFramebuffer(framebufferInfo));
        } catch (std::exception& e) {
            std::cerr << "Error while creating framebuffer : " << e.what() << '\n';
            exit(-1);
        }
    }
}

Pipeline::Pipeline(std::shared_ptr<const render::Device> pDevice,
                   std::shared_ptr<const render::SwapChain> pSwapChain)
    : _pDevice(pDevice), _pSwapChain(pSwapChain) {
    createVertShaderModule();
    createFragShaderModule();
    // createDescriptorSetLayout();
    createPipelineLayout();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
}
} // namespace render