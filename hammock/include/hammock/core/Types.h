#pragma once
#include <variant>
#include "hammock/core/Device.h"
#include "hammock/core/CoreUtils.h"

namespace hammock {

    typedef uint16_t float16_t;
    typedef float float32_t;
    typedef double float64_t;
    typedef unsigned char uchar8_t;


    // Define resource types enum
    enum class ResourceType : uint32_t {
        Invalid = 0,
        Image,
        Sampler,
        Buffer,
        MaxTypes
    };

    // Handle type that stores both resource type and ID
    class ResourceHandle {
    private:
        static constexpr uint64_t TYPE_SHIFT = 56;
        static constexpr uint64_t INDEX_MASK = (1ULL << TYPE_SHIFT) - 1;
        uint64_t packed_handle;

    public:
        ResourceHandle() : packed_handle(0) {
        }

        // Allow implicit conversion from existing ResourceHandle(uint64_t) constructor
        ResourceHandle(uint64_t id) : packed_handle(id & INDEX_MASK) {
        }

        static ResourceHandle create(ResourceType type, uint64_t resource_id) {
            ResourceHandle handle;
            handle.packed_handle = (static_cast<uint64_t>(type) << TYPE_SHIFT) | (resource_id & INDEX_MASK);
            return handle;
        }

        ResourceType getType() const {
            return static_cast<ResourceType>(packed_handle >> TYPE_SHIFT);
        }

        uint64_t getUid() const {
            return packed_handle & INDEX_MASK;
        }

        bool isValid() const {
            return packed_handle != 0 && getType() != ResourceType::Invalid;
        }

        bool operator==(const ResourceHandle &other) const {
            return packed_handle == other.packed_handle;
        }

        bool operator!=(const ResourceHandle &other) const {
            return packed_handle != other.packed_handle;
        }

        // TODO Helper for debugging
        const char *getTypeName() const {
            switch (getType()) {
                default: return "Invalid";
            }
        }
    };

    // Type mapping traits
    template<typename T>
    struct ResourceTypeTraits {
        static constexpr ResourceType type = ResourceType::Invalid;
    };

    // Base resource class
    class Resource : public NonCopyable {
    protected:
        Device &device;
        uint64_t uid;
        std::string debug_name;
        VkDeviceSize size = 0;
        bool resident; // Whether the resource is currently in GPU memory

        Resource(Device &device, uint64_t uid, const std::string &name)
            : uid(uid), debug_name(name), resident(false), device(device) {
        }

    public:
        virtual ~Resource(){}

        virtual void create() = 0;

        virtual void release() = 0;

        uint64_t getUid() const { return uid; }
        const std::string &getName() const { return debug_name; }
        bool isResident() const { return resident; }
        VkDeviceSize getSize() const { return size; } // for now
    };


    /**
     * Describes the base for the relative size
     */
    enum class RelativeSize {
        SwapChainRelative,
        FrameBufferRelative,
    };

    enum class CommandQueueFamily {
        Ignored, Graphics, Compute, Transfer
    };

    /**
    * Describes relative size of a viewport
    */
    enum class RelativeViewPortSize {
        SwapChainRelative,
        Fixed,
    };
    /**
      * Describes general buffer
      */
    struct BufferDesc {
        VkDeviceSize instanceSize;
        uint32_t instanceCount;
        VkBufferUsageFlags usageFlags;
        VmaAllocationCreateFlags allocationFlags;
        VkDeviceSize minOffsetAlignment;
        CommandQueueFamily currentQueueFamily = CommandQueueFamily::Ignored;
        std::vector<CommandQueueFamily> queueFamilies{};
        VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    };

    /**
    * Describes general image.
    */
    struct ImageDesc {
        uint32_t width, height, channels = 4, depth = 1, layers = 1, mips = 1;
        VkFormat format;
        VkImageUsageFlags usage;
        VkImageType imageType = VK_IMAGE_TYPE_2D;
        VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D;
        VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        VkClearValue clearValue = {};
        CommandQueueFamily currentQueueFamily = CommandQueueFamily::Ignored;
        std::vector<CommandQueueFamily> queueFamilies{};
        VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    };

    struct SamplerDesc {
        VkFilter magFilter = VK_FILTER_LINEAR;
        VkFilter minFilter = VK_FILTER_LINEAR;
        VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkBool32 anisotropyEnable = VK_TRUE;
        VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        uint32_t mips = 1;
        float mipLodBias = 0.0f;
    };


}
