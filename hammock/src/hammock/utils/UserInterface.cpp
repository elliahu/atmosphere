#include "hammock/utils/UserInterface.h"
#include "hammock/core/GraphicsPipeline.h"
#include "backends/imgui_impl_vulkan.h"
#include "hammock/core/CoreUtils.h"
#include <deque>
#include <string>


hammock::UserInterface::UserInterface(Device &device, VkRenderPass renderPass, VkDescriptorPool descriptorPool,
                                      Window &window) : device{device}, renderPass(renderPass),
                                                        window{window}, imguiPool{descriptorPool} {
    init();
    setupStyle();

    window.listenKeyPress([this](Surfer::KeyCode key) {
        if (ImGui::GetCurrentContext()) {
            auto &io = ImGui::GetIO();
            if ((int) key >= 0 && (int) key <= 25) {
                io.AddInputCharacter('a' + (int) key);
            }
            switch (key) {
                case Surfer::KeyCode::Num0: {
                    io.AddKeyEvent(ImGuiKey_0, true);
                }
                case Surfer::KeyCode::Num1: {
                    io.AddKeyEvent(ImGuiKey_1, true);
                }
                case Surfer::KeyCode::Num2: {
                    io.AddKeyEvent(ImGuiKey_2, true);
                }
                case Surfer::KeyCode::Num3: {
                    io.AddKeyEvent(ImGuiKey_3, true);
                }
                case Surfer::KeyCode::Num4: {
                    io.AddKeyEvent(ImGuiKey_4, true);
                }
                case Surfer::KeyCode::Num5: {
                    io.AddKeyEvent(ImGuiKey_5, true);
                }
                case Surfer::KeyCode::Num6: {
                    io.AddKeyEvent(ImGuiKey_6, true);
                }
                case Surfer::KeyCode::Num7: {
                    io.AddKeyEvent(ImGuiKey_7, true);
                }
                case Surfer::KeyCode::Num8: {
                    io.AddKeyEvent(ImGuiKey_8, true);
                }
                case Surfer::KeyCode::Num9: {
                    io.AddKeyEvent(ImGuiKey_9, true);
                }
                case Surfer::KeyCode::Numpad0: {
                    io.AddKeyEvent(ImGuiKey_Keypad0, true);
                }
                case Surfer::KeyCode::Numpad1: {
                    io.AddKeyEvent(ImGuiKey_Keypad1, true);
                }
                case Surfer::KeyCode::Numpad2: {
                    io.AddKeyEvent(ImGuiKey_Keypad2, true);
                }
                case Surfer::KeyCode::Numpad3: {
                    io.AddKeyEvent(ImGuiKey_Keypad3, true);
                }
                case Surfer::KeyCode::Numpad4: {
                    io.AddKeyEvent(ImGuiKey_Keypad4, true);
                }
                case Surfer::KeyCode::Numpad5: {
                    io.AddKeyEvent(ImGuiKey_Keypad5, true);
                }
                case Surfer::KeyCode::Numpad6: {
                    io.AddKeyEvent(ImGuiKey_Keypad6, true);
                }
                case Surfer::KeyCode::Numpad7: {
                    io.AddKeyEvent(ImGuiKey_Keypad7, true);
                }
                case Surfer::KeyCode::Numpad8: {
                    io.AddKeyEvent(ImGuiKey_Keypad8, true);
                }
                case Surfer::KeyCode::Numpad9: {
                    io.AddKeyEvent(ImGuiKey_Keypad9, true);
                }
            }
        }
    });

    window.listenKeyRelease([this](Surfer::KeyCode key) {
        if (ImGui::GetCurrentContext()) {
            auto &io = ImGui::GetIO();
            switch (key) {
                case Surfer::KeyCode::Num0: {
                    io.AddKeyEvent(ImGuiKey_0, false);
                }
                case Surfer::KeyCode::Num1: {
                    io.AddKeyEvent(ImGuiKey_1, false);
                }
                case Surfer::KeyCode::Num2: {
                    io.AddKeyEvent(ImGuiKey_2, false);
                }
                case Surfer::KeyCode::Num3: {
                    io.AddKeyEvent(ImGuiKey_3, false);
                }
                case Surfer::KeyCode::Num4: {
                    io.AddKeyEvent(ImGuiKey_4, false);
                }
                case Surfer::KeyCode::Num5: {
                    io.AddKeyEvent(ImGuiKey_5, false);
                }
                case Surfer::KeyCode::Num6: {
                    io.AddKeyEvent(ImGuiKey_6, false);
                }
                case Surfer::KeyCode::Num7: {
                    io.AddKeyEvent(ImGuiKey_7, false);
                }
                case Surfer::KeyCode::Num8: {
                    io.AddKeyEvent(ImGuiKey_8, false);
                }
                case Surfer::KeyCode::Num9: {
                    io.AddKeyEvent(ImGuiKey_9, false);
                }
                case Surfer::KeyCode::Numpad0: {
                    io.AddKeyEvent(ImGuiKey_Keypad0, false);
                }
                case Surfer::KeyCode::Numpad1: {
                    io.AddKeyEvent(ImGuiKey_Keypad1, false);
                }
                case Surfer::KeyCode::Numpad2: {
                    io.AddKeyEvent(ImGuiKey_Keypad2, false);
                }
                case Surfer::KeyCode::Numpad3: {
                    io.AddKeyEvent(ImGuiKey_Keypad3, false);
                }
                case Surfer::KeyCode::Numpad4: {
                    io.AddKeyEvent(ImGuiKey_Keypad4, false);
                }
                case Surfer::KeyCode::Numpad5: {
                    io.AddKeyEvent(ImGuiKey_Keypad5, false);
                }
                case Surfer::KeyCode::Numpad6: {
                    io.AddKeyEvent(ImGuiKey_Keypad6, false);
                }
                case Surfer::KeyCode::Numpad7: {
                    io.AddKeyEvent(ImGuiKey_Keypad7, false);
                }
                case Surfer::KeyCode::Numpad8: {
                    io.AddKeyEvent(ImGuiKey_Keypad8, false);
                }
                case Surfer::KeyCode::Numpad9: {
                    io.AddKeyEvent(ImGuiKey_Keypad9, false);
                }
            }
        }
    });
}

