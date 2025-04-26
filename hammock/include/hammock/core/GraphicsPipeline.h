#pragma once
#include <string>
#include <vector>
#include "hammock/core/Device.h"
#include <memory>

namespace hammock {
    class GraphicsPipeline {
        struct GraphicsPipelineConfig {
            GraphicsPipelineConfig() = default;

            GraphicsPipelineConfig(const GraphicsPipelineConfig &) = delete;

            GraphicsPipelineConfig &operator=(const GraphicsPipelineConfig &) = delete;

            VkPipelineViewportStateCreateInfo viewportInfo;
            VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
            VkPipelineRasterizationStateCreateInfo rasterizationInfo;
            VkPipelineMultisampleStateCreateInfo multisampleInfo;
            VkPipelineColorBlendAttachmentState colorBlendAttachment;
            VkPipelineColorBlendStateCreateInfo colorBlendInfo;
            VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
            std::vector<VkDynamicState> dynamicStateEnables;
            VkPipelineDynamicStateCreateInfo dynamicStateInfo;
        };

        struct GraphicsPipelineCreateInfo {
            std::string debugName;
            Device &device;

            struct ShaderModuleInfo {
                const std::vector<char> &byteCode{};
                std::string entryFunc = "main";
            };

            ShaderModuleInfo vertexShader{};
            ShaderModuleInfo fragmentShader{};

            std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
            std::vector<VkPushConstantRange> pushConstantRanges;

            struct GraphicsStateInfo {
                VkBool32 depthTest = VK_TRUE;
                VkCompareOp depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
                VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
                VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
                std::vector<VkPipelineColorBlendAttachmentState> blendAtaAttachmentStates;

                struct VertexBufferBindingsInfo {
                    std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
                    std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
                } vertexBufferBindings;
            } graphicsState;

            struct DynamicStateInfo {
                std::vector<VkDynamicState> dynamicStateEnables{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
            } dynamicState;

            struct DynamicRendering {
                bool enabled = false;
                uint32_t colorAttachmentCount = 0;
                std::vector<VkFormat> colorAttachmentFormats{};
                VkFormat depthAttachmentFormat = VK_FORMAT_UNDEFINED;
                VkFormat stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
            } dynamicRendering;

            VkRenderPass renderPass = VK_NULL_HANDLE;
        };

    public:
        GraphicsPipeline(GraphicsPipelineCreateInfo &createInfo);

        GraphicsPipeline(const GraphicsPipeline &) = delete;
        GraphicsPipeline &operator =(const GraphicsPipeline &) = delete;

        ~GraphicsPipeline();

        static std::unique_ptr<GraphicsPipeline> create(GraphicsPipelineCreateInfo createInfo);

        void bind(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);

        VkPipelineLayout pipelineLayout;

    private:

        static void defaultRenderPipelineConfig(GraphicsPipelineConfig &configInfo);

        void createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule) const;

        Device &device;
        VkPipeline graphicsPipeline;
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
    };
}
