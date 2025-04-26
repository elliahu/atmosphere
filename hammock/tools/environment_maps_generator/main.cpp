#include <iostream>

#include <hammock/hammock.h>


#define DIM 512

int main(int argc, char *argv[])
{
    // create resources
    // unable to run headless as of now, so will be using a flash window
    hammock::VulkanInstance instance{};
    hammock::Window window{instance, "Generating Environment Maps", 1, 1};
    hammock::Device device{instance, window.getSurface()};
    hammock::DeviceStorage resources{device};
    hammock::Generator generator{};

    // get the source hdr file
    std::string hdrFilename(argv[1]); // first argument
    if (!hammock::Filesystem::fileExists(hdrFilename))
    {
        std::cerr << "File does not exist " << hdrFilename << std::endl;
        return EXIT_FAILURE;
    }

    // get the irradiance map destination
    std::string irradianceFilename(argv[2]);

    // get the brdf lut map destination
    std::string brdflutFilename(argv[3]);

    float deltaPhi = 180.0f;
    float deltaTheta = 64.0f;

    if (argc > 4)
    {
        deltaPhi = std::stof(argv[4]);
    }

    if (argc > 5)
    {
        deltaTheta = std::stof(argv[5]);
    }

    std::cout << "Generating environment maps..." << std::endl;

    // load the source env and generate rest
    int32_t w, h, c;
    hammock::ScopedMemory environmentData = hammock::ScopedMemory(hammock::Filesystem::readImage(hdrFilename, w, h, c, hammock::Filesystem::ImageFormat::R32G32B32A32_SFLOAT));
    const uint32_t mipLevels = hammock::getNumberOfMipLevels(w, h);
    hammock::DeviceStorageResourceHandle<hammock::Texture2D> environmentMap = resources.createTexture2D({.buffer = environmentData.get(),
                                                                                      .instanceSize = sizeof(float),
                                                                                      .width = static_cast<uint32_t>(w),
                                                                                      .height = static_cast<uint32_t>(h),
                                                                                      .channels = static_cast<uint32_t>(c),
                                                                                      .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                                                                                      .samplerInfo = {
                                                                                          .filter = VK_FILTER_LINEAR,
                                                                                          .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                                                                          .maxLod = static_cast<float>(mipLevels)}});
    hammock::DeviceStorageResourceHandle<hammock::Texture2D> prefilteredMap = generator.generatePrefilteredMap(device, environmentMap, resources);
    hammock::DeviceStorageResourceHandle<hammock::Texture2D> irradianceMap = generator.generateIrradianceMap(device, environmentMap, resources, VK_FORMAT_R32G32B32A32_SFLOAT, deltaPhi, deltaTheta);
    hammock::DeviceStorageResourceHandle<hammock::Texture2D> brdfLUT = generator.generateBRDFLookUpTable(device, resources, 512, VK_FORMAT_R32G32_SFLOAT);

    // Irradiance map
    {
        // create host visible image
        VkImageCreateInfo irradianceImageCreateInfo(hammock::Init::imageCreateInfo());
        irradianceImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        irradianceImageCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        irradianceImageCreateInfo.extent.width = w;
        irradianceImageCreateInfo.extent.height = h;
        irradianceImageCreateInfo.extent.depth = 1;
        irradianceImageCreateInfo.arrayLayers = 1;
        irradianceImageCreateInfo.mipLevels = 1;
        irradianceImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        irradianceImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        irradianceImageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
        irradianceImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VkImage dstIrradianceImage;
        VkDeviceMemory dstIrradianceImageMemory;
        device.createImageWithInfo(irradianceImageCreateInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dstIrradianceImage, dstIrradianceImageMemory);

        // copy on-device image contents to host-visible image
        device.copyImageToHostVisibleImage(resources.getTexture2D(irradianceMap)->image, dstIrradianceImage, w, h);

        // map memory
        const float *irradianceImageData;
        // Get layout of the image (including row pitch)
        VkImageSubresource subResource{};
        subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        VkSubresourceLayout subResourceLayout;
        vkGetImageSubresourceLayout(device.device(), dstIrradianceImage, &subResource, &subResourceLayout);
        // Map image memory so we can start copying from it
        vkMapMemory(device.device(), dstIrradianceImageMemory, 0, VK_WHOLE_SIZE, 0, (void **)&irradianceImageData);
        irradianceImageData += subResourceLayout.offset;

        // write the image
        hammock::Filesystem::writeImage(irradianceFilename, irradianceImageData, sizeof(float), w, h, 4, hammock::Filesystem::WriteImageDefinition::HDR);

        // clean
        vkUnmapMemory(device.device(), dstIrradianceImageMemory);
        vkFreeMemory(device.device(), dstIrradianceImageMemory, nullptr);
        vkDestroyImage(device.device(), dstIrradianceImage, nullptr);

        std::cout << "Irradiance map created at " << irradianceFilename << std::endl;
    }

    // BRDF lut
    {
        // create host visible image
        VkImageCreateInfo brdflutImageCreateInfo(hammock::Init::imageCreateInfo());
        brdflutImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        brdflutImageCreateInfo.format = VK_FORMAT_R32G32_SFLOAT;
        brdflutImageCreateInfo.extent.width = DIM;
        brdflutImageCreateInfo.extent.height = DIM;
        brdflutImageCreateInfo.extent.depth = 1;
        brdflutImageCreateInfo.arrayLayers = 1;
        brdflutImageCreateInfo.mipLevels = 1;
        brdflutImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        brdflutImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        brdflutImageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
        brdflutImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VkImage dstBrdflutImage;
        VkDeviceMemory dstBrdflutImageMemory;
        device.createImageWithInfo(brdflutImageCreateInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dstBrdflutImage, dstBrdflutImageMemory);

        // copy on-device image contents to host-visible image
        device.copyImageToHostVisibleImage(resources.getTexture2D(brdfLUT)->image, dstBrdflutImage, DIM, DIM);

        // map memory
        const float *brdflutImageData;
        // Get layout of the image (including row pitch)
        VkImageSubresource subResource{};
        subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        VkSubresourceLayout subResourceLayout;
        vkGetImageSubresourceLayout(device.device(), dstBrdflutImage, &subResource, &subResourceLayout);
        // Map image memory so we can start copying from it
        vkMapMemory(device.device(), dstBrdflutImageMemory, 0, VK_WHOLE_SIZE, 0, (void **)&brdflutImageData);
        brdflutImageData += subResourceLayout.offset;

        // RG to RGB
        std::vector<float> hdrData(DIM * DIM * 3); // Allocate buffer for 3 channels

        for (uint32_t y = 0; y < DIM; ++y)
        {
            for (uint32_t x = 0; x < DIM; ++x)
            {
                size_t srcIndex = (y * DIM + x) * 2; // 2 channels in source data
                size_t dstIndex = (y * DIM + x) * 3; // 3 channels in destination data

                // Copy R and G, set B to 0
                hdrData[dstIndex + 0] = brdflutImageData[srcIndex + 0]; // R
                hdrData[dstIndex + 1] = brdflutImageData[srcIndex + 1]; // G
                hdrData[dstIndex + 2] = 0.0f;                           // B
            }
        }

        // write the image
        hammock::Filesystem::writeImage(brdflutFilename, hdrData.data(), sizeof(float), DIM, DIM, 3, hammock::Filesystem::WriteImageDefinition::HDR);

        // clean
        vkUnmapMemory(device.device(), dstBrdflutImageMemory);
        vkFreeMemory(device.device(), dstBrdflutImageMemory, nullptr);
        vkDestroyImage(device.device(), dstBrdflutImage, nullptr);

        std::cout << "BRDF Look-up table created at " << brdflutFilename << std::endl;
    }

    // end
    return EXIT_SUCCESS;
}
