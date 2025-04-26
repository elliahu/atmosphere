#include "PostProcessingPass.h"

void PostProcessingPass::initialize() {
    prepareSampler();
    prepareDescriptors();
    preparePipelines();
    device.waitIdle();
    processDeletionQueue();
}

void PostProcessingPass::recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) {

    profiler.resetTimestamp(commandBuffer, 20);
    profiler.resetTimestamp(commandBuffer, 21);

    // Transition intermediate image
    if (input->getLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        input->transition(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    profiler.writeTimestamp(commandBuffer, 20, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
    VkRenderingAttachmentInfo swapChainImageAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    swapChainImageAttachment.imageView = swapChainImageView;
    swapChainImageAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    swapChainImageAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    swapChainImageAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    swapChainImageAttachment.clearValue.color = {0.0f, 0.0f, 0.2f, 0.0f};

    VkRenderingInfo postProcRenderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
    postProcRenderingInfo.renderArea = {0, 0, renderingExtent.width, renderingExtent.height};
    postProcRenderingInfo.layerCount = 1;
    postProcRenderingInfo.colorAttachmentCount = 1;
    postProcRenderingInfo.pColorAttachments = &swapChainImageAttachment;

    // Begin rendering into swap chain image
    vkCmdBeginRendering(commandBuffer, &postProcRenderingInfo);

    // Viewport
    VkViewport postprocessViewport{
        0.0f, 0.0f, static_cast<float>(renderingExtent.width), static_cast<float>(renderingExtent.height), 0.0f, 1.0f
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &postprocessViewport);

    // Scissors
    VkRect2D postprocessScissors{0, 0, renderingExtent.width, renderingExtent.height};
    vkCmdSetScissor(commandBuffer, 0, 1, &postprocessScissors);

    // Bind postprocess descriptor set
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout, 0, 1,
                            &descriptor, 0, nullptr);

    // Push postprocess data
    vkCmdPushConstants(commandBuffer,pipeline->pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(PostProcessingPushConstantData), &data);

    // Bind postprocessing pipeline
    pipeline->bind(commandBuffer);

    // Draw
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    // End rendering
    vkCmdEndRendering(commandBuffer);

    profiler.writeTimestamp(commandBuffer, 21, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
}

void PostProcessingPass::prepareDescriptors() {
    layout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

    Sampler *s = resourceManager.getResource<Sampler>(sampler);
    VkDescriptorImageInfo compositedColorInfo = input->getDescriptorImageInfo(s->getSampler());
    DescriptorWriter(*layout, *descriptorPool)
            .writeImage(0, &compositedColorInfo)
            .build(descriptor);
}

void PostProcessingPass::preparePipelines() {
    pipeline = GraphicsPipeline::create({
        .debugName = "postprocess-pipeline",
        .device = device,
        .vertexShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("postprocess.vert")),},
        .fragmentShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("postprocess.frag")),},
        .descriptorSetLayouts = {layout->getDescriptorSetLayout()},
        .pushConstantRanges{{VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PostProcessingPushConstantData)}},
        .graphicsState{
            .cullMode = VK_CULL_MODE_NONE,
            .vertexBufferBindings{}
        },
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 1, // We are rendering to single color attachment (swap chain image)
            .colorAttachmentFormats = {swapChainImageFormat},
        }
    });
}
