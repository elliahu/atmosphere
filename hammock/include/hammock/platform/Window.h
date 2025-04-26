#pragma once

#include <memory>
#include <string>
#if defined(_WIN32)
#define SURFER_PLATFORM_WIN32
#endif
#if defined(__linux__)
#define SURFER_PLATFORM_X11
#endif
#include <VulkanSurfer.h>
#include "hammock/core/VulkanInstance.h"
#include "hammock/core/HandmadeMath.h"

namespace hammock
{
    class Window
    {
    public:
        Window(VulkanInstance &instance, const std::string &_windowName, int windowWidth, int windowHeight);

        ~Window();

        // delete copy constructor and copy destructor
        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;

        VkSurfaceKHR getSurface() const { return window->getSurface(); }
        std::string getWindowName() const { return windowName; }
        VkExtent2D getExtent() const
        {
            return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        }
        bool wasWindowResized() const { return framebufferResized; }
        void resetWindowResizedFlag() { framebufferResized = false; }

        bool isKeyDown(Surfer::KeyCode keyCode);
        bool isInFocus() const {return inFocus;}
        HmckVec2 getMousePosition() const;

        void listenKeyPress(std::function<void(Surfer::KeyCode)> callback) {
            keyPressCallbacks.push_back(callback);
        }
        void listenKeyRelease(std::function<void(Surfer::KeyCode)> callback) {
            keyReleaseCallbacks.push_back(callback);
        }

        bool shouldClose() const;
        void pollEvents();
    private:
        Surfer::Window * window;
        VulkanInstance &instance;
        int width;
        int height;
        bool framebufferResized = false;
        bool inFocus = true;
        std::string windowName;
        std::unordered_map<Surfer::KeyCode, bool> keysDown;

        std::vector<std::function<void(Surfer::KeyCode)>> keyPressCallbacks;
        std::vector<std::function<void(Surfer::KeyCode)>> keyReleaseCallbacks;
    };
}