hammock::UserInterface::~UserInterface() {
    ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();
}

void hammock::UserInterface::beginUserInterface() {
    // Handle UI forwarding from windowing system
    forwardWindowEvents();

    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
}

void hammock::UserInterface::endUserInterface(VkCommandBuffer commandBuffer) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void hammock::UserInterface::showDebugStats(const HmckMat4 &inverseView, float frameTime) {
    const ImGuiIO &io = ImGui::GetIO();
    constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                              ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                              ImGuiWindowFlags_NoNav;
    ImGui::SetNextWindowPos({10, 10});
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::Begin(window.getWindowName().c_str(), (bool *) nullptr, window_flags);
    const auto cameraPosition = inverseView.Columns[3].XYZ;

    ImGui::Text("Camera world position: ( %.2f, %.2f, %.2f )", cameraPosition.X, cameraPosition.Y, cameraPosition.Z);
    if (ImGui::IsMousePosValid())
        ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
    else
        ImGui::Text("Mouse Position: <invalid or hidden>");
    ImGui::Text("Window resolution: (%d x %d)", window.getExtent().width, window.getExtent().height);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", frameTime * 1000.f, 1.0f / frameTime);
    ImGui::End();
}


void hammock::UserInterface::showColorSettings(float *exposure, float *gamma, float *whitePoint) {
    constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
    beginWindow("Color settings", (bool *) false, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::DragFloat("Exposure", exposure, 0.01f, 0.01f);
    ImGui::DragFloat("Gamma", gamma, 0.01f, 0.01f);
    ImGui::DragFloat("White point", whitePoint, 0.01f, 0.01f);
    endWindow();
}

void hammock::UserInterface::init() {
    ImGui::CreateContext();

    //this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = device.getInstance();
    init_info.PhysicalDevice = device.getPhysicalDevice();
    init_info.Device = device.device();
    init_info.Queue = device.graphicsQueue();
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.RenderPass = renderPass;

    ImGui_ImplVulkan_Init(&init_info);

    const ImVec2 display_size(window.getExtent().width, window.getExtent().height);
    ImGui::GetIO().DisplaySize = display_size;

    //execute a gpu command to upload imgui font textures
    ImGui_ImplVulkan_CreateFontsTexture();
}

void hammock::UserInterface::forwardWindowEvents() {
    if (ImGui::GetCurrentContext()) {
        auto &io = ImGui::GetIO();

        auto mousePos = window.getMousePosition();
        io.AddMousePosEvent(mousePos.X, mousePos.Y);
        io.AddMouseButtonEvent(ImGuiMouseButton_Left, window.isKeyDown(Surfer::KeyCode::MouseLeft));
        io.AddMouseButtonEvent(ImGuiMouseButton_Right, window.isKeyDown(Surfer::KeyCode::MouseRight));
    }
}

void hammock::UserInterface::setupStyle() {
    ImGuiStyle &style = ImGui::GetStyle();
    // Setup ImGUI style

    style.Colors[ImGuiCol_TitleBg] = ImVec4(245 / 255.0f, 245 / 255.0f, 245 / 255.0f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(245 / 255.0f, 245 / 255.0f, 245 / 255.0f, 1.0f); // Light blue
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(245 / 255.0f, 245 / 255.0f, 245 / 255.0f, 0.8f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(252 / 255.0f, 252 / 255.0f, 252 / 255.0f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(235 / 255.0f, 235 / 255.0f, 235 / 255.0f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(220 / 255.0f, 220 / 255.0f, 220 / 255.0f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(245 / 255.0f, 245 / 255.0f, 245 / 255.0f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(156 / 255.0f, 183 / 255.0f, 205 / 255.0f, 1.0f); // Pastel blue
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(156 / 255.0f, 183 / 255.0f, 205 / 255.0f, 0.8f); // Pastel blue
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(137 / 255.0f, 175 / 255.0f, 201 / 255.0f, 1.0f); // Slightly darker pastel blue
    style.Colors[ImGuiCol_Header] = ImVec4(235 / 255.0f, 235 / 255.0f, 235 / 255.0f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(173 / 255.0f, 216 / 255.0f, 230 / 255.0f, 0.8f); // Light blue
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(173 / 255.0f, 216 / 255.0f, 230 / 255.0f, 1.0f); // Light blue
    style.Colors[ImGuiCol_Separator] = ImVec4(220 / 255.0f, 220 / 255.0f, 220 / 255.0f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(200 / 255.0f, 200 / 255.0f, 200 / 255.0f, 0.6f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(173 / 255.0f, 216 / 255.0f, 230 / 255.0f, 0.8f); // Light blue
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(173 / 255.0f, 216 / 255.0f, 230 / 255.0f, 1.0f); // Light blue
    style.Colors[ImGuiCol_Tab] = ImVec4(245 / 255.0f, 245 / 255.0f, 245 / 255.0f, 0.8f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(235 / 255.0f, 235 / 255.0f, 235 / 255.0f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(173 / 255.0f, 216 / 255.0f, 230 / 255.0f, 1.0f); // Light blue
    style.Colors[ImGuiCol_Button] = ImVec4(245 / 255.0f, 245 / 255.0f, 245 / 255.0f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(173 / 255.0f, 216 / 255.0f, 230 / 255.0f, 0.8f); // Light blue
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(173 / 255.0f, 216 / 255.0f, 230 / 255.0f, 1.0f); // Light blue

    // Additional Windows pastel light theme elements
    style.Colors[ImGuiCol_WindowBg] = ImVec4(252 / 255.0f, 252 / 255.0f, 252 / 255.0f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(252 / 255.0f, 252 / 255.0f, 252 / 255.0f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(220 / 255.0f, 220 / 255.0f, 220 / 255.0f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0.0f);
    style.Colors[ImGuiCol_Text] = ImVec4(70 / 255.0f, 70 / 255.0f, 70 / 255.0f, 1.0f); // Softer than black
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(170 / 255.0f, 170 / 255.0f, 170 / 255.0f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(245 / 255.0f, 245 / 255.0f, 245 / 255.0f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(220 / 255.0f, 220 / 255.0f, 220 / 255.0f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(200 / 255.0f, 200 / 255.0f, 200 / 255.0f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(180 / 255.0f, 180 / 255.0f, 180 / 255.0f, 1.0f);

    // Style adjustments for a more Windows-like appearance
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(4, 3);
    style.CellPadding = ImVec2(4, 3);
    style.ItemSpacing = ImVec2(8, 4);
    style.ItemInnerSpacing = ImVec2(3, 3);
    style.TouchExtraPadding = ImVec2(0, 0);
    style.IndentSpacing = 21;
    style.ScrollbarSize = 14;
    style.GrabMinSize = 8;
    style.WindowBorderSize = 0;
    style.ChildBorderSize = 0;
    style.PopupBorderSize = 0;
    style.FrameBorderSize = 1;
    style.TabBorderSize = 0;
    style.WindowRounding = 8;
    style.ChildRounding = 8;
    style.FrameRounding = 4;
    style.PopupRounding = 4;
    style.ScrollbarRounding = 6;
    style.GrabRounding = 6;
    style.LogSliderDeadzone = 4;
    style.TabRounding = 6;
}

VkCommandBuffer hammock::UserInterface::beginSingleTimeCommands() const {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = device.getGraphicsCommandPool();
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device.device(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void hammock::UserInterface::endSingleTimeCommands(const VkCommandBuffer commandBuffer) const {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(device.graphicsQueue());

    vkFreeCommandBuffers(device.device(), device.getGraphicsCommandPool(), 1, &commandBuffer);
}

void hammock::UserInterface::beginWindow(const char *title, bool *open, ImGuiWindowFlags flags) {
    ImGui::Begin(title, open, flags);
}

void hammock::UserInterface::endWindow() {
    ImGui::End();
}
