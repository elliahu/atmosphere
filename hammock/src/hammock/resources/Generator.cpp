#include "hammock/resources/Generator.h"
#include <chrono>

#include "hammock/core/GraphicsPipeline.h"
#include "hammock/core/Framebuffer.h"
#include "hammock/core/CoreUtils.h"
#include "hammock/core/Shader.h"
#include "hammock/utils/Filesystem.h"

hammock:: DeviceStorageResourceHandle<hammock::Texture2D> hammock::Generator::generatePrefilteredMap(Device &device,  DeviceStorageResourceHandle<Texture2D> environmentMap, DeviceStorage &resources, VkFormat format) {
    auto tStart = std::chrono::high_resolution_clock::now();
    uint32_t width = resources.getTexture2D(environmentMap)->width;
    uint32_t height = resources.getTexture2D(environmentMap)->height;
    std::unique_ptr<GraphicsPipeline> pipeline{};
    DeviceStorageResourceHandle<Texture2D> prefilteredMap = resources.createEmptyTexture2D();
    const uint32_t mipLevels = getNumberOfMipLevels(width, height);

    // Image
    VkImageCreateInfo imageCI = Init::imageCreateInfo();
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = format;
    imageCI.extent.width = width;
    imageCI.extent.height = height;
    imageCI.extent.depth = 1;
    imageCI.mipLevels = mipLevels;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    device.createImageWithInfo(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               resources.getTexture2D(prefilteredMap)->image,
                               resources.getTexture2D(prefilteredMap)->memory);
    // View
    VkImageViewCreateInfo viewCI = Init::imageViewCreateInfo();
    viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCI.format = format;
    viewCI.subresourceRange = {};
    viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCI.subresourceRange.levelCount = mipLevels;
    viewCI.subresourceRange.layerCount = 1;
    viewCI.image = resources.getTexture2D(prefilteredMap)->image;
    checkResult(vkCreateImageView(device.device(), &viewCI, nullptr, &resources.getTexture2D(prefilteredMap)->view));
    // sampler and descriptor
    resources.getTexture2D(prefilteredMap)->createSampler(device,
                                                          VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                                          VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                                                          VK_SAMPLER_MIPMAP_MODE_LINEAR, mipLevels);
    resources.getTexture2D(prefilteredMap)->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    resources.getTexture2D(prefilteredMap)->updateDescriptor();

    // FB, Att, RP, Pipe, etc.
    VkAttachmentDescription attDesc = {};
    // Color attachment
    attDesc.format = format;
    attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;

    // Use subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;
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

    // Renderpass
    VkRenderPassCreateInfo renderPassCI = Init::renderPassCreateInfo();
    renderPassCI.attachmentCount = 1;
    renderPassCI.pAttachments = &attDesc;
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = &subpassDescription;
    renderPassCI.dependencyCount = 2;
    renderPassCI.pDependencies = dependencies.data();
    VkRenderPass renderpass;
    checkResult(vkCreateRenderPass(device.device(), &renderPassCI, nullptr, &renderpass));

    struct {
        VkImage image;
        VkImageView view;
        VkDeviceMemory memory;
        VkFramebuffer framebuffer;
    } offscreen;

    // Offfscreen framebuffer
    {
        // Color attachment
        VkImageCreateInfo imageCreateInfo = Init::imageCreateInfo();
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = format;
        imageCreateInfo.extent.width = width;
        imageCreateInfo.extent.height = height;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        checkResult(vkCreateImage(device.device(), &imageCreateInfo, nullptr, &offscreen.image));

        VkMemoryAllocateInfo memAlloc = Init::memoryAllocateInfo();
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device.device(), offscreen.image, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = device.findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        checkResult(vkAllocateMemory(device.device(), &memAlloc, nullptr, &offscreen.memory));
        checkResult(vkBindImageMemory(device.device(), offscreen.image, offscreen.memory, 0));

        VkImageViewCreateInfo colorImageView = Init::imageViewCreateInfo();
        colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorImageView.format = format;
        colorImageView.flags = 0;
        colorImageView.subresourceRange = {};
        colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorImageView.subresourceRange.baseMipLevel = 0;
        colorImageView.subresourceRange.levelCount = 1;
        colorImageView.subresourceRange.baseArrayLayer = 0;
        colorImageView.subresourceRange.layerCount = 1;
        colorImageView.image = offscreen.image;
        checkResult(vkCreateImageView(device.device(), &colorImageView, nullptr, &offscreen.view));

        VkFramebufferCreateInfo fbufCreateInfo = Init::framebufferCreateInfo();
        fbufCreateInfo.renderPass = renderpass;
        fbufCreateInfo.attachmentCount = 1;
        fbufCreateInfo.pAttachments = &offscreen.view;
        fbufCreateInfo.width = width;
        fbufCreateInfo.height = height;
        fbufCreateInfo.layers = 1;
        checkResult(vkCreateFramebuffer(device.device(), &fbufCreateInfo, nullptr, &offscreen.framebuffer));

        VkCommandBuffer layoutCmd = device.beginSingleTimeCommands();
        transitionImageLayout(
            layoutCmd,
            offscreen.image,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        device.endSingleTimeCommands(layoutCmd);
    }

    // Descriptors
    VkDescriptorSetLayout descriptorsetlayout;
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        Init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
    };
    VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCI = Init::descriptorSetLayoutCreateInfo(setLayoutBindings);
    checkResult(vkCreateDescriptorSetLayout(device.device(), &descriptorsetlayoutCI, nullptr, &descriptorsetlayout));

    // Descriptor Pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        Init::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
    };
    VkDescriptorPoolCreateInfo descriptorPoolCI = Init::descriptorPoolCreateInfo(poolSizes, 2);
    VkDescriptorPool descriptorpool;
    checkResult(vkCreateDescriptorPool(device.device(), &descriptorPoolCI, nullptr, &descriptorpool));

    // Descriptor sets
    VkDescriptorSet descriptorset;
    VkDescriptorSetAllocateInfo allocInfo = Init::descriptorSetAllocateInfo(descriptorpool, &descriptorsetlayout, 1);
    checkResult(vkAllocateDescriptorSets(device.device(), &allocInfo, &descriptorset));
    VkWriteDescriptorSet writeDescriptorSet = Init::writeDescriptorSet(descriptorset,
                                                                       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                                                       &resources.getTexture2D(environmentMap)->
                                                                       descriptor);
    vkUpdateDescriptorSets(device.device(), 1, &writeDescriptorSet, 0, nullptr);

    // Pipeline layout
    struct PushBlock {
        float roughness;
        uint32_t numSamples = 32u;
    } pushBlock;

    pipeline = GraphicsPipeline::create({
        .debugName = "PrefilteredMap_generation",
        .device = device,
        .vertexShader{
            .byteCode = hammock::Filesystem::readFile(
                Shader::getCompiledShaderPath("fullscreen_headless.vert.spv").string()),
            .entryFunc = "main"
        },
        .fragmentShader{
            .byteCode = hammock::Filesystem::readFile(
                Shader::getCompiledShaderPath("generate_prefilteredmap.frag.spv").string()),
            .entryFunc = "main"
        },
        .descriptorSetLayouts = {
            descriptorsetlayout
        },
        .pushConstantRanges{
            {
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(PushBlock)
            }
        },
        .graphicsState{
            .depthTest = VK_FALSE,
            .cullMode = VK_CULL_MODE_NONE,
            .blendAtaAttachmentStates{},
            .vertexBufferBindings{}
        },
        .renderPass = renderpass
    });

    // Render
    VkClearValue clearValues[1];
    clearValues[0].color = {{0.0f, 0.0f, 0.2f, 0.0f}};

    VkRenderPassBeginInfo renderPassBeginInfo = Init::renderPassBeginInfo();
    // Reuse render pass from example pass
    renderPassBeginInfo.renderPass = renderpass;
    renderPassBeginInfo.framebuffer = offscreen.framebuffer;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues;

    VkCommandBuffer cmdBuf = device.beginSingleTimeCommands();

    VkViewport viewport = Init::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
    VkRect2D scissor = Init::rect2D(width, height, 0, 0);

    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.layerCount = 1;

    // Change image layout for all cubemap faces to transfer destination
    transitionImageLayout(
        cmdBuf,
        resources.getTexture2D(prefilteredMap)->image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        subresourceRange);

    for (uint32_t m = 0; m < mipLevels; m++) {
        pushBlock.roughness = static_cast<float>(m) / static_cast<float>(mipLevels - 1);
        viewport.width = static_cast<float>(width * std::pow(0.5f, m));
        viewport.height = static_cast<float>(height * std::pow(0.5f, m));
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

        // Render scene
        vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


        vkCmdPushConstants(cmdBuf, pipeline->pipelineLayout,
                           VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlock), &pushBlock);

        pipeline->bind(cmdBuf);
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout, 0, 1,
                                &descriptorset, 0, NULL);

        vkCmdDraw(cmdBuf, 3, 1, 0, 0);

        vkCmdEndRenderPass(cmdBuf);

        transitionImageLayout(
            cmdBuf,
            offscreen.image,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        // Copy region for transfer from framebuffer to cube face
        VkImageCopy copyRegion = {};

        copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.srcSubresource.baseArrayLayer = 0;
        copyRegion.srcSubresource.mipLevel = 0;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.srcOffset = {0, 0, 0};

        copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.dstSubresource.baseArrayLayer = 0;
        copyRegion.dstSubresource.mipLevel = m;
        copyRegion.dstSubresource.layerCount = 1;
        copyRegion.dstOffset = {0, 0, 0};

        copyRegion.extent.width = static_cast<uint32_t>(viewport.width);
        copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
        copyRegion.extent.depth = 1;

        vkCmdCopyImage(
            cmdBuf,
            offscreen.image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            resources.getTexture2D(prefilteredMap)->image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRegion);

        // Transform framebuffer color attachment back
        transitionImageLayout(
            cmdBuf,
            offscreen.image,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    transitionImageLayout(
        cmdBuf,
        resources.getTexture2D(prefilteredMap)->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        subresourceRange);

    device.endSingleTimeCommands(cmdBuf);
    vkQueueWaitIdle(device.graphicsQueue());

    vkDestroyRenderPass(device.device(), renderpass, nullptr);
    vkDestroyFramebuffer(device.device(), offscreen.framebuffer, nullptr);
    vkFreeMemory(device.device(), offscreen.memory, nullptr);
    vkDestroyImageView(device.device(), offscreen.view, nullptr);
    vkDestroyImage(device.device(), offscreen.image, nullptr);
    vkDestroyDescriptorPool(device.device(), descriptorpool, nullptr);
    vkDestroyDescriptorSetLayout(device.device(), descriptorsetlayout, nullptr);

    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    Logger::log(LogLevel::LOG_LEVEL_DEBUG, "Generating prefiltered sphere took %f ms\n", tDiff);

    return prefilteredMap;
}

hammock::DeviceStorageResourceHandle<hammock::Texture2D> hammock::Generator::generatePrefilteredMapWithStaticRoughness(Device &device, DeviceStorageResourceHandle<Texture2D> environmentMap, DeviceStorage &resources,
                                                                     VkFormat format) {
    auto tStart = std::chrono::high_resolution_clock::now();
    uint32_t width = resources.getTexture2D(environmentMap)->width;
    uint32_t height = resources.getTexture2D(environmentMap)->height;
    std::unique_ptr<GraphicsPipeline> pipeline{};
    DeviceStorageResourceHandle<Texture2D> prefilteredMap = resources.createEmptyTexture2D();

    // image, view, sampler
    VkImageCreateInfo imageCI = Init::imageCreateInfo();
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = format;
    imageCI.extent.width = width;
    imageCI.extent.height = height;
    imageCI.extent.depth = 1;
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    device.createImageWithInfo(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               resources.getTexture2D(prefilteredMap)->image,
                               resources.getTexture2D(prefilteredMap)->memory);

    VkImageViewCreateInfo viewCI = Init::imageViewCreateInfo();
    viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCI.format = format;
    viewCI.subresourceRange = {};
    viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCI.subresourceRange.levelCount = 1;
    viewCI.subresourceRange.layerCount = 1;
    viewCI.image = resources.getTexture2D(prefilteredMap)->image;
    checkResult(vkCreateImageView(device.device(), &viewCI, nullptr, &resources.getTexture2D(prefilteredMap)->view));

    resources.getTexture2D(prefilteredMap)->createSampler(device, VK_FILTER_LINEAR);
    resources.getTexture2D(prefilteredMap)->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    resources.getTexture2D(prefilteredMap)->updateDescriptor();

    // FB, RP , Att
    VkAttachmentDescription attDesc = {};
    attDesc.format = format;
    attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkAttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;

    // Use subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;
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

    // Create the actual renderpass

    VkRenderPassCreateInfo renderPassCI = Init::renderPassCreateInfo();
    renderPassCI.attachmentCount = 1;
    renderPassCI.pAttachments = &attDesc;
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = &subpassDescription;
    renderPassCI.dependencyCount = 2;
    renderPassCI.pDependencies = dependencies.data();

    VkRenderPass renderpass;
    checkResult(vkCreateRenderPass(device.device(), &renderPassCI, nullptr, &renderpass));

    VkFramebufferCreateInfo framebufferCI = Init::framebufferCreateInfo();
    framebufferCI.renderPass = renderpass;
    framebufferCI.attachmentCount = 1;
    framebufferCI.pAttachments = &resources.getTexture2D(prefilteredMap)->view;
    framebufferCI.width = width;
    framebufferCI.height = height;
    framebufferCI.layers = 1;

    VkFramebuffer framebuffer;
    checkResult(vkCreateFramebuffer(device.device(), &framebufferCI, nullptr, &framebuffer));

    // Descriptors
    VkDescriptorSetLayout descriptorsetlayout;
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        Init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
    };
    VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCI = Init::descriptorSetLayoutCreateInfo(setLayoutBindings);
    checkResult(vkCreateDescriptorSetLayout(device.device(), &descriptorsetlayoutCI, nullptr, &descriptorsetlayout));

    // Descriptor Pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        Init::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
    };
    VkDescriptorPoolCreateInfo descriptorPoolCI = Init::descriptorPoolCreateInfo(poolSizes, 2);
    VkDescriptorPool descriptorpool;
    checkResult(vkCreateDescriptorPool(device.device(), &descriptorPoolCI, nullptr, &descriptorpool));

    // Descriptor sets
    VkDescriptorSet descriptorset;
    VkDescriptorSetAllocateInfo allocInfo = Init::descriptorSetAllocateInfo(descriptorpool, &descriptorsetlayout, 1);
    checkResult(vkAllocateDescriptorSets(device.device(), &allocInfo, &descriptorset));
    VkWriteDescriptorSet writeDescriptorSet = Init::writeDescriptorSet(descriptorset,
                                                                       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                                                       &resources.getTexture2D(environmentMap)->
                                                                       descriptor);
    vkUpdateDescriptorSets(device.device(), 1, &writeDescriptorSet, 0, nullptr);

    struct PushBlock {
        float roughness = 0.5f;
        unsigned int numSamples = 32u;
    } push;

    pipeline = GraphicsPipeline::create({
        .debugName = "PrefilteredMap_generation",
        .device = device,
        .vertexShader{
            .byteCode = hammock::Filesystem::readFile(
                Shader::getCompiledShaderPath("fullscreen_headless.vert.spv").string()),
            .entryFunc = "main"
        },
        .fragmentShader{
            .byteCode = hammock::Filesystem::readFile(
                Shader::getCompiledShaderPath("generate_prefilteredmap.frag.spv").string()),
            .entryFunc = "main"
        },
        .descriptorSetLayouts = {
            descriptorsetlayout
        },
        .pushConstantRanges{
            {
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(PushBlock)
            }
        },
        .graphicsState{
            .depthTest = VK_FALSE,
            .cullMode = VK_CULL_MODE_NONE,
            .blendAtaAttachmentStates{},
            .vertexBufferBindings{}
        },
        .renderPass = renderpass
    });

    // Render
    VkClearValue clearValues[1];
    clearValues[0].color = {{0.0f, 0.0f, 0.2f, 1.0f}};


    VkRenderPassBeginInfo renderPassBeginInfo = Init::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = renderpass;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.framebuffer = framebuffer;

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.layerCount = 1;

    VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();


    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport viewport = Init::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
    VkRect2D scissor = Init::rect2D(width, height, 0, 0);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    pipeline->bind(commandBuffer);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout, 0, 1,
                            &descriptorset, 0, nullptr);
    vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(PushBlock), &push);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    device.endSingleTimeCommands(commandBuffer);

    vkQueueWaitIdle(device.graphicsQueue());

    vkDestroyRenderPass(device.device(), renderpass, nullptr);
    vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
    vkDestroyDescriptorSetLayout(device.device(), descriptorsetlayout, nullptr);
    vkDestroyDescriptorPool(device.device(), descriptorpool, nullptr);

    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    Logger::log(LogLevel::LOG_LEVEL_DEBUG, "Generating prefiltered sphere took %f ms\n", tDiff);

    return prefilteredMap;
}

