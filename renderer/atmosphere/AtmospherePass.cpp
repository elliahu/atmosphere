#include "AtmospherePass.h"

void AtmospherePass::initialize() {
    prepareBuffers();
    prepareDescriptors();
    device.waitIdle();
    processDeletionQueue();

    transmittance.initialize(layout->getDescriptorSetLayout());
    multipleScattering.setTransmittance(transmittance.getLut());
    multipleScattering.initialize(layout->getDescriptorSetLayout());
    skyView.setTransmittance(transmittance.getLut());
    skyView.setMultipleScattering(multipleScattering.getLut());
    skyView.initialize(layout->getDescriptorSetLayout());
    aerialPerspective.setTransmittance(transmittance.getLut());
    aerialPerspective.setMultipleScattering(multipleScattering.getLut());
    aerialPerspective.initialize(layout->getDescriptorSetLayout());
}

void AtmospherePass::recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
    // Update the buffer
    resourceManager.getResource<Buffer>(atmosphereBuffer)->writeToBuffer(&atmosphere);

    // Bind the common descriptor set
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, transmittance.getPipelineLayout(), 0, 1,
                            &descriptor, 0, nullptr);

    profiler.resetTimestamp(commandBuffer, 4);
    profiler.resetTimestamp(commandBuffer, 5);
    profiler.resetTimestamp(commandBuffer, 6);
    profiler.resetTimestamp(commandBuffer, 7);
    profiler.resetTimestamp(commandBuffer, 8);
    profiler.resetTimestamp(commandBuffer, 9);
    profiler.resetTimestamp(commandBuffer, 10);
    profiler.resetTimestamp(commandBuffer, 11);


    // Record transmittance

    transmittance.recordCommands(commandBuffer, frameIndex);
    multipleScattering.recordCommands(commandBuffer, frameIndex);
    skyView.recordCommands(commandBuffer, frameIndex);
    aerialPerspective.recordCommands(commandBuffer, frameIndex);

}

void AtmospherePass::prepareBuffers() {
    atmosphereBuffer = resourceManager.createResource<Buffer>(
        "atmosphere-buffer", BufferDesc{
            .instanceSize = sizeof(AtmosphereUniformBufferData),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    );
    // Map the buffers
    resourceManager.getResource<Buffer>(atmosphereBuffer)->map();
}

void AtmospherePass::prepareDescriptors() {
    layout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .build();

    VkDescriptorBufferInfo atmosphereBufferInfo = resourceManager.getResource<Buffer>(atmosphereBuffer)->descriptorInfo();
    DescriptorWriter(*layout, *descriptorPool)
            .writeBuffer(0, &atmosphereBufferInfo)
            .build(descriptor);
}
