#include "hammock/core/FrameManager.h"

hammock::FrameManager::FrameManager(Window &window, Device &device) : window{window}, device{device} {
    recreateSwapChain();
}

hammock::FrameManager::~FrameManager() {
}



void hammock::FrameManager::recreateSwapChain() {
    auto extent = window.getExtent();

    while (extent.width == 0 || extent.height == 0) {
        window.pollEvents();
        extent = window.getExtent();
    }
    vkDeviceWaitIdle(device.device());

    if (swapChain == nullptr) {
        swapChain = std::make_unique<SwapChain>(device, extent);
    } else {
        std::shared_ptr<SwapChain> oldSwapChain = std::move(swapChain);
        swapChain = std::make_unique<SwapChain>(device, extent, oldSwapChain);

        if (!oldSwapChain->compareSwapFormats(*swapChain.get())) {
            throw std::runtime_error("SwapChain image format has changed");
        }
    }
}


void hammock::FrameManager::submitPresentCommandBuffer(VkCommandBuffer commandBuffer,  const std::vector<VkSemaphore>& wait, const std::vector<VkPipelineStageFlags>& waitStages) {
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer");
    }

    auto result = swapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex, wait, waitStages);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasWindowResized()) {
        // Window was resized (resolution was changed)
        window.resetWindowResizedFlag();
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image");
    }
}

bool hammock::FrameManager::beginFrame() {
    assert(!isFrameStarted && "Cannot call beginFrame while already in progress");

    auto result = swapChain->acquireNextImage(&currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return false;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to aquire swap chain image!");
    }

    isFrameStarted = true;
    return true;
}

void hammock::FrameManager::endFrame() {
    assert(isFrameStarted && "Cannot call endFrame while frame is not in progress");
    isFrameStarted = false;
    currentFrameIndex = (currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
}
