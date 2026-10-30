#pragma once

#include <ozz/animation/runtime/animation.h>

#include <string>
#include <vector>

namespace Solace {

    // ============================================================
    // FrameEvent — событие внутри клипа, триггерится при проходе кадра.
    //
    // frame считается по FPS клипа (обычно 30). При dt = 0.016 (60 FPS)
    // один игровой кадр = ~0.5 кадра анимации.
    //
    // Пример: attack_sword (30 FPS, длина 24 кадра):
    //   frame 8  → "swing" (замах)
    //   frame 14 → "hit"   (hitbox active start)
    //   frame 18 → "hit_end" (hitbox active end)
    // ============================================================
    struct FrameEvent {
        int         frame;
        std::string name;
    };


    // ============================================================
    // AnimationClip — runtime animation + metadata.
    //
    // - animation    : ozz::animation::Animation (shared, из ModelAsset)
    // - name         : "idle", "run", "attack_1", "death"
    // - fps          : frame rate для events (30 обычно)
    // - loop         : зациклен? (idle/run = true, attack/death = false)
    // - events       : frame-triggered events
    // - interrupts   : какие клипы можно прервать этим клипом (для ASM)
    // ============================================================
    struct AnimationClip {
        const ozz::animation::Animation* animation = nullptr;
        std::string name;
        float fps = 30.0f;
        bool  loop = true;
        std::vector<FrameEvent> events;

        float duration() const {
            return animation ? animation->duration() : 0.0f;
        }

        int total_frames() const {
            return static_cast<int>(duration() * fps);
        }

        int frame_at(float time) const {
            return static_cast<int>(time * fps);
        }
    };

} // namespace Solace