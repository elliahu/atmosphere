#include "Renderer.h"

#include <future>

void Renderer::processDeletionQueue() {
    while (!deletionQueue.empty()) {
        auto item = deletionQueue.front();
        resourceManager.releaseResource(item.getUid());
        deletionQueue.pop();
    }
}

void Renderer::allocateCommandBuffers() {
    // Allocate graphics command buffers
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = device.getGraphicsCommandPool();
    allocInfo.commandBufferCount = SwapChain::MAX_FRAMES_IN_FLIGHT;

    ASSERT(vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.terrain.data()) == VK_SUCCESS,
           "Failed to allocate terrain command buffers!");
    ASSERT(vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.depth.data()) == VK_SUCCESS,
           "Failed to allocate depth command buffers!");
    ASSERT(vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.composition.data()) == VK_SUCCESS,
           "Failed to allocate post process command buffers!");
    ASSERT(vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.graphicsToComputeTransferRelease.data()) == VK_SUCCESS,
           "Failed to allocate graphics to compute transfer release command buffers!");
    ASSERT(vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.computeToGraphicsTransferAcquire.data()) == VK_SUCCESS,
           "Failed to allocate compute to graphics transfer acquire buffers!");


    // Allocate compute command buffers
    allocInfo.commandPool = device.getComputeCommandPool();
    ASSERT(vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.clouds.data()) == VK_SUCCESS,
           "Failed to allocate clouds command buffers!");
    ASSERT(vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.atmosphere.data()) == VK_SUCCESS,
           "Failed to allocate atmosphere command buffers!");
    ASSERT(vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.computeToGraphicsTransferRelease.data()) == VK_SUCCESS,
           "Failed to allocate compute to graphics transfer release buffers!");
    ASSERT(vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.graphicsToComputeTransferAcquire.data()) == VK_SUCCESS,
           "Failed to allocate graphics to compute transfer acquire command buffers!");
}

void Renderer::destroyCommandBuffers() {
    // Free graphics command buffers
    vkFreeCommandBuffers(
        device.device(),
        device.getGraphicsCommandPool(),
        static_cast<uint32_t>(commandBuffers.terrain.size()),
        commandBuffers.terrain.data());

    vkFreeCommandBuffers(
        device.device(),
        device.getGraphicsCommandPool(),
        static_cast<uint32_t>(commandBuffers.composition.size()),
        commandBuffers.composition.data());

    vkFreeCommandBuffers(
        device.device(),
        device.getGraphicsCommandPool(),
        static_cast<uint32_t>(commandBuffers.depth.size()),
        commandBuffers.depth.data());

    vkFreeCommandBuffers(
        device.device(),
        device.getGraphicsCommandPool(),
        static_cast<uint32_t>(commandBuffers.graphicsToComputeTransferRelease.size()),
        commandBuffers.graphicsToComputeTransferRelease.data());

    // Free compute command buffers
    vkFreeCommandBuffers(
        device.device(),
        device.getComputeCommandPool(),
        static_cast<uint32_t>(commandBuffers.clouds.size()),
        commandBuffers.clouds.data());

    vkFreeCommandBuffers(
        device.device(),
        device.getComputeCommandPool(),
        static_cast<uint32_t>(commandBuffers.atmosphere.size()),
        commandBuffers.atmosphere.data());

    vkFreeCommandBuffers(
        device.device(),
        device.getComputeCommandPool(),
        static_cast<uint32_t>(commandBuffers.computeToGraphicsTransferRelease.size()),
        commandBuffers.computeToGraphicsTransferRelease.data());
}

void Renderer::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        ASSERT(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &semaphores.depthReady[i]) == VK_SUCCESS,
               "Failed to create depth ready semaphore!");
        ASSERT(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &semaphores.cloudsReady[i]) == VK_SUCCESS,
               "Failed to create clouds ready semaphore!");
        ASSERT(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &semaphores.atmosphereReady[i]) == VK_SUCCESS,
               "Failed to create atmosphere ready semaphore!");
        ASSERT(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &semaphores.terrainColorReady[i]) == VK_SUCCESS,
               "Failed to create terrain ready semaphore!");
        ASSERT(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &semaphores.graphicsToComputeTransferClouds[i]) == VK_SUCCESS,
               "Failed to create graphics to compute transfer clouds semaphore!");
        ASSERT(
            vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &semaphores.graphicsToComputeTransferAtmosphere[i]) == VK_SUCCESS,
            "Failed to create graphics to compute transfer atmosphere semaphore!");
        ASSERT(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &semaphores.graphicsToComputeTransferGeometry[i]) == VK_SUCCESS,
               "Failed to create graphics to compute transfer G semaphore!");
        ASSERT(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &semaphores.computeToGraphicsTransfer[i]) == VK_SUCCESS,
               "Failed to create compute to graphics transfer semaphore!");
        ASSERT(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &semaphores.computeToGraphicsSync[i]) == VK_SUCCESS,
               "Failed to create compute to graphics sync semaphore!");
        ASSERT(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &semaphores.graphicsToComputeSync[i]) == VK_SUCCESS,
               "Failed to create graphics to compute sync  semaphore!");
        ASSERT(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &semaphores.computeToComputeSync[i]) == VK_SUCCESS,
               "Failed to create compute to compute sync  semaphore!");
    }
}

