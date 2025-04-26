#include "hammock/platform/Window.h"
#include <imgui.h>
#include <iostream>

hammock::Window::Window(VulkanInstance &instance, const std::string &_windowName, int windowWidth,
                        int windowHeight) : instance{instance}, width{windowWidth}, height{windowHeight}, windowName{_windowName} {
    window = Surfer::Window::createWindow(_windowName, instance.getInstance(), windowWidth, windowHeight, 100, 100);

    window->registerKeyPressCallback([this](Surfer::KeyCode key) {
        keysDown[key] = true;
        for (auto &callback: keyPressCallbacks) {
            callback(key);
        }
    });

    window->registerKeyReleaseCallback([this](Surfer::KeyCode key) {
        keysDown[key] = false;
        for (auto &callback: keyReleaseCallbacks) {
            callback(key);
        }
    });

    window->registerMouseMotionCallback([this](unsigned int x, unsigned int y) {
    });

    window->registerResizeCallback([this](unsigned int width, unsigned int height) {
        framebufferResized = true;
        this->width = static_cast<int32_t>(width);
        this->height = static_cast<int32_t>(height);
    });

    window->registerMoveCallback([this](int x, int y) {
    });

    window->registerCloseCallback([this]() {
        std::cout << "Closing..." << std::endl;
    });

    window->registerMouseEnterExitCallback([this](bool enter) {
    });

    window->registerFocusCallback([this](bool focused) {
        inFocus = focused;
    });

#ifdef __linux__
    window->registerNativeKeyPressCallback([this](KeySym key) {
    });

    window->registerNativeKeyReleaseCallback([this](KeySym key) {
    });
#endif
#ifdef _WIN32
    window->registerNativeKeyPressCallback([this](WPARAM wParam) {

    });

    window->registerNativeKeyReleaseCallback([this](WPARAM wParam) {

   });
#endif
}

hammock::Window::~Window() {
    Surfer::Window::destroyWindow(window);
}

bool hammock::Window::isKeyDown(Surfer::KeyCode keyCode) {
    if (keysDown.contains(keyCode)) {
        return keysDown[keyCode];
    }
    return false;
}

HmckVec2 hammock::Window::getMousePosition() const {
    unsigned int x, y;
    window->getCursorPosition(x, y);
    return {static_cast<float>(x), static_cast<float>(y)};
}

bool hammock::Window::shouldClose() const {
    return window->shouldClose();
}

void hammock::Window::pollEvents() {
    window->pollEvents();
}
