#pragma once
#include <algorithm>
#include <array>
#include <iterator>
#include <vector>
#include <cassert>
#include <vulkan/vulkan.h>

#include "Device.h"
#include "hammock/core/CoreUtils.h"

#define VK_FLAGS_NONE 0

namespace hammock {
    // Loosely based of framebuffer abstraction
    // https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanFrameBuffer.hpp by Sascha Willems

    struct FramebufferAttachment {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
        VkFormat format;
        VkImageSubresourceRange subresourceRange;
        VkAttachmentDescription description;

        /**
        * @brief Returns true if the attachment has a depth component
        */
        [[nodiscard]] bool hasDepth() const {
            std::vector<VkFormat> formats =
            {
                VK_FORMAT_D16_UNORM,
                VK_FORMAT_X8_D24_UNORM_PACK32,
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D16_UNORM_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
            };
            return std::ranges::find(formats, format) != std::end(formats);
        }

        /**
        * @brief Returns true if the attachment has a stencil component
        */
        [[nodiscard]] bool hasStencil() const {
            std::vector<VkFormat> formats =
            {
                VK_FORMAT_S8_UINT,
                VK_FORMAT_D16_UNORM_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
            };
            return std::ranges::find(formats, format) != std::end(formats);
        }

        /**
        * @brief Returns true if the attachment is a depth and/or stencil attachment
        */
        [[nodiscard]] bool isDepthStencil() const {
            return (hasDepth() || hasStencil());
        }

        VkDescriptorImageInfo getDescriptorImageInfo(const VkSampler sampler, const VkImageLayout layout) const {
            return {
                .sampler = sampler,
                .imageView = view,
                .imageLayout = layout
            };
        }
    };

    /**
    * @brief Describes the attributes of an attachment to be created
    */
    struct FramebufferAttachmentCreateInfo {
        uint32_t width, height;
        uint32_t layerCount;
        VkFormat format;
        VkImageUsageFlags usage;
        VkSampleCountFlagBits imageSampleCount = VK_SAMPLE_COUNT_1_BIT;
        VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    /**
    * @brief Encapsulates a complete Vulkan framebuffer with an arbitrary number and combination of attachments
    */
    struct Framebuffer {
        struct FramebufferCreateInfo {
            Device &device;
            uint32_t width = 0, height = 0;

            struct FrameBufferSamplerInfo {
                VkFilter magFilter = VK_FILTER_LINEAR;
                VkFilter minFilter = VK_FILTER_LINEAR;
                VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            } sampler;

            std::vector<FramebufferAttachmentCreateInfo> attachments;
        };

    public:
        uint32_t width = 0, height = 0;
        VkFramebuffer framebuffer = nullptr;
        VkRenderPass renderPass = nullptr;
        VkSampler sampler = nullptr;
        std::vector<FramebufferAttachment> attachments;
        std::vector<FramebufferAttachment> sharedAttachments;

        /**
        * Default constructor
        *
        * @param device Device instance
        * @param vulkanDevice Pointer to a valid VulkanDevice
        */
        explicit Framebuffer(Device &device) : device{device} {
            assert(&device && "Failed to create Framebuffer - device is NULL!");
        }

        /**
        * Destroy and free Vulkan resources used for the framebuffer and all of its attachments
        */
        ~Framebuffer() {
            for (const auto [image, memory, view, format, subresourceRange, description]: attachments) {
                vkDestroyImage(device.device(), image, nullptr);
                vkDestroyImageView(device.device(), view, nullptr);
                vkFreeMemory(device.device(), memory, nullptr);
            }
            vkDestroySampler(device.device(), sampler, nullptr);
            vkDestroyRenderPass(device.device(), renderPass, nullptr);
            vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
        }

        /**
        * Create framebuffer with renderpass and attachments using configuration struct
        * @param createInfo Configuration struct
        * @return created Framebuffer
        */
        static Framebuffer createFramebuffer(FramebufferCreateInfo createInfo) {
            Framebuffer fb{createInfo.device};
            fb.width = createInfo.width;
            fb.height = createInfo.height;
            checkResult(fb.createSampler(createInfo.sampler.magFilter, createInfo.sampler.minFilter,
                                         createInfo.sampler.addressMode));
            for (const FramebufferAttachmentCreateInfo &at: createInfo.attachments) {
                fb.addAttachment(at);
            }
            checkResult(fb.createRenderPass());
            return fb;
        }