void Renderer::destroySyncObjects() {
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device.device(), semaphores.depthReady[i], nullptr);
        vkDestroySemaphore(device.device(), semaphores.cloudsReady[i], nullptr);
        vkDestroySemaphore(device.device(), semaphores.atmosphereReady[i], nullptr);
        vkDestroySemaphore(device.device(), semaphores.terrainColorReady[i], nullptr);
        vkDestroySemaphore(device.device(), semaphores.graphicsToComputeTransferClouds[i], nullptr);
        vkDestroySemaphore(device.device(), semaphores.graphicsToComputeTransferAtmosphere[i], nullptr);
        vkDestroySemaphore(device.device(), semaphores.graphicsToComputeTransferGeometry[i], nullptr);
        vkDestroySemaphore(device.device(), semaphores.computeToGraphicsTransfer[i], nullptr);
        vkDestroySemaphore(device.device(), semaphores.graphicsToComputeSync[i], nullptr);
        vkDestroySemaphore(device.device(), semaphores.computeToGraphicsSync[i], nullptr);
        vkDestroySemaphore(device.device(), semaphores.computeToComputeSync[i], nullptr);
    }
}

void Renderer::prepareGeometry() {
    Loader(geometry, device, resourceManager).loadglTF(ASSET_PATH("terrain.glb"));

    ASSERT(!geometry.vertices.empty(), "No vertices loaded!");

    vertexBuffer = resourceManager.createResource<Buffer>(
        "vertex-buffer", BufferDesc{
            .instanceSize = sizeof(Vertex),
            .instanceCount = static_cast<uint32_t>(geometry.vertices.size()),
            .usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .currentQueueFamily = CommandQueueFamily::Ignored,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        }
    );

    // Create vertex staging buffer
    ResourceHandle vertexStagingBuffer = queueForDeletion(resourceManager.createResource<Buffer>(
        "vertex-staging-buffer", BufferDesc{
            .instanceSize = sizeof(Vertex),
            .instanceCount = static_cast<uint32_t>(geometry.vertices.size()),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    ));

    // Write vertex data to the buffer
    resourceManager.getResource<Buffer>(vertexStagingBuffer)->map();
    resourceManager.getResource<Buffer>(vertexStagingBuffer)->writeToBuffer(geometry.vertices.data());

    // Copy data from staging buffer to actual vertex buffer
    VkDeviceSize vertexBufferSize = sizeof(Vertex) * geometry.vertices.size();
    resourceManager.getResource<Buffer>(vertexBuffer)->queuCopyFromBuffer(
        resourceManager.getResource<Buffer>(vertexStagingBuffer)->getBuffer(), vertexBufferSize);
    resourceManager.getResource<Buffer>(vertexStagingBuffer)->unmap();

    // Create index buffer
    indexBuffer = resourceManager.createResource<Buffer>(
        "index-buffer", BufferDesc{
            .instanceSize = sizeof(uint32_t),
            .instanceCount = static_cast<uint32_t>(geometry.indices.size()),
            .usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .currentQueueFamily = CommandQueueFamily::Ignored,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        }
    );

    // Create index staging buffer
    ResourceHandle indexStagingBuffer = queueForDeletion(resourceManager.createResource<Buffer>(
        "index-staging-buffer", BufferDesc{
            .instanceSize = sizeof(uint32_t),
            .instanceCount = static_cast<uint32_t>(geometry.indices.size()),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    ));

    // Write index data into the staging buffer
    resourceManager.getResource<Buffer>(indexStagingBuffer)->map();
    resourceManager.getResource<Buffer>(indexStagingBuffer)->writeToBuffer(geometry.indices.data());

    // Copy the data from staging buffer into actual index buffer
    VkDeviceSize indexBufferSize = sizeof(uint32_t) * geometry.indices.size();
    resourceManager.getResource<Buffer>(indexBuffer)->queuCopyFromBuffer(
        resourceManager.getResource<Buffer>(indexStagingBuffer)->getBuffer(), indexBufferSize);
    resourceManager.getResource<Buffer>(indexStagingBuffer)->unmap();
}

void Renderer::init() {
    allocateCommandBuffers();
    createSyncObjects();
    prepareGeometry();

    // Initialize threadpool
    processorCount = std::thread::hardware_concurrency();
    ASSERT(processorCount > 0, "Failed to detect number of processors!");
    Logger::log(LOG_LEVEL_DEBUG, "Found %d available threads\n", processorCount);
    threadPool.setThreadCount(2);

    // Wait again
    device.waitIdle();

    // Delete staging buffers
    processDeletionQueue();

    // Initialize passes
    depthPass.setVertexBuffer(resourceManager.getResource<Buffer>(vertexBuffer));
    depthPass.setIndexBuffer(resourceManager.getResource<Buffer>(indexBuffer));
    depthPass.initialize(HmckVec2{(float) lWidth, (float) lHeight});

    atmospherePass.setShadowMap(depthPass.getSunDepth());
    atmospherePass.initialize();


    geometryPass.setVertexBuffer(resourceManager.getResource<Buffer>(vertexBuffer));
    geometryPass.setIndexBuffer(resourceManager.getResource<Buffer>(indexBuffer));
    geometryPass.initialize(HmckVec2{static_cast<float>(lWidth), static_cast<float>(lHeight)});

    cloudsPass.setCameraDepth(depthPass.getCameraDepth());
    cloudsPass.initialize(HmckVec2{static_cast<float>(lWidth), static_cast<float>(lHeight)});

    godRaysPass.setCloudsImage(cloudsPass.getColorTarget());
    godRaysPass.setTerrainDepth(geometryPass.getDepthTarget());
    godRaysPass.initialize(HmckVec2{static_cast<float>(lWidth) * 0.5f, static_cast<float>(lHeight) * 0.5f});

    compositionPass.setCloudsColor(cloudsPass.getColorTarget());
    compositionPass.setTerrainColor(geometryPass.getColorTarget());
    compositionPass.setTerrainDepth(geometryPass.getDepthTarget());
    compositionPass.setTransmittanceLUT(atmospherePass.transmittance.getLut());
    compositionPass.setSkyViewLUT(atmospherePass.skyView.getLut());
    compositionPass.setAerialPerspectiveLUT(atmospherePass.aerialPerspective.getLut());
    compositionPass.setSunShadow(depthPass.getSunDepth());
    compositionPass.setGodRaysTexture(godRaysPass.getGodRaysTexture());
    compositionPass.initialize(HmckVec2{static_cast<float>(lWidth), static_cast<float>(lHeight)});

    postProcessingPass.setIinput(compositionPass.getColorTarget());
    postProcessingPass.setSwapChainImageFormat(frameManager.getSwapChain()->getSwapChainImageFormat());
    postProcessingPass.initialize();


    ui->setCamera(&camera);
    ui->setCloudsPushData(&cloudsPass.properties);
    ui->setCloudsUniformData(&cloudsPass.uniform);
    ui->setPostProccessingData(&postProcessingPass.data);
    ui->setCompositionData(&compositionPass.data);
    ui->setGodRaysCoefficients(&godRaysPass.coefficients);
}


void Renderer::recordSwapChainImageTransition(VkImageLayout from, VkImageLayout to, uint32_t frameIndex, uint32_t imageIndex) {
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;


    // Transition the swap chain image into present layout
    transitionImageLayout(commandBuffers.composition[frameIndex],
                          frameManager.getSwapChain()->getImage(imageIndex),
                          from, to, subresourceRange);
}

void Renderer::recordGraphicsToComputeTransfers(uint32_t frameIndex) {
    // Release first
    {
        VkCommandBuffer releaseCommandBuffer = commandBuffers.graphicsToComputeTransferRelease[frameIndex];
        frameManager.beginCommandBuffer(releaseCommandBuffer);

        depthPass.getCameraDepth()->pipelineBarrier(
            releaseCommandBuffer,
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // Changed from NONE
            VK_ACCESS_SHADER_READ_BIT, // Changed from NONE
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            device.getGraphicsQueueFamilyIndex(),
            device.getComputeQueueFamilyIndex()

        );

        depthPass.getSunDepth()->pipelineBarrier(
            releaseCommandBuffer,
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // Changed from NONE
            VK_ACCESS_SHADER_READ_BIT, // Changed from NONE
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            device.getGraphicsQueueFamilyIndex(),
            device.getComputeQueueFamilyIndex()

        );

        VkSemaphoreSubmitInfo waitSemaphore = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = semaphores.depthReady[frameIndex],
            .stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        };

        VkSemaphoreSubmitInfo signalSemaphore = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = semaphores.graphicsToComputeSync[frameIndex],
            .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        };

        VkCommandBufferSubmitInfo cmdBufInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = releaseCommandBuffer,
        };

        VkSubmitInfo2 submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .waitSemaphoreInfoCount = 1,
            .pWaitSemaphoreInfos = &waitSemaphore,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &cmdBufInfo,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signalSemaphore,
        };

        vkEndCommandBuffer(releaseCommandBuffer);
        vkQueueSubmit2(device.graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    }

    // Acquire next
    {
        VkCommandBuffer acquireCommandBuffer = commandBuffers.graphicsToComputeTransferAcquire[frameIndex];
        frameManager.beginCommandBuffer(acquireCommandBuffer);
        depthPass.getCameraDepth()->pipelineBarrier(
            acquireCommandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // Changed from NONE
            VK_ACCESS_MEMORY_READ_BIT, // Changed from NONE
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            device.getGraphicsQueueFamilyIndex(),
            device.getComputeQueueFamilyIndex()

        );
        depthPass.getSunDepth()->pipelineBarrier(
            acquireCommandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // Changed from NONE
            VK_ACCESS_MEMORY_READ_BIT, // Changed from NONE
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            device.getGraphicsQueueFamilyIndex(),
            device.getComputeQueueFamilyIndex()

        );


        VkSemaphoreSubmitInfo waitSemaphore = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = semaphores.graphicsToComputeSync[frameIndex],
            .stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, // Changed from NONE
        };

        VkSemaphoreSubmitInfo signalSemaphores[] = {
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = semaphores.graphicsToComputeTransferClouds[frameIndex],
                .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            },
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = semaphores.graphicsToComputeTransferAtmosphere[frameIndex],
                .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            },
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = semaphores.graphicsToComputeTransferGeometry[frameIndex],
                .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            }
        };

        VkCommandBufferSubmitInfo cmdBufInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = acquireCommandBuffer,
        };

        VkSubmitInfo2 submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .waitSemaphoreInfoCount = 1,
            .pWaitSemaphoreInfos = &waitSemaphore,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &cmdBufInfo,
            .signalSemaphoreInfoCount = 3,
            .pSignalSemaphoreInfos = signalSemaphores,
        };

        vkEndCommandBuffer(acquireCommandBuffer);
        vkQueueSubmit2(device.computeQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    }
}

