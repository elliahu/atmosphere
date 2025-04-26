#include "UserInterface.h"

void ::UserInterface::showCameraWindow() {
    hideAll = true;
    ImGui::SetNextWindowPos(
        ImVec2(window.getExtent().width / 2.0f, window.getExtent().height / 2.0f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f)
    );
    ImGui::Begin("Camera settings", nullptr,
                 ImGuiWindowFlags_AlwaysAutoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse );

    ImGui::DragFloat3("World space position", &camera->position.Elements[0]);
    ImGui::SliderFloat("Yaw", &camera->yaw, 2*HmckPI, -2*HmckPI);
    ImGui::SliderFloat("Pitch",&camera->pitch, HmckPI, -HmckPI);
    ImGui::SliderFloat("Roll", &camera->roll, HmckPI, -HmckPI);
    ImGui::SliderFloat("Field of view", &camera->fov, HmckToRad(HmckAngleDeg(0.1f)), HmckToRad(HmckAngleDeg(110.0f)));
    ImGui::DragFloat("Draw distance", &camera->zfar, 1.f, 0.f);

    ImGui::Separator();
    if (ImGui::Button("Close")) {
        showCamera = false;
        hideAll = false;
    }
    ImGui::End();
}

void ::UserInterface::showPostProcsSettingsWindow() {
    hideAll = true;
    ImGui::SetNextWindowPos(
       ImVec2(window.getExtent().width / 2.0f, window.getExtent().height / 2.0f),
       ImGuiCond_Always,
       ImVec2(0.5f, 0.5f)
   );
    ImGui::Begin("Post processing settings", nullptr,
                 ImGuiWindowFlags_AlwaysAutoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse );

    // Color Grading parameters
    ImGui::SeparatorText("Color Grading");

    ImGui::SliderFloat("Contrast", &postProcessingPushConstant->contrast, 0.5f, 2.0f);
    ImGui::SliderFloat("Brightness", &postProcessingPushConstant->brightness, -0.5f, 0.5f);

    // Vignette parameters
    ImGui::SeparatorText("Vignette");
    ImGui::SliderFloat("Vignette Strength", &postProcessingPushConstant->vignetteStrength, 0.0f, 3.0f);
    ImGui::SliderFloat("Vignette Softness", &postProcessingPushConstant->vignetteSoftness, 0.0f, 2.0f);

    // Effects parameters
    ImGui::SeparatorText("Effects");
    ImGui::ColorEdit4("Tint", reinterpret_cast<float *>(&postProcessingPushConstant->colorTint.Elements[0]));
    //ImGui::BeginDisabled(true);
    ImGui::SliderFloat("Grain Amount", &postProcessingPushConstant->grainAmount, 0.0f, 1.0f);

    ImGui::Separator();
    if (ImGui::Button("Close")) {
        showPostProc = false;
        hideAll = false;
    }
    ImGui::End();
}

void ::UserInterface::showDebugWindow() {
    ImGui::Begin("Debug and Performance", (bool *) false,
                 ImGuiWindowFlags_AlwaysAutoResize );

    ImGui::SeparatorText("Performance");
    ImGui::Text("%.1f FPS ", 1.0f / deltaTime);
    ImGui::Text("Frametime: %.2f ms", deltaTime * 1000.0f);
    ImGui::PlotLines("Frame Times", frameTimes, FRAMETIME_BUFFER_SIZE, frameTimeFrameIndex, nullptr,
                     0.0f, 33.0f,
                     ImVec2(0, 80));
    ImGui::SeparatorText("Rendering");
    ImGui::DragInt("Max density samples", &cloudsPushConstant->DEBUG_maxSamples, 0.1f, 2, 2048);
    ImGui::DragInt("Max light density samples",  &cloudsPushConstant->DEBUG_maxLightSamples, 0.1f, 2, 64);
    ImGui::DragFloat("Large step multiplier",  &cloudsPushConstant->DEBUG_longStepMulti, 0.1f, 1.f, 5.0f);
    ImGui::DragInt("Cheap sample distance",  &cloudsPushConstant->DEBUG_cheapSampleDistance, 10, 0,
                   1000000);
    ImGui::Checkbox("Enable epic view", (bool *)  &cloudsPushConstant->DEBUG_epicView);

    ImGui::SeparatorText("Debug views");
    ImGui::Checkbox("Early termination regions", (bool *)  &cloudsPushConstant->DEBUG_earlyTermination);
    ImGui::Checkbox("Late termination regions", (bool *)  &cloudsPushConstant->DEBUG_lateTermination);

    ImGui::End();
}