        /**
        * Create framebuffer smart pointer with renderpass and attachments using configuration struct
        * @param createInfo Configuration struct
        * @return created Framebuffer smart pointer
        */
        static std::unique_ptr<Framebuffer> createFramebufferPtr(FramebufferCreateInfo createInfo) {
            auto fb = std::make_unique<Framebuffer>(createInfo.device);
            fb->width = createInfo.width;
            fb->height = createInfo.height;
            checkResult(fb->createSampler(createInfo.sampler.magFilter, createInfo.sampler.minFilter,
                                          createInfo.sampler.addressMode));
            for (const FramebufferAttachmentCreateInfo &at: createInfo.attachments) {
                fb->addAttachment(at);
            }
            checkResult(fb->createRenderPass());
            return fb;
        }

        /**
        * Create framebuffer smart pointer with renderpass and attachments using configuration struct
        * @param createInfo Configuration struct
        * @param sharedAttachments Attachments belonging to a different render pass
        * @return created Framebuffer smart pointer
        * TODO allow for custom order
        */
        static std::unique_ptr<Framebuffer> createFrameBufferPtrWithSharedAttachments(FramebufferCreateInfo createInfo, const std::vector<std::reference_wrapper<hammock::FramebufferAttachment>>& sharedAttachments) {
            auto fb = std::make_unique<Framebuffer>(createInfo.device);
            fb->width = createInfo.width;
            fb->height = createInfo.height;
            checkResult(fb->createSampler(createInfo.sampler.magFilter, createInfo.sampler.minFilter,
                                          createInfo.sampler.addressMode));
            for (const FramebufferAttachmentCreateInfo &at: createInfo.attachments) {
                fb->addAttachment(at);
            }
            for (const auto& shared_at: sharedAttachments) {
                FramebufferAttachment& at = shared_at.get();
                at.description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                at.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                if(at.hasDepth()) {
                    at.description.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL  ;
                }
                fb->sharedAttachments.push_back(at);
            }
            checkResult(fb->createRenderPass());
            return fb;
        }

        /**
        * Add a new attachment described by createinfo to the framebuffer's attachment list
        *
        * @param createinfo Structure that specifies the framebuffer to be constructed
        *
        * @return Index of the new attachment
        */
        uint32_t addAttachment(FramebufferAttachmentCreateInfo createinfo) {
            FramebufferAttachment attachment{};

            attachment.format = createinfo.format;

            VkImageAspectFlags aspectMask = VK_FLAGS_NONE;

            // Select aspect mask and layout depending on usage

            // Color attachment
            if (createinfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
                aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            }

            // Depth (and/or stencil) attachment
            if (createinfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                if (attachment.hasDepth()) {
                    aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                }
                if (attachment.hasStencil()) {
                    aspectMask = aspectMask | VK_IMAGE_ASPECT_STENCIL_BIT;
                }
            }

            assert(aspectMask > 0);

            VkImageCreateInfo image = hammock::Init::imageCreateInfo();
            image.imageType = VK_IMAGE_TYPE_2D;
            image.format = createinfo.format;
            image.extent.width = createinfo.width;
            image.extent.height = createinfo.height;
            image.extent.depth = 1;
            image.mipLevels = 1;
            image.arrayLayers = createinfo.layerCount;
            image.samples = createinfo.imageSampleCount;
            image.tiling = VK_IMAGE_TILING_OPTIMAL;
            image.usage = createinfo.usage;

            VkMemoryAllocateInfo memAlloc = hammock::Init::memoryAllocateInfo();
            VkMemoryRequirements memReqs;

            // Create image for this attachment
            if (vkCreateImage(device.device(), &image, nullptr, &attachment.image) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image");
            }
            vkGetImageMemoryRequirements(device.device(), attachment.image, &memReqs);
            memAlloc.allocationSize = memReqs.size;
            memAlloc.memoryTypeIndex = device.findMemoryType(memReqs.memoryTypeBits,
                                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            if (vkAllocateMemory(device.device(), &memAlloc, nullptr, &attachment.memory) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate memory");
            }
            if (vkBindImageMemory(device.device(), attachment.image, attachment.memory, 0) != VK_SUCCESS) {
                throw std::runtime_error("Failed to bind memory");
            }

            attachment.subresourceRange = {};
            attachment.subresourceRange.aspectMask = aspectMask;
            attachment.subresourceRange.levelCount = 1;
            attachment.subresourceRange.layerCount = createinfo.layerCount;

            VkImageViewCreateInfo imageView = hammock::Init::imageViewCreateInfo();
            imageView.viewType = (createinfo.layerCount == 1) ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            imageView.format = createinfo.format;
            imageView.subresourceRange = attachment.subresourceRange;
            //TODO: workaround for depth+stencil attachments
            imageView.subresourceRange.aspectMask = (attachment.hasDepth()) ? VK_IMAGE_ASPECT_DEPTH_BIT : aspectMask;
            imageView.image = attachment.image;
            if (vkCreateImageView(device.device(), &imageView, nullptr, &attachment.view) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image view");
            }

            // Fill attachment description
            attachment.description = {};
            attachment.description.samples = createinfo.imageSampleCount;
            attachment.description.loadOp = createinfo.loadOp;
            attachment.description.storeOp = (createinfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT)
                                                 ? VK_ATTACHMENT_STORE_OP_STORE
                                                 : createinfo.storeOp;
            attachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.description.format = createinfo.format;
            attachment.description.initialLayout = createinfo.initialLayout;
            // Final layout
            // If not, final layout depends on attachment type
            if (attachment.hasDepth() || attachment.hasStencil()) {
                attachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            } else {
                attachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }

            attachments.push_back(attachment);

            return static_cast<uint32_t>(attachments.size() - 1);
        }

        /**
        * Creates a default sampler for sampling from any of the framebuffer attachments
        * Applications are free to create their own samplers for different use cases
        *
        * @param magFilter Magnification filter for lookups
        * @param minFilter Minification filter for lookups
        * @param adressMode Addressing mode for the U,V and W coordinates
        *
        * @return VkResult for the sampler creation
        */
        VkResult createSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode adressMode) {
            VkSamplerCreateInfo samplerInfo = hammock::Init::samplerCreateInfo();
            samplerInfo.magFilter = magFilter;
            samplerInfo.minFilter = minFilter;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.addressModeU = adressMode;
            samplerInfo.addressModeV = adressMode;
            samplerInfo.addressModeW = adressMode;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.maxAnisotropy = 1.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = 1.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            return vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler);
        }

