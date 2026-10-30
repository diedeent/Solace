#pragma once

#include <engine/core/config_cache.hpp>
#include <engine/core/entity_registry.hpp>
#include <engine/core/event_bus.hpp>
#include <engine/core/clock.hpp>
#include <engine/graphics/camera.hpp>
#include <engine/graphics/shader.hpp>
#include <engine/assets/asset_cache.hpp>

#include <SDL3/SDL_events.h>

#include <memory>
#include <string>

namespace Solace {

    // ============================================================
    // SceneContext — shared services для всех сцен.
    //
    // Создаётся один раз в Engine, передаётся по & в Scene::on_*.
    // Reference-члены → non-copyable, non-movable.
    //
    // Layer: 5 (знает про 0-4, но не про конкретные сцены).
    // ============================================================
    struct SceneContext {
        EventBus& bus;
        Clock& clock;
        ConfigCache& config;
        EntityRegistry& registry;
        AssetCache& assets;

        // Legacy (Phase 0.4) — graphics через SettingsManager
        // Phase 0.7 мигрирует на ConfigCache
        class Window& window;
        class PostProcess& post_process;

        SceneContext(
            EventBus& b,
            Clock& c,
            ConfigCache& cfg,
            EntityRegistry& reg,
            AssetCache& a,
            Window& w,
            PostProcess& pp
        ) : bus(b), clock(c), config(cfg), registry(reg),
            assets(a), window(w), post_process(pp) {
        }

        SceneContext(const SceneContext&) = delete;
        SceneContext& operator=(const SceneContext&) = delete;
    };


    // ============================================================
    // SceneTransition — отложенные команды SceneManager.
    // ============================================================
    enum class SceneTransitionType {
        None,
        Push,
        Pop,
        Replace,
    };


    // ============================================================
    // Scene — интерфейс жизненного цикла.
    //
    // Контракт:
    //   - on_enter(ctx):    сцена получает ctx, инициализирует ресурсы
    //   - on_update:        системы обновляются
    //   - on_render:        рисует в низкое разрешение (PostProcess уже активен)
    //   - on_ui_render:     ImGui поверх (не обязательно)
    //   - on_event:         SDL_Event (до InputSystem)
    //   - on_exit(ctx):     ОБЯЗАН освободить все ресурсы (registry.clear,
    //                       ScopedSubscriptions clear)
    //
    // Сцена НЕ должна:
    //   - вызывать SceneManager напрямую (только через request_*)
    //   - знать про другие сцены
    // ============================================================
    class Scene {
    public:
        virtual ~Scene() = default;

        virtual void on_enter(SceneContext& ctx) = 0;
        virtual void on_exit(SceneContext& ctx) = 0;

        virtual void on_update(float gameplay_dt, float real_dt, SceneContext& ctx) = 0;
        virtual void on_render(SceneContext& ctx) = 0;
        virtual void on_ui_render(SceneContext& ctx) {}
        virtual void on_event(const SDL_Event& event, SceneContext& ctx) {}

        // Отложенные переходы — SceneManager применит в process_transitions()
        void request_push(std::unique_ptr<Scene> next);
        void request_pop();
        void request_replace(std::unique_ptr<Scene> next);

    protected:
        Scene() = default;

    private:
        friend class SceneManager;

        SceneTransitionType m_pending_type = SceneTransitionType::None;
        std::unique_ptr<Scene> m_pending_scene;
    };

} // namespace Solace