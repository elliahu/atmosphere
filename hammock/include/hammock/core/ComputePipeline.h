#pragma once
#include <string>
#include <vector>
#include "hammock/core/Device.h"
#include <memory>

namespace hammock {
    class ComputePipeline {
        struct ComputePipelineConfig {
            ComputePipelineConfig() = default;
            ComputePipelineConfig(const ComputePipelineConfig &) = delete;
            ComputePipelineConfig &operator=(const ComputePipelineConfig &) = delete;

            // Descriptor set layouts used by the compute pipeline.
            std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
            // Push constant ranges used by the compute pipeline.
            std::vector<VkPushConstantRange> pushConstantRanges;
        };

        struct ComputePipelineCreateInfo {
            std::string debugName;
            Device &device;
            struct ShaderModuleInfo {
                const std::vector<char> &byteCode;
                std::string entryFunc = "main";
            } computeShader;
            std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
            std::vector<VkPushConstantRange> pushConstantRanges;
        };

    public:
        ComputePipeline(const ComputePipelineCreateInfo &config);

        ComputePipeline(const ComputePipeline &) = delete;
        ComputePipeline &operator =(const ComputePipeline &) = delete;

        ~ComputePipeline();

        static std::unique_ptr<ComputePipeline> create(const ComputePipelineCreateInfo &createInfo) {
            return std::make_unique<ComputePipeline>(createInfo);
        }

        void bind(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE) {
            vkCmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
        }

        VkPipelineLayout pipelineLayout;
    private:

        void createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule) const;

        Device &device;
        VkPipeline pipeline;
        VkShaderModule computeShaderModule;
    };
}
