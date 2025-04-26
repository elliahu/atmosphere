#include "CloudsPass.h"

void CloudsPass::initialize(HmckVec2 resolution) {
    prepareResources();
    prepareTargets(static_cast<uint32_t>(resolution.X), static_cast<uint32_t>(resolution.Y));
    prepareBuffers();
    prepareDescriptors();
    preparePipelines();
    device.waitIdle();
    processDeletionQueue();
}

void CloudsPass::recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
    // Update buffer
    resourceManager.getResource<Buffer>(uniformBuffers[frameIndex])->writeToBuffer(&uniform);

    // Get target pointers
    // We are drawing into two storage images
    Image *cloudsColorTarget = resourceManager.getResource<Image>(color);
    VkExtent3D cloudsDispatchSize = cloudsColorTarget->getExtent();

    // Storage image do not change layout from VK_IMAGE_LAYOUT_GENERAL the entire frame, so no need for layout transition

    profiler.resetTimestamp(commandBuffer, 2);
    profiler.resetTimestamp(commandBuffer, 3);


    profiler.writeTimestamp(commandBuffer, 2, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

    // Bind descriptor set
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipelineLayout, 0, 1,
                            &descriptors[frameIndex], 0, nullptr);

    // Bind the cloud pipeline
    pipeline->bind(commandBuffer);

    // Push data
    vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                       0, sizeof(CloudsPushConstantData), &properties);


    // Record the first dispatch that performs the raymarching and writes clouds and occlusion mask
    vkCmdDispatch(commandBuffer, CLOUDS_GROUPS_X(cloudsDispatchSize.width), CLOUDS_GROUPS_Y(cloudsDispatchSize.height), 1);

    profiler.writeTimestamp(commandBuffer, 3, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
}

void CloudsPass::prepareBuffers() {
    // Create global uniform buffers
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        uniformBuffers[i] = resourceManager.createResource<Buffer>(
            "clouds-uniform-buffer-" + std::to_string(i), BufferDesc{
                .instanceSize = sizeof(CloudsUniformBufferData),
                .instanceCount = 1,
                .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                .queueFamilies = {CommandQueueFamily::Compute, CommandQueueFamily::Graphics},
                // Global buffer is enabled to be access from both queues at the same time without the need for barriers
                .sharingMode = VK_SHARING_MODE_CONCURRENT,
            }
        );
        // Map the buffers
        resourceManager.getResource<Buffer>(uniformBuffers[i])->map();
    }
}

