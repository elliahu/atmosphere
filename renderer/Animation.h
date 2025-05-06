#pragma once
#include <vector>
#include <functional>
#include <chrono>
#include <memory>
#include <cmath>

class Animation {
public:
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    using Duration = std::chrono::milliseconds;
    using EasingFunction = std::function<float(float)>;
    using CompletionCallback = std::function<void()>;

    // Easing functions
    class Easing {
    public:
        static float linear(float t) { return t; }

        static float easeInQuad(float t) { return t * t; }
        static float easeOutQuad(float t) { return t * (2.0f - t); }
        static float easeInOutQuad(float t) {
            return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
        }

        static float easeInCubic(float t) { return t * t * t; }
        static float easeOutCubic(float t) {
            float t1 = t - 1.0f;
            return t1 * t1 * t1 + 1.0f;
        }
        static float easeInOutCubic(float t) {
            return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
        }

        static float easeInExpo(float t) { return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f)); }
        static float easeOutExpo(float t) { return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t); }
        static float easeInOutExpo(float t) {
            if (t == 0.0f || t == 1.0f) return t;
            if (t < 0.5f) return 0.5f * std::pow(2.0f, 20.0f * t - 10.0f);
            return 0.5f * (2.0f - std::pow(2.0f, -20.0f * t + 10.0f));
        }

        static float bounce(float t) {
            if (t < (1.0f / 2.75f)) return 7.5625f * t * t;
            if (t < (2.0f / 2.75f)) {
                t -= (1.5f / 2.75f);
                return 7.5625f * t * t + 0.75f;
            }
            if (t < (2.5f / 2.75f)) {
                t -= (2.25f / 2.75f);
                return 7.5625f * t * t + 0.9375f;
            }
            t -= (2.625f / 2.75f);
            return 7.5625f * t * t + 0.984375f;
        }

        static float easeOutBounce(float t) { return bounce(t); }
        static float easeInBounce(float t) { return 1.0f - bounce(1.0f - t); }

        static EasingFunction getEasingFunction(const std::string& name) {
            if (name == "easeInQuad") return easeInQuad;
            if (name == "easeOutQuad") return easeOutQuad;
            if (name == "easeInOutQuad") return easeInOutQuad;
            if (name == "easeInCubic") return easeInCubic;
            if (name == "easeOutCubic") return easeOutCubic;
            if (name == "easeInOutCubic") return easeInOutCubic;
            if (name == "easeInExpo") return easeInExpo;
            if (name == "easeOutExpo") return easeOutExpo;
            if (name == "easeInOutExpo") return easeInOutExpo;
            if (name == "easeInBounce") return easeInBounce;
            if (name == "easeOutBounce") return easeOutBounce;
            return linear; // Default
        }
    };

    // Base animation class
    class AnimationBase {
    public:
        virtual ~AnimationBase() = default;
        virtual void update(float progress) = 0;
        virtual bool isFinished() const = 0;
        virtual void setFinished(bool value) = 0;
        virtual Duration getDelay() const = 0;
        virtual void decreaseDelay(Duration delta) = 0;
        virtual Duration getElapsedTime() const = 0;
        virtual void addElapsedTime(Duration delta) = 0;
        virtual Duration getTotalDuration() const = 0;
        virtual float calculateProgress() const = 0;
    };

    // Generic animation for any property
    template<typename ValueType>
    class PropertyAnimation : public AnimationBase {
    private:
        // Function to get the current property value
        std::function<ValueType&()> propertyAccessor;
        ValueType startValue;
        ValueType targetValue;
        Duration totalDuration;
        Duration delay;
        Duration elapsedTime;
        EasingFunction easingFunc;
        bool finished = false;

    public:
        PropertyAnimation(
            std::function<ValueType&()> accessor,
            ValueType target,
            Duration duration,
            EasingFunction easing,
            Duration delayTime = Duration(0)
        ) : propertyAccessor(accessor),
            targetValue(target),
            totalDuration(duration),
            delay(delayTime),
            elapsedTime(Duration(0)),
            easingFunc(easing) {
            // Store initial value
            startValue = propertyAccessor();
        }

        void update(float progress) override {
            ValueType& property = propertyAccessor();
            property = startValue + (targetValue - startValue) * progress;
        }

        bool isFinished() const override {
            return finished;
        }

        void setFinished(bool value) override {
            finished = value;
        }

        Duration getDelay() const override {
            return delay;
        }

        void decreaseDelay(Duration delta) override {
            if (delay > delta) {
                delay -= delta;
            } else {
                delay = Duration(0);
            }
        }

        Duration getElapsedTime() const override {
            return elapsedTime;
        }

        void addElapsedTime(Duration delta) override {
            elapsedTime += delta;
        }

        Duration getTotalDuration() const override {
            return totalDuration;
        }

        float calculateProgress() const override {
            float linearProgress = std::min(1.0f,
                static_cast<float>(elapsedTime.count()) /
                static_cast<float>(totalDuration.count()));
            return easingFunc(linearProgress);
        }
    };

private:
    std::vector<std::unique_ptr<AnimationBase>> animations;
    bool running;
    TimePoint lastUpdateTime;
    CompletionCallback onCompleteCallback;