hammock::DeviceStorageResourceHandle<hammock::Texture2D> hammock::Generator::generateIrradianceMap(Device &device, DeviceStorageResourceHandle<Texture2D> environmentMap, DeviceStorage &resources, VkFormat format,
                                                 float _deltaPhi, float _deltaTheta) {
    auto tStart = std::chrono::high_resolution_clock::now();
    uint32_t width = resources.getTexture2D(environmentMap)->width;
    uint32_t height = resources.getTexture2D(environmentMap)->height;
    std::unique_ptr<GraphicsPipeline> pipeline{};
    DeviceStorageResourceHandle<Texture2D> irradianceMap = resources.createEmptyTexture2D();

    // image, view, sampler
    VkImageCreateInfo imageCI = Init::imageCreateInfo();
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = format;
    imageCI.extent.width = width;
    imageCI.extent.height = height;
    imageCI.extent.depth = 1;
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    device.createImageWithInfo(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               resources.getTexture2D(irradianceMap)->image,
                               resources.getTexture2D(irradianceMap)->memory);

    VkImageViewCreateInfo viewCI = Init::imageViewCreateInfo();
    viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCI.format = format;
    viewCI.subresourceRange = {};
    viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCI.subresourceRange.levelCount = 1;
    viewCI.subresourceRange.layerCount = 1;
    viewCI.image = resources.getTexture2D(irradianceMap)->image;
    checkResult(vkCreateImageView(device.device(), &viewCI, nullptr, &resources.getTexture2D(irradianceMap)->view));

    resources.getTexture2D(irradianceMap)->createSampler(device, VK_FILTER_LINEAR);
    resources.getTexture2D(irradianceMap)->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    resources.getTexture2D(irradianceMap)->updateDescriptor();

    // FB, RP , Att
    VkAttachmentDescription attDesc = {};
    attDesc.format = format;
    attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkAttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;

    // Use subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;
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

    // Create the actual renderpass

    VkRenderPassCreateInfo renderPassCI = Init::renderPassCreateInfo();
    renderPassCI.attachmentCount = 1;
    renderPassCI.pAttachments = &attDesc;
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = &subpassDescription;
    renderPassCI.dependencyCount = 2;
    renderPassCI.pDependencies = dependencies.data();

    VkRenderPass renderpass;
    checkResult(vkCreateRenderPass(device.device(), &renderPassCI, nullptr, &renderpass));

    VkFramebufferCreateInfo framebufferCI = Init::framebufferCreateInfo();
    framebufferCI.renderPass = renderpass;
    framebufferCI.attachmentCount = 1;
    framebufferCI.pAttachments = &resources.getTexture2D(irradianceMap)->view;
    framebufferCI.width = width;
    framebufferCI.height = height;
    framebufferCI.layers = 1;

    VkFramebuffer framebuffer;
    checkResult(vkCreateFramebuffer(device.device(), &framebufferCI, nullptr, &framebuffer));

    // Descriptors
    VkDescriptorSetLayout descriptorsetlayout;
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        Init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
    };
    VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCI = Init::descriptorSetLayoutCreateInfo(setLayoutBindings);
    checkResult(vkCreateDescriptorSetLayout(device.device(), &descriptorsetlayoutCI, nullptr, &descriptorsetlayout));

    // Descriptor Pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        Init::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
    };
    VkDescriptorPoolCreateInfo descriptorPoolCI = Init::descriptorPoolCreateInfo(poolSizes, 2);
    VkDescriptorPool descriptorpool;
    checkResult(vkCreateDescriptorPool(device.device(), &descriptorPoolCI, nullptr, &descriptorpool));

    // Descriptor sets
    VkDescriptorSet descriptorset;
    VkDescriptorSetAllocateInfo allocInfo = Init::descriptorSetAllocateInfo(descriptorpool, &descriptorsetlayout, 1);
    checkResult(vkAllocateDescriptorSets(device.device(), &allocInfo, &descriptorset));
    VkWriteDescriptorSet writeDescriptorSet = Init::writeDescriptorSet(descriptorset,
                                                                       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                                                       &resources.getTexture2D(environmentMap)->
                                                                       descriptor);
    vkUpdateDescriptorSets(device.device(), 1, &writeDescriptorSet, 0, nullptr);

    pipeline = GraphicsPipeline::create({
        .debugName = "IrradianceMap_generation",
        .device = device,
        .vertexShader{
            .byteCode = hammock::Filesystem::readFile(
                Shader::getCompiledShaderPath("fullscreen_headless.vert.spv").string()),
            .entryFunc = "main"
        },
        .fragmentShader{
            .byteCode = hammock::Filesystem::readFile(
                Shader::getCompiledShaderPath("generate_irradiancemap.frag.spv").string()),
            .entryFunc = "main"
        },
        .descriptorSetLayouts = {
            descriptorsetlayout
        },
        .pushConstantRanges{
            {
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = 2 * sizeof(float)
            }
        },
        .graphicsState{
            .depthTest = VK_FALSE,
            .cullMode = VK_CULL_MODE_NONE,
            .blendAtaAttachmentStates{},
            .vertexBufferBindings{}
        },
        .renderPass = renderpass
    });

    // Render
    VkClearValue clearValues[1];
    clearValues[0].color = {{0.0f, 0.0f, 0.2f, 1.0f}};


    VkRenderPassBeginInfo renderPassBeginInfo = Init::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = renderpass;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.framebuffer = framebuffer;

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.layerCount = 1;

    VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();


    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport viewport = Init::viewport((float) width, (float) height, 0.0f, 1.0f);
    VkRect2D scissor = Init::rect2D(width, height, 0, 0);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    pipeline->bind(commandBuffer);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout, 0, 1,
                            &descriptorset, 0, nullptr);
    struct PushBlock {
        // Sampling deltas
        float deltaPhi;
        float deltaTheta;
    } pushBlock{
                .deltaPhi = (2.0f * static_cast<float>(M_PI)) / _deltaPhi,
                .deltaTheta = (0.5f * static_cast<float>(M_PI)) / _deltaTheta
            };
    vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       2 * sizeof(float), &pushBlock);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    device.endSingleTimeCommands(commandBuffer);

    vkQueueWaitIdle(device.graphicsQueue());

    vkDestroyRenderPass(device.device(), renderpass, nullptr);
    vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
    vkDestroyDescriptorSetLayout(device.device(), descriptorsetlayout, nullptr);
    vkDestroyDescriptorPool(device.device(), descriptorpool, nullptr);

    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    Logger::log(LogLevel::LOG_LEVEL_DEBUG, "Generating irradiance sphere took %f ms\n", tDiff);

    return irradianceMap;
}

