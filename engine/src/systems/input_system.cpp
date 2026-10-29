#include <engine/systems/input_system.hpp>
#include <engine/core/logger.hpp>
#include <algorithm>
#include <iostream>

namespace Solace {
    std::bitset<(size_t)GameAction::Count> InputSystem::s_actionStates;
    std::bitset<(size_t)GameAction::Count> InputSystem::s_actionPrevStates;
    float InputSystem::s_axisValues[(size_t)GameAxis::Count] = { 0.0f };
    float InputSystem::s_mouseX = 0.0f;
    float InputSystem::s_mouseY = 0.0f;

    std::unordered_map<SDL_Scancode, GameAction> InputSystem::s_keyToActionMap;
    std::unordered_map<Uint8, GameAction> InputSystem::s_mouseButtonToActionMap;

    void InputSystem::init() {
        bindKeyToAction(GameAction::Dash, SDL_SCANCODE_SPACE);
        bindKeyToAction(GameAction::FlyDown, SDL_SCANCODE_LSHIFT);
        bindKeyToAction(GameAction::AttackPrimary, SDL_SCANCODE_J);
        bindKeyToAction(GameAction::AttackHeavy, SDL_SCANCODE_K);
        bindKeyToAction(GameAction::Interact, SDL_SCANCODE_E);
        bindKeyToAction(GameAction::Pause, SDL_SCANCODE_ESCAPE);

        bindKeyToAction(GameAction::ToggleFullscreen, SDL_SCANCODE_F11);

        bindMouseButtonToAction(GameAction::AttackPrimary, SDL_BUTTON_LEFT);
        bindMouseButtonToAction(GameAction::AttackHeavy, SDL_BUTTON_RIGHT);

        Logger::info("InputSystem initialized successfully with default action mappings.");
    }

    void InputSystem::update() {
        s_actionPrevStates = s_actionStates;

        s_axisValues[(size_t)GameAxis::MoveX] = 0.0f;
        s_axisValues[(size_t)GameAxis::MoveY] = 0.0f;
        s_axisValues[(size_t)GameAxis::FlyY] = 0.0f;

        s_axisValues[(size_t)GameAxis::MouseDeltaX] = 0.0f;
        s_axisValues[(size_t)GameAxis::MouseDeltaY] = 0.0f;

        const bool* state = SDL_GetKeyboardState(nullptr);
        if (state[SDL_SCANCODE_A] || state[SDL_SCANCODE_LEFT])  s_axisValues[(size_t)GameAxis::MoveX] -= 1.0f;
        if (state[SDL_SCANCODE_D] || state[SDL_SCANCODE_RIGHT]) s_axisValues[(size_t)GameAxis::MoveX] += 1.0f;
        if (state[SDL_SCANCODE_S] || state[SDL_SCANCODE_DOWN])  s_axisValues[(size_t)GameAxis::MoveY] -= 1.0f;
        if (state[SDL_SCANCODE_W] || state[SDL_SCANCODE_UP])    s_axisValues[(size_t)GameAxis::MoveY] += 1.0f;

        if (state[SDL_SCANCODE_SPACE])  s_axisValues[(size_t)GameAxis::FlyY] += 1.0f;
        if (state[SDL_SCANCODE_LSHIFT]) s_axisValues[(size_t)GameAxis::FlyY] -= 1.0f;
    }

    void InputSystem::handleEvent(const SDL_Event& event) {
        if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
            auto it = s_keyToActionMap.find(event.key.scancode);
            if (it != s_keyToActionMap.end()) {
                setActionState(it->second, true);
            }
        }
        else if (event.type == SDL_EVENT_KEY_UP) {
            auto it = s_keyToActionMap.find(event.key.scancode);
            if (it != s_keyToActionMap.end()) {
                setActionState(it->second, false);
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            auto it = s_mouseButtonToActionMap.find(event.button.button);
            if (it != s_mouseButtonToActionMap.end()) {
                setActionState(it->second, true);
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            auto it = s_mouseButtonToActionMap.find(event.button.button);
            if (it != s_mouseButtonToActionMap.end()) {
                setActionState(it->second, false);
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION) {
            s_mouseX = event.motion.x;
            s_mouseY = event.motion.y;

            s_axisValues[(size_t)GameAxis::AimX] = s_mouseX;
            s_axisValues[(size_t)GameAxis::AimY] = s_mouseY;

            s_axisValues[(size_t)GameAxis::MouseDeltaX] = event.motion.xrel;
            s_axisValues[(size_t)GameAxis::MouseDeltaY] = event.motion.yrel;
        }
    }

    void InputSystem::setActionState(GameAction action, bool pressed) {
        s_actionStates.set((size_t)action, pressed);
    }

    bool InputSystem::isActionDown(GameAction action) {
        return s_actionStates.test((size_t)action);
    }

    bool InputSystem::isActionJustPressed(GameAction action) {
        return s_actionStates.test((size_t)action) && !s_actionPrevStates.test((size_t)action);
    }

    bool InputSystem::isActionJustReleased(GameAction action) {
        return !s_actionStates.test((size_t)action) && s_actionPrevStates.test((size_t)action);
    }

    float InputSystem::getAxis(GameAxis axis) {
        return s_axisValues[(size_t)axis];
    }

    void InputSystem::getMousePosition(float& outX, float& outY) {
        outX = s_mouseX;
        outY = s_mouseY;
    }

    void InputSystem::bindKeyToAction(GameAction action, SDL_Scancode scancode) {
        s_keyToActionMap[scancode] = action;
    }

    void InputSystem::bindMouseButtonToAction(GameAction action, Uint8 button) {
        s_mouseButtonToActionMap[button] = action;
    }

} // namespace Solace