public:
    Animation() : running(false) {}

    // Animate a property through a raw pointer
    template<typename ObjectType, typename ValueType>
    Animation& animate(
        ObjectType* object,
        ValueType ObjectType::* property,
        ValueType targetValue,
        int durationMs = 1000,
        const std::string& easingName = "linear",
        int delayMs = 0
    ) {
        EasingFunction easingFunc = Easing::getEasingFunction(easingName);

        auto accessor = [object, property]() -> ValueType& {
            return object->*property;
        };

        animations.push_back(std::make_unique<PropertyAnimation<ValueType>>(
            accessor,
            targetValue,
            Duration(durationMs),
            easingFunc,
            Duration(delayMs)
        ));

        if (!running) {
            start();
        }

        std::cout << "Animation added: " << accessor() << " -> " << targetValue
                  << " over " << durationMs << "ms" << std::endl;

        return *this;
    }

    // Animate a property in a unique_ptr
    template<typename ObjectType, typename ValueType>
    Animation& animate(
        std::unique_ptr<ObjectType>& object,
        ValueType ObjectType::* property,
        ValueType targetValue,
        int durationMs = 1000,
        const std::string& easingName = "linear",
        int delayMs = 0
    ) {
        EasingFunction easingFunc = Easing::getEasingFunction(easingName);

        auto accessor = [&object, property]() -> ValueType& {
            return object.get()->*property;
        };

        animations.push_back(std::make_unique<PropertyAnimation<ValueType>>(
            accessor,
            targetValue,
            Duration(durationMs),
            easingFunc,
            Duration(delayMs)
        ));

        if (!running) {
            start();
        }

        std::cout << "Animation added (unique_ptr): " << accessor() << " -> " << targetValue
                  << " over " << durationMs << "ms" << std::endl;

        return *this;
    }

    // Animate a property in a shared_ptr
    template<typename ObjectType, typename ValueType>
    Animation& animate(
        std::shared_ptr<ObjectType>& object,
        ValueType ObjectType::* property,
        ValueType targetValue,
        int durationMs = 1000,
        const std::string& easingName = "linear",
        int delayMs = 0
    ) {
        EasingFunction easingFunc = Easing::getEasingFunction(easingName);

        auto accessor = [&object, property]() -> ValueType& {
            return object.get()->*property;
        };

        animations.push_back(std::make_unique<PropertyAnimation<ValueType>>(
            accessor,
            targetValue,
            Duration(durationMs),
            easingFunc,
            Duration(delayMs)
        ));

        if (!running) {
            start();
        }

        std::cout << "Animation added (shared_ptr): " << accessor() << " -> " << targetValue
                  << " over " << durationMs << "ms" << std::endl;

        return *this;
    }

    // Animate a direct reference to a property
    template<typename ValueType>
    Animation& animate(
        ValueType& property,
        ValueType targetValue,
        int durationMs = 1000,
        const std::string& easingName = "linear",
        int delayMs = 0
    ) {
        EasingFunction easingFunc = Easing::getEasingFunction(easingName);

        auto accessor = [&property]() -> ValueType& {
            return property;
        };

        animations.push_back(std::make_unique<PropertyAnimation<ValueType>>(
            accessor,
            targetValue,
            Duration(durationMs),
            easingFunc,
            Duration(delayMs)
        ));

        if (!running) {
            start();
        }

        std::cout << "Animation added (direct ref): " << property << " -> " << targetValue
                  << " over " << durationMs << "ms" << std::endl;

        return *this;
    }

    void start() {
        if (running) return;

        running = true;
        lastUpdateTime = std::chrono::high_resolution_clock::now();
        std::cout << "Animation started" << std::endl;
    }

    void stop() {
        running = false;
        animations.clear();
        std::cout << "Animation stopped" << std::endl;
    }

    void setCompletionCallback(CompletionCallback callback) {
        onCompleteCallback = callback;
    }

    void update() {
        if (!running || animations.empty()) {
            return;
        }

        TimePoint currentTime = std::chrono::high_resolution_clock::now();
        Duration deltaTime = std::chrono::duration_cast<Duration>(currentTime - lastUpdateTime);
        lastUpdateTime = currentTime;

        // Check for zero time elapsed to avoid division by zero
        if (deltaTime.count() == 0) {
            return;
        }

        // Process all animations
        bool allFinished = true;

        for (auto& anim : animations) {
            if (anim->isFinished()) continue;

            // Handle delay
            if (anim->getDelay() > Duration(0)) {
                anim->decreaseDelay(deltaTime);
                allFinished = false;
                continue;
            }

            anim->addElapsedTime(deltaTime);

            if (anim->getElapsedTime() >= anim->getTotalDuration()) {
                // Animation complete
                anim->update(1.0f);
                anim->setFinished(true);
            } else {
                // Animation in progress
                float progress = anim->calculateProgress();
                anim->update(progress);
                allFinished = false;
            }
        }

        if (allFinished && !animations.empty()) {
            running = false;
            std::cout << "All animations completed" << std::endl;
            if (onCompleteCallback) {
                onCompleteCallback();
            }
        }
    }

    bool isRunning() const {
        return running;
    }
};