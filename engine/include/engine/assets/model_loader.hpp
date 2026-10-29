#pragma once

#include <engine/graphics/mesh.hpp>
#include <string>
#include <vector>

namespace Solace {
    class ModelLoader {
    public:
        // Теперь может загружать .glb, .gltf, .obj, .fbx и возвращать набор мешей модели
        static std::vector<MeshData> loadFromFile(const std::string& path);

        // Обратная совместимость для старого кода (возвращает первый меш или объединяет их)
        static MeshData loadObj(const std::string& path);
    };
}