#pragma once
#include "ILookUpTable.h"

#define MULTI_SCATTER_LUT_SIZE_X 32
#define MULTI_SCATTER_LUT_SIZE_Y 32
/**
 * Responsible for creating multiscatter LUT
 */
class MultipleScattering final : public ILookUpTable {
public:
    MultipleScattering(Device &device, ResourceManager &resourceManager, Profiler& profiler)
        : ILookUpTable(device, resourceManager, profiler) {
    }

    void initialize(VkDescriptorSetLayout descriptorSetLayout) override;

    void setTransmittance(Image * image) {transmittance = image;};

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

protected:
    Image * transmittance;

    void prepareDescriptors() override;

    void prepareLut() override;

    void preparePipeline(VkDescriptorSetLayout descriptorSetLayout) override;
};