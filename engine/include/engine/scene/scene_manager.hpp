#pragma once

#include <engine/core/logger.hpp>
#include <engine/scene/scene.hpp>

#include <memory>
#include <vector>

namespace Solace {

    // ============================================================
    // SceneManager — стек сцен с отложенными переходами.
    //
    // Гарантии:
    //   - В любой момент активна только верхняя сцена
    //   - request_push/pop/replace применяются в process_transitions()
    //   - on_exit старой вызывается ДО on_enter новой
    //   - Если стек пустой — update/render безопасны (no-op + warning)
    // ============================================================
    class SceneManager {
    public:
        explicit SceneManager(SceneContext& ctx) : m_ctx(ctx) {}
        ~SceneManager() {
            // Уничтожаем сцены сверху вниз (обратный порядок стек)
            while (!m_stack.empty()) {
                m_stack.back()->on_exit(m_ctx);
                m_stack.pop_back();
            }
        }

        SceneManager(const SceneManager&) = delete;
        SceneManager& operator=(const SceneManager&) = delete;

        // Немедленный push (только в начальной инициализации)
        void push_immediate(std::unique_ptr<Scene> scene) {
            if (!scene) return;
            Logger::info("SceneManager: push_immediate");
            scene->on_enter(m_ctx);
            m_stack.push_back(std::move(scene));
        }

        // ---- Применение отложенных переходов ----
        void process_transitions() {
            // Собираем pending команды со всех сцен в стеке
            // (обычно только с верхней, но on_event может ставить на любую)
            for (auto& scene : m_stack) {
                if (scene->m_pending_type != SceneTransitionType::None) {
                    apply_transition(*scene);
                    return;  // один переход за кадр — проще и предсказуемо
                }
            }
        }

        // ---- Lifecycle ----
        void update(float gameplay_dt, float real_dt) {
            if (m_stack.empty()) {
                Logger::warning("SceneManager: update with empty stack");
                return;
            }
            m_stack.back()->on_update(gameplay_dt, real_dt, m_ctx);
        }

        void render() {
            if (m_stack.empty()) return;
            m_stack.back()->on_render(m_ctx);
        }

        void ui_render() {
            if (m_stack.empty()) return;
            m_stack.back()->on_ui_render(m_ctx);
        }

        void on_event(const SDL_Event& event) {
            if (m_stack.empty()) return;
            m_stack.back()->on_event(event, m_ctx);
        }

        // ---- Debug ----
        size_t depth() const { return m_stack.size(); }

        const char* top_scene_name() const {
            return m_stack.empty() ? "(empty)" : typeid(*m_stack.back()).name();
        }

    private:
        void apply_transition(Scene& current) {
            const auto type = current.m_pending_type;
            auto pending = std::move(current.m_pending_scene);
            current.m_pending_type = SceneTransitionType::None;

            switch (type) {
            case SceneTransitionType::Push:
                if (pending) {
                    Logger::info("SceneManager: push");
                    // Текущая сцена на паузе — on_exit НЕ вызывается
                    pending->on_enter(m_ctx);
                    m_stack.push_back(std::move(pending));
                }
                break;

            case SceneTransitionType::Pop:
                if (m_stack.size() > 1) {
                    Logger::info("SceneManager: pop");
                    m_stack.back()->on_exit(m_ctx);
                    m_stack.pop_back();
                    // Возвращаемся к предыдущей — она уже в стеке, on_enter НЕ нужен
                }
                else {
                    Logger::warning("SceneManager: pop on bottom scene (ignored)");
                }
                break;

            case SceneTransitionType::Replace:
                if (pending) {
                    Logger::info("SceneManager: replace");
                    m_stack.back()->on_exit(m_ctx);
                    m_stack.pop_back();
                    pending->on_enter(m_ctx);
                    m_stack.push_back(std::move(pending));
                }
                break;

            case SceneTransitionType::None:
                break;
            }
        }

        SceneContext& m_ctx;
        std::vector<std::unique_ptr<Scene>> m_stack;
    };


    // ---- Out-of-line: Scene::request_* (после SceneManager) ----
    inline void Scene::request_push(std::unique_ptr<Scene> next) {
        m_pending_type = SceneTransitionType::Push;
        m_pending_scene = std::move(next);
    }

    inline void Scene::request_pop() {
        m_pending_type = SceneTransitionType::Pop;
        m_pending_scene.reset();
    }

    inline void Scene::request_replace(std::unique_ptr<Scene> next) {
        m_pending_type = SceneTransitionType::Replace;
        m_pending_scene = std::move(next);
    }

} // namespace Solace