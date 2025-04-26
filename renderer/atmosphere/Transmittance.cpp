#include "Transmittance.h"

void Transmittance::initialize(VkDescriptorSetLayout descriptorSetLayout) {
    Transmittance::prepareLut();
    Transmittance::prepareDescriptors();
    Transmittance::preparePipeline(descriptorSetLayout);

    device.waitIdle();
    processDeletionQueue();
}

void Transmittance::recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) {

    profiler.writeTimestamp(commandBuffer, 4, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
    // Bind the descriptor set
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipelineLayout, 1, 1,
                            &descriptor, 0, nullptr);

    // Bind the pipeline
    pipeline->bind(commandBuffer);

    // Dispatch
    vkCmdDispatch(commandBuffer, GROUPS_COUNT(TRANSMITTANCE_LUT_SIZE_X,16), GROUPS_COUNT(TRANSMITTANCE_LUT_SIZE_Y,16), 1);
    profiler.writeTimestamp(commandBuffer, 5, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
}

void Transmittance::prepareDescriptors() {
    layout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .build();

    Sampler *s = resourceManager.getResource<Sampler>(sampler);
    VkDescriptorImageInfo lutInfo = resourceManager.getResource<Image>(lut)->getDescriptorImageInfo(s->getSampler());
    DescriptorWriter(*layout, *descriptorPool)
            .writeImage(0, &lutInfo)
            .build(descriptor);
}

void Transmittance::prepareLut() {
    lut = resourceManager.createResource<Image>(
        "transmittance-lut", ImageDesc{
            .width = TRANSMITTANCE_LUT_SIZE_X,
            .height = TRANSMITTANCE_LUT_SIZE_Y,
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

    sampler = resourceManager.createResource<Sampler>("transmittance-sampler", SamplerDesc{});
}



void Transmittance::preparePipeline(VkDescriptorSetLayout descriptorSetLayout) {
    pipeline = ComputePipeline::create({
        .debugName = "transmittance-compute-pipeline",
        .device = device,
        .computeShader{.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("transmittance.comp")),},
        .descriptorSetLayouts = {
            descriptorSetLayout,
            layout->getDescriptorSetLayout()
        },
        .pushConstantRanges{}
    });
}
