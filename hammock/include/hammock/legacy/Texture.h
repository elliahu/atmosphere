#pragma once// TODO sampler info

#include <vulkan/vulkan.h>
#include <memory>
#include "hammock/core/Device.h"
#include "hammock/core/CoreUtils.h"

namespace hammock {



    class [[deprecated("Image should be used")]] ITexture {
    public:

        // Recommended:
        // VK_FORMAT_R8G8B8A8_UNORM for normals
        // VK_FORMAT_R8G8B8A8_SRGB for images
        VkSampler sampler = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
        int width{0}, height{0}, channels{0}, mipLevels{1};
        uint32_t layerCount{1};
        VkDescriptorImageInfo descriptor{};
        Device& device;

        ITexture(Device& device): device{device} {};

        // Not copyable or movable
        ITexture(const ITexture &) = delete;
        ITexture &operator=(const ITexture &) = delete;
        ITexture(ITexture &&) = delete;
        ITexture &operator=(ITexture &&) = delete;

        virtual ~ITexture();

        void updateDescriptor();
    };


    class [[deprecated("Image should be used")]] Texture2D final: public ITexture{
    public:

        Texture2D(Device &device)
            : ITexture(device) {
        }

         void loadFromBuffer(
            const void *buffer,
            uint32_t instanceSize,
            uint32_t width, uint32_t height, uint32_t channels,
            Device &device,
            VkFormat format,
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            uint32_t mipLevels = 1
        );

        void createSampler(const Device &device,
            VkFilter filter = VK_FILTER_LINEAR,
            VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            float numMips = 0.0f
            );

        void generateMipMaps(const Device &device, uint32_t mipLevels) const;
    };

    class [[deprecated("Image should be used")]] TextureCubeMap : public ITexture {
    public:
        // TODO implement loadFromBuffer

        // TODO to be removed
        void loadFromFiles(
            const std::vector<std::string> &filenames,
            VkFormat format,
            Device &device,
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        void createSampler(const Device &device, VkFilter filter = VK_FILTER_LINEAR);
    };

    class [[deprecated("Image should be used")]] Texture3D final: public ITexture {
    public:

        Texture3D(Device &device)
            : ITexture(device) {
        }

        int depth{0};

        // Recommended format VK_FORMAT_R8_UNORM
        void loadFromBuffer(Device &device,
            const void * buffer,
            VkDeviceSize instanceSize,
            uint32_t width,
            uint32_t height,
            uint32_t channels,
            uint32_t depth,
            VkFormat format,
            VkImageLayout imageLayout);

        void createSampler(Device& device,
            VkFilter filter = VK_FILTER_LINEAR,
            VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    };
}
