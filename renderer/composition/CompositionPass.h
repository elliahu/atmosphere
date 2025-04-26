#pragma once
#include "../IRenderGroup.h"
#include "../Types.h"

class CompositionPass final : public IRenderGroup {
public:
    CompositionPass(Device &device, ResourceManager &resourceManager, Profiler& profiler)
        : IRenderGroup(device, resourceManager, profiler) {
    }

    void initialize(HmckVec2 resolution);

    void setTerrainColor(Image *image) { terrainColor = image; }
    void setTerrainDepth(Image *image) { terrainDepth = image; }
    void setCloudsColor(Image *image) { cloudsColor = image; }
    void setTransmittanceLUT(Image * image) {transmittanceLUT = image; }
    void setSkyViewLUT(Image * image) {skyViewLUT = image; }
    void setAerialPerspectiveLUT(Image * image) {aerialPerspectiveLUT = image; }
    void setSunShadow(Image *image) { sunShadow = image; }
    void setInvView(HmckMat4 mat) {data.inverseView = mat; }
    void setInvProjection(HmckMat4 mat) {data.inverseProjection = mat; }
    void setShadowViewProj(HmckMat4 mat) {data.shadowViewProj = mat; }
    void setSunDirection(HmckVec4 dir) {data.sunDirection = dir; }
    void setSunColor(HmckVec4 color) {data.sunColor = color; }
    void setAmbientColor(HmckVec4 color) {data.ambientColor = color; }
    void setGodRaysTexture(Image * image) {godRaysTexture = image;}
    void setCameraPosition(HmckVec4 pos) {data.cameraPosition = pos; }
    Image* getColorTarget() { return resourceManager.getResource<Image>(compositedImage); }

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

    CompositionData data{};

private:

    // Target
    ResourceHandle compositedImage; // Final composited image that is passed to the post procsessing pass
    ResourceHandle skyColor; // Sky color image up-sampled from skyViewLUT
    ResourceHandle sampler;
    ResourceHandle buffer;

    // Inputs
    Image *terrainColor;
    Image *terrainDepth;
    Image *cloudsColor;
    Image *transmittanceLUT;
    Image *skyViewLUT;
    Image *aerialPerspectiveLUT;
    Image *sunShadow;
    Image *godRaysTexture;
    ResourceHandle blueNoise;

    // Descriptors
    std::unique_ptr<DescriptorSetLayout> compositionLayout;
    VkDescriptorSet compositionDescriptor;
    std::unique_ptr<DescriptorSetLayout> skyLayout;
    VkDescriptorSet skyDescriptor;

    // Pipeline
    std::unique_ptr<GraphicsPipeline> compositionPipeline;
    std::unique_ptr<GraphicsPipeline> skyPipeline;

    void prepareBlueNoise();
    void prepareBuffer();
    void prepareTargets(uint32_t width, uint32_t height);
    void prepareDescriptors();
    void preparePipelines();
};
