#include "SkyView.h"

void SkyView::recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) {

    profiler.writeTimestamp(commandBuffer, 8, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
    // Bind the descriptor set
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipelineLayout, 1, 1,
                            &descriptor, 0, nullptr);

    // Bind the pipeline
    pipeline->bind(commandBuffer);

    // Dispatch
    vkCmdDispatch(commandBuffer, GROUPS_COUNT(SKY_VIEW_LUT_SIZE_X, 8), GROUPS_COUNT(SKY_VIEW_LUT_SIZE_Y, 8), 1);
    profiler.writeTimestamp(commandBuffer, 9, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
}

void SkyView::initialize(VkDescriptorSetLayout descriptorSetLayout) {
    SkyView::prepareLut();
    SkyView::prepareDescriptors();
    SkyView::preparePipeline(descriptorSetLayout);

    device.waitIdle();
    processDeletionQueue();
}

void SkyView::prepareLut() {
    lut = resourceManager.createResource<Image>(
        "sky-view-lut", ImageDesc{
            .width = SKY_VIEW_LUT_SIZE_X,
            .height = SKY_VIEW_LUT_SIZE_Y,
            .channels = 4,
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .currentQueueFamily = CommandQueueFamily::Compute,
            .queueFamilies = {CommandQueueFamily::Compute, CommandQueueFamily::Graphics},
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        }
    );
    // transition
    resourceManager.getResource<Image>(lut)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_GENERAL);

    sampler = resourceManager.createResource<Sampler>("sky-view-sampler", SamplerDesc{});
}

void SkyView::prepareDescriptors() {
    layout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // Transmittance
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // Multiple scattering
            .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT) // Sky view
            .build();

    Sampler *s = resourceManager.getResource<Sampler>(sampler);
    VkDescriptorImageInfo lutInfo = resourceManager.getResource<Image>(lut)->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo transmittanceInfo = transmittance->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo multipleScatteringInfo = multipleScattering->getDescriptorImageInfo(s->getSampler());
    DescriptorWriter(*layout, *descriptorPool)
            .writeImage(0, &transmittanceInfo)
            .writeImage(1, &multipleScatteringInfo)
            .writeImage(2, &lutInfo)
            .build(descriptor);
}

void SkyView::preparePipeline(VkDescriptorSetLayout descriptorSetLayout) {
    pipeline = ComputePipeline::create({
        .debugName = "sky-view-compute-pipeline",
        .device = device,
        .computeShader{.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("skyview.comp")),},
        .descriptorSetLayouts = {
            descriptorSetLayout,
            layout->getDescriptorSetLayout()
        },
        .pushConstantRanges{}
    });
}
