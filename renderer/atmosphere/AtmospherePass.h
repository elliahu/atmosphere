#pragma once
#include "AerialPerspective.h"
#include "Transmittance.h"
#include "MultipleScattering.h"
#include "SkyView.h"
#include "../IRenderGroup.h"
#include "../Types.h"


class AtmospherePass final : public IRenderGroup {
public:
    AtmospherePass(Device &device, ResourceManager &resourceManager, Profiler& profiler)
        : IRenderGroup(device, resourceManager, profiler),
          transmittance(device, resourceManager, profiler),
          multipleScattering(device, resourceManager, profiler),
          skyView(device, resourceManager, profiler),
          aerialPerspective(device, resourceManager, profiler) {
    }

    void initialize();

    void setEye(HmckVec3 eye) {
        atmosphere.eye = HmckVec4{eye, 0.0f};
    }

    void setSunDirection(HmckVec3 sunDirection) {
        atmosphere.sunDirection = HmckVec4{sunDirection, 0.0f};
    }

    void setShadowMap(Image *image) { aerialPerspective.setShadowMap(image); }
    void setShadowViewProjection(HmckMat4 mat) { atmosphere.shadowViewProj = mat; }
    void setCameraInverseView(HmckMat4 mat) { atmosphere.inverseView = mat; }
    void setCameraInverseProjection(HmckMat4 mat) { atmosphere.inverseProjection = mat; }

    void setCameraFrustum(HmckVec4 a, HmckVec4 b, HmckVec4 c, HmckVec4 d) {
        atmosphere.frustumA = a;
        atmosphere.frustumB = b;
        atmosphere.frustumC = c;
        atmosphere.frustumD = d;
    }

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

    AtmosphereUniformBufferData atmosphere;

    // Luts
    Transmittance transmittance;
    MultipleScattering multipleScattering;
    SkyView skyView;
    AerialPerspective aerialPerspective;

private:
    // Buffers
    // There is one common buffer for all luts bound once at the start of the pass
    ResourceHandle atmosphereBuffer;
    // Then each dispatch uses its own buffer for its custom data

    // Descriptors
    std::unique_ptr<DescriptorSetLayout> layout;
    VkDescriptorSet descriptor;

    void prepareBuffers();

    void prepareDescriptors();
};
