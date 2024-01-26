#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vector>

#include "instance.hh"
#include "device.hh"

#include "vk_mem_alloc.h"

namespace render {
class Buffer {
private:
    size_t _size;
    vk::Buffer _buffer;
    VmaAllocation _allocation;
    VmaAllocator _allocator;

public:
    Buffer(const render::Device& device, size_t size, VkBufferUsageFlags usage,
           std::vector<uint32_t> queueFamilyIndices);
    ~Buffer();
    void mapData(void* data, size_t size);

    const vk::Buffer& buffer() const {
        return _buffer;
    }
};
} // namespace render