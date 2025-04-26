#pragma once

#include <hammock/hammock.h>
using namespace hammock;

class IScene {
protected:
    VulkanInstance instance;
    hammock::Window window;
    Device device;
    ResourceManager rm;
    FrameManager fm;
    std::unique_ptr<DescriptorPool> descriptorPool;
    std::unique_ptr<RenderGraph> renderGraph;
    std::unique_ptr<UserInterface> ui;
public:
    IScene(const std::string &name, const uint32_t width, const uint32_t height) :
    window{instance, name, static_cast<int>(width), static_cast<int>(height)},
    device{instance, window.getSurface()},
    rm{device},
    fm{window,device} {
        descriptorPool = DescriptorPool::Builder(device)
            .setMaxSets(20000)
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10000)
            .build();

        renderGraph = std::make_unique<RenderGraph>(device, rm, fm, *descriptorPool);
        ui = std::make_unique<UserInterface>(device, fm.getSwapChain()->getRenderPass() , descriptorPool->descriptorPool, window);
    }

    virtual ~IScene() = default;

    virtual void init() = 0;
    virtual void render() = 0;

    static std::string assetPath(const std::string &asset) {
        return "../../../data/" + asset;
    }

    static std::string compiledShaderPath(const std::string &shader) {
        return "../../../examples/cloud_renderer/spv/" + shader + ".spv";
    }
};