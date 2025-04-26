#include <iostream>
#include <vector>
#include <functional>

namespace hammock{
    class EventEmitter {
    public:
        using Subscriber = std::function<void(void *)>;

        // Subscribe to an event by name
        void subscribe(const uint32_t& eventName, const Subscriber& subscriber) {
            eventMap[eventName].push_back(subscriber);
        }

        // Trigger an event by name
        void trigger(const uint32_t& event, void * eventData) {
            if (eventMap.find(event) != eventMap.end()) {
                for (const auto& subscriber : eventMap[event]) {
                    subscriber(eventData);
                }
            }
        }

    protected:
        // Map to hold event names and their subscribers
        std::unordered_map<uint32_t, std::vector<Subscriber>> eventMap;
    };
}