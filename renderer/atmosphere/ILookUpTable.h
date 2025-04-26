#pragma once
#include "../IRenderGroup.h"
#include "../Types.h"


/**
 * Look Up Table common interface
 */
class ILookUpTable : public IRenderGroup {
public:
    ILookUpTable(Device &device, ResourceManager &resourceManager, Profiler& profiler)
        : IRenderGroup(device, resourceManager, profiler) {
    }

    virtual void initialize(VkDescriptorSetLayout descriptorSetLayout) = 0;

    Image *getLut() const { return resourceManager.getResource<Image>(lut); }
    VkPipelineLayout getPipelineLayout() const { return pipeline->pipelineLayout; }

protected:
    ResourceHandle lut;
    ResourceHandle sampler;
    std::unique_ptr<DescriptorSetLayout> layout;
    VkDescriptorSet descriptor;
    std::unique_ptr<ComputePipeline> pipeline;

    virtual void prepareLut() = 0;
    virtual void prepareDescriptors() = 0;
    virtual void preparePipeline(VkDescriptorSetLayout descriptorSetLayout) = 0;
};
