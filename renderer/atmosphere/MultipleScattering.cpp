#include "MultipleScattering.h"

void MultipleScattering::initialize(VkDescriptorSetLayout descriptorSetLayout) {
    MultipleScattering::prepareLut();
    MultipleScattering::prepareDescriptors();
    MultipleScattering::preparePipeline(descriptorSetLayout);

    device.waitIdle();
    processDeletionQueue();
}

void MultipleScattering::recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) {

    profiler.writeTimestamp(commandBuffer, 6, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
    // Bind the descriptor set
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipelineLayout, 1, 1,
                            &descriptor, 0, nullptr);

    // Bind the pipeline
    pipeline->bind(commandBuffer);

    // Dispatch
    vkCmdDispatch(commandBuffer, GROUPS_COUNT(MULTI_SCATTER_LUT_SIZE_X, 16), GROUPS_COUNT(MULTI_SCATTER_LUT_SIZE_Y, 16), 1);
    profiler.writeTimestamp(commandBuffer, 7, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
}

void MultipleScattering::prepareDescriptors() {
    layout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // Transmittance
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT) // Multiple scattering
            .build();

    Sampler *s = resourceManager.getResource<Sampler>(sampler);
    VkDescriptorImageInfo lutInfo = resourceManager.getResource<Image>(lut)->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo transmittanceInfo = transmittance->getDescriptorImageInfo(s->getSampler());
    DescriptorWriter(*layout, *descriptorPool)
            .writeImage(0, &transmittanceInfo)
            .writeImage(1, &lutInfo)
            .build(descriptor);
}

void MultipleScattering::prepareLut() {
    lut = resourceManager.createResource<Image>(
        "multiple-scattering-lut", ImageDesc{
            .width = MULTI_SCATTER_LUT_SIZE_X,
            .height = MULTI_SCATTER_LUT_SIZE_Y,
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

    sampler = resourceManager.createResource<Sampler>("multiple-scattering-sampler", SamplerDesc{});
}

void MultipleScattering::preparePipeline(VkDescriptorSetLayout descriptorSetLayout) {
    pipeline = ComputePipeline::create({
        .debugName = "multiple-scattering-compute-pipeline",
        .device = device,
        .computeShader{.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("multiplescattering.comp")),},
        .descriptorSetLayouts = {
            descriptorSetLayout,
            layout->getDescriptorSetLayout()
        },
        .pushConstantRanges{}
    });
}