hammock::DeviceStorageResourceHandle<hammock::Texture2D> hammock::Generator::generateBRDFLookUpTable(Device &device, DeviceStorage &resources, uint32_t dim, VkFormat format) {
    auto tStart = std::chrono::high_resolution_clock::now();
    std::unique_ptr<GraphicsPipeline> brdfLUTPipeline{};
    DeviceStorageResourceHandle<Texture2D> brdfLookUpTable = resources.createEmptyTexture2D();

    // image, view, sampler
    VkImageCreateInfo imageCI = Init::imageCreateInfo();
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = format;
    imageCI.extent.width = dim;
    imageCI.extent.height = dim;
    imageCI.extent.depth = 1;
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    device.createImageWithInfo(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, resources.getTexture2D(brdfLookUpTable)->image,
                               resources.getTexture2D(brdfLookUpTable)->memory);

    VkImageViewCreateInfo viewCI = Init::imageViewCreateInfo();
    viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCI.format = format;
    viewCI.subresourceRange = {};
    viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCI.subresourceRange.levelCount = 1;
    viewCI.subresourceRange.layerCount = 1;
    viewCI.image = resources.getTexture2D(brdfLookUpTable)->image;
    checkResult(vkCreateImageView(device.device(), &viewCI, nullptr, &resources.getTexture2D(brdfLookUpTable)->view));

    resources.getTexture2D(brdfLookUpTable)->createSampler(device, VK_FILTER_LINEAR);
    resources.getTexture2D(brdfLookUpTable)->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    resources.getTexture2D(brdfLookUpTable)->updateDescriptor();

    // FB, RP , Att
    VkAttachmentDescription attDesc = {};
    attDesc.format = format;
    attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkAttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;

    // Use subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;
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

    // Create the actual renderpass

    VkRenderPassCreateInfo renderPassCI = Init::renderPassCreateInfo();
    renderPassCI.attachmentCount = 1;
    renderPassCI.pAttachments = &attDesc;
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = &subpassDescription;
    renderPassCI.dependencyCount = 2;
    renderPassCI.pDependencies = dependencies.data();

    VkRenderPass renderpass;
    checkResult(vkCreateRenderPass(device.device(), &renderPassCI, nullptr, &renderpass));

    VkFramebufferCreateInfo framebufferCI = Init::framebufferCreateInfo();
    framebufferCI.renderPass = renderpass;
    framebufferCI.attachmentCount = 1;
    framebufferCI.pAttachments = &resources.getTexture2D(brdfLookUpTable)->view;
    framebufferCI.width = dim;
    framebufferCI.height = dim;
    framebufferCI.layers = 1;

    VkFramebuffer framebuffer;
    checkResult(vkCreateFramebuffer(device.device(), &framebufferCI, nullptr, &framebuffer));

    // Descriptors
    VkDescriptorSetLayout descriptorsetlayout;
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {};
    VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCI = Init::descriptorSetLayoutCreateInfo(setLayoutBindings);
    checkResult(vkCreateDescriptorSetLayout(device.device(), &descriptorsetlayoutCI, nullptr, &descriptorsetlayout));

    // Descriptor Pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        Init::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
    };
    VkDescriptorPoolCreateInfo descriptorPoolCI = Init::descriptorPoolCreateInfo(poolSizes, 2);
    VkDescriptorPool descriptorpool;
    checkResult(vkCreateDescriptorPool(device.device(), &descriptorPoolCI, nullptr, &descriptorpool));

    // Descriptor sets
    VkDescriptorSet descriptorset;
    VkDescriptorSetAllocateInfo allocInfo = Init::descriptorSetAllocateInfo(descriptorpool, &descriptorsetlayout, 1);
    checkResult(vkAllocateDescriptorSets(device.device(), &allocInfo, &descriptorset));

    brdfLUTPipeline = GraphicsPipeline::create({
        .debugName = "BRDFLUT_generation",
        .device = device,
        .vertexShader{
            .byteCode = hammock::Filesystem::readFile(
                Shader::getCompiledShaderPath("fullscreen_headless.vert.spv").string()),
            .entryFunc = "main"
        },
        .fragmentShader{
            .byteCode = hammock::Filesystem::readFile(Shader::getCompiledShaderPath("generate_brdflut.frag.spv").string()),
            .entryFunc = "main"
        },
        .descriptorSetLayouts = {
            descriptorsetlayout
        },
        .pushConstantRanges{},
        .graphicsState{
            .depthTest = VK_FALSE,
            .cullMode = VK_CULL_MODE_NONE,
            .blendAtaAttachmentStates{},
            .vertexBufferBindings{}
        },
        .renderPass = renderpass
    });

    // Render
    VkClearValue clearValues[1];
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};

    VkRenderPassBeginInfo renderPassBeginInfo = Init::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = renderpass;
    renderPassBeginInfo.renderArea.extent.width = dim;
    renderPassBeginInfo.renderArea.extent.height = dim;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.framebuffer = framebuffer;

    VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport viewport = Init::viewport(static_cast<float>(dim), static_cast<float>(dim), 0.0f, 1.0f);
    VkRect2D scissor = Init::rect2D(dim, dim, 0, 0);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    brdfLUTPipeline->bind(commandBuffer);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
    device.endSingleTimeCommands(commandBuffer);

    vkQueueWaitIdle(device.graphicsQueue());

    vkDestroyRenderPass(device.device(), renderpass, nullptr);
    vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
    vkDestroyDescriptorSetLayout(device.device(), descriptorsetlayout, nullptr);
    vkDestroyDescriptorPool(device.device(), descriptorpool, nullptr);

    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    Logger::log(LogLevel::LOG_LEVEL_DEBUG, "Generating BRDF LUT took %f ms\n", tDiff);

    return brdfLookUpTable;
}
