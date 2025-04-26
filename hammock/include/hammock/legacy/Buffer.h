#pragma once

#include "hammock/core/Device.h"

namespace hammock {

    class [[deprecated("Buffer should be used")]] LegacyBuffer {
    public:
        LegacyBuffer(
            Device &device,
            VkDeviceSize instanceSize,
            uint32_t instanceCount,
            VkBufferUsageFlags usageFlags,
            VkMemoryPropertyFlags memoryPropertyFlags,
            VkDeviceSize minOffsetAlignment = 1);

        ~LegacyBuffer();

        LegacyBuffer(const LegacyBuffer &) = delete;

        LegacyBuffer &operator=(const LegacyBuffer &) = delete;

        VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        void unmap();

        void destroy();

        void writeToBuffer(const void *data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

        VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

        VkDescriptorBufferInfo descriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

        VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

        void writeToIndex(const void *data, int index) const;

        VkResult flushIndex(int index) const;

        VkDescriptorBufferInfo descriptorInfoForIndex(int index) const;

        VkResult invalidateIndex(int index) const;

        [[nodiscard]] VkBuffer getBuffer() const { return buffer; }
        [[nodiscard]] void *getMappedMemory() const { return mapped; }
        [[nodiscard]] uint32_t getInstanceCount() const { return instanceCount; }
        [[nodiscard]] VkDeviceSize getInstanceSize() const { return instanceSize; }
        [[nodiscard]] VkDeviceSize getAlignmentSize() const { return instanceSize; }
        [[nodiscard]] VkBufferUsageFlags getUsageFlags() const { return usageFlags; }
        [[nodiscard]] VkMemoryPropertyFlags getMemoryPropertyFlags() const { return memoryPropertyFlags; }
        [[nodiscard]] VkDeviceSize getBufferSize() const { return bufferSize; }

    private:
        static VkDeviceSize getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);

        Device &device;
        void *mapped = nullptr;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;

        VkDeviceSize bufferSize;
        uint32_t instanceCount;
        VkDeviceSize instanceSize;
        VkDeviceSize alignmentSize;
        VkBufferUsageFlags usageFlags;
        VkMemoryPropertyFlags memoryPropertyFlags;
    };
}
