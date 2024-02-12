#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "instance.hh"
#include "device.hh"

#include "vk_mem_alloc.h"

namespace render {
class Buffer {
private:
    size_t _size;
    vk::Buffer _stagingBuffer;
    VmaAllocation _stagingAllocation;
    vk::Buffer _buffer;
    VmaAllocation _allocation;
    VmaAllocator _allocator;

public:
    Buffer(const render::Device& device, size_t size, VkBufferUsageFlags usage);
    ~Buffer();
    void mapData(const render::Device& device, void* data, size_t size);

    const vk::Buffer& buffer() const {
        return _buffer;
    }
};

class HostBuffer {
private:
    size_t _size;
    vk::Buffer _buffer;
    VmaAllocation _allocation;
    void *_mapBinding;
    VmaAllocator _allocator;

public:
    HostBuffer(const render::Device& device, size_t size, VkBufferUsageFlags usage);
    ~HostBuffer();
    void mapData(const render::Device& device, void* data, size_t size);

    const vk::Buffer& buffer() const {
        return _buffer;
    }
};
} // namespace render