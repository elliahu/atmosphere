#pragma once
// vulkan headers
#include <vulkan/vulkan.h>
#include "hammock/core/Device.h"


namespace hammock {
    // Holds information for a ray tracing acceleration structure
    struct AccelerationStructure {
        VkAccelerationStructureKHR handle;
        uint64_t deviceAddress = 0;
        VkDeviceMemory memory;
        VkBuffer buffer;
    };

    // Holds information for a ray tracing scratch buffer that is used as a temporary storage
    // TODO make this a handle to MemoryManager managed buffer
    struct ScratchBuffer {
        uint64_t deviceAddress = 0;
        VkBuffer handle = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
    };


    class IRayTracingSupport {
    public:
        uint64_t getBufferDeviceAddress(const Device &device, VkBuffer buffer) const;

        void createAccelerationStructure(const Device &device, AccelerationStructure &accelerationStructure,
                                         VkAccelerationStructureTypeKHR type,
                                         const VkAccelerationStructureBuildSizesInfoKHR &buildSizeInfo) const;

        ScratchBuffer createScratchBuffer(const Device &device, VkDeviceSize size) const;

        static void deleteScratchBuffer(const Device &device, const ScratchBuffer &scratchBuffer);

        void deleteAccelerationStructure(const Device &device, const AccelerationStructure &accelerationStructure) const;

        // Function pointers for ray tracing related stuff
        PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
        PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
        PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
        PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
        PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
        PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
        PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
        PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
        PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
        PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

        void loadFunctionPointers(const Device &device);
    };
}
