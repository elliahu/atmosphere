#pragma once
#include <string>
#include <filesystem>


#include "hammock/core/Device.h"
#include "hammock/legacy/DeviceStorage.h"


namespace hammock {
    class Generator {
    public:
         DeviceStorageResourceHandle<Texture2D> generatePrefilteredMap(Device &device, DeviceStorageResourceHandle<Texture2D> environmentMap, DeviceStorage &resources,
                                    VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT);

         DeviceStorageResourceHandle<Texture2D> generatePrefilteredMapWithStaticRoughness(Device &device, DeviceStorageResourceHandle<Texture2D> environmentMap, DeviceStorage &resources,
                                                       VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT);

         DeviceStorageResourceHandle<Texture2D> generateIrradianceMap(Device &device,  DeviceStorageResourceHandle<Texture2D> environmentMap, DeviceStorage &resources,
                                   VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT,
                                   float _deltaPhi = 180.0f, float _deltaTheta = 64.0f);

         DeviceStorageResourceHandle<Texture2D> generateBRDFLookUpTable(Device &device, DeviceStorage &resources, uint32_t dim = 512,
                                     VkFormat format = VK_FORMAT_R32G32_SFLOAT);

    };
}