void Renderer::recordComputeToGraphicsTransfers(uint32_t frameIndex) {
    // Release first
    {
        VkCommandBuffer releaseCommandBuffer = commandBuffers.computeToGraphicsTransferRelease[frameIndex];
        frameManager.beginCommandBuffer(releaseCommandBuffer);

        cloudsPass.getColorTarget()->pipelineBarrier(
            releaseCommandBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_NONE,
            VK_ACCESS_NONE,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_GENERAL,
            device.getComputeQueueFamilyIndex(),
            device.getGraphicsQueueFamilyIndex()
        );

        atmospherePass.transmittance.getLut()->pipelineBarrier(
            releaseCommandBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_NONE,
            VK_ACCESS_NONE,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_GENERAL,
            device.getComputeQueueFamilyIndex(),
            device.getGraphicsQueueFamilyIndex()
        );

        atmospherePass.skyView.getLut()->pipelineBarrier(
            releaseCommandBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_NONE,
            VK_ACCESS_NONE,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_GENERAL,
            device.getComputeQueueFamilyIndex(),
            device.getGraphicsQueueFamilyIndex()
        );

        atmospherePass.aerialPerspective.getLut()->pipelineBarrier(
            releaseCommandBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_NONE,
            VK_ACCESS_NONE,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_GENERAL,
            device.getComputeQueueFamilyIndex(),
            device.getGraphicsQueueFamilyIndex()
        );

        VkSemaphoreSubmitInfo waitSemaphores[] = {
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = semaphores.cloudsReady[frameIndex],
                .stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            },
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = semaphores.atmosphereReady[frameIndex],
                .stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            }
        };

        VkSemaphoreSubmitInfo signalSemaphore = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = semaphores.computeToGraphicsSync[frameIndex],
            .stageMask = VK_PIPELINE_STAGE_2_NONE,
        };

        VkCommandBufferSubmitInfo cmdBufInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = releaseCommandBuffer,
        };

        VkSubmitInfo2 submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .waitSemaphoreInfoCount = 2,
            .pWaitSemaphoreInfos = waitSemaphores,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &cmdBufInfo,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signalSemaphore,
        };

        vkEndCommandBuffer(releaseCommandBuffer);
        vkQueueSubmit2(device.computeQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    }

    // Acquire next
    {
        VkCommandBuffer acquireCommandBuffer = commandBuffers.computeToGraphicsTransferAcquire[frameIndex];
        frameManager.beginCommandBuffer(acquireCommandBuffer);

        cloudsPass.getColorTarget()->pipelineBarrier(
            acquireCommandBuffer,
            VK_PIPELINE_STAGE_NONE,
            VK_ACCESS_NONE,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_GENERAL,
            device.getComputeQueueFamilyIndex(),
            device.getGraphicsQueueFamilyIndex()
        );

        atmospherePass.transmittance.getLut()->pipelineBarrier(
            acquireCommandBuffer,
            VK_PIPELINE_STAGE_NONE,
            VK_ACCESS_NONE,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_GENERAL,
            device.getComputeQueueFamilyIndex(),
            device.getGraphicsQueueFamilyIndex()
        );

        atmospherePass.skyView.getLut()->pipelineBarrier(
            acquireCommandBuffer,
            VK_PIPELINE_STAGE_NONE,
            VK_ACCESS_NONE,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_GENERAL,
            device.getComputeQueueFamilyIndex(),
            device.getGraphicsQueueFamilyIndex()
        );

        atmospherePass.aerialPerspective.getLut()->pipelineBarrier(
            acquireCommandBuffer,
            VK_PIPELINE_STAGE_NONE,
            VK_ACCESS_NONE,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_GENERAL,
            device.getComputeQueueFamilyIndex(),
            device.getGraphicsQueueFamilyIndex()
        );

        VkSemaphoreSubmitInfo waitSemaphore = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = semaphores.computeToGraphicsSync[frameIndex],
            .stageMask = VK_PIPELINE_STAGE_2_NONE, // Only barriers in the command buffer
        };

        VkSemaphoreSubmitInfo signalSemaphore = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = semaphores.computeToGraphicsTransfer[frameIndex],
            .stageMask = VK_PIPELINE_STAGE_2_NONE,
        };

        VkCommandBufferSubmitInfo cmdBufInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = acquireCommandBuffer,
        };

        VkSubmitInfo2 submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .waitSemaphoreInfoCount = 1,
            .pWaitSemaphoreInfos = &waitSemaphore,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &cmdBufInfo,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signalSemaphore,
        };

        vkEndCommandBuffer(acquireCommandBuffer);
        vkQueueSubmit2(device.graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    }
}


