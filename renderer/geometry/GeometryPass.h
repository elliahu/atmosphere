#pragma once
#include "../IRenderGroup.h"
#include "../Types.h"

/**
 * GeometryPass is responsible to draw a geometry into its color and depth buffer
 */
class GeometryPass final : public IRenderGroup {
public:
    explicit GeometryPass(Device &device, ResourceManager &resourceManager, Profiler& profiler, Geometry &geometry): IRenderGroup(device, resourceManager, profiler),
        geometry(geometry) {
    };

    void initialize(HmckVec2 resolution);

    void setLightDirection(HmckVec3 lightDirection) { shaderData.lightDirection = HmckVec4{lightDirection, 0.0f}; }
    void setLightColor(HmckVec3 lightColor) { shaderData.lightColor = HmckVec4{lightColor, 0.0f}; }
    void setView(HmckMat4 view) { this->view = view; }
    void setProjection(HmckMat4 projection) { this->projection = projection; }
    void setVertexBuffer(Buffer *buffer) { vertexBuffer = buffer; }
    void setIndexBuffer(Buffer *buffer) { indexBuffer = buffer; }

    Image *getColorTarget() const { return resourceManager.getResource<Image>(color); }
    Image *getDepthTarget() const { return resourceManager.getResource<Image>(depth); }

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

private:
    GeometryPushConstantData shaderData{};
    HmckMat4 view{}, projection{};


    // Buffers
    Buffer *vertexBuffer;
    Buffer *indexBuffer;
    Geometry &geometry;


    // Pipelines
    std::unique_ptr<GraphicsPipeline> colorAndDepthPipeline;


    // Targets
    ResourceHandle depth;
    ResourceHandle color;
    ResourceHandle sampler;

    void prepareTargets(uint32_t width, uint32_t height);
    void preparePipelines();
};
