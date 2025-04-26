#pragma once
#include "ILookUpTable.h"

#define SKY_VIEW_LUT_SIZE_X 200
#define SKY_VIEW_LUT_SIZE_Y 100

class SkyView final : public ILookUpTable {
public:
    SkyView(Device &device, ResourceManager &resourceManager, Profiler& profiler)
        : ILookUpTable(device, resourceManager, profiler) {
    }

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

    void setTransmittance(Image * image) {transmittance = image;};
    void setMultipleScattering(Image * image) {multipleScattering = image;};

    void initialize(VkDescriptorSetLayout descriptorSetLayout) override;

protected:
    Image * transmittance;
    Image * multipleScattering;

    void prepareLut() override;

    void prepareDescriptors() override;

    void preparePipeline(VkDescriptorSetLayout descriptorSetLayout) override;
};