void CloudsPass::prepareResources() {
    int w, h, c, d;
    // Low frequency noise
    {
        // Read the data from disk
        AutoDelete lowFreqNoiseData(readVolume(Filesystem::ls(ASSET_PATH("base")), w, h, c, d,
                                               Filesystem::ImageFormat::R16G16B16A16_SFLOAT), [](const void *p) {
            delete[] static_cast<const float16_t *>(p);
        });
        // Create staging buffer
        ResourceHandle lowFreqNoiseStagingBuffer = queueForDeletion(resourceManager.createResource<Buffer>(
            "base-noise-staging-buffer",
            BufferDesc{
                .instanceSize = sizeof(float16_t),
                .instanceCount = static_cast<uint32_t>(w * h * d * c),
                .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            }
        ));
        // Write data to the buffer
        resourceManager.getResource<Buffer>(lowFreqNoiseStagingBuffer)->map();
        resourceManager.getResource<Buffer>(lowFreqNoiseStagingBuffer)->writeToBuffer(lowFreqNoiseData.get());

        // Create the actual image resource
        lowFrequencyNoise = resourceManager.createResource<Image>(
            "base-noise",
            ImageDesc{
                .width = static_cast<uint32_t>(w),
                .height = static_cast<uint32_t>(h),
                .channels = static_cast<uint32_t>(c),
                .depth = static_cast<uint32_t>(d),
                .mips = getNumberOfMipLevels(static_cast<uint32_t>(w), static_cast<uint32_t>(h)),
                .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .imageType = VK_IMAGE_TYPE_3D,
                .imageViewType = VK_IMAGE_VIEW_TYPE_3D,
            }
        );

        // Copy data from buffer to image
        resourceManager.getResource<Image>(lowFrequencyNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        resourceManager.getResource<Image>(lowFrequencyNoise)->queueCopyFromBuffer(
            resourceManager.getResource<Buffer>(lowFreqNoiseStagingBuffer)->getBuffer());
        resourceManager.getResource<Image>(lowFrequencyNoise)->generateMips();
        resourceManager.getResource<Image>(lowFrequencyNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    // High frequency noise
    {
        // Read the data from disk
        AutoDelete highFrequencyNoiseData(readVolume(Filesystem::ls(ASSET_PATH("detail")), w, h, c, d,
                                                     Filesystem::ImageFormat::R16G16B16A16_SFLOAT), [](const void *p) {
            delete[] static_cast<const float16_t *>(p);
        });
        // Create staging buffer
        ResourceHandle highFrequencyNoiseStagingBuffer = queueForDeletion(resourceManager.createResource<Buffer>(
            "detail-noise-staging-buffer",
            BufferDesc{
                .instanceSize = sizeof(float16_t),
                .instanceCount = static_cast<uint32_t>(w * h * d * c),
                .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            }
        ));

        // Write data to the buffer
        resourceManager.getResource<Buffer>(highFrequencyNoiseStagingBuffer)->map();
        resourceManager.getResource<Buffer>(highFrequencyNoiseStagingBuffer)->writeToBuffer(highFrequencyNoiseData.get());

        // Create the actual image resource
        highFrequencyNoise = resourceManager.createResource<Image>(
            "detail-noise",
            ImageDesc{
                .width = static_cast<uint32_t>(w),
                .height = static_cast<uint32_t>(h),
                .channels = static_cast<uint32_t>(c),
                .depth = static_cast<uint32_t>(d),
                .mips = getNumberOfMipLevels(static_cast<uint32_t>(w), static_cast<uint32_t>(h)),
                .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                // We only use RGB channels but most devices do not support 3D RGB textures so we use RGBA
                .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .imageType = VK_IMAGE_TYPE_3D,
                .imageViewType = VK_IMAGE_VIEW_TYPE_3D,
            }
        );

        // Copy data from buffer to image
        resourceManager.getResource<Image>(highFrequencyNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        resourceManager.getResource<Image>(highFrequencyNoise)->queueCopyFromBuffer(
            resourceManager.getResource<Buffer>(highFrequencyNoiseStagingBuffer)->getBuffer());
        resourceManager.getResource<Image>(highFrequencyNoise)->generateMips();
        resourceManager.getResource<Image>(highFrequencyNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    // Weather map
    {
        std::string weatherMapName = ASSET_PATH("weather/stratocumulus.png");
        if(weatherMapEnum == WeatherMap::Stratus) {
            weatherMapName = ASSET_PATH("weather/stratus.png");
        } else if(weatherMapEnum == WeatherMap::Stratocumulus) {
            weatherMapName = ASSET_PATH("weather/stratocumulus.png");
        } else if(weatherMapEnum == WeatherMap::Cumulus) {
            weatherMapName = ASSET_PATH("weather/cumulus.png");
        } else if(weatherMapEnum == WeatherMap::Nubis) {
            weatherMapName = ASSET_PATH("weather/nubis.png");
        }

        AutoDelete weatherMapData(readImage(weatherMapName, w, h, c,
                                            Filesystem::ImageFormat::R8G8B8A8_UNORM), [](const void *p) {
            delete[] static_cast<const uchar8_t *>(p);
        });

        // Create host visible staging buffer on device
        ResourceHandle weatherMapStagingBuffer = queueForDeletion(resourceManager.createResource<Buffer>(
            "weather-staging-buffer",
            BufferDesc{
                .instanceSize = sizeof(uchar8_t),
                .instanceCount = static_cast<uint32_t>(w * h * c),
                .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            }
        ));

        // Write the data into the staging buffer
        resourceManager.getResource<Buffer>(weatherMapStagingBuffer)->map();
        resourceManager.getResource<Buffer>(weatherMapStagingBuffer)->writeToBuffer(weatherMapData.get());

        // Create the image resource
        weatherMap = resourceManager.createResource<Image>(
            "weather-map",
            ImageDesc{
                .width = static_cast<uint32_t>(w),
                .height = static_cast<uint32_t>(h),
                .channels = static_cast<uint32_t>(c),
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .imageType = VK_IMAGE_TYPE_2D,
                .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            }
        );

        // Copy the data from buffer into the image
        resourceManager.getResource<Image>(weatherMap)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        resourceManager.getResource<Image>(weatherMap)->queueCopyFromBuffer(
            resourceManager.getResource<Buffer>(weatherMapStagingBuffer)->getBuffer());
        resourceManager.getResource<Image>(weatherMap)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    // Curl noise
    {
        AutoDelete curlNoiseData(readImage(ASSET_PATH("curlNoise.png"), w, h, c,
                                           Filesystem::ImageFormat::R8G8B8A8_UNORM), [](const void *p) {
            delete[] static_cast<const uchar8_t *>(p);
        });

        // Create host visible staging buffer on device
        ResourceHandle curlNoiseStagingBuffer = queueForDeletion(resourceManager.createResource<Buffer>(
            "curl-noise-staging-buffer",
            BufferDesc{
                .instanceSize = sizeof(uchar8_t),
                .instanceCount = static_cast<uint32_t>(w * h * c),
                .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            }
        ));

        // Write the data into the staging buffer
        resourceManager.getResource<Buffer>(curlNoiseStagingBuffer)->map();
        resourceManager.getResource<Buffer>(curlNoiseStagingBuffer)->writeToBuffer(curlNoiseData.get());

        // Create the image resource
        curlNoise = resourceManager.createResource<Image>(
            "curl-noise",
            ImageDesc{
                .width = static_cast<uint32_t>(w),
                .height = static_cast<uint32_t>(h),
                .channels = static_cast<uint32_t>(c),
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .imageType = VK_IMAGE_TYPE_2D,
                .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            }
        );

        // Copy the data from buffer into the image
        resourceManager.getResource<Image>(curlNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        resourceManager.getResource<Image>(curlNoise)->queueCopyFromBuffer(
            resourceManager.getResource<Buffer>(curlNoiseStagingBuffer)->getBuffer());
        resourceManager.getResource<Image>(curlNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}

void CloudsPass::prepareTargets(uint32_t width, uint32_t height) {
    color = resourceManager.createResource<Image>(
        "clouds-image", ImageDesc{
            .width = width,
            .height = height,
            .channels = 4,
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .currentQueueFamily = CommandQueueFamily::Compute,
            .queueFamilies = {CommandQueueFamily::Compute, CommandQueueFamily::Graphics},
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        }
    );
    // Set initial layout
    resourceManager.getResource<Image>(color)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_GENERAL);

    sampler = resourceManager.createResource<Sampler>("clouds-sampler", SamplerDesc{});
}

void CloudsPass::prepareDescriptors() {
    // Layout
    layout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT) // buffer
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT) // Clouds image
            .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // low freq noise
            .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // high freq noise
            .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // weather map
            .addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // Curl
            .addBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // Camera depth
            .build();

    // Default sampler
    Sampler *s = resourceManager.getResource<Sampler>(sampler);
    VkDescriptorImageInfo cloudsImageInfo = resourceManager.getResource<Image>(color)->getDescriptorImageInfo(
        s->getSampler());
    VkDescriptorImageInfo lowFreqNoiseInfo = resourceManager.getResource<Image>(lowFrequencyNoise)->getDescriptorImageInfo(
        s->getSampler());
    VkDescriptorImageInfo highFreqNoiseInfo = resourceManager.getResource<Image>(highFrequencyNoise)->getDescriptorImageInfo(
        s->getSampler());
    VkDescriptorImageInfo weatherMapInfo = resourceManager.getResource<Image>(weatherMap)->getDescriptorImageInfo(
        s->getSampler());
    VkDescriptorImageInfo curlNoiseInfo = resourceManager.getResource<Image>(curlNoise)->getDescriptorImageInfo(
        s->getSampler());
    VkDescriptorImageInfo cameraDepthInfo = cameraDepth->getDescriptorImageInfo(s->getSampler());

    // global descriptor set
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo = resourceManager.getResource<Buffer>(uniformBuffers[i])->descriptorInfo();
        DescriptorWriter(*layout, *descriptorPool)
                .writeBuffer(0, &bufferInfo)
                .writeImage(1, &cloudsImageInfo)
                .writeImage(2, &lowFreqNoiseInfo)
                .writeImage(3, &highFreqNoiseInfo)
                .writeImage(4, &weatherMapInfo)
                .writeImage(5, &curlNoiseInfo)
                .writeImage(6, &cameraDepthInfo)
                .build(descriptors[i]);
    }
}

void CloudsPass::preparePipelines() {
    pipeline = ComputePipeline::create({
        .debugName = "clouds-compute-pipeline",
        .device = device,
        .computeShader{.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH(CLOUDS_SHADER_NAME)),},
        .descriptorSetLayouts = {
            layout->getDescriptorSetLayout()
        },
        .pushConstantRanges{{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(CloudsPushConstantData)}}
    });
}