        /**
        * Creates a default render pass setup with one sub pass
        *
        * @return VK_SUCCESS if all resources have been created successfully
        */
        VkResult createRenderPass() {
            assert(width > 0 && height > 0 && "Cannot create renderpass - width has to > 0, height has to be > 0");

            std::vector<std::reference_wrapper<FramebufferAttachment>> _attachments{};
            for(auto& attachment : attachments) {_attachments.push_back(attachment);}
            for (auto& attachment : sharedAttachments) {_attachments.push_back(attachment);}

            std::vector<VkAttachmentDescription> attachmentDescriptions;
            for (auto &attachment: _attachments) {
                attachmentDescriptions.push_back(attachment.get().description);
            };

            // Collect attachment references
            std::vector<VkAttachmentReference> colorReferences;
            VkAttachmentReference depthReference = {};
            bool hasDepth = false;
            bool hasColor = false;

            uint32_t attachmentIndex = 0;

            for (auto &attachment: _attachments) {
                if (attachment.get().isDepthStencil()) {
                    // Only one depth attachment allowed
                    assert(!hasDepth);
                    depthReference.attachment = attachmentIndex;
                    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    hasDepth = true;
                } else {
                    colorReferences.push_back({attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
                    hasColor = true;
                }
                attachmentIndex++;
            };

            // Default render pass setup uses only one subpass
            VkSubpassDescription subpass = {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            if (hasColor) {
                subpass.pColorAttachments = colorReferences.data();
                subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
            }
            if (hasDepth) {
                subpass.pDepthStencilAttachment = &depthReference;
            }

            // Use subpass dependencies for attachment layout transitions
            std::array<VkSubpassDependency, 2> dependencies{};
            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            dependencies[1].srcSubpass = 0;
            dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            // Create render pass
            VkRenderPassCreateInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.pAttachments = attachmentDescriptions.data();
            renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 2;
            renderPassInfo.pDependencies = dependencies.data();
            if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create render pass");
            }

            std::vector<VkImageView> attachmentViews;
            for (auto& at: _attachments) {
                attachmentViews.push_back(at.get().view);
            }

            // Find. max number of layers across attachments
            uint32_t maxLayers = 0;
            for (const auto&at: _attachments) {
                if (at.get().subresourceRange.layerCount > maxLayers) {
                    maxLayers = at.get().subresourceRange.layerCount;
                }
            }

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.pAttachments = attachmentViews.data();
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
            framebufferInfo.width = width;
            framebufferInfo.height = height;
            framebufferInfo.layers = maxLayers;
            if (vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer");
            }

            return VK_SUCCESS;
        }

    private:
        Device &device;
    };
}
