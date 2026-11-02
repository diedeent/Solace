#include <game/core/game.hpp>
#include <engine/debug/logger.hpp>
#include <filesystem>

int main(int argc, char* argv[]) {
    try {
        App::Game game;
        game.run();
    }
    catch (const std::exception& e) {
        SOLACE_CRITICAL(e.what());
        return -1;
    }

    SOLACE_INFO("Game exited successfully.");
    return 0;
}