#pragma once

#include <SDL3/SDL.h>
#include <unordered_map>
#include <bitset>

namespace Solace {
    enum class GameAction {
        Dash,
        FlyDown,
        AttackPrimary,
        AttackHeavy,
        Interact,
        ToggleFullscreen,
        Pause,
        Count
    };
    enum class GameAxis {
        MoveX,
        MoveY,
        FlyY,
        AimX,
        AimY,
        MouseDeltaX,
        MouseDeltaY,
        Count
    };

    class InputSystem {
    public:
        InputSystem() = delete;
        ~InputSystem() = delete;

        static void init();
        static void update();
        static void handleEvent(const SDL_Event& event);

        static bool isActionDown(GameAction action);
        static bool isActionJustPressed(GameAction action);
        static bool isActionJustReleased(GameAction action);

        static float getAxis(GameAxis axis);

        static void getMousePosition(float& outX, float& outY);

        static void bindKeyToAction(GameAction action, SDL_Scancode scancode);
        static void bindMouseButtonToAction(GameAction action, Uint8 button);

    private:
        static std::bitset<(size_t)GameAction::Count> s_actionStates;
        static std::bitset<(size_t)GameAction::Count> s_actionPrevStates;

        static float s_axisValues[(size_t)GameAxis::Count];
        static float s_mouseX;
        static float s_mouseY;

        static std::unordered_map<SDL_Scancode, GameAction> s_keyToActionMap;
        static std::unordered_map<Uint8, GameAction> s_mouseButtonToActionMap;

        static void setActionState(GameAction action, bool pressed);
    };

} // namespace Solace 