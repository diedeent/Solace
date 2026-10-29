#include <game/core/game.hpp>
#include <engine/core/logger.hpp>
#include <filesystem>

int main(int argc, char* argv[]) {
    try {
        App::Game game;
        game.run();
    }
    catch (const std::exception& e) {
        Solace::Logger::error("Fatal Error: ", e.what());
        return -1;
    }

    Solace::Logger::info("Game exited successfully.");
    return 0;
}