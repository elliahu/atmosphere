#pragma once

#include <memory>
#include <vector>
#include <cassert>
#include <queue>

#include "hammock/platform/Window.h"
#include "hammock/core/Device.h"
#include "hammock/core/SwapChain.h"
#include "hammock/core/Framebuffer.h"


namespace hammock {
    class FrameManager {
    public:
        FrameManager(Window &window, Device &device);

        ~FrameManager();

        // delete copy constructor and copy destructor
        FrameManager(const FrameManager &) = delete;

        FrameManager &operator=(const FrameManager &) = delete;

        [[nodiscard]] SwapChain *getSwapChain() const { return swapChain.get(); };
        [[nodiscard]] float getAspectRatio() const { return swapChain->extentAspectRatio(); }
        [[nodiscard]] bool isFrameInProgress() const { return isFrameStarted; }


        [[nodiscard]] int getFrameIndex() const {
            assert(isFrameStarted && "Cannot get frame index when frame not in progress");
            return currentFrameIndex;
        }

        [[nodiscard]] int getSwapChainImageIndex() const {
            assert(isFrameStarted && "Cannot get image index when frame not in progress");
            return currentImageIndex;
        }

        // TODO command buffer will not be stored by the frame manager. frame manager will only create buffer on demand and submit provided buffers.
        // Maybe even make these methods in Device


        template<CommandQueueFamily Queue>
        VkCommandBuffer createCommandBuffer() {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            if (Queue == CommandQueueFamily::Graphics) {
                allocInfo.commandPool = device.getGraphicsCommandPool();
            }
            if (Queue == CommandQueueFamily::Compute) {
                allocInfo.commandPool = device.getComputeCommandPool();
            }
            if (Queue == CommandQueueFamily::Transfer) {
                allocInfo.commandPool = device.getTransferCommandPool();
            }
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;

            if (vkAllocateCommandBuffers(device.device(), &allocInfo, &commandBuffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate command buffer");
            }
            return commandBuffer;
        }

        void beginCommandBuffer(VkCommandBuffer commandBuffer) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording a command buffer");
            }
        }

        template<CommandQueueFamily Queue>
        void submitCommandBuffer(VkCommandBuffer commandBuffer, const std::vector<VkSemaphore> &waitSemaphores,
                                 const std::vector<VkSemaphore> &signalSemaphores, const std::vector<VkPipelineStageFlags>& waitStages,  VkFence fence = VK_NULL_HANDLE) {
            if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer");
            }

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
            submitInfo.pWaitSemaphores = waitSemaphores.data();
            submitInfo.pWaitDstStageMask = waitStages.data();

            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());;
            submitInfo.pSignalSemaphores = signalSemaphores.data();

            VkResult submitResult;

            if (Queue == CommandQueueFamily::Graphics) {
                submitResult = vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, fence);
            }

            if (Queue == CommandQueueFamily::Transfer) {
                submitResult = vkQueueSubmit(device.transferQueue(), 1, &submitInfo, fence);
            }

            if (Queue == CommandQueueFamily::Compute) {
                submitResult = vkQueueSubmit(device.computeQueue(), 1, &submitInfo, fence);
            }

            if (submitResult != VK_SUCCESS) {
                throw std::runtime_error("failed to submit command buffer");
            }
        }

        void submitPresentCommandBuffer(VkCommandBuffer commandBuffer, const std::vector<VkSemaphore>& wait, const std::vector<VkPipelineStageFlags>& waitStages);

        bool beginFrame();

        void endFrame();

    private:

        void recreateSwapChain();


        Window &window;
        Device &device;
        std::unique_ptr<SwapChain> swapChain;

        uint32_t currentImageIndex;
        int currentFrameIndex{0};
        bool isFrameStarted{false};
    };
}
