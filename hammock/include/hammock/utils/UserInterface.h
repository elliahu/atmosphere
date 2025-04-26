#pragma once

#include "hammock/core/Device.h"
#include "imgui.h"
#include "hammock/core/FrameManager.h"
#include "hammock/core/HandmadeMath.h"
#include <memory>



namespace hammock {
    class UserInterface {
    public:
        UserInterface(Device &device, VkRenderPass renderPass, VkDescriptorPool descriptorPool, Window &window);

        ~UserInterface();

        // Ui rendering
        void beginUserInterface();

        void endUserInterface(VkCommandBuffer commandBuffer);

        void showDemoWindow() { ImGui::ShowDemoWindow(); }

        void showDebugStats(const HmckMat4 &inverseView, float frameTime);

        void showColorSettings(float *exposure, float *gamma, float *whitePoint);

    private:
        void init();

        void forwardWindowEvents();

        static void setupStyle();

        VkCommandBuffer beginSingleTimeCommands() const;

        void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

        void beginWindow(const char *title, bool *open = (bool *) 0, ImGuiWindowFlags flags = 0);

        void endWindow();


        Device &device;
        Window &window;
        VkDescriptorPool imguiPool;
        VkRenderPass renderPass;
    };
}
