#include "hammock/legacy/Texture.h"

#include <format>
#include <stb_image.h>
#include <string.h>

#include "hammock/legacy/Buffer.h"
#include "../../../include/hammock/core/CoreUtils.h"


hammock::ITexture::~ITexture() {
    // if image have sampler, destroy it
    if (sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device.device(), sampler, nullptr);
        sampler = VK_NULL_HANDLE;
    }

    vkDestroyImageView(device.device(), view, nullptr);
    vkDestroyImage(device.device(), image, nullptr);
    vkFreeMemory(device.device(), memory, nullptr);
}

void hammock::ITexture::updateDescriptor() {
    descriptor.sampler = sampler;
    descriptor.imageView = view;
    descriptor.imageLayout = layout;
}

void hammock::TextureCubeMap::loadFromFiles(const std::vector<std::string> &filenames, const VkFormat format,
                                         Device &device, const VkImageLayout imageLayout) {
    std::vector<stbi_uc *> pixels{6};
    stbi_set_flip_vertically_on_load(true);
    for (int i = 0; i < filenames.size(); i++) {
        pixels[i] = stbi_load(filenames[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels[i]) {
            throw std::runtime_error("Could not load image from disk");
        }
    }
    stbi_set_flip_vertically_on_load(false);
    layout = imageLayout;

    VkImageCreateInfo imageCreateInfo = Init::imageCreateInfo();
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format; // Specify the format of the image
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 6; // Number of faces in the cubemap
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    device.createImageWithInfo(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);

    LegacyBuffer stagingBuffer
    {
        device,
        sizeof(pixels[0][0]),
        static_cast<uint32_t>(width * height * 4),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };
    stagingBuffer.map();


    // copy data from staging buffer to VkImage
    device.transitionImageLayout(
        image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        6
    );

    for (int i = 0; i < 6; i++) {
        stagingBuffer.writeToBuffer((void *) pixels[i]);

        stbi_image_free(pixels[i]);

        device.copyBufferToImage(
            stagingBuffer.getBuffer(),
            image, static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            1, i
        );
    }

    device.transitionImageLayout(
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        imageLayout,
        6
    );

    // create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

    if (vkCreateImageView(device.device(), &viewInfo, nullptr, &view) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view!");
    }
}

void hammock::TextureCubeMap::createSampler(const Device &device, const VkFilter filter) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = filter;
    samplerInfo.minFilter = filter;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;

    // retrieve max anisotropy from physical device
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(device.getPhysicalDevice(), &properties);

    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image sampler!");
    }
}

void hammock::Texture2D::loadFromBuffer(const void *buffer, const uint32_t instanceSize, const uint32_t width,
                                     const uint32_t height, const uint32_t channels, Device &device, VkFormat format,
                                     const VkImageLayout imageLayout, uint32_t mipLevels) {
    this->width = width;
    this->height = height;
    const uint32_t bufferSize = width * height * channels;

    LegacyBuffer stagingBuffer
    {
        device,
        instanceSize,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    stagingBuffer.map();
    stagingBuffer.writeToBuffer(buffer);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0; // Optional

    device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);

    // copy data from staging buffer to VkImage
    device.transitionImageLayout(
        image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );
    device.copyBufferToImage(
        stagingBuffer.getBuffer(),
        image,
        width,height,
        1
    );
    device.transitionImageLayout(
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        imageLayout
    );
    layout = imageLayout;

    // create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device.device(), &viewInfo, nullptr, &view) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view!");
    }
}

void hammock::Texture2D::createSampler(const Device &device,
                                    VkFilter filter,
                                    VkSamplerAddressMode addressMode,
                                    VkBorderColor borderColor,
                                    VkSamplerMipmapMode mipmapMode,
                                    float numMips) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = filter;
    samplerInfo.minFilter = filter;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    samplerInfo.anisotropyEnable = VK_TRUE;

    // retrieve max anisotropy from physical device
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(device.getPhysicalDevice(), &properties);

    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = borderColor;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = mipmapMode;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = numMips;

    if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image sampler!");
    }
}

void hammock::Texture2D::generateMipMaps(const Device &device, const uint32_t mipLevels) const {
    VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

    transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {
                       .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                       .baseMipLevel = 0,
                       .levelCount = mipLevels,
                       .layerCount = 1
                   });

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = width;
    int32_t mipHeight = height;

    for (uint32_t i = 1; i < mipLevels; i++) {
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
                       image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    device.endSingleTimeCommands(commandBuffer);
    vkQueueWaitIdle(device.graphicsQueue());
}

