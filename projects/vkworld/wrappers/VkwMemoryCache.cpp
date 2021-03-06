#include "VkwMemoryCache.h"

#include <vulkan/vulkan.hpp>

#include "Vulkan.h"

namespace world {


// SEGMENT

VkwMemoryCacheSegment::VkwMemoryCacheSegment(u32 size,
                                             DescriptorType descriptorType,
                                             u32 memTypeIndex)
        : _data(new char[size]), _totalSize(size), _sizeAllocated(0),
          _updated(false) {

    VulkanContext &ctx = Vulkan::context();
    auto properties = ctx._physicalDevice.getProperties();

    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = size;

    switch (descriptorType) {
    case DescriptorType::STORAGE_BUFFER:
        bufferInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer;
        _alignment = properties.limits.minStorageBufferOffsetAlignment;
        break;
    case DescriptorType::UNIFORM_BUFFER:
        bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
        _alignment = properties.limits.minUniformBufferOffsetAlignment;
        break;
    case DescriptorType::VERTEX_BUFFER:
        bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
        break;
    case DescriptorType::INDEX_BUFFER:
        bufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;
        break;
    case DescriptorType ::TRANSFER_DST:
        bufferInfo.usage = vk::BufferUsageFlagBits ::eTransferDst;
        break;
    case DescriptorType ::TRANSFER_SRC:
        bufferInfo.usage = vk::BufferUsageFlagBits ::eTransferSrc;
    }

    // TODO handle concurrent shared buffer
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    _buffer = ctx._device.createBuffer(bufferInfo, nullptr);

    // create memory
    vk::MemoryRequirements memRequirements =
        ctx._device.getBufferMemoryRequirements(_buffer);


    vk::MemoryAllocateInfo memoryInfo = {};
    memoryInfo.pNext = nullptr;
    memoryInfo.allocationSize = memRequirements.size;
    memoryInfo.memoryTypeIndex = memTypeIndex;
    // TODO check if GPU_ONLY memory is supported

    _memory = ctx._device.allocateMemory(memoryInfo);

    ctx._device.bindBufferMemory(_buffer, _memory, 0);
}

VkwMemoryCacheSegment::~VkwMemoryCacheSegment() {
    VulkanContext &ctx = Vulkan::context();
    if (_buffer) {
        ctx._device.destroy(_buffer);
    }

    ctx._device.free(_memory);
}

void VkwMemoryCacheSegment::setData(const void *data, u32 count, u32 offset) {
    VulkanContext &ctx = Vulkan::context();

    void *mapped = ctx._device.mapMemory(_memory, offset, count);
    memcpy(mapped, data, count);
    ctx._device.unmapMemory(_memory);
}

void VkwMemoryCacheSegment::getData(void *data, u32 count, u32 offset) {
    VulkanContext &ctx = Vulkan::context();

    void *mapped = ctx._device.mapMemory(_memory, offset, count);
    memcpy(data, mapped, count);
    ctx._device.unmapMemory(_memory);
}


// MEMORY CACHE

VkwMemoryCache::VkwMemoryCache(u32 segmentSize, DescriptorType usage,
                               MemoryUsage memUse)
        : _segmentSize(segmentSize), _usage(usage), _memType(memUse) {
    VulkanContext &ctx = Vulkan::context();

    _memTypeIndex = ctx.getMemoryType(memUse, segmentSize);
}

// https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html
// #VUID-VkWriteDescriptorSet-descriptorType-00327
// TODO Check for buffer max size and alignment according to specs above
u32 VkwMemoryCache::allocate(u32 size) {
    const u32 segmentSize = _segmentSize;

    if (size > segmentSize) {
        throw std::invalid_argument("Requested size " + std::to_string(size) +
                                    " > Segment size " +
                                    std::to_string(segmentSize));
    }

    if (sizeRemaining() < size) {
        _segments.emplace_back(std::make_unique<VkwMemoryCacheSegment>(
            segmentSize, _usage, _memTypeIndex));
    }

    auto &lastSegment = *_segments.back();
    const u32 offset = lastSegment._sizeAllocated;
    lastSegment._sizeAllocated += size;

    // Complete to respect alignment required by standard
    u32 rem = lastSegment._sizeAllocated % lastSegment._alignment;

    if (rem != 0) {
        lastSegment._sizeAllocated += lastSegment._alignment - rem;
    }
    return offset;
}

VkwSubBuffer VkwMemoryCache::allocateBuffer(u32 size) {
    u32 offset = allocate(size);
    auto &lastSegment = *_segments.back();
    return VkwSubBuffer{lastSegment, lastSegment._buffer, size, offset};
}

void VkwMemoryCache::allocateImage(VkwImage &image) {}

void VkwMemoryCache::flush() {
    // Nothing yet
}

u32 VkwMemoryCache::sizeRemaining() const {
    if (_segments.empty()) {
        return 0;
    }

    auto &lastSegment = *_segments.back();
    return lastSegment._totalSize - lastSegment._sizeAllocated;
}
} // namespace world