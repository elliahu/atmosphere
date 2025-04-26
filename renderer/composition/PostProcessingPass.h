#pragma once
#include "../IRenderGroup.h"
#include "../Types.h"


class PostProcessingPass final : public IRenderGroup {
public:
    PostProcessingPushConstantData data{};

    PostProcessingPass(Device &device, ResourceManager &resourceManager, Profiler& profiler)
        : IRenderGroup(device, resourceManager, profiler) {
    }

    void initialize();

    void setIinput(Image * image) {input = image;}
    void setSwapChainImage(VkImage image) { swapChainImage = image; }
    void setSwapChainImageView(VkImageView imageView) { swapChainImageView = imageView; }
    void setSwapChainImageFormat(VkFormat format) { swapChainImageFormat = format; }
    void setSwapChainRenderingExtent(VkExtent2D extent) {renderingExtent = extent; }

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

private:

    // Target
    VkImage swapChainImage;
    VkImageView swapChainImageView;
    ResourceHandle sampler;

    // Input
    Image * input;
    VkFormat swapChainImageFormat;
    VkExtent2D renderingExtent;


    // Descriptors
    std::unique_ptr<DescriptorSetLayout> layout;
    VkDescriptorSet descriptor;

    // Pipeline
    std::unique_ptr<GraphicsPipeline> pipeline;

    void prepareSampler() {
        sampler = resourceManager.createResource<Sampler>("composition-sampler", SamplerDesc{});
    }
    void prepareDescriptors();
    void preparePipelines();
};
