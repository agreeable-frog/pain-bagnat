#include <vulkan/vulkan_raii.hpp>

#include "instance.hh"
#include "display.hh"
#include "device.hh"
#include "swap_chain.hh"
#include "pipeline.hh"
#include "buffer.hh"

int main(void) {
    render::Instance instance = render::Instance();
    render::Display display = render::Display(instance, "window", 800, 450);

    render::Device device = render::Device(instance, display.surface());
    render::SwapChain swapChain = render::SwapChain(display, device);

    render::Pipeline pipeline = render::Pipeline(device, swapChain);

    std::vector<VertexBasic> vertices = {{{-0.5, 0.5, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0}},
                                         {{0.5, 0.5, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0}},
                                         {{0.0, -0.5, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0}}

    };

    render::Buffer vertexBuffer = render::Buffer(
        device, sizeof(vertices[0]) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        {device.graphicsQueueFamily().index, device.transferQueueFamily().index});
    vertexBuffer.mapData(vertices.data(), sizeof(vertices[0]) * vertices.size());

    vk::raii::Semaphore imageAvailableSemaphore = 0;
    vk::raii::Semaphore renderFinishedSemaphore = 0;
    vk::raii::Fence inFlightFence = 0;
    try {
        vk::SemaphoreCreateInfo semaphoreInfo;
        imageAvailableSemaphore = device.device().createSemaphore(semaphoreInfo);
        renderFinishedSemaphore = device.device().createSemaphore(semaphoreInfo);

        vk::FenceCreateInfo fenceInfo;
        fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
        inFlightFence = device.device().createFence(fenceInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating sync elements : " << e.what() << '\n';
        exit(-1);
    }

    // Main loop
    while (!glfwWindowShouldClose(display.pWindow())) {
        static double lastFrameTime = glfwGetTime();
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastFrameTime;
        if (deltaTime < (1.0 / 60.0)) {
            continue;
        }
        lastFrameTime = currentTime;
        // FPS counter
        {
            static int counter = 0;
            static double timeRef = 0.0;
            counter++;
            timeRef += deltaTime;
            if (timeRef > 5.0) {
                double framerate = counter / timeRef;
                std::cout << "FRAMERATE : " << framerate << " fps\n";
                counter = 0;
                timeRef = 0.0;
            }
        }
        auto result = device.device().waitForFences(*inFlightFence, true, UINT_FAST64_MAX);
        device.device().resetFences(*inFlightFence);
        uint32_t imageIndex =
            swapChain.swapChain()
                .acquireNextImage(UINT_FAST64_MAX, *imageAvailableSemaphore, nullptr)
                .second;
        device.graphicsCommandBuffer().reset();

        vk::CommandBufferBeginInfo beginInfo;
        device.graphicsCommandBuffer().begin(beginInfo);
        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = *pipeline.renderPass();
        renderPassInfo.framebuffer = *pipeline.framebuffers()[imageIndex];
        renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
        renderPassInfo.renderArea.extent = swapChain.extent();
        vk::ClearValue clearValue = vk::ClearValue{{0.0f, 0.0f, 0.0f, 1.0f}};
        renderPassInfo.setClearValues(clearValue);
        device.graphicsCommandBuffer().beginRenderPass(renderPassInfo,
                                                       vk::SubpassContents::eInline);
        device.graphicsCommandBuffer().bindPipeline(vk::PipelineBindPoint::eGraphics,
                                                    *pipeline.pipeline());
        device.graphicsCommandBuffer().bindVertexBuffers(0, {vertexBuffer.buffer()}, {0});

        vk::Viewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChain.extent().width);
        viewport.height = static_cast<float>(swapChain.extent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        device.graphicsCommandBuffer().setViewport(0, viewport);

        vk::Rect2D scissor;
        scissor.offset = vk::Offset2D{0, 0};
        scissor.extent = swapChain.extent();
        device.graphicsCommandBuffer().setScissor(0, scissor);

        device.graphicsCommandBuffer().draw((uint32_t)vertices.size(), 1, 0, 0);
        device.graphicsCommandBuffer().endRenderPass();
        device.graphicsCommandBuffer().end();

        vk::SubmitInfo submitInfo;
        submitInfo.setWaitSemaphores(*imageAvailableSemaphore);
        vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        submitInfo.setWaitDstStageMask(waitStages);
        submitInfo.setCommandBuffers(*device.graphicsCommandBuffer());
        submitInfo.setSignalSemaphores(*renderFinishedSemaphore);
        device.graphicsQueue().submit(submitInfo, *inFlightFence);

        vk::PresentInfoKHR presentInfo;
        presentInfo.setWaitSemaphores(*renderFinishedSemaphore);
        presentInfo.setSwapchains(*swapChain.swapChain());
        presentInfo.setImageIndices(imageIndex);
        device.graphicsQueue().presentKHR(presentInfo);

        glfwPollEvents();
    }
    device.device().waitIdle();
}