cmake_minimum_required(VERSION 3.22)
project(engine LANGUAGES C CXX)

# Собираем статические модули движка
add_library(engine STATIC
    # Core
    include/engine/core/engine.hpp
    include/engine/core/window.hpp
    include/engine/core/logger.hpp
    include/engine/core/graphics_settings.hpp
    include/engine/core/settings_manager.hpp
    src/core/window.cpp
    src/core/engine.cpp
    src/core/settings_manager.cpp

    # Debug
    include/engine/debug/debug_overlay.hpp
    include/engine/debug/fps.hpp
    src/debug/debug_overlay.cpp
    src/debug/fps.cpp

    # Graphics & Render
    include/engine/graphics/frame_buffer.hpp
    include/engine/graphics/mesh.hpp
    include/engine/graphics/post_process.hpp
    include/engine/graphics/renderer.hpp
    include/engine/graphics/shader.hpp
    include/engine/graphics/shader_loader.hpp
    include/engine/graphics/camera.hpp
    src/graphics/frame_buffer.cpp
    src/graphics/mesh.cpp
    src/graphics/post_process.cpp
    src/graphics/renderer.cpp
    src/graphics/shader.cpp
    src/graphics/shader_loader.cpp
    src/graphics/camera.cpp

    # Systems
    include/engine/systems/combat_system.hpp
    include/engine/systems/input_system.hpp
    include/engine/systems/item_system.hpp
    src/systems/combat_system.cpp
    src/systems/input_system.cpp
    src/systems/item_system.cpp

    # Animations
    include/engine/animations/animator.hpp
    src/animations/animator.cpp

    # Assets
    include/engine/assets/model_loader.hpp
    src/assets/model_loader.cpp

    # Utils
    include/engine/utils/math_utils.hpp
    include/engine/utils/random_gen.hpp
    src/utils/math_utils.cpp
    src/utils/random_gen.cpp
    src/utils/stb_image_impl.cpp

    # AI
    include/engine/ai/behavior_tree.hpp
    include/engine/ai/enemy_modules.hpp
)

# Прописываем пути к заголовкам всех внешних библиотек
target_include_directories(engine PUBLIC
    include
    $<TARGET_PROPERTY:glad,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:assimp,INTERFACE_INCLUDE_DIRECTORIES>
    ${assimp_BINARY_DIR}/include
)

# Линкуем движок с системными и сторонними библиотеками
target_link_libraries(engine PUBLIC
    SDL3::SDL3
    assimp::assimp
    imgui
    glad
    stb
    ozz_animation
    ozz_base
    ozz_geometry
    OpenGL::GL
)

target_link_libraries(engine PRIVATE
    nlohmann_json::nlohmann_json
)