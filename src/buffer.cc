#include "buffer.hh"

#include <iostream>
#include <vector>

namespace render {
Buffer::Buffer(const render::Device& device, size_t size, VkBufferUsageFlags usage) : _size(size) {
    VkBuffer stagingBuffer;
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    std::vector<uint32_t> queueFamilyIndices = {device.transferQueueFamily().index};
    bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    bufferCreateInfo.queueFamilyIndexCount = 1;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    auto result = vmaCreateBuffer(device.allocator(), &bufferCreateInfo, &allocCreateInfo,
                                  &stagingBuffer, &_stagingAllocation, 0);
    if (result != VkResult::VK_SUCCESS) {
        std::cerr << "vmaCreateBuffer failed : " << result << "\n";
        exit(-1);
    }
    _stagingBuffer = vk::Buffer(stagingBuffer);

    VkBuffer buffer;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage;
    bufferCreateInfo.sharingMode =
        (device.graphicsQueueFamily().index == device.transferQueueFamily().index)
            ? VK_SHARING_MODE_EXCLUSIVE
            : VK_SHARING_MODE_CONCURRENT;
    queueFamilyIndices = {device.graphicsQueueFamily().index,
                                                device.transferQueueFamily().index};
    bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    bufferCreateInfo.queueFamilyIndexCount = 2;

    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocCreateInfo.flags = 0;

    result = vmaCreateBuffer(device.allocator(), &bufferCreateInfo, &allocCreateInfo, &buffer,
                                  &_allocation, 0);
    if (result != VkResult::VK_SUCCESS) {
        std::cerr << "vmaCreateBuffer failed : " << result << "\n";
        exit(-1);
    }
    _buffer = vk::Buffer(buffer);
    _allocator = device.allocator();
    // we copy the handle of the device's allocator
    // there is no reason for the buffer to outlive the device so it should be safe
}

Buffer::~Buffer() {
    vmaDestroyBuffer(_allocator, _stagingBuffer, _stagingAllocation);
    vmaDestroyBuffer(_allocator, _buffer, _allocation);
}

void Buffer::mapData(const render::Device& device, void* data, size_t size) {
    void* tmp;
    vmaMapMemory(_allocator, _stagingAllocation, &tmp);
    std::memcpy(tmp, data, (uint32_t)(std::min(size, _size)));
    vmaUnmapMemory(_allocator, _stagingAllocation);

    vk::CommandBufferAllocateInfo commandBufferAllocInfo;
    commandBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    commandBufferAllocInfo.commandPool = *device.transferCommandPool();
    commandBufferAllocInfo.commandBufferCount = 1;
    vk::raii::CommandBuffer commandBuffer = 0;
    try {
        commandBuffer =
            std::move(device.device().allocateCommandBuffers(commandBufferAllocInfo).at(0));
    } catch (std::exception& e) {
        std::cerr << "Error while creating temporary command buffer : " << e.what() << '\n';
        exit(-1);
    }

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    commandBuffer.begin(beginInfo);
    {
        vk::BufferCopy copyRegion;
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = _size;
        commandBuffer.copyBuffer(_stagingBuffer, _buffer, {copyRegion});
    }
    commandBuffer.end();

    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(*commandBuffer);

    device.transferQueue().submit(submitInfo, nullptr);
    device.transferQueue().waitIdle();
}

HostBuffer::HostBuffer(const render::Device& device, size_t size, VkBufferUsageFlags usage) : _size(size) {
    VkBuffer buffer;
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode =
        (device.graphicsQueueFamily().index == device.transferQueueFamily().index)
            ? VK_SHARING_MODE_EXCLUSIVE
            : VK_SHARING_MODE_CONCURRENT;
    std::vector<uint32_t> queueFamilyIndices = {device.graphicsQueueFamily().index,
                                                device.transferQueueFamily().index};
    bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    bufferCreateInfo.queueFamilyIndexCount = 2;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;;

    VkResult result = vmaCreateBuffer(device.allocator(), &bufferCreateInfo, &allocCreateInfo, &buffer,
                                  &_allocation, 0);
    if (result != VkResult::VK_SUCCESS) {
        std::cerr << "vmaCreateBuffer failed : " << result << "\n";
        exit(-1);
    }
    _buffer = vk::Buffer(buffer);
    _allocator = device.allocator();
    // we copy the handle of the device's allocator
    // there is no reason for the buffer to outlive the device so it should be safe
    vmaMapMemory(_allocator, _allocation, &_mapBinding);
}

HostBuffer::~HostBuffer() {
    vmaUnmapMemory(_allocator, _allocation);
    _mapBinding = nullptr;
    vmaDestroyBuffer(_allocator, _buffer, _allocation);
}

void HostBuffer::mapData(const render::Device& device, void* data, size_t size) {
    std::memcpy(_mapBinding, data, (uint32_t)(std::min(size, _size)));
}
} // namespace render