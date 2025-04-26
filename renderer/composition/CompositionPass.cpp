#include "CompositionPass.h"

void CompositionPass::initialize(HmckVec2 resolution) {
    prepareBlueNoise();
    prepareBuffer();
    prepareTargets(static_cast<uint32_t>(resolution.X), static_cast<uint32_t>(resolution.Y));
    prepareDescriptors();
    preparePipelines();
    device.waitIdle();
    processDeletionQueue();

    data.resX = resolution.X;
    data.resY = resolution.Y;
}

void CompositionPass::recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
    Image *compositedColorTarget = resourceManager.getResource<Image>(compositedImage);
    Image *skyColorTarget = resourceManager.getResource<Image>(skyColor);
    VkExtent2D extent = {compositedColorTarget->getExtent().width, compositedColorTarget->getExtent().height};

    profiler.resetTimestamp(commandBuffer, 16);
    profiler.resetTimestamp(commandBuffer, 17);
    profiler.resetTimestamp(commandBuffer, 18);
    profiler.resetTimestamp(commandBuffer, 19);

    resourceManager.getResource<Buffer>(buffer)->writeToBuffer(&data);

    // Transition attachments into required layouts
    if (terrainColor->getLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        terrainColor->transition(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    if (terrainDepth->getLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        terrainDepth->transition(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    if (compositedColorTarget->getLayout() != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        compositedColorTarget->transition(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    if (skyColorTarget->getLayout() != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        skyColorTarget->transition(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    if (godRaysTexture->getLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        godRaysTexture->transition(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    profiler.writeTimestamp(commandBuffer, 16, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);

    // Begin rendering into intermediate composition image
    VkRenderingAttachmentInfo colorTarget = skyColorTarget->getRenderingAttachmentInfo();
    colorTarget.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
    renderingInfo.renderArea = {0, 0, extent.width, extent.height};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorTarget;

    // Sky
    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    // Viewport
    VkViewport compositionViewport{
        0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &compositionViewport);

    // Scissors
    VkRect2D compositionScissors{0, 0, extent.width, extent.height};
    vkCmdSetScissor(commandBuffer, 0, 1, &compositionScissors);

    // Bind composition descriptor set
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyPipeline->pipelineLayout, 0, 1,
                            &compositionDescriptor, 0, nullptr);

    // Bind sky descriptor set
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyPipeline->pipelineLayout, 1, 1,
                            &skyDescriptor, 0, nullptr);

    // Bind composition pipeline
    skyPipeline->bind(commandBuffer);

    // Draw
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    // Finish the rendering
    vkCmdEndRendering(commandBuffer);

    profiler.writeTimestamp(commandBuffer, 17, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);

    // transition sky image
    if (skyColorTarget->getLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        skyColorTarget->transition(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    profiler.writeTimestamp(commandBuffer, 18, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
    // Composition
    colorTarget = compositedColorTarget->getRenderingAttachmentInfo();
    colorTarget.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    renderingInfo.renderArea = {0, 0, extent.width, extent.height};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorTarget;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);
    vkCmdSetViewport(commandBuffer, 0, 1, &compositionViewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &compositionScissors);

    compositionPipeline->bind(commandBuffer);

    // Draw
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    // Finish the rendering
    vkCmdEndRendering(commandBuffer);

    profiler.writeTimestamp(commandBuffer, 19, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);
}

void CompositionPass::prepareBlueNoise() {
    int w, h, c, d;
    AutoDelete blueNoiseData(readImage(ASSET_PATH("blue_noise.png"), w, h, c,
                                       Filesystem::ImageFormat::R16G16B16A16_SFLOAT), [](const void *p) {
        delete[] static_cast<const float16_t *>(p);
    });

    // Create host visible staging buffer on device
    ResourceHandle blueNoiseStagingBuffer = queueForDeletion(resourceManager.createResource<Buffer>(
        "blue-noise-staging-buffer",
        BufferDesc{
            .instanceSize = sizeof(float16_t),
            .instanceCount = static_cast<uint32_t>(w * h * c),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    ));

    // Write the data into the staging buffer
    resourceManager.getResource<Buffer>(blueNoiseStagingBuffer)->map();
    resourceManager.getResource<Buffer>(blueNoiseStagingBuffer)->writeToBuffer(blueNoiseData.get());

    // Create the image resource
    blueNoise = resourceManager.createResource<Image>(
        "blue-noise",
        ImageDesc{
            .width = static_cast<uint32_t>(w),
            .height = static_cast<uint32_t>(h),
            .channels = static_cast<uint32_t>(c),
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        }
    );

    // Copy the data from buffer into the image
    resourceManager.getResource<Image>(blueNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    resourceManager.getResource<Image>(blueNoise)->queueCopyFromBuffer(
        resourceManager.getResource<Buffer>(blueNoiseStagingBuffer)->getBuffer());
    resourceManager.getResource<Image>(blueNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void CompositionPass::prepareBuffer() {
    buffer = resourceManager.createResource<Buffer>(
        "composition-uniform-buffer", BufferDesc{
            .instanceSize = sizeof(CompositionData),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .queueFamilies = {CommandQueueFamily::Graphics},
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        }
    );
    // Map the buffers
    resourceManager.getResource<Buffer>(buffer)->map();
}

void CompositionPass::prepareTargets(uint32_t width, uint32_t height) {
    compositedImage = resourceManager.createResource<Image>(
        "composited-color-image", ImageDesc{
            .width = width,
            .height = height,
            .channels = 4,
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .clearValue = {.color = {0.f, 0.f, 0.f, 0.f}},
        }
    );
    // Set initial layout
    resourceManager.getResource<Image>(compositedImage)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    skyColor = resourceManager.createResource<Image>(
        "sky-color-image", ImageDesc{
            .width = width,
            .height = height,
            .channels = 4,
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .clearValue = {.color = {0.f, 0.f, 0.f, 0.f}},
        }
    );
    // Set initial layout
    resourceManager.getResource<Image>(skyColor)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


    sampler = resourceManager.createResource<Sampler>("composition-sampler", SamplerDesc{});
}

void CompositionPass::prepareDescriptors() {
    compositionLayout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Terrain color image
            .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Terrain depth image
            .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Clouds color image
            .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // transmittance LUT
            .addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // aeriel LUT
            .addBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // sun shadow
            .addBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // blue noise
            .addBinding(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Sky color
            .addBinding(9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // God rays image
            .build();

    skyLayout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Sky view LUT
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Transmittance LUT
            .build();

    Sampler *s = resourceManager.getResource<Sampler>(sampler);
    VkDescriptorBufferInfo bufferInfo = resourceManager.getResource<Buffer>(buffer)->descriptorInfo();
    VkDescriptorImageInfo terrainColorImageInfo = terrainColor->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo terrainDepthImageInfo = terrainDepth->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo cloudsColorTarget = cloudsColor->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo transmittanceLUTInfo = transmittanceLUT->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo skyViewLUTInfo = skyViewLUT->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo aerialPerspectiveLUTInfo = aerialPerspectiveLUT->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo sunShadowInfo = sunShadow->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo blueNoiseInfo = resourceManager.getResource<Image>(blueNoise)->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo skyColorInfo = resourceManager.getResource<Image>(skyColor)->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo godRaysImageInfo = godRaysTexture->getDescriptorImageInfo(s->getSampler());
    DescriptorWriter(*compositionLayout, *descriptorPool)
            .writeBuffer(0, &bufferInfo)
            .writeImage(1, &terrainColorImageInfo)
            .writeImage(2, &terrainDepthImageInfo)
            .writeImage(3, &cloudsColorTarget)
            .writeImage(4, &transmittanceLUTInfo)
            .writeImage(5, &aerialPerspectiveLUTInfo)
            .writeImage(6, &sunShadowInfo)
            .writeImage(7, &blueNoiseInfo)
            .writeImage(8, &skyColorInfo)
            .writeImage(9, &godRaysImageInfo)
            .build(compositionDescriptor);

    DescriptorWriter(*skyLayout, *descriptorPool)
            .writeImage(0, &skyViewLUTInfo)
            .writeImage(1, &transmittanceLUTInfo)
            .build(skyDescriptor);
}

void CompositionPass::preparePipelines() {
    compositionPipeline = GraphicsPipeline::create({
        .debugName = "composition-pipeline",
        .device = device,
        .vertexShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("compose.vert")),},
        .fragmentShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("compose.frag")),},
        .descriptorSetLayouts = {compositionLayout->getDescriptorSetLayout()},
        .pushConstantRanges{},
        .graphicsState{
            .cullMode = VK_CULL_MODE_NONE,
            .vertexBufferBindings{}
        },
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 1, // We are rendering to single color attachment
            .colorAttachmentFormats = {resourceManager.getResource<Image>(compositedImage)->getFormat()},
        }
    });

    skyPipeline = GraphicsPipeline::create({
        .debugName = "sky-pipeline",
        .device = device,
        .vertexShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("sky.vert")),},
        .fragmentShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("sky.frag")),},
        .descriptorSetLayouts = {
            compositionLayout->getDescriptorSetLayout(),
            skyLayout->getDescriptorSetLayout(),
        },
        .pushConstantRanges{},
        .graphicsState{
            .cullMode = VK_CULL_MODE_NONE,
            .vertexBufferBindings{}
        },
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 1, // We are rendering to single color attachment
            .colorAttachmentFormats = {resourceManager.getResource<Image>(skyColor)->getFormat()},
        }
    });
}
