#include <catch2/catch_test_macros.hpp>
#include <engine/core/event_bus.hpp>

using namespace Solace;

struct Ping { int value; };

TEST_CASE("EventBus: immediate emit", "[events]") {
    EventBus bus;
    int received = 0;
    bus.subscribe<Ping>([&](const Ping& p) { received += p.value; });

    bus.emit(Ping{ 5 });
    REQUIRE(received == 5);
}

TEST_CASE("EventBus: queued emit is deferred", "[events]") {
    EventBus bus;
    int received = 0;
    bus.subscribe<Ping>([&](const Ping& p) { received += p.value; });

    bus.queue(Ping{ 3 });
    bus.queue(Ping{ 7 });
    REQUIRE(received == 0);      // ещё не обработано
    bus.processQueue();
    REQUIRE(received == 10);     // FIFO, оба обработаны
}

TEST_CASE("EventBus: ScopedSubscriptions auto-unsubscribe", "[events]") {
    EventBus bus;
    int received = 0;
    {
        ScopedSubscriptions subs(bus);
        subs.subscribe<Ping>([&](const Ping&) { received++; });
        bus.emit(Ping{ 1 });
        REQUIRE(received == 1);
    } // subs разрушается здесь
    bus.emit(Ping{ 2 });
    REQUIRE(received == 1); // не выросло — отписка сработала
}