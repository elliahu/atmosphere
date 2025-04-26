#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

#include "hammock/core/Device.h"
#include "hammock/core/Types.h"


#define FB_COLOR_FORMAT VK_FORMAT_R8G8B8A8_UNORM

namespace hammock {
    class SwapChain {
    public:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
        static constexpr bool CREATE_SWAPCHAIN_RENDERPASS = true;

        SwapChain(Device &deviceRef, VkExtent2D windowExtent);

        SwapChain(Device &deviceRef, VkExtent2D windowExtent, const std::shared_ptr<SwapChain> &previous);

        ~SwapChain();

        SwapChain(const SwapChain &) = delete;

        SwapChain &operator=(const SwapChain &) = delete;

        [[nodiscard]] VkImage getImage(const int index) const { return swapChainImages[index]; }
        [[nodiscard]] VkImageView getImageView(const int index) const { return swapChainImageViews[index]; }
        [[nodiscard]] size_t imageCount() const { return swapChainImages.size(); }
        [[nodiscard]] VkFormat getSwapChainImageFormat() const { return swapChainImageFormat; }
        [[nodiscard]] VkExtent2D getSwapChainExtent() const { return swapChainExtent; }
        [[nodiscard]] uint32_t width() const { return swapChainExtent.width; }
        [[nodiscard]] uint32_t height() const { return swapChainExtent.height; }
        [[nodiscard]] VkRenderPass getRenderPass() const { return renderPass; }
        [[nodiscard]] VkFramebuffer getFramebuffer(uint32_t index) const { return swapChainFramebuffers[index]; }
        [[nodiscard]] float extentAspectRatio() const {
            return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
        }

        VkFormat findDepthFormat() const;

        VkResult acquireNextImage(uint32_t *imageIndex) const;

        VkResult submitCommandBuffers(const VkCommandBuffer *buffers, const uint32_t *imageIndex, const std::vector<VkSemaphore>& waitForSemaphore, const std::vector<VkPipelineStageFlags>& waitStages);

        [[nodiscard]] bool compareSwapFormats(const SwapChain &swapChain) const {
            return swapChain.swapChainImageFormat == swapChainImageFormat;
        }

    private:
        void init();

        void createSwapChain();

        void createImageViews();

        void createRenderPass();

        void createFramebuffers();

        void createSyncObjects();

        // Helper functions
        static VkSurfaceFormatKHR chooseSwapSurfaceFormat(
            const std::vector<VkSurfaceFormatKHR> &availableFormats);

        static VkPresentModeKHR chooseSwapPresentMode(
            const std::vector<VkPresentModeKHR> &availablePresentModes);

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const;

        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;

        std::vector<VkFramebuffer> swapChainFramebuffers;
        VkRenderPass renderPass;


        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;


        Device &device;
        VkExtent2D windowExtent;

        VkSwapchainKHR swapChain;
        std::shared_ptr<SwapChain> oldSwapChain;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        size_t currentFrame = 0;
    };
} // namespace Hmck
