#pragma once

#include <engine/graphics/mesh.hpp>
#include <engine/graphics/shader.hpp>
#include <engine/utils/math_utils.hpp>
#include <memory>

namespace App {

    struct Vec3 {
        float x = 0.0f, y = 0.0f, z = 0.0f;
    };

    class WorldGen {
    public:
        WorldGen();

        void loadAssets();

        void render(const Solace::Shader& shader,
            Solace::Mat4 view,
            Solace::Mat4 proj) const;

        static Solace::MeshData buildPillarData(float radius, float height, int segments);

    private:
        std::unique_ptr<Solace::Mesh> m_monolith;
        std::unique_ptr<Solace::Mesh> m_pillar;

        Vec3 m_pillarPos = { 0.0f, 0.0f, 0.0f };
        Vec3 m_monolithPos = { 3.0f, 0.0f, 0.0f };
    };

} // namespace App 