#pragma once
#include <functional>
#include <hammock/core/CoreUtils.h>
#include "hammock/core/Types.h"
#include "hammock/core/Image.h"
#include "hammock/core/Buffer.h"
#include "hammock/core/Device.h"

namespace hammock {
    class ResourceManager;

    class ResourceFactory final {
        friend class ResourceManager;

        template<typename T, typename... Args>
        static std::unique_ptr<T> create(Device &device, uint64_t id, Args &&... args) {
            return std::make_unique<T>(device, id, std::forward<Args>(args)...);
        }
    };


    // Resource Manager class
    class ResourceManager final {
    private:
        using ResourceMap = std::unordered_map<uint64_t, std::unique_ptr<Resource> >;

        Device &device;
        ResourceMap resources;

        VkDeviceSize totalMemoryUsed;
        VkDeviceSize memoryBudget;
        uint64_t nextId;

        // Cache for frequently used resources
        struct CacheEntry {
            uint64_t lastUsed;
            uint64_t useCount;
        };

        std::unordered_map<uint64_t, CacheEntry> resourceCache;

    public:
        explicit ResourceManager(Device &device, VkDeviceSize memoryBudget = 6ULL * 1024 * 1024 * 1024)
        // 6GB default
            : device(device), totalMemoryUsed(0), memoryBudget(memoryBudget), nextId(1) {
        }

        template<typename T, typename... Args>
        ResourceHandle createResource(Args &&... args) {
            static_assert(ResourceTypeTraits<T>::type != ResourceType::Invalid,
                          "Resource type not registered in ResourceTypeTraits");

            auto resource = ResourceFactory::create<T>(device, nextId, std::forward<Args>(args)...);
            uint64_t id = nextId++;

            // if (totalMemoryUsed + resource->getSize() > memoryBudget) {
            //     evictResources(resource->getSize());
            // }

            resource->create();

            resources[id] = std::move(resource);
            resourceCache[id] = {getCurrentTimestamp(), 0};

            return ResourceHandle::create(ResourceTypeTraits<T>::type, id);
        }

        template<typename T>
        T *getResource(ResourceHandle handle) {
            // Type check
            if (ResourceTypeTraits<T>::type != handle.getType()) {
                return nullptr;
            }

            auto it = resources.find(handle.getUid());
            if (it != resources.end()) {
                auto *resource = static_cast<T *>(it->second.get());

                // Update cache information
                resourceCache[handle.getUid()].lastUsed = getCurrentTimestamp();
                resourceCache[handle.getUid()].useCount++;

                // Load if not resident
                if (!resource->isResident()) {
                    resource->create();
                    totalMemoryUsed += resource->getSize();
                }
                return resource;
            }
            return nullptr;
        } /**
             * @deprecated Should use transfer queue instead
             */
        ResourceHandle createVertexBuffer(VkDeviceSize vertexSize, uint32_t vertexCount, void *data,
                                          VkBufferUsageFlags usageFlags = 0) {
            auto stagingBufferHandle = createResource<Buffer>("staging-vertex-buffer", BufferDesc{
                                                                  .instanceSize = vertexSize,
                                                                  .instanceCount = vertexCount,
                                                                  .usageFlags =
                                                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                                  .allocationFlags =
                                                                  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                                                              });

            auto stagingBuffer = getResource<Buffer>(stagingBufferHandle);

            stagingBuffer->map();
            stagingBuffer->writeToBuffer(data);

            auto vertexBufferHandle = createResource<Buffer>("vertex-buffer", BufferDesc{
                                                                 .instanceSize = vertexSize,
                                                                 .instanceCount = vertexCount,
                                                                 .usageFlags =
                                                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                                 VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                                 usageFlags,
                                                                 .allocationFlags =
                                                                 VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                                                             });

            auto vertexBuffer = getResource<Buffer>(vertexBufferHandle);

            device.copyBuffer(stagingBuffer->getBuffer(), vertexBuffer->getBuffer(),
                              vertexCount * vertexSize);
            return vertexBufferHandle;
        }


        /**
         * @deprecated Should use transfer queue instead
         */
        ResourceHandle createIndexBuffer(VkDeviceSize indexSize, uint32_t indexCount, void *data,
                                         VkBufferUsageFlags usageFlags = 0) {
            auto stagingBufferHandle = createResource<Buffer>("staging-index-buffer", BufferDesc{
                                                                  .instanceSize = indexSize,
                                                                  .instanceCount = indexCount,
                                                                  .usageFlags =
                                                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                                  .allocationFlags =
                                                                  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                                                              });

            auto stagingBuffer = getResource<Buffer>(stagingBufferHandle);

            stagingBuffer->map();
            stagingBuffer->writeToBuffer(data);

            auto indexBufferHandle = createResource<Buffer>("vertex-buffer", BufferDesc{
                                                                .instanceSize = indexSize,
                                                                .instanceCount = indexCount,
                                                                .usageFlags =
                                                                VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                                                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                                usageFlags,
                                                                .allocationFlags =
                                                                VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                                                            });

            auto indexBuffer = getResource<Buffer>(indexBufferHandle);

            device.copyBuffer(stagingBuffer->getBuffer(), indexBuffer->getBuffer(),
                              indexCount * indexSize);
            return indexBufferHandle;
        }

        // Helper to get resource type name
        const char *getResourceTypeName(ResourceHandle handle) {
            return handle.getTypeName();
        }

        void releaseResource(uint64_t id);

    private:
        uint64_t getCurrentTimestamp();

        void evictResources(VkDeviceSize requiredSize);
    };
}
