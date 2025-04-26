#pragma once
#include <hammock/hammock.h>

#include "Profiler.h"

using namespace hammock;

class IRenderGroup {
protected:
    Device& device;
    ResourceManager& resourceManager;
    Profiler& profiler;
    std::unique_ptr<DescriptorPool> descriptorPool;
    std::queue<ResourceHandle> deletionQueue;
public:
    virtual ~IRenderGroup() = default;

    IRenderGroup(Device& device, ResourceManager& resourceManager, Profiler& profiler) : device(device), resourceManager(resourceManager), profiler(profiler) {
        descriptorPool = DescriptorPool::Builder(device)
            .setMaxSets(20000)
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000)
            .build();
    };

    /**
     * Queues resource for deletion
     * @param resource Resource that will be deleted
     * @return ResourceHandle
     */
    ResourceHandle queueForDeletion(ResourceHandle resource) {
        deletionQueue.push(resource);
        return resource;
    }

    /**
     * Deletes all items in the queue
     */
    void processDeletionQueue() {
        while (!deletionQueue.empty()) {
            auto item = deletionQueue.front();
            resourceManager.releaseResource(item.getUid());
            deletionQueue.pop();
        }
    }

    virtual void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) = 0;
};