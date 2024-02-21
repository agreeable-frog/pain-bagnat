#include <vulkan/vulkan_raii.hpp>
#include <memory>

#include "instance.hh"
#include "display.hh"
#include "device.hh"
#include "swap_chain.hh"
#include "pipeline.hh"
#include "buffer.hh"

int main(void) {
    std::shared_ptr<render::Instance> pInstance = std::make_shared<render::Instance>();
    std::shared_ptr<render::Display> pDisplay = std::make_shared<render::Display>(pInstance, "window", 800, 450);

    std::shared_ptr<render::Device> pDevice = std::make_shared<render::Device>(pInstance, pDisplay->surface());
    std::shared_ptr<render::SwapChain> pSwapChain = std::make_shared<render::SwapChain>(pDisplay, pDevice);

    std::shared_ptr<render::Pipeline> pPipeline = std::make_shared<render::Pipeline>(pDevice, pSwapChain);

    std::vector<VertexBasic> vertices = {{{0.8, -0.8, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0}},
                                         {{-0.8, -0.8, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0}},
                                         {{-0.8, 0.8, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0}},
                                         {{0.8, 0.8, 0.8}, {0.0, 0.0, 0.0}, {0.0, 0.0}}

    };

    std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};

    render::Buffer vertexBuffer = render::Buffer(*pDevice, sizeof(VertexBasic) * vertices.size(),
                                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vertexBuffer.mapData(*pDevice, vertices.data(), sizeof(VertexBasic) * vertices.size());

    render::Buffer indexBuffer =
        render::Buffer(*pDevice, sizeof(uint32_t) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    indexBuffer.mapData(*pDevice, indices.data(), sizeof(uint32_t) * indices.size());

    vk::raii::Semaphore imageAvailableSemaphore = 0;
    vk::raii::Semaphore renderFinishedSemaphore = 0;
    vk::raii::Fence inFlightFence = 0;
    try {
        vk::SemaphoreCreateInfo semaphoreInfo;
        imageAvailableSemaphore = pDevice->device().createSemaphore(semaphoreInfo);
        renderFinishedSemaphore = pDevice->device().createSemaphore(semaphoreInfo);

        vk::FenceCreateInfo fenceInfo;
        fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
        inFlightFence = pDevice->device().createFence(fenceInfo);
    } catch (std::exception& e) {
        std::cerr << "Error while creating sync elements : " << e.what() << '\n';
        exit(-1);
    }

    // Main loop
    while (!glfwWindowShouldClose(pDisplay->pWindow())) {
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
        auto result = pDevice->device().waitForFences(*inFlightFence, true, UINT_FAST64_MAX);
        pDevice->device().resetFences(*inFlightFence);
        uint32_t imageIndex =
            pSwapChain->swapChain()
                .acquireNextImage(UINT_FAST64_MAX, *imageAvailableSemaphore, nullptr)
                .second;
        pDevice->graphicsCommandBuffer().reset();

        vk::CommandBufferBeginInfo beginInfo;
        pDevice->graphicsCommandBuffer().begin(beginInfo);
        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = *pPipeline->renderPass();
        renderPassInfo.framebuffer = *pPipeline->framebuffers()[imageIndex];
        renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
        renderPassInfo.renderArea.extent = pSwapChain->extent();
        vk::ClearValue clearValue = vk::ClearValue{{0.0f, 0.0f, 0.0f, 1.0f}};
        renderPassInfo.setClearValues(clearValue);
        pDevice->graphicsCommandBuffer().beginRenderPass(renderPassInfo,
                                                       vk::SubpassContents::eInline);
        pDevice->graphicsCommandBuffer().bindPipeline(vk::PipelineBindPoint::eGraphics,
                                                    *pPipeline->pipeline());
        pDevice->graphicsCommandBuffer().bindVertexBuffers(0, {vertexBuffer.buffer()}, {0});
        pDevice->graphicsCommandBuffer().bindIndexBuffer(indexBuffer.buffer(), 0, vk::IndexType::eUint32);

        vk::Viewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(pSwapChain->extent().width);
        viewport.height = static_cast<float>(pSwapChain->extent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        pDevice->graphicsCommandBuffer().setViewport(0, viewport);

        vk::Rect2D scissor;
        scissor.offset = vk::Offset2D{0, 0};
        scissor.extent = pSwapChain->extent();
        pDevice->graphicsCommandBuffer().setScissor(0, scissor);

        pDevice->graphicsCommandBuffer().drawIndexed((uint32_t)indices.size(), 1, 0, 0, 0);
        pDevice->graphicsCommandBuffer().endRenderPass();
        pDevice->graphicsCommandBuffer().end();

        vk::SubmitInfo submitInfo;
        submitInfo.setWaitSemaphores(*imageAvailableSemaphore);
        vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        submitInfo.setWaitDstStageMask(waitStages);
        submitInfo.setCommandBuffers(*pDevice->graphicsCommandBuffer());
        submitInfo.setSignalSemaphores(*renderFinishedSemaphore);
        pDevice->graphicsQueue().submit(submitInfo, *inFlightFence);

        vk::PresentInfoKHR presentInfo;
        presentInfo.setWaitSemaphores(*renderFinishedSemaphore);
        presentInfo.setSwapchains(*pSwapChain->swapChain());
        presentInfo.setImageIndices(imageIndex);
        pDevice->graphicsQueue().presentKHR(presentInfo);

        glfwPollEvents();
    }
    pDevice->device().waitIdle();
}