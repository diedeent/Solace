cmake_minimum_required(VERSION 3.22)
project(game LANGUAGES CXX)

add_executable(Solace
    # Core
    include/game/core/game_state.hpp
    include/game/core/game.hpp
    src/core/game.cpp
    src/core/game_state.cpp
    src/main.cpp

    # Systems

    # Components
    include/game/systems/player_movement_system.hpp
    src/systems/player_movement_system.cpp

    # Entities
    include/game/entities/player.hpp
    include/game/entities/enemy.hpp
    src/entities/player.cpp

    # Utils
    include/game/utils/player_model.hpp
    src/utils/player_model.cpp

    # World
    include/game/world/structures.hpp
    include/game/world/world_gen.hpp
    src/world/structures.cpp
    src/world/world_gen.cpp
)

target_include_directories(Solace PRIVATE
    include
)

target_link_libraries(Solace PRIVATE
    engine
)

set_target_properties(Solace PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Так как всё собирается статически, SDL3.dll больше не нужен.
# Копируем в директорию с .exe только папки с ассетами игры.
add_custom_command(TARGET Solace POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:Solace>/assets"
    COMMENT "Copying game assets to output directory..."
)