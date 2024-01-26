#include "buffer.hh"

#include <iostream>

static bool isExclusive(const std::vector<uint32_t>& vec) {
    if (vec.size() == 0) return true;
    uint32_t first = vec.at(0);
    for (uint32_t i : vec) {
        if (i != first) return false;
    }
    return true;
}

namespace render {
Buffer::Buffer(const render::Device& device, size_t size, VkBufferUsageFlags usage,
               std::vector<uint32_t> queueFamilyIndices)
    : _size(size) {
    VkBuffer buffer;
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode =
        isExclusive(queueFamilyIndices) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
    bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    bufferCreateInfo.queueFamilyIndexCount = queueFamilyIndices.size();

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    auto result = vmaCreateBuffer(device.allocator(), &bufferCreateInfo, &allocCreateInfo, &buffer,
                                  &_allocation, 0);
    if (result != VkResult::VK_SUCCESS) {
        std::cerr << "vmaCreateBuffer failed : " << result << "\n";
        exit(-1);
    }
    _buffer = vk::Buffer(buffer);
    _allocator = device.allocator();
}

Buffer::~Buffer() {
    vmaDestroyBuffer(_allocator, _buffer, _allocation);
}

void Buffer::mapData(void* data, size_t size) {
    if (size != _size) {
        std::cout << "Data given to buffer has incorrect size.\n";
    }
    void* tmp;
    vmaMapMemory(_allocator, _allocation, &tmp);
    std::memcpy(tmp, data, (uint32_t)(size));
    vmaUnmapMemory(_allocator, _allocation);
}
} // namespace render