void Renderer::handleInput() {
    // Process rotation input using arrow keys.
    // Rotate left/right (yaw)
    if (window.isKeyDown(Surfer::KeyCode::ArrowLeft))
        camera.yaw += rotationSpeed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::ArrowRight))
        camera.yaw -= rotationSpeed * deltaTime;

    // Rotate up/down (pitch)
    if (window.isKeyDown(Surfer::KeyCode::ArrowUp))
        camera.pitch += rotationSpeed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::ArrowDown))
        camera.pitch -= rotationSpeed * deltaTime;

    // Clamp pitch to avoid excessive rotation
    const float pitchLimit = 1.55334f; // ~89 degrees in radians
    if (camera.pitch > pitchLimit)
        camera.pitch = pitchLimit;
    if (camera.pitch < -pitchLimit)
        camera.pitch = -pitchLimit;

    // Process translation input (WASD keys)
    if (window.isKeyDown(Surfer::KeyCode::KeyW))
        camera.position += camera.forwardDirection() * movementSpeed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::KeyS))
        camera.position -= camera.forwardDirection() * movementSpeed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::KeyA))
        camera.position += camera.rightDirection() * movementSpeed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::KeyD))
        camera.position -= camera.rightDirection() * movementSpeed * deltaTime;

    if (window.isKeyDown(Surfer::KeyCode::Space))
        camera.position -= camera.upDirection() * movementSpeed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::LeftShift))
        camera.position += camera.upDirection() * movementSpeed * deltaTime;
}

