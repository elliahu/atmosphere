#include "GodRaysPass.h"

void GodRaysPass::initialize(HmckVec2 resolution) {
    prepareTargets(static_cast<uint32_t>(resolution.X), static_cast<uint32_t>(resolution.Y));
    prepareDescriptors();
    preparePipelines();
    device.waitIdle();
    processDeletionQueue();
}

void GodRaysPass::recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
    Image *godRaysImage = getGodRaysTexture();
    Image *maskImage = resourceManager.getResource<Image>(maskTexture);
    VkExtent2D extent = {maskImage->getExtent().width, maskImage->getExtent().height};

    profiler.resetTimestamp(commandBuffer, 12);
    profiler.resetTimestamp(commandBuffer, 13);
    profiler.resetTimestamp(commandBuffer, 14);
    profiler.resetTimestamp(commandBuffer, 15);

    if (terrainDepth->getLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        terrainDepth->transition(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    if (godRaysImage->getLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        godRaysImage->transition(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    if (maskImage->getLayout() != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        maskImage->transition(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    // First, create the mask
    profiler.writeTimestamp(commandBuffer, 12, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
    VkRenderingAttachmentInfo colorTarget = maskImage->getRenderingAttachmentInfo();
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

    // Bind mask descriptor set
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, maskPipeline->pipelineLayout, 0, 1,
                            &maskDescriptor, 0, nullptr);

    // Bind mask pipeline
    maskPipeline->bind(commandBuffer);

    // Draw
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    // Finish the rendering
    vkCmdEndRendering(commandBuffer);

    profiler.writeTimestamp(commandBuffer, 13, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);

    if (godRaysImage->getLayout() != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        godRaysImage->transition(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    if (maskImage->getLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        maskImage->transition(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    profiler.writeTimestamp(commandBuffer, 14, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
    // Then blur the mask to create god rays texture
    colorTarget = godRaysImage->getRenderingAttachmentInfo();
    colorTarget.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    renderingInfo.renderArea = {0, 0, extent.width, extent.height};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorTarget;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);
    vkCmdSetViewport(commandBuffer, 0, 1, &compositionViewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &compositionScissors);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, raysPipeline->pipelineLayout, 0, 1,
                            &raysDescriptor, 0, nullptr);


    raysPipeline->bind(commandBuffer);

    vkCmdPushConstants(commandBuffer, raysPipeline->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GodRaysCoefficients), &coefficients);

    // Draw
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    // Finish the rendering
    vkCmdEndRendering(commandBuffer);

    profiler.writeTimestamp(commandBuffer, 15, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);
}

void GodRaysPass::prepareTargets(uint32_t width, uint32_t height) {
    maskTexture = resourceManager.createResource<Image>(
        "mask-image", ImageDesc{
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
    resourceManager.getResource<Image>(maskTexture)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    godRaysTexture = resourceManager.createResource<Image>(
        "god-rays-image", ImageDesc{
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
    resourceManager.getResource<Image>(godRaysTexture)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    sampler = resourceManager.createResource<Sampler>("god-rays-sampler", SamplerDesc{});
}

void GodRaysPass::prepareDescriptors() {
    maskLayout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

    raysLayout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

    Sampler *s = resourceManager.getResource<Sampler>(sampler);
    VkDescriptorImageInfo cloudsImageInfo = cloudsImage->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo terrainDepthInfo = terrainDepth->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo maskInfo = resourceManager.getResource<Image>(maskTexture)->getDescriptorImageInfo(s->getSampler());

    DescriptorWriter(*maskLayout, *descriptorPool)
            .writeImage(0, &cloudsImageInfo)
            .writeImage(1, &terrainDepthInfo)
            .build(maskDescriptor);

    DescriptorWriter(*raysLayout, *descriptorPool)
            .writeImage(0, &maskInfo)
            .build(raysDescriptor);
}

void GodRaysPass::preparePipelines() {
    maskPipeline = GraphicsPipeline::create({
        .debugName = "mask-pipeline",
        .device = device,
        .vertexShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("godrays.vert")),},
        .fragmentShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("mask.frag")),},
        .descriptorSetLayouts = {maskLayout->getDescriptorSetLayout()},
        .pushConstantRanges{},
        .graphicsState{
            .cullMode = VK_CULL_MODE_NONE,
            .vertexBufferBindings{}
        },
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 1,
            .colorAttachmentFormats = {resourceManager.getResource<Image>(maskTexture)->getFormat()},
        }
    });

    raysPipeline = GraphicsPipeline::create({
        .debugName = "god-rays-pipeline",
        .device = device,
        .vertexShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("godrays.vert")),},
        .fragmentShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("blur.frag")),},
        .descriptorSetLayouts = {raysLayout->getDescriptorSetLayout()},
        .pushConstantRanges{
            {VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GodRaysCoefficients)}
        },
        .graphicsState{
            .cullMode = VK_CULL_MODE_NONE,
            .vertexBufferBindings{}
        },
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 1,
            .colorAttachmentFormats = {resourceManager.getResource<Image>(godRaysTexture)->getFormat()},
        }
    });
}
