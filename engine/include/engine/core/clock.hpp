#pragma once

#include <algorithm>
#include <cstdint>
#include <chrono>

namespace Solace {

    // ============================================================
    // Clock — управление временем для gameplay и UI.
    //
    // Три времени:
    //   - realDt:      реальное время кадра (clamp [1ms, 100ms])
    //   - gameplayDt:  realDt * scale, НО = 0 во время hitstop или pause
    //
    // HitStop: request_hitstop(duration) замораживает gameplayDt
    //          на `duration` секунд REAL-времени.
    //
    // Проблема, которую решает: при hitstop UI/input/camera shake
    // не должны замедляться. Только gameplay.
    // ============================================================
    class Clock {
    public:
        Clock() = default;
        ~Clock() = default;

        Clock(const Clock&) = delete;
        Clock& operator=(const Clock&) = delete;
        Clock(Clock&&) = delete;
        Clock& operator=(Clock&&) = delete;

        // ============ Жизненный цикл кадра ============

        // Вызывается ПЕРВЫМ в Application::run()
        void tick() {
            const uint64_t now = now_ns();

            if (m_last_ns == 0) {
                // Первый кадр — delta не определена
                m_real_dt = 0.0f;
                m_last_ns = now;
                update_gameplay_dt();
                return;
            }

            const uint64_t diff_ns = now - m_last_ns;
            m_last_ns = now;

            // Clamp: [1ms, 100ms]
            // Минимум — защита от деления на ~0 в FPS
            // Максимум — защита от spiral of death при лагах
            const float raw_dt = static_cast<float>(diff_ns) * 1e-9f;
            m_real_dt = std::clamp(raw_dt, MIN_DT, MAX_DT);

            // HitStop отсчитывается по REAL-времени
            if (m_hitstop_remaining > 0.0f) {
                m_hitstop_remaining -= m_real_dt;
                if (m_hitstop_remaining < 0.0f) m_hitstop_remaining = 0.0f;
            }

            m_total_real += m_real_dt;
            update_gameplay_dt();
        }

        // Вызывается ПОСЛЕДНИМ (после render, UI)
        void frame_end() {
            m_frame_count++;

            // FPS: обновляем раз в секунду real-времени
            m_fps_accum += m_real_dt;
            if (m_fps_accum >= 1.0f) {
                m_fps = static_cast<float>(m_frame_count) / m_fps_accum;
                m_fps_accum = 0.0f;
                m_frame_count = 0;
            }
        }

        // ============ Доступ к времени ============

        float real_dt() const { return m_real_dt; }
        float gameplay_dt() const { return m_gameplay_dt; }
        float total_real() const { return m_total_real; }
        float total_gameplay() const { return m_total_gameplay; }

        // ============ Управление временем ============

        // Пауза: gameplay замирает, real идёт
        void set_paused(bool paused) { m_paused = paused; }
        bool is_paused() const { return m_paused; }

        // Slow-mo (для будущих боссов / спецэффектов)
        // 1.0 = норма, 0.5 = в 2 раза медленнее, 0.0 = стоп
        void set_gameplay_scale(float scale) {
            m_gameplay_scale = std::clamp(scale, 0.0f, 10.0f);
        }
        float gameplay_scale() const { return m_gameplay_scale; }

        // HitStop: замораживает gameplayDt на duration REAL-секунд.
        // Если вызвать повторно — берётся max (не суммируется).
        // Если hitstop уже активен — новый заменяет старый, если длиннее.
        void request_hitstop(float duration) {
            if (duration <= 0.0f) return;
            m_hitstop_remaining = std::max(m_hitstop_remaining, duration);
        }

        bool is_in_hitstop() const { return m_hitstop_remaining > 0.0f; }

        // ============ Debug ============

        float fps() const { return m_fps; }

        struct Stats {
            float real_dt;
            float gameplay_dt;
            float total_real;
            float total_gameplay;
            float fps;
            bool paused;
            bool in_hitstop;
            float hitstop_remaining;
            float gameplay_scale;
        };

        Stats debug_stats() const {
            return Stats{
                m_real_dt, m_gameplay_dt, m_total_real, m_total_gameplay,
                m_fps, m_paused, is_in_hitstop(), m_hitstop_remaining, m_gameplay_scale
            };
        }

        // ============ Сброс (при смене сцены) ============
        void reset() {
            m_last_ns = 0;
            m_real_dt = 0.0f;
            m_gameplay_dt = 0.0f;
            m_total_real = 0.0f;
            m_total_gameplay = 0.0f;
            m_hitstop_remaining = 0.0f;
            m_paused = false;
            m_gameplay_scale = 1.0f;
            m_fps = 0.0f;
            m_fps_accum = 0.0f;
            m_frame_count = 0;
        }

    private:
        static constexpr float MIN_DT = 0.001f;  // 1 ms (1000 FPS максимум)
        static constexpr float MAX_DT = 0.100f;  // 100 ms (10 FPS минимум)

        void update_gameplay_dt() {
            if (m_paused || is_in_hitstop()) {
                m_gameplay_dt = 0.0f;
            }
            else {
                m_gameplay_dt = m_real_dt * m_gameplay_scale;
            }
            m_total_gameplay += m_gameplay_dt;
        }

        static uint64_t now_ns() {
            // В будущем — вынести в Platform layer.
            // Пока используем std::chrono, чтобы Clock не зависел от SDL.
            return static_cast<uint64_t>(
                std::chrono::high_resolution_clock::now().time_since_epoch().count()
                );
        }

        uint64_t m_last_ns = 0;
        float m_real_dt = 0.0f;
        float m_gameplay_dt = 0.0f;
        float m_total_real = 0.0f;
        float m_total_gameplay = 0.0f;

        float m_hitstop_remaining = 0.0f;
        bool  m_paused = false;
        float m_gameplay_scale = 1.0f;

        // FPS counter (встроен, чтобы не плодить сущности)
        float m_fps = 0.0f;
        float m_fps_accum = 0.0f;
        int   m_frame_count = 0;
    };

} // namespace Solace