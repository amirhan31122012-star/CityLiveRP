#include <SAMPCpp/Core/Events.hpp>
#include <array>
#include <cassert>
#include <iostream>
struct Receiver : samp_cpp::IEventReceiver {
    int &calls;
    explicit Receiver(int &value) : calls(value) {}
    void receive() { ++calls; }
};
int main() {
    std::array<samp_cpp::EventDispatcher<>, 8> events;
    int calls = 0;
    {
        Receiver receiver(calls);
        for (auto &event : events)
            event += {receiver, &Receiver::receive};
        for (auto &event : events)
            event.emit();
        assert(calls == 8);
    }
    // Every dispatcher must drop its hook when the receiver goes away.
    for (auto &event : events) {
        event.emit();
        event.clear();
    }
    assert(calls == 8);
    // Also cover the reverse lifetime: a receiver outlives a dispatcher.
    Receiver receiver(calls);
    {
        samp_cpp::EventDispatcher<> event;
        event += {receiver, &Receiver::receive};
        event.emit();
    }
    assert(calls == 9);
    std::cout << "PASS: event receiver cleanup in both lifetime orders\n";
}
