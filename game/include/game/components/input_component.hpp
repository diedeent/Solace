#pragma once

namespace App {

    // ============================================================
    // InputComponent — кеш ввода на кадр (для AI/Player).
    //
    // Заполняется в InputSystem или Scene::on_update.
    // ============================================================
    struct InputComponent {
        float move_x = 0.0f;
        float move_y = 0.0f;
        float aim_x = 0.0f;
        float aim_y = 0.0f;

        bool  attack_pressed = false;  // JustPressed в этом кадре
        bool  dash_pressed = false;
        bool  interact_pressed = false;
    };

} // namespace App