Renderer::Renderer(const int32_t width, const int32_t height)
    : window{instance, "Vulkan atmospheric renderer", static_cast<int>(width), static_cast<int>(height)},
      device{instance, window.getSurface()}, resourceManager{device}, frameManager{window, device}, profiler{device, 12},
      lWidth{static_cast<uint32_t>(width)}, lHeight{static_cast<uint32_t>(height)}, depthPass(device, resourceManager, profiler, geometry),
      geometryPass(device, resourceManager, profiler, geometry), cloudsPass(device, resourceManager, profiler),
      atmospherePass(device, resourceManager, profiler),
      godRaysPass(device, resourceManager, profiler),
      compositionPass(device, resourceManager, profiler), postProcessingPass(device, resourceManager, profiler) {
    // Initialize the descriptor pool object from which descriptors will be allocated
    descriptorPool = DescriptorPool::Builder(device)
            .setMaxSets(20000)
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000)
            .build();
    ui = std::make_unique<::UserInterface>(device, frameManager, descriptorPool->descriptorPool, window, deltaTime, frameTimes,
                                           FRAMETIME_BUFFER_SIZE, frameTimeFrameIndex);
    init();
}

Renderer::~Renderer() {
    destroyCommandBuffers();
    destroySyncObjects();
}

void Renderer::update() {
    // Update camera
    camera.aspect = frameManager.getAspectRatio();

    HmckMat4 projection = camera.getProjection();
    HmckMat4 view = camera.getView();
    HmckMat4 inverseProjection = HmckInvGeneral(projection);
    HmckMat4 inverseView = HmckInvGeneral(view);
    Camera::FrustumDirections frustum = camera.getFrustumDirections();

    // Update cloud pass
    cloudsPass.setProjection(projection);
    cloudsPass.setView(view);
    cloudsPass.uniform.fov = HmckToDeg(HmckAngleRad(camera.fov));
    cloudsPass.uniform.cameraPosition = HmckVec4{camera.position, 0.0f};
    cloudsPass.uniform.resX = lWidth;
    cloudsPass.uniform.resY = lHeight;
    cloudsPass.uniform.znear = camera.znear;
    cloudsPass.uniform.zfar = camera.zfar;
    cloudsPass.uniform.frameIndexMod16 = frameIndex % 16;
    if (progressTime) {
        cloudsPass.uniform.time = elapsedTime;
    }


    // Update geometry pass
    geometryPass.setProjection(projection);
    geometryPass.setView(camera.getView());
    geometryPass.setLightColor(cloudsPass.uniform.lightColor.XYZ);
    geometryPass.setLightDirection(cloudsPass.uniform.lightDirection.XYZ);


    // Update post processing data
    postProcessingPass.setSwapChainImage(frameManager.getSwapChain()->getImage(frameManager.getSwapChainImageIndex()));
    postProcessingPass.setSwapChainImageView(frameManager.getSwapChain()->getImageView(frameManager.getSwapChainImageIndex()));
    postProcessingPass.setSwapChainRenderingExtent(frameManager.getSwapChain()->getSwapChainExtent());
    postProcessingPass.data.time = elapsedTime;

    // Update atmosphere
    atmospherePass.setEye(camera.position);
    atmospherePass.setSunDirection(cloudsPass.uniform.lightDirection.XYZ);

    HmckMat4 shadowProjection = Projection().orthographic(-120.0, 120.0, 120.0, -120.0, camera.znear, camera.zfar, true);
    HmckMat4 shadowView = HmckLookAt_RH(cloudsPass.uniform.lightDirection.XYZ * 200.0, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    HmckMat4 shadowViewProjection = shadowProjection * shadowView;

    atmospherePass.setShadowViewProjection(shadowViewProjection);
    atmospherePass.setCameraInverseProjection(inverseProjection);
    atmospherePass.setCameraInverseView(inverseView);
    atmospherePass.setCameraFrustum(
        HmckVec4{frustum.frustumA, 0.0f},
        HmckVec4{frustum.frustumB, 0.0f},
        HmckVec4{frustum.frustumC, 0.0f},
        HmckVec4{frustum.frustumD, 0.0f}
    );

    // Update depth pass
    depthPass.setCameraProjection(projection);
    depthPass.setCameraView(view);

    depthPass.setSunProjection(shadowProjection);
    depthPass.setSunView(shadowView);

    HmckVec3 simulatedSunPos = camera.position - cloudsPass.uniform.lightDirection.XYZ * 1000.0f;
    HmckVec4 clipSpaceSunPos = projection * view * HmckVec4{simulatedSunPos.X, simulatedSunPos.Y, simulatedSunPos.Z, 1.0f};
    HmckVec3 ndcSunPos = {
        clipSpaceSunPos.X / clipSpaceSunPos.W,
        clipSpaceSunPos.Y / clipSpaceSunPos.W,
        clipSpaceSunPos.Z / clipSpaceSunPos.W
    };

    // Convert NDC [-1,1] to screen space [0,1]
    HmckVec2 screenSpaceSunPos = {
        (ndcSunPos.X + 1.0f) * 0.5f,
        (ndcSunPos.Y + 1.0f) * 0.5f
    };

    godRaysPass.setSunScreenSpacePosition(screenSpaceSunPos.X, screenSpaceSunPos.Y);

    compositionPass.setCameraPosition(HmckVec4{camera.position, 0.0f});
    compositionPass.setInvView(inverseView);
    compositionPass.setInvProjection(inverseProjection);
    compositionPass.setShadowViewProj(shadowViewProjection);
    compositionPass.setSunDirection(cloudsPass.uniform.lightDirection);
    compositionPass.setSunColor(cloudsPass.uniform.lightColor);
    compositionPass.setAmbientColor(cloudsPass.uniform.skyColorZenith);
}


void Renderer::render() {
    // Get the current time
    auto currentTime = std::chrono::high_resolution_clock::now();

    // Initialize the rendering loop
    while (!window.shouldClose()) {
        // Poll for events
        window.pollEvents();

        // Handle input
        handleInput();

        // Update the timing
        auto newTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        elapsedTime += deltaTime;
        currentTime = newTime;

        // Update frame time tracking
        frameTimes[frameTimeFrameIndex] = deltaTime * 1000.0f;
        frameTimeFrameIndex = (frameTimeFrameIndex + 1) % FRAMETIME_BUFFER_SIZE;
        frameIndex++;

        // Record and submit frame
        if (frameManager.beginFrame()) {
            // Update the data for the frame
            update();

            // Record render passes
            uint32_t frame = frameManager.getFrameIndex();
            uint32_t image = frameManager.getSwapChainImageIndex();

            // First depth pass
            frameManager.beginCommandBuffer(commandBuffers.depth[frame]);

            depthPass.recordCommands(commandBuffers.depth[frame], frame);
             {
                VkSemaphoreSubmitInfo signalSemaphore = {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                    .semaphore = semaphores.depthReady[frame],
                    .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT
                };

                VkCommandBufferSubmitInfo cmdBufInfo = {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                    .commandBuffer = commandBuffers.depth[frame],
                };

                VkSubmitInfo2 submitInfo = {
                    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                    .commandBufferInfoCount = 1,
                    .pCommandBufferInfos = &cmdBufInfo,
                    .signalSemaphoreInfoCount = 1,
                    .pSignalSemaphoreInfos = &signalSemaphore,
                };

                vkEndCommandBuffer(commandBuffers.depth[frame]);
                vkQueueSubmit2(device.graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
            }

            // Graphics to compute Sync and transfer
            recordGraphicsToComputeTransfers(frame);
            device.waitIdle();

            // Next, compute and terrain in parallel
            // Compute thread
            threadPool.submit([&, frame, image]() {
                // Cloud pass
                frameManager.beginCommandBuffer(commandBuffers.clouds[frame]);
                cloudsPass.recordCommands(commandBuffers.clouds[frame], frame);
                // Atmosphere pass
                frameManager.beginCommandBuffer(commandBuffers.atmosphere[frame]);
                atmospherePass.recordCommands(commandBuffers.atmosphere[frame], frame); {
                    VkSemaphoreSubmitInfo waitSemaphore = {
                        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                        .semaphore = semaphores.graphicsToComputeTransferClouds[frame],
                        .stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                    };

                    VkSemaphoreSubmitInfo signalSemaphore = {
                        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                        .semaphore = semaphores.cloudsReady[frame],
                        .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT
                    };

                    VkCommandBufferSubmitInfo cmdBufInfo = {
                        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                        .commandBuffer = commandBuffers.clouds[frame],
                    };

                    VkSubmitInfo2 submitInfo = {
                        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                        .waitSemaphoreInfoCount = 1,
                        .pWaitSemaphoreInfos = &waitSemaphore,
                        .commandBufferInfoCount = 1,
                        .pCommandBufferInfos = &cmdBufInfo,
                        .signalSemaphoreInfoCount = 1,
                        .pSignalSemaphoreInfos = &signalSemaphore,
                    };

                    vkEndCommandBuffer(commandBuffers.clouds[frame]);
                    vkQueueSubmit2(device.computeQueue(), 1, &submitInfo, VK_NULL_HANDLE);
                } {
                    VkSemaphoreSubmitInfo waitSemaphore = {
                        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                        .semaphore = semaphores.graphicsToComputeTransferAtmosphere[frame],
                        .stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                    };

                    VkSemaphoreSubmitInfo signalSemaphore = {
                        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                        .semaphore = semaphores.atmosphereReady[frame],
                        .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT
                    };

                    VkCommandBufferSubmitInfo cmdBufInfo = {
                        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                        .commandBuffer = commandBuffers.atmosphere[frame],
                    };

                    VkSubmitInfo2 submitInfo = {
                        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                        .waitSemaphoreInfoCount = 1,
                        .pWaitSemaphoreInfos = &waitSemaphore,
                        .commandBufferInfoCount = 1,
                        .pCommandBufferInfos = &cmdBufInfo,
                        .signalSemaphoreInfoCount = 1,
                        .pSignalSemaphoreInfos = &signalSemaphore,
                    };

                    vkEndCommandBuffer(commandBuffers.atmosphere[frame]);
                    vkQueueSubmit2(device.computeQueue(), 1, &submitInfo, VK_NULL_HANDLE);
                }
            });
            // Graphics thread
            threadPool.submit([&, frame, image]() {
                // Geometry pass
                frameManager.beginCommandBuffer(commandBuffers.terrain[frame]);
                geometryPass.recordCommands(commandBuffers.terrain[frame], frame); {
                    VkSemaphoreSubmitInfo waitSemaphore = {
                        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                        .semaphore = semaphores.graphicsToComputeTransferGeometry[frame],
                        .stageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                    };

                    VkSemaphoreSubmitInfo signalSemaphore = {
                        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                        .semaphore = semaphores.terrainColorReady[frame],
                        .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT
                    };

                    VkCommandBufferSubmitInfo cmdBufInfo = {
                        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                        .commandBuffer = commandBuffers.terrain[frame],
                    };

                    VkSubmitInfo2 submitInfo = {
                        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                        .waitSemaphoreInfoCount = 1,
                        .pWaitSemaphoreInfos = &waitSemaphore,
                        .commandBufferInfoCount = 1,
                        .pCommandBufferInfos = &cmdBufInfo,
                        .signalSemaphoreInfoCount = 1,
                        .pSignalSemaphoreInfos = &signalSemaphore,
                    };

                    vkEndCommandBuffer(commandBuffers.terrain[frame]);
                    vkQueueSubmit2(device.graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
                }
            });

            // Compute to graphics sync and transfer
            threadPool.wait();
            recordComputeToGraphicsTransfers(frame);

            // Lastly, the composition passes
            frameManager.beginCommandBuffer(commandBuffers.composition[frame]);
            if (compositionPass.data.applyGodRays) {
                godRaysPass.recordCommands(commandBuffers.composition[frame], frame);
            }
            else {
                // This is here for the validation layers to not throw error when the god rays pass is skipped
                profiler.resetTimestamp(commandBuffers.composition[frame], 12);
                profiler.resetTimestamp(commandBuffers.composition[frame], 13);
                profiler.resetTimestamp(commandBuffers.composition[frame], 14);
                profiler.resetTimestamp(commandBuffers.composition[frame], 15);

                profiler.writeTimestamp(commandBuffers.composition[frame], 12, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
                profiler.writeTimestamp(commandBuffers.composition[frame], 13, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
                profiler.writeTimestamp(commandBuffers.composition[frame], 14, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
                profiler.writeTimestamp(commandBuffers.composition[frame], 15, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
            }
            compositionPass.recordCommands(commandBuffers.composition[frame], frame);
            recordSwapChainImageTransition(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, frame, image);
            postProcessingPass.recordCommands(commandBuffers.composition[frame], frame);
            ui->recordUserInterface(commandBuffers.composition[frame]);
            recordSwapChainImageTransition(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, frame, image);

            // Submit composition command buffer
            // This one is submitted for presentation
            frameManager.submitPresentCommandBuffer(commandBuffers.composition[frame],
                                                    {semaphores.computeToGraphicsTransfer[frame], semaphores.terrainColorReady[frame]},
                                                    {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT});

            // Submit frame
            frameManager.endFrame();

            // Process results from the profiler
            if (profiler.getResultsIfAvailable()) {
                std::vector<float> results = profiler.getResults();

                benchmarkResult.depthPrePass.add(results[0]);
                benchmarkResult.cloudComputePass.add(results[1]);
                benchmarkResult.transmittanceLUT.add(results[2]);
                benchmarkResult.multipleScatteringLUT.add(results[3]);
                benchmarkResult.skyViewLUT.add(results[4]);
                benchmarkResult.aerialPerspectiveLUT.add(results[5]);
                if (compositionPass.data.applyGodRays) {
                    benchmarkResult.godRaysMask.add(results[6]);
                    benchmarkResult.godRaysBlur.add(results[7]);
                }
                else {
                    benchmarkResult.godRaysMask.add(0);
                    benchmarkResult.godRaysBlur.add(0);
                }
                benchmarkResult.skyUpsample.add(results[8]);
                benchmarkResult.composition.add(results[9]);
                benchmarkResult.post.add(results[10]);
                benchmarkResult.terrainDraw.add(results[11]);
                benchmarkResult.total.add(deltaTime * 1000.0f);
            }
        }
    }

    // Wait for queues to finish before deallocating resources
    device.waitIdle();
}
