#pragma once
#include <hammock/hammock.h>
#include "../Types.h"
#include "../Camera.h"

namespace hmck = hammock;

class UserInterface final {
    hmck::Device &device;
    hmck::FrameManager &frameManager;
    VkDescriptorPool descriptorPool;
    hmck::Window &window;
    hmck::UserInterface ui;

    Camera * camera;
    CloudsUniformBufferData * cloudsUniformBuffer;
    CloudsPushConstantData * cloudsPushConstant;
    PostProcessingPushConstantData * postProcessingPushConstant;
    CompositionData * compositionData;
    GodRaysCoefficients * godRaysCoefficients;




    float &deltaTime;
    float *frameTimes;
    int FRAMETIME_BUFFER_SIZE;
    int &frameTimeFrameIndex;

    bool showEditor = false;
    bool showDebug = false;
    bool showPostProc = false;
    bool showCamera = false;
    bool hideAll = false;



    void showCameraWindow();

    void showPostProcsSettingsWindow();

    void showDebugWindow();

    void showEditorWindow();

public:
    float angle = 233.f;
    float sunA = 334.286f, sunE = 48.673f;

    UserInterface(
        hmck::Device &device,
        hmck::FrameManager &frameManager,
        VkDescriptorPool descriptorPool,
        hmck::Window &window,
        float &deltaTime,
        float *frameTimes,
        int FRAMETIME_BUFFER_SIZE,
        int &frameTimeFrameIndex
    ): device(device),
       frameManager(frameManager),
       descriptorPool(descriptorPool),
       window(window),
       ui(device, frameManager.getSwapChain()->getRenderPass(), descriptorPool, window),
       deltaTime(deltaTime),
       frameTimes(frameTimes),
       FRAMETIME_BUFFER_SIZE(FRAMETIME_BUFFER_SIZE),
       frameTimeFrameIndex(frameTimeFrameIndex) {
    }

    void setCamera(Camera * c) { camera = c; }
    void setCloudsUniformData(CloudsUniformBufferData * data) { cloudsUniformBuffer = data; }
    void setPostProccessingData(PostProcessingPushConstantData * data) { postProcessingPushConstant = data; }
    void setCloudsPushData(CloudsPushConstantData * data) { cloudsPushConstant = data; }
    void setCompositionData(CompositionData * data) { compositionData = data; }
    void setGodRaysCoefficients(GodRaysCoefficients * data) {godRaysCoefficients = data;}

    void recordUserInterface(VkCommandBuffer commandBuffer);
};
