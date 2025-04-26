#pragma once
#include "../IRenderGroup.h"
#include "../Types.h"

#define CLOUDS_WORK_GROUP_SIZE_X 16
#define CLOUDS_WORK_GROUP_SIZE_Y 16
#ifndef CLOUD_RENDER_SUBSAMPLE
#define CLOUDS_GROUPS_X(w) ((w + CLOUDS_WORK_GROUP_SIZE_X - 1) / CLOUDS_WORK_GROUP_SIZE_X)
#define CLOUDS_GROUPS_Y(h) ((h + CLOUDS_WORK_GROUP_SIZE_Y - 1) / CLOUDS_WORK_GROUP_SIZE_Y)
#else
#define CLOUDS_GROUPS_X(w) (ceil(w/4/8))
#define CLOUDS_GROUPS_Y(h) (ceil(h/4/8))
#endif

class CloudsPass final : public IRenderGroup {
public:

#ifdef CLOUD_RENDER_SUBSAMPLE
#define CLOUDS_SHADER_NAME "clouds_repr.comp"
#else
#define CLOUDS_SHADER_NAME "clouds.comp"
#endif



    CloudsPass(Device &device, ResourceManager &resourceManager, Profiler& profiler)
        : IRenderGroup(device, resourceManager, profiler) {
    }

    // This is passed as buffer
    CloudsUniformBufferData uniform{};

    // This is passed as push data
    CloudsPushConstantData properties{};


    void initialize(HmckVec2 resolution);


    void setView(const HmckMat4 &view) {
        uniform.view = view;
        uniform.invView = HmckInvGeneral(view);
    }

    void setProjection(const HmckMat4 &projection) {
        uniform.proj = projection;
        uniform.invProj = HmckInvGeneral(projection);
    }

    void setCameraDepth(Image *image) { cameraDepth = image; }

    Image *getColorTarget() const { return resourceManager.getResource<Image>(color); }

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

private:
    // The uniform buffers
    std::array<ResourceHandle, SwapChain::MAX_FRAMES_IN_FLIGHT> uniformBuffers;

    // Descriptors
    std::unique_ptr<DescriptorSetLayout> layout;
    std::array<VkDescriptorSet, SwapChain::MAX_FRAMES_IN_FLIGHT> descriptors;

    // Compute pipeline
    std::unique_ptr<ComputePipeline> pipeline;

    // Targets
    ResourceHandle color;

    // Resources
    ResourceHandle lowFrequencyNoise;
    ResourceHandle highFrequencyNoise;
    ResourceHandle weatherMap;
    ResourceHandle curlNoise;
    ResourceHandle sampler;

    // Inputs
    Image *cameraDepth;

    void prepareBuffers();

    void prepareResources();

    void prepareTargets(uint32_t width, uint32_t height);

    void prepareDescriptors();

    void preparePipelines();
};
