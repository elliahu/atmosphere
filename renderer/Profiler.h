#pragma once
#include <hammock/hammock.h>

using namespace hammock;

// Profiler is used to measure accurately how much time exactly each pass / dispatch takes using
// timestamp queries https://docs.vulkan.org/samples/latest/samples/api/timestamp_queries/README.html
class Profiler final {
public:
    Profiler(Device &device, uint32_t numOfPasses): device(device), numOfPasses(numOfPasses) {
        // First, we need to check if the device supports time stamp queries (most should)
        ASSERT(device.properties.limits.timestampPeriod > 0.f,
               "Device does not support timestamp queries: timestampPeriod is not greater than 0");
        ASSERT(device.properties.limits.timestampComputeAndGraphics,
               "Device does not support timestamp queries: timestampComputeAndGraphics is not true");

        // Allocate space for the timestamps
        // Timestamps are stored along with their availability where n = timestamp, n + 1 = availability value
        // if availability value is > 0, timestamp is avialable
        timestamps.resize(4 * numOfPasses, 1);
        results.resize(numOfPasses);

        // Create a query pool
        VkQueryPoolCreateInfo queryPoolInfo{};
        queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolInfo.queryCount = static_cast<uint32_t>(numOfPasses * 2);
        ASSERT(vkCreateQueryPool(device.device(), &queryPoolInfo, nullptr, &queryPoolTimestamps) == VK_SUCCESS,
               "Failed to allocate query pool timestamps!");
    }

    ~Profiler() {
        vkDestroyQueryPool(device.device(), queryPoolTimestamps, nullptr);
    }

    // This has to be called at the start of the frame to reset the query pool
    void resetTimestamp(VkCommandBuffer cmd, uint32_t query) {
       // if (timestamps[query * 2 + 1] != 0) {
            vkCmdResetQueryPool(cmd, queryPoolTimestamps, query, 1);
       // }

    }

    void writeTimestamp(VkCommandBuffer cmd, uint32_t query, VkPipelineStageFlags2 stage) {
       // if (timestamps[query * 2 + 1] != 0) {
            vkCmdWriteTimestamp2(cmd, stage, queryPoolTimestamps, query);
       // }
    }


    // If available, results are read and stored
    bool getResultsIfAvailable() {
        VkResult result = vkGetQueryPoolResults(
            device.device(),
            queryPoolTimestamps,
            0,
            numOfPasses * 2,
            numOfPasses * 2 * 2 * sizeof(uint64_t),
            timestamps.data(),
            2 * sizeof(uint64_t),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

        ASSERT(result == VK_SUCCESS || result == VK_NOT_READY, "vkGetQueryPoolResults failed!");


        // If the query is available, we can read the results
        for (int i = 0; i < numOfPasses; ++i) {
            uint64_t start = timestamps[i * 4 + 0];
            uint64_t end   = timestamps[i * 4 + 2];
            uint64_t availStart = timestamps[i * 4 + 1];
            uint64_t availEnd   = timestamps[i * 4 + 3];

            if (availStart != 0 && availEnd != 0) {
                float deltaTimeMs = (end - start) * device.properties.limits.timestampPeriod / 1000000.0f;
                results[i] = deltaTimeMs;
            } else {
                return false;
            }
        }

        return true;
    }

    // Get the results may be empty or partially filled
    // Call getResultsIfAvailable before to check if available
    std::vector<float> getResults() {
        return results;
    }

private:


    Device &device;
    VkQueryPool queryPoolTimestamps = VK_NULL_HANDLE;
    uint32_t numOfPasses;
    std::vector<uint64_t> timestamps;
    std::vector<float> results;
};
