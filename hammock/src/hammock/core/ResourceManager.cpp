#include "hammock/core/ResourceManager.h"

#include <chrono>

void hammock::ResourceManager::releaseResource(uint64_t id) {
    auto it = resources.find(id);
    if (it != resources.end()) {
        if (it->second->isResident()) {
            totalMemoryUsed -= it->second->getSize();
            it->second->release();
        }
        resources.erase(it);
        resourceCache.erase(id);
    }
}

uint64_t hammock::ResourceManager::getCurrentTimestamp() {
    // Get the current time point
    auto now = std::chrono::system_clock::now();
    // Convert to duration since epoch in milliseconds
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    // Return the count of milliseconds as uint64_t
    return static_cast<uint64_t>(duration.count());
}

void hammock::ResourceManager::evictResources(VkDeviceSize requiredSize) {
    // Sort resources by last used time and use count
    std::vector<std::pair<uint64_t, CacheEntry> > sortedCache;
    for (const auto &entry: resourceCache) {
        sortedCache.push_back(entry);
    }

    std::sort(sortedCache.begin(), sortedCache.end(),
              [](const auto &a, const auto &b) {
                  // Consider both last used time and use frequency
                  return (a.second.lastUsed * a.second.useCount) <
                         (b.second.lastUsed * b.second.useCount);
              });

    // Unload resources until we have enough space
    VkDeviceSize freedMemory = 0;
    for (const auto &entry: sortedCache) {
        auto *resource = resources[entry.first].get();
        if (resource->isResident()) {
            resource->release();
            freedMemory += resource->getSize();
            totalMemoryUsed -= resource->getSize();

            if (freedMemory >= requiredSize) {
                break;
            }
        }
    }
}
