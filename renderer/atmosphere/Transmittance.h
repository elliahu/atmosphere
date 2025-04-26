#pragma once
#include "ILookUpTable.h"

#define TRANSMITTANCE_LUT_SIZE_X 256
#define TRANSMITTANCE_LUT_SIZE_Y 64

/**
 * Responsible for creating transmittance LUT
 */
class Transmittance : public ILookUpTable {
public:
    Transmittance(Device &device, ResourceManager &resourceManager, Profiler& profiler)
        : ILookUpTable(device, resourceManager, profiler) {
    }

    void initialize(VkDescriptorSetLayout descriptorSetLayout) override;

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

protected:
    void prepareDescriptors() override;

    void prepareLut() override;

    void preparePipeline(VkDescriptorSetLayout descriptorSetLayout) override;

};
