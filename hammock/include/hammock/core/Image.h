#pragma once
#include "hammock/core/Types.h"
#include "hammock/core/CoreUtils.h"

namespace hammock {
    class Image : public Resource {
    protected:
        // Format and usage
        VkFormat m_format;
        VkImageUsageFlags m_usage;
        VkImageType m_type;
        VkImageViewType m_viewType;

        // Image dimensions
        uint32_t m_width, m_height, m_channels, m_depth = 1, m_layers = 1, m_mips = 1;

        // Vulkan handles
        VkImage m_image = VK_NULL_HANDLE;
        VkImageView m_view = VK_NULL_HANDLE;
        VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VmaAllocation m_allocation = VK_NULL_HANDLE;

        // Attachment
        VkClearValue m_clearValue = {};

        CommandQueueFamily m_queueFamily;
        std::vector<uint32_t> m_queueFamilyIndices{};
        VkSharingMode m_sharingMode;

        VkImageTiling m_tiling;
        VkMemoryPropertyFlags m_memoryFlags;

        VkImageAspectFlags m_aspectFlags;

    public:
        Image(Device &device, uint64_t id, const std::string &name, const ImageDesc &desc) : Resource(
            device, id, name) {
            // Fromat and usage
            m_format = desc.format;
            m_usage = desc.usage;
            m_type = desc.imageType;
            m_viewType = desc.imageViewType;
            m_aspectFlags = desc.aspectFlags;

            // Image dimensions
            m_width = desc.width;
            m_height = desc.height;
            m_channels = desc.channels;
            m_depth = desc.depth;
            m_layers = desc.layers;
            m_mips = desc.mips;

            m_queueFamily = desc.currentQueueFamily;
            // Vulkan handles are created in load function on demand

            // attachment
            m_clearValue = desc.clearValue;

            // queue family indices
            for (auto &family: desc.queueFamilies) {
                if (family == CommandQueueFamily::Graphics) m_queueFamilyIndices.push_back(device.getGraphicsQueueFamilyIndex());
                if (family == CommandQueueFamily::Compute) m_queueFamilyIndices.push_back(device.getComputeQueueFamilyIndex());
                if (family == CommandQueueFamily::Transfer) m_queueFamilyIndices.push_back(device.getTransferQueueFamilyIndex());
            }

            m_sharingMode = desc.sharingMode;

            m_tiling = desc.tiling;

            m_memoryFlags = desc.memoryFlags;


            // Check for support
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(device.getPhysicalDevice(), m_format, &formatProperties);

            ASSERT(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_DST_BIT,
                   "Device does not support flag TRANSFER_DST for selected format!");

            if (m_type == VK_IMAGE_TYPE_3D) {
                uint32_t maxImageDimension3D(device.properties.limits.maxImageDimension3D);
                ASSERT(
                    m_width <= maxImageDimension3D && m_height <= maxImageDimension3D && m_depth <=
                    maxImageDimension3D,
                    "Requested 3D texture dimensions are greater than supported 3D texture dimension!");
            }
        }

        ~Image() override {
            if (isResident()) {
                release();
            }
        }

        [[nodiscard]] VkImageLayout getLayout() const { return m_layout; }
        [[nodiscard]] VkImage getImage() const { return m_image; }
        [[nodiscard]] VkImageView getView() const { return m_view; }
        [[nodiscard]] VkFormat getFormat() const { return m_format; }
        [[nodiscard]] uint32_t getMipLevel() const { return m_mips; }
        [[nodiscard]] uint32_t getLayerLevel() const { return m_layers; }
        [[nodiscard]] CommandQueueFamily getQueueFamily() const { return m_queueFamily; }
        [[nodiscard]] VkExtent3D getExtent() const { return {m_width, m_height, m_depth}; }

        [[nodiscard]] VkRenderingAttachmentInfo getRenderingAttachmentInfo() const {
            return {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = m_view,
                .imageLayout = m_layout,
                .clearValue = m_clearValue
            };
        }