void hammock::Texture3D::loadFromBuffer(Device &device, const void *buffer, VkDeviceSize instanceSize, uint32_t width,
                                     uint32_t height, uint32_t channels, uint32_t depth, VkFormat format,
                                     VkImageLayout imageLayout) {
    this->width = width;
    this->height = height;
    this->depth = depth;
    this->channels = channels;
    this->layout = imageLayout;

    // Format support check
    // 3D texture support in Vulkan is mandatory so there is no need to check if it is supported
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(device.getPhysicalDevice(), format, &formatProperties);
    // Check if format supports transfer
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_DST_BIT)) {
        Logger::log(LOG_LEVEL_ERROR,
                    "Error: Device does not support flag TRANSFER_DST for selected texture format!\n");
        throw std::runtime_error("Error: Device does not support flag TRANSFER_DST for selected!");
    }
    // Check if GPU supports requested 3D texture dimensions
    uint32_t maxImageDimension3D(device.properties.limits.maxImageDimension3D);
    if (width > maxImageDimension3D || height > maxImageDimension3D || depth > maxImageDimension3D) {
        Logger::log(LOG_LEVEL_ERROR,
                    "Error: Requested texture dimensions is greater than supported 3D texture dimension!\n");
        throw std::runtime_error("Error: Requested texture dimensions is greater than supported 3D texture dimension!");
    }

    // Calculate aligned dimensions based on device properties
    VkDeviceSize alignedRowPitch = (width * instanceSize + 
        device.properties.limits.optimalBufferCopyRowPitchAlignment - 1) & 
        ~(device.properties.limits.optimalBufferCopyRowPitchAlignment - 1);
    
    VkDeviceSize alignedSlicePitch = alignedRowPitch * height;
    VkDeviceSize totalSize = alignedSlicePitch * depth;


    VkImageCreateInfo imageCreateInfo = Init::imageCreateInfo();
    imageCreateInfo.imageType = VK_IMAGE_TYPE_3D;
    imageCreateInfo.format = format;
    imageCreateInfo.mipLevels = this->mipLevels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.extent.width = this->width;
    imageCreateInfo.extent.height = this->height;
    imageCreateInfo.extent.depth = this->depth;
    // Set initial layout of the image to undefined
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    checkResult(vkCreateImage(device.device(), &imageCreateInfo, nullptr, &this->image));

    // Device local memory to back up image
    VkMemoryAllocateInfo memAllocInfo = Init::memoryAllocateInfo();
    VkMemoryRequirements memReqs = {};
    vkGetImageMemoryRequirements(device.device(), this->image, &memReqs);
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = device.findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    checkResult(vkAllocateMemory(device.device(), &memAllocInfo, nullptr, &this->memory));
    checkResult(vkBindImageMemory(device.device(), this->image, this->memory, 0));

    // Create a host-visible staging buffer that contains the raw image data
    LegacyBuffer stagingBuffer{
        device,
        instanceSize, // Size of each element (4 bytes for float)
        static_cast<uint32_t>(totalSize),  // Total size including padding
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    stagingBuffer.map();

    // Copy data slice by slice with proper alignment
    const float* srcData = static_cast<const float*>(buffer);
    float* dstData = static_cast<float*>(stagingBuffer.getMappedMemory());

    // Calculate pitches in bytes
    VkDeviceSize unalignedRowPitch = width * sizeof(float);
    VkDeviceSize unalignedSlicePitch = unalignedRowPitch * height;

    // Debug prints
    Logger::log(LOG_LEVEL_DEBUG, "Width: %d, Height: %d, Depth: %d\n", width, height, depth);
    Logger::log(LOG_LEVEL_DEBUG, "Unaligned row pitch: %zu bytes\n", unalignedRowPitch);
    Logger::log(LOG_LEVEL_DEBUG, "Aligned row pitch: %zu bytes\n", alignedRowPitch);
    Logger::log(LOG_LEVEL_DEBUG, "Aligned slice pitch: %zu bytes\n", alignedSlicePitch);
    Logger::log(LOG_LEVEL_DEBUG, "Unligned slice pitch: %zu bytes\n", alignedSlicePitch);
    
    for (uint32_t z = 0; z < depth; z++) {
        for (uint32_t y = 0; y < height; y++) {
            const float* srcRow = srcData + (z * height * width) + (y * width);
            float* dstRow = dstData + (z * alignedSlicePitch / sizeof(float)) + 
                                    (y * alignedRowPitch / sizeof(float));
            
            // Copy one row of floats
            memcpy(dstRow, srcRow, unalignedRowPitch);
        }
    }
    stagingBuffer.unmap();

    // Setup buffer copy regions with proper alignment
    VkBufferImageCopy copyRegion{};
    copyRegion.bufferOffset = 0;
    copyRegion.bufferRowLength = alignedRowPitch / instanceSize;  // In texels
    copyRegion.bufferImageHeight = height;
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageExtent = {width, height, depth};

    // Transition the texture image layout to transfer destination
    device.transitionImageLayout(
        this->image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    // Copy the data from the staging buffer to the texture image
    VkCommandBuffer cmdBuffer = device.beginSingleTimeCommands();
    vkCmdCopyBufferToImage(
        cmdBuffer,
        stagingBuffer.getBuffer(),
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copyRegion
    );
    device.endSingleTimeCommands(cmdBuffer);

    // Transition the texture image layout to shader read after copying the data
    device.transitionImageLayout(
        this->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        imageLayout
    );

    // create image view
    VkImageViewCreateInfo view = Init::imageViewCreateInfo();
    view.image = this->image;
    view.viewType = VK_IMAGE_VIEW_TYPE_3D;
    view.format = format;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.baseMipLevel = 0;
    view.subresourceRange.baseArrayLayer = 0;
    view.subresourceRange.layerCount = 1;
    view.subresourceRange.levelCount = 1;
    checkResult(vkCreateImageView(device.device(), &view, nullptr, &this->view));
}

void hammock::Texture3D::createSampler(Device &device, VkFilter filter, VkSamplerAddressMode addressMode) {
    // Create sampler
    VkSamplerCreateInfo sampler = Init::samplerCreateInfo();
    sampler.magFilter = filter;
    sampler.minFilter = filter;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = addressMode;
    sampler.addressModeV = addressMode;
    sampler.addressModeW = addressMode;
    sampler.mipLodBias = 0.0f;
    sampler.compareOp = VK_COMPARE_OP_NEVER;
    sampler.minLod = 0.0f;
    sampler.maxLod = 0.0f;
    sampler.maxAnisotropy = 1.0;
    sampler.anisotropyEnable = VK_FALSE;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    checkResult(vkCreateSampler(device.device(), &sampler, nullptr, &this->sampler));
}
