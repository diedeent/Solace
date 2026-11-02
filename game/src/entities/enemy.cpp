#include <game/entities/enemy.hpp>
#include <engine/debug/logger.hpp>
#include <cmath>
#include <algorithm>

namespace App {

    Enemy::Enemy(Solace::EntityID id, Solace::EventBus& bus, float x, float y, float z)
        : m_animId(id), m_bus(bus), m_posX(x), m_posY(y), m_posZ(z)
    {
        glGenBuffers(1, &m_boneSsbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_boneSsbo);

        // Инициализируем identity-матрицами — тогда даже без ASM
        // вершины отрисуются в bind pose (T-поза), а не схлопнутся в 0
        std::vector<float> identityData(128 * 16, 0.0f);
        for (size_t i = 0; i < 128; ++i) {
            identityData[i * 16 + 0] = 1.0f;
            identityData[i * 16 + 5] = 1.0f;
            identityData[i * 16 + 10] = 1.0f;
            identityData[i * 16 + 15] = 1.0f;
        }
        glBufferData(GL_SHADER_STORAGE_BUFFER, identityData.size() * sizeof(float),
            identityData.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        SOLACE_INFO("Enemy spawned at (", x, ",", y, ",", z, ")");
    }

    Enemy::~Enemy() {
        if (m_boneSsbo) glDeleteBuffers(1, &m_boneSsbo);
    }

    void Enemy::setup_from_asset(const std::shared_ptr<Solace::ModelAsset>& asset) {
        if (!asset || !asset->has_skeleton()) {
            SOLACE_WARN("Enemy: asset has no skeleton, animation disabled");
            return;
        }
        m_asset = asset;
        m_asm = std::make_unique<Solace::AnimationStateMachine>(m_animId, m_bus, asset);

        // Bone map: assimp bone index → ozz joint index
        auto ozz_names = asset->skeleton.joint_names();
        m_boneMap.assign(asset->bone_names.size(), -1);
        int mapped = 0;
        for (size_t i = 0; i < asset->bone_names.size(); ++i) {
            std::string a = asset->bone_names[i];
            auto c = a.find(':');
            if (c != std::string::npos) a = a.substr(c + 1);
            for (size_t j = 0; j < ozz_names.size(); ++j) {
                std::string o = ozz_names[j];
                auto c2 = o.find(':');
                if (c2 != std::string::npos) o = o.substr(c2 + 1);
                if (a == o) { m_boneMap[i] = (int)j; ++mapped; break; }
            }
        }
        SOLACE_INFO("Enemy bone map: ", mapped, "/", asset->bone_names.size());

        // Клипы
        if (asset->animations.count("idle")) {
            Solace::AnimationClip c;
            c.animation = &asset->animations.at("idle");
            c.loop = true;
            m_asm->add_clip("idle", c);
        }
        if (asset->animations.count("run")) {
            Solace::AnimationClip c;
            c.animation = &asset->animations.at("run");
            c.loop = true;
            m_asm->add_clip("run", c);
        }
        m_asm->play("idle");
    }

    void Enemy::take_damage(float dmg) {
        if (m_state == EnemyState::Dead) return;
        m_hp -= dmg;
        m_hitFlashTimer = 0.1f;
        SOLACE_INFO("Enemy: took ", dmg, " dmg, HP=", m_hp);
        if (m_hp <= 0.0f) { m_hp = 0.0f; m_state = EnemyState::Dead; }
    }

    float Enemy::update(float dt, float px, float py, float pz) {
        if (m_state == EnemyState::Dead) return 0.0f;

        const float dx = px - m_posX;
        const float dz = pz - m_posZ;
        const float dist = std::sqrt(dx * dx + dz * dz);

        // Face player — atan2(dz, dx) согласовано с player (forward = (cos,sin))
        if (dist > 0.001f) {
            const float targetYaw = std::atan2(dz, dx) * 180.0f / 3.14159265f;
            const float diff = std::remainder(targetYaw - m_yaw, 360.0f);
            m_yaw += diff * std::min(1.0f, 10.0f * dt);
        }

        if (m_cooldownTimer > 0.0f) m_cooldownTimer -= dt;
        if (m_hitFlashTimer > 0.0f) m_hitFlashTimer -= dt;

        float dealt = 0.0f;
        if (m_windupTimer > 0.0f) {
            m_windupTimer -= dt;
            if (m_windupTimer <= 0.0f) {
                m_windupTimer = 0.0f;
                if (dist <= m_attackRange * 1.2f) dealt = m_attackDamage;
                m_cooldownTimer = m_attackCooldown;
            }
        }
        if (m_windupTimer <= 0.0f && m_cooldownTimer <= 0.0f
            && dist <= m_attackRange && dist > 0.001f) {
            m_windupTimer = m_attackWindup;
        }

        const bool moving = (m_windupTimer <= 0.0f && dist > m_attackRange - 0.1f && dist > 0.001f);
        if (moving) {
            const float step = std::min(m_speed * dt, dist - (m_attackRange - 0.2f));
            if (step > 0.0f) {
                m_posX += (dx / dist) * step;
                m_posZ += (dz / dist) * step;
            }
        }

        if (m_hp <= 0.0f) m_state = EnemyState::Dead;
        else if (m_windupTimer > 0.0f) m_state = EnemyState::Windup;
        else if (m_cooldownTimer > 0.0f) m_state = EnemyState::Cooldown;
        else if (dist <= m_attackRange) m_state = EnemyState::Idle;
        else m_state = EnemyState::Chase;

        // Animation switch
        if (m_asm) {
            if (moving && m_asm->is_playing("idle")) m_asm->play("run", 0.15f);
            else if (!moving && m_asm->is_playing("run")) m_asm->play("idle", 0.15f);
            m_asm->update(dt);

            auto mats = m_asm->get_skinning_matrices();
            if (!mats.empty() && !m_boneMap.empty()) {
                std::vector<ozz::math::Float4x4> remapped(
                    m_boneMap.size(), ozz::math::Float4x4::identity());
                for (size_t i = 0; i < m_boneMap.size(); ++i) {
                    int oi = m_boneMap[i];
                    if (oi >= 0 && oi < (int)mats.size()) remapped[i] = mats[oi];
                }
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_boneSsbo);
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                    remapped.size() * sizeof(ozz::math::Float4x4), remapped.data());
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            }
        }

        return dealt;
    }

    void Enemy::render(Solace::Shader& shader,
        const Solace::Mat4& view, const Solace::Mat4& proj) {
        if (m_state == EnemyState::Dead || !m_asset) return;

        shader.bind();
        shader.setMat4("u_view", view.data());
        shader.setMat4("u_proj", proj.data());

        constexpr float MODEL_YAW_OFFSET = 1.5707963f;  // как у игрока
        const float yawRad = m_yaw * 3.14159265f / 180.0f;
        Solace::Mat4 model = Solace::mat4Multiply(
            Solace::mat4Translate(m_posX, m_posY, m_posZ),
            Solace::mat4RotateY(-yawRad + MODEL_YAW_OFFSET));
        shader.setMat4("u_model", model.data());

        if (m_asm) glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_boneSsbo);

        shader.setInt("u_textureDiffuse", 0);
        for (auto& mesh : m_asset->meshes) {
            // Враг — красный (без текстуры, чтобы отличать)
            shader.setInt("u_hasTexture", 0);
            shader.setVec3("u_baseColor", 0.85f, 0.15f, 0.15f);
            mesh->draw();
        }
        shader.unbind();
    }

} // namespace App