        [[nodiscard]] VkDescriptorImageInfo getDescriptorImageInfo(VkSampler sampler) const {
            return {
                .sampler = sampler,
                .imageView = m_view,
                .imageLayout = m_layout,
            };
        }

        VkImageAspectFlags getAspectMask() const {
            return isDepthStencilImage()
                       ? VK_IMAGE_ASPECT_DEPTH_BIT
                       : VK_IMAGE_ASPECT_COLOR_BIT;
        };

        [[nodiscard]] VkSampler createAndGetSampler() const {
            VkSampler sampler = VK_NULL_HANDLE;
            VkSamplerCreateInfo samplerInfo = Init::samplerCreateInfo();
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = static_cast<float>(m_mips);

            vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler);

            return sampler;
        }

        /**
         * Transitions to new layout. Transition is recorder to separate command buffer that is submitted after at the end of the call.
         * Might cause sync hazard. Do not call in frame.
         * @param newLayout New layout
         */
        void queueImageLayoutTransition(VkImageLayout newLayout) {
            if (newLayout == m_layout) { return; }
            device.transitionImageLayout(m_image, m_layout, newLayout, m_layers, 0, m_mips, 0, getAspectMask());
            m_layout = newLayout;
        }


        VkImageSubresourceRange getSubresourceRange(uint32_t baseMipLevel = 0, uint32_t baseArrayLayer = 0) const {
            return {
                .aspectMask = getAspectMask(),
                .baseMipLevel = baseMipLevel,
                .levelCount = m_mips,
                .baseArrayLayer = baseArrayLayer,
                .layerCount = m_layers
            };
        }

        void pipelineBarrier(
            VkCommandBuffer cmd,
            VkPipelineStageFlags2 srcStageMask,
            VkAccessFlags2 srcAccessMask,
            VkPipelineStageFlags2 dstStageMask,
            VkAccessFlags2 dstAccessMask,
            VkImageLayout oldLayout,
            VkImageLayout newLayout,
            uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex) {
            // Subresource range
            VkImageSubresourceRange subresourceRange = getSubresourceRange();

            VkImageMemoryBarrier2 imageBarrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = srcStageMask,
                .srcAccessMask = srcAccessMask,
                .dstStageMask = dstStageMask,
                .dstAccessMask = dstAccessMask,
                .oldLayout = oldLayout,
                .newLayout = newLayout, // Optional: layout transition
                .srcQueueFamilyIndex = srcQueueFamilyIndex,
                .dstQueueFamilyIndex = dstQueueFamilyIndex,
                .image = m_image,
                .subresourceRange = subresourceRange,
            };

            VkDependencyInfo depInfo = {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &imageBarrier,
            };

            vkCmdPipelineBarrier2(cmd, &depInfo);

