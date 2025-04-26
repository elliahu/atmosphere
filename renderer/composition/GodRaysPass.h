#pragma once
#include "../IRenderGroup.h"
#include "../Types.h"


class GodRaysPass final : public IRenderGroup {
public:
    GodRaysPass(Device &device, ResourceManager &resourceManager, Profiler& profiler)
        : IRenderGroup(device, resourceManager, profiler) {
    }
    void initialize(HmckVec2 resolution);
    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

    void setCloudsImage(Image * image) { cloudsImage = image; }
    void setTerrainDepth(Image * image) { terrainDepth = image; }
    void setSunScreenSpacePosition(float x, float y) {coefficients.lssposX = x; coefficients.lssposY = y; }

    Image * getGodRaysTexture() {return resourceManager.getResource<Image>(godRaysTexture);}

    GodRaysCoefficients coefficients;

private:



    // Targets
    ResourceHandle maskTexture;
    ResourceHandle godRaysTexture;
    ResourceHandle sampler;

    // Inputs
    Image * cloudsImage;
    Image * terrainDepth;

    // Descriptors
    std::unique_ptr<DescriptorSetLayout> maskLayout;
    VkDescriptorSet maskDescriptor;
    std::unique_ptr<DescriptorSetLayout> raysLayout;
    VkDescriptorSet raysDescriptor;

    // Pipeline
    std::unique_ptr<GraphicsPipeline> maskPipeline;
    std::unique_ptr<GraphicsPipeline> raysPipeline;

    void prepareTargets(uint32_t width, uint32_t height);
    void prepareDescriptors();
    void preparePipelines();

};
