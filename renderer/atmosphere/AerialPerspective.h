#pragma once
#include "ILookUpTable.h"

#define AERIAL_PERSPECTIVE_LUT_SIZE_X 32
#define AERIAL_PERSPECTIVE_LUT_SIZE_Y 32
#define AERIAL_PERSPECTIVE_LUT_SIZE_Z 32


class AerialPerspective final : public ILookUpTable {
public:
    AerialPerspective(Device &device, ResourceManager &resourceManager, Profiler& profiler)
        : ILookUpTable(device, resourceManager, profiler) {
    }

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

    void initialize(VkDescriptorSetLayout descriptorSetLayout) override;

    void setMultipleScattering(Image *image) { multipleScattering = image; }
    void setTransmittance(Image *image) { transmittance = image; }
    void setShadowMap(Image *image) { shadowMap = image; }

protected:


    Image *multipleScattering;
    Image *transmittance;
    Image *shadowMap;


    void prepareLut() override;

    void prepareDescriptors() override;

    void preparePipeline(VkDescriptorSetLayout descriptorSetLayout) override;
};
