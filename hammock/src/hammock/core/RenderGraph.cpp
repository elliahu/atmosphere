#include "hammock/core/RenderGraph.h"

void hammock::PipelineBarrier::apply() const {
    if (node.isImage()) {
        VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (transitionStage == TransitionStage::RequiredLayout) {
            newLayout = access.requiredLayout;
        } else if (transitionStage == TransitionStage::FinalLayout) {
            newLayout = access.finalLayout;
        }

        if (node.isSwapChainImage()) {
            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = node.isDepthAttachment()
                    ? VK_IMAGE_ASPECT_DEPTH_BIT
                    : VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount = 1;


            // Swapchain attachment will probably never change queue family  so we leave it ignored

            transitionImageLayout(
                commandBuffer,
                 renderContext.getSwapChain()->getImage(renderContext.getSwapChainImageIndex()),
                VK_IMAGE_LAYOUT_UNDEFINED, newLayout, subresourceRange);
            return;
        }
        const ResourceHandle handle = node.resolve(rm, renderContext.getFrameIndex());
        auto * image = rm.getResource<Image>(handle);

        if (!layoutChangeNeeded) {
            newLayout = image->getLayout();
        }

        image->transition(commandBuffer, newLayout, access.queueFamily);
    }
    else if (node.isBuffer()) {
        // TODO support for buffer transition
    }
}
