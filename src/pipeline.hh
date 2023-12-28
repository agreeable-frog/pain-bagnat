#pragma once

#include <vulkan/vulkan.hpp>

#include "device.hh"
#include "swap_chain.hh"
#include "shader_compiler.hh"

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