void ::UserInterface::showEditorWindow() {
    ImGui::Begin("Atmosphere editor", (bool *) false,
                 ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::SeparatorText("Clouds");
    ImGui::SliderFloat("Anvil bias",  &cloudsPushConstant->anvilBias, 0.0f, 1.f);
    ImGui::SliderFloat("Base scale",  &cloudsPushConstant->baseScale, 0.0f, 200.f);
    ImGui::SliderFloat("Detail scale",  &cloudsPushConstant->detailScale, 0.0f, 200.f);
    ImGui::SliderFloat("Curls",  &cloudsPushConstant->curliness, 0.0f, 50.f);
    ImGui::SliderFloat("Low frequency",  &cloudsPushConstant->baseMultiplier, 0.0f, 1.f);
    ImGui::SliderFloat("High frequency",  &cloudsPushConstant->detailMultiplier, 0.0f, 1.f);
    ImGui::DragFloat("Absorption",  &cloudsPushConstant->absorption, 0.0001f, 0.0f, 1.0f, "%.7f");

    ImGui::SeparatorText("Phase");
    ImGui::SliderFloat("Eccentricity",  &cloudsPushConstant->eccentricity, -.99f, .99f);
    ImGui::SliderFloat("Intensity",  &cloudsPushConstant->intensity, 0.f, 5.0f);
    ImGui::SliderFloat("Spread",  &cloudsPushConstant->spread, 0.f, 1.0f);

    ImGui::SeparatorText("Weather");
    ImGui::SliderFloat("Global coverage",  &cloudsPushConstant->globalCoverage, 0.0f, 1.f);
    ImGui::SliderFloat("Global density",  &cloudsPushConstant->globalDensity, 0.0f, 1.f);
    ImGui::SliderFloat("Wind speed",  &cloudsPushConstant->cloudSpeed, 0.0f, 5000.f);
    ImGui::SliderFloat("Wind direction", &angle, 0.f, 360.f);



    ImGui::SeparatorText("Light");
    ImGui::ColorEdit3("Light color", &cloudsUniformBuffer->lightColor.Elements[0]);
    ImGui::SliderFloat("Sun azimuth", &sunA, 0.f, 360.f);
    ImGui::SliderFloat("Sun elevation", &sunE, 0.f, 90.f);


    ImGui::ColorEdit3("Zenith sky color", &cloudsUniformBuffer->skyColorZenith.Elements[0]);
    ImGui::SliderFloat("Sun light strength", &cloudsUniformBuffer->lightColor.Elements[0], 0.0f, 15.f);
    ImGui::SliderFloat("Ambient light strength",  &cloudsPushConstant->ambientStrength, 0.0f, 1.f);

    ImGui::SeparatorText("God rays");
    ImGui::Checkbox("Screen space god rays", (bool *)&compositionData->applyGodRays);
    if (compositionData->applyGodRays == 1){
        ImGui::SliderInt("Number of samples", &godRaysCoefficients->num_samples, 1, 512);
        ImGui::SliderFloat("Density", &godRaysCoefficients->density, 0.0f, 5.0f);
        ImGui::SliderFloat("Exposure", &godRaysCoefficients->exposure, 0.0f, 5.f);
        ImGui::SliderFloat("Decay", &godRaysCoefficients->decay, 0.0f, 5.0f);
        ImGui::SliderFloat("Weight", &godRaysCoefficients->weight, 0.0f, 5.0f);
        ImGui::SliderFloat("Alpha", &godRaysCoefficients->alpha, 0.0f, 1.0f);
    }


    ImGui::End();
}

void ::UserInterface::recordUserInterface(VkCommandBuffer commandBuffer) {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = frameManager.getSwapChain()->getRenderPass();
    renderPassInfo.framebuffer = frameManager.getSwapChain()->getFramebuffer(frameManager.getSwapChainImageIndex());
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = frameManager.getSwapChain()->getSwapChainExtent();
    VkClearValue clearColor = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    // Begin the render pass
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    ui.beginUserInterface();


    ImGui::PushStyleColor(ImGuiCol_Border, {0.f, 0.f, 0.f, 0.f});
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Window")) {
            if (ImGui::MenuItem("Atmosphere editor", NULL, showEditor)) {
                showEditor = !showEditor;
            }
            if (ImGui::MenuItem("Debug and Performance", NULL, showDebug)) {
                showDebug = !showDebug;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Options")) {
            if (ImGui::MenuItem("Camera settings", NULL, showCamera)) {
                showCamera = !showCamera;
            }
            if (ImGui::MenuItem("Post processing", NULL, showPostProc)) {
                showPostProc = !showPostProc;
            }
            if (ImGui::MenuItem("Hide all", NULL, hideAll)) {
                hideAll = !hideAll;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    ImGui::PopStyleColor();


    if (showEditor && !hideAll) {
        showEditorWindow();
    }

    float rad = angle * (3.14159265359f / 180.0f);
    cloudsUniformBuffer->windDirection = HmckVec4{HmckVec3{cos(rad), 0.0f,sin(rad)},  0.0f};

    float azimuth = HmckToRad(HmckAngleDeg(sunA));
    float elevation = HmckToRad(HmckAngleDeg(sunE));

    HmckVec3 sunDir;
    // Correct mapping for azimuth (horizontal rotation) and elevation (vertical angle)
    sunDir.X = cos(elevation) * sin(azimuth); // East-West component
    sunDir.Y = sin(elevation);                // Up-Down component (zenith to horizon)
    sunDir.Z = cos(elevation) * cos(azimuth); // North-South component

    cloudsUniformBuffer->lightDirection = HmckVec4{HmckNorm(sunDir), 0.0f};

    if (showDebug && !hideAll) {
        showDebugWindow();
    }

    if (showCamera) {
        showCameraWindow();
    }

    if (showPostProc) {
        showPostProcsSettingsWindow();
    }



    ui.endUserInterface(commandBuffer);
    // End the render pass
    vkCmdEndRenderPass(commandBuffer);
}