            m_layout = newLayout;
        }

         /**
         * Transitions to new layout. Transition is recorder in the command buffer.
         * @param cmd Command buffer
         * @param newLayout New layout
         */
        void transition(VkCommandBuffer cmd, VkImageLayout newLayout,
                        CommandQueueFamily newQueueFamily = CommandQueueFamily::Ignored) {
            transition(cmd, m_layout, newLayout, newQueueFamily);
        }

        /**
         * Transitions to new layout. Transition is recorder in the command buffer.
         * @param cmd Command buffer
         * @param oldLayout Old layout
         * @param newLayout New layout
         */
        void transition(VkCommandBuffer cmd,VkImageLayout oldLayout, VkImageLayout newLayout,
                       CommandQueueFamily newQueueFamily = CommandQueueFamily::Ignored) {
            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = getAspectMask();
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = m_mips;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount = m_layers;

            uint32_t oldFamily = VK_QUEUE_FAMILY_IGNORED;
            uint32_t newFamily = VK_QUEUE_FAMILY_IGNORED;

            if (newQueueFamily == CommandQueueFamily::Graphics) {
                newFamily = device.getGraphicsQueueFamilyIndex();
            }

            if (newQueueFamily == CommandQueueFamily::Compute) {
                newFamily = device.getComputeQueueFamilyIndex();
            }

            if (newQueueFamily == CommandQueueFamily::Transfer) {
                newFamily = device.getTransferQueueFamilyIndex();
            }

            if (m_queueFamily == CommandQueueFamily::Graphics && newFamily != VK_QUEUE_FAMILY_IGNORED) {
                oldFamily = device.getGraphicsQueueFamilyIndex();
            }

            if (m_queueFamily == CommandQueueFamily::Compute && newFamily != VK_QUEUE_FAMILY_IGNORED) {
                oldFamily = device.getComputeQueueFamilyIndex();
            }

            if (m_queueFamily == CommandQueueFamily::Transfer && newFamily != VK_QUEUE_FAMILY_IGNORED) {
                oldFamily = device.getTransferQueueFamilyIndex();
            }

            transitionImageLayout(cmd, m_image, oldLayout, newLayout, subresourceRange,
                                  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, oldFamily,
                                  newFamily);

            if (newFamily != VK_QUEUE_FAMILY_IGNORED) {
                m_queueFamily = newQueueFamily;
            }

            m_layout = newLayout;
        }
        

        /**
         * Copy data from buffer into this image
         * @param buffer Buffer to copy from
         */
        void queueCopyFromBuffer(VkBuffer buffer) {
            device.copyBufferToImage(
                buffer,
                m_image,
                m_width,
                m_height,
                m_layers,
                0, m_depth
            );
        }

        /**
         * Creates the resource on device. This is called when ever this resource is requested and is not resident
         */
        void create() override {
            Logger::log(LOG_LEVEL_DEBUG, "Creating image %s\n", getName().c_str());
            // Create the image
            VkImageCreateInfo imageCreateInfo = Init::imageCreateInfo();
            imageCreateInfo.imageType = m_type;
            imageCreateInfo.format = m_format;
            imageCreateInfo.mipLevels = m_mips;
            imageCreateInfo.arrayLayers = m_layers;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.tiling = m_tiling;
            imageCreateInfo.sharingMode = m_sharingMode;
            imageCreateInfo.queueFamilyIndexCount = m_queueFamilyIndices.size();
            imageCreateInfo.pQueueFamilyIndices = m_queueFamilyIndices.data();
            imageCreateInfo.extent.width = m_width;
            imageCreateInfo.extent.height = m_height;
            imageCreateInfo.extent.depth = m_depth;
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageCreateInfo.usage = m_usage;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocInfo.requiredFlags = m_memoryFlags;

            checkResult(vmaCreateImage(device.allocator(), &imageCreateInfo, &allocInfo, &m_image, &m_allocation,
                                       nullptr));

            // create image view
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_image;
            viewInfo.viewType = m_viewType;
            viewInfo.format = m_format;
            viewInfo.subresourceRange.aspectMask = m_aspectFlags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = m_mips;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = m_layers;

            checkResult(vkCreateImageView(device.device(), &viewInfo, nullptr, &m_view));

            resident = true;
        }

        /**
         * Clears the resource from device memory. This is called if the resource is being destroyed ot space is needed in device memory.
         */
        void release() override {
            Logger::log(LOG_LEVEL_DEBUG, "Releasing image %s\n", getName().c_str());
            if (m_image != VK_NULL_HANDLE) {
                vmaDestroyImage(device.allocator(), m_image, m_allocation);
            }

            if (m_view != VK_NULL_HANDLE) {
                vkDestroyImageView(device.device(), m_view, nullptr);
            }
            resident = false;
        }

        /**
         * Generates mip map chain for this image
         */
        void generateMips() {
            VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

            transitionImageLayout(commandBuffer, m_image, VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {
                                      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                      .baseMipLevel = 0,
                                      .levelCount = m_mips,
                                      .layerCount = 1
                                  });

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image = m_image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;

            auto mipWidth = static_cast<int32_t>(m_width);
            auto mipHeight = static_cast<int32_t>(m_height);

            for (uint32_t i = 1; i < m_mips; i++) {
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier(commandBuffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);

                VkImageBlit blit{};
                blit.srcOffsets[0] = {0, 0, 0};
                blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount = 1;
                blit.dstOffsets[0] = {0, 0, 0};
                blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount = 1;

                vkCmdBlitImage(commandBuffer,
                               m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &blit,
                               VK_FILTER_LINEAR);

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(commandBuffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);

                if (mipWidth > 1) mipWidth /= 2;
                if (mipHeight > 1) mipHeight /= 2;
            }

            barrier.subresourceRange.baseMipLevel = m_mips - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            device.endSingleTimeCommands(commandBuffer);
            vkQueueWaitIdle(device.graphicsQueue());
        }

        bool isDepthImage() const {
            return isDepthFormat(m_format);
        }

        bool isSteniclImage() const {
            return isStencilFormat(m_format);
        }

        bool isDepthStencilImage() const {
            return isDepthStencil(m_format);
        }
    };

    template<>
    struct ResourceTypeTraits<Image> {
        static constexpr ResourceType type = ResourceType::Image;
    };

    class Sampler : public Resource {
        VkSampler m_sampler = VK_NULL_HANDLE;
        VkFilter magFilter;
        VkFilter minFilter;
        VkSamplerAddressMode addressModeU;
        VkSamplerAddressMode addressModeV;
        VkSamplerAddressMode addressModeW;
        VkBool32 anisotropyEnable;
        float maxAnisotropy;
        VkBorderColor borderColor;
        VkSamplerMipmapMode mipmapMode;
        uint32_t mips;
        float mipLodBias;

    public:
        Sampler(Device &device, uint64_t id, const std::string &name, const SamplerDesc &desc) : Resource(
            device, id, name) {
            magFilter = desc.magFilter;
            minFilter = desc.minFilter;
            addressModeU = desc.addressModeU;
            addressModeV = desc.addressModeV;
            addressModeW = desc.addressModeW;
            anisotropyEnable = desc.anisotropyEnable;
            borderColor = desc.borderColor;
            mipmapMode = desc.mipmapMode;
            mips = desc.mips;
            mipLodBias = desc.mipLodBias;

            // retrieve max anisotropy from physical device
            maxAnisotropy = device.properties.limits.maxSamplerAnisotropy;
        }

        ~Sampler() {
            if (resident) {
                Sampler::release();
            }
        }

        void create() override {
            Logger::log(LOG_LEVEL_DEBUG, "Creating sampler %s\n", getName().c_str());
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = magFilter;
            samplerInfo.minFilter = minFilter;
            samplerInfo.addressModeU = addressModeU;
            samplerInfo.addressModeV = addressModeV;
            samplerInfo.addressModeW = addressModeW;
            samplerInfo.anisotropyEnable = anisotropyEnable;
            samplerInfo.maxAnisotropy = maxAnisotropy;
            samplerInfo.borderColor = borderColor;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = mipmapMode;
            samplerInfo.mipLodBias = mipLodBias;
            samplerInfo.minLod = 0.0;
            samplerInfo.maxLod = mips;

            checkResult(vkCreateSampler(device.device(), &samplerInfo, nullptr, &m_sampler));
            resident = true;
        }

        void release() override {
            Logger::log(LOG_LEVEL_DEBUG, "Releasing sampler %s\n", getName().c_str());
            if (m_sampler != VK_NULL_HANDLE) {
                vkDestroySampler(device.device(), m_sampler, nullptr);
            }
            resident = false;
        }

        [[nodiscard]] VkSampler getSampler() const {
            return m_sampler;
        }
    };

    template<>
    struct ResourceTypeTraits<Sampler> {
        static constexpr ResourceType type = ResourceType::Sampler;
    };
}
