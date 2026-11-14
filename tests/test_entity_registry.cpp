#include <catch2/catch_test_macros.hpp>
#include <engine/core/entity_registry.hpp>
#include <engine/core/event_bus.hpp>

using namespace Solace;

struct Health { float hp = 100.0f; };
struct Position { float x, y, z; };

TEST_CASE("EntityRegistry: create and destroy", "[ecs]") {
    EventBus bus;
    EntityRegistry reg(bus);

    auto id = reg.create();
    REQUIRE(id != INVALID_ENTITY);
    REQUIRE(reg.is_alive(id));

    reg.add_component(id, Health{ 50.0f });
    REQUIRE(reg.has_component<Health>(id));

    reg.destroy(id);
    REQUIRE_FALSE(reg.is_alive(id));      // сразу после destroy
    REQUIRE(reg.pending_count() == 1);

    reg.process_destroy();                // ← вот здесь проверяется баг #1
    REQUIRE(reg.pending_count() == 0);
    REQUIRE(reg.alive_count() == 0);
    REQUIRE_FALSE(reg.has_component<Health>(id)); // компонент должен быть удалён
}