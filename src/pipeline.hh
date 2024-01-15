#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "device.hh"
#include "swap_chain.hh"
#include "shader_compiler.hh"

struct VertexBasic {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uvcoords;

    static vk::VertexInputBindingDescription bindingDescription() {
        vk::VertexInputBindingDescription bindingDescription;
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(VertexBasic);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;
        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions() {
        std::array <vk::VertexInputAttributeDescription, 3> attributeDescriptions;
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[0].offset = offsetof(VertexBasic, position);
    
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = offsetof(VertexBasic, normal);
    
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[2].offset = offsetof(VertexBasic, uvcoords);
        return attributeDescriptions;
    }
};

namespace render {
class Pipeline {
private:
    vk::raii::ShaderModule _vertShaderModule = 0;
    vk::raii::ShaderModule _fragShaderModule = 0;
    vk::raii::PipelineLayout _layout = 0;
    vk::raii::RenderPass _renderPass = 0;
    vk::raii::Pipeline _pipeline = 0;
    std::vector<vk::raii::Framebuffer> _framebuffers;

    void createVertShaderModule(const vk::raii::Device& device);
    void createFragShaderModule(const vk::raii::Device& device);
    static vk::raii::ShaderModule createShaderModule(const vk::raii::Device& device,
                                                     const std::string& path, SHADER_TYPE type);
    void createPipelineLayout(const vk::raii::Device& device);
    void createRenderPass(const vk::raii::Device& device, vk::Format format);
    void createGraphicsPipeline(const vk::raii::Device& device);
    void createFramebuffers(const vk::raii::Device& device,
                            const std::vector<vk::raii::ImageView>& imageViews,
                            vk::Extent2D extent);

public:
    Pipeline(const render::Device& device, const render::SwapChain& swapChain);
    const vk::raii::RenderPass& renderPass() const {
        return _renderPass;
    }
    const vk::raii::Pipeline& pipeline() const {
        return _pipeline;
    }
    const std::vector<vk::raii::Framebuffer>& framebuffers() const {
        return _framebuffers;
    }
};
} // namespace render