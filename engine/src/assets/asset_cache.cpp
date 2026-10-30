#include <engine/assets/asset_cache.hpp>
#include <engine/assets/model_loader.hpp>
#include <engine/core/logger.hpp>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <ozz/animation/runtime/local_to_model_job.h>
//#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>

namespace Solace {

    namespace {
        // Маппинг Assimp boneID → ozz joint index (Phase 1)
        // Пока просто сохраняем имена.
        void build_bone_names(ModelAsset& asset, const std::vector<MeshData>& meshes) {
            if (meshes.empty()) return;
            const auto& names = meshes[0].boneNames;
            asset.bone_names.assign(names.begin(), names.end());
        }

        // Считаем inverse bind pose для скелета (Phase 1 — уже есть в Animator,
        // здесь дублируем, т.к. ModelAsset — shared)
        void compute_inverse_bind_poses(ModelAsset& asset) {
            if (!asset.has_skeleton()) return;

            const size_t joints = asset.skeleton.num_joints();
            std::vector<ozz::math::SoaTransform> locals(asset.skeleton.num_soa_joints());
            std::memcpy(locals.data(),
                asset.skeleton.joint_rest_poses().data(),
                locals.size() * sizeof(ozz::math::SoaTransform));

            std::vector<ozz::math::Float4x4> models(joints);
            ozz::animation::LocalToModelJob ltm;
            ltm.skeleton = &asset.skeleton;
            ltm.input = ozz::make_span(locals);
            ltm.output = ozz::make_span(models);
            if (!ltm.Run()) {
                Logger::error("AssetCache: failed to compute inverse bind poses");
                return;
            }

            asset.inverse_bind_poses.resize(joints);
            for (size_t i = 0; i < joints; ++i) {
                asset.inverse_bind_poses[i] = ozz::math::Invert(models[i]);
            }
        }
    } // namespace

    std::shared_ptr<ModelAsset> AssetCache::get_model(const std::string& path) {
        auto it = m_models.find(path);
        if (it != m_models.end()) return it->second;

        auto asset = std::make_shared<ModelAsset>();
        asset->path = path;

        try {
            auto mesh_datas = ModelLoader::loadFromFile(path);
            if (mesh_datas.empty()) {
                Logger::error("AssetCache: empty model: ", path);
                return nullptr;
            }

            for (const auto& md : mesh_datas) {
                asset->meshes.push_back(std::make_unique<Mesh>(md));
            }
            build_bone_names(*asset, mesh_datas);
        }
        catch (const std::exception& e) {
            Logger::error("AssetCache: failed to load '", path, "': ", e.what());
            return nullptr;
        }

        // Skeleton + animations: Phase 0.7 — заглушка.
        // Phase 1.1: загрузка .ozz + маппинг bone_names.
        // Пока asset->has_skeleton() = false.

        m_models.emplace(path, asset);
        Logger::info("AssetCache: loaded '", path, "' (", asset->meshes.size(), " meshes)");
        return asset;
    }

    void AssetCache::clear() {
        m_models.clear();
    }

    std::vector<std::string> AssetCache::debug_model_paths() const {
        std::vector<std::string> paths;
        paths.reserve(m_models.size());
        for (const auto& [p, _] : m_models) paths.push_back(p);
        std::sort(paths.begin(), paths.end());
        return paths;
    }

    // ModelAsset::total_vertices
    size_t ModelAsset::total_vertices() const {
        size_t n = 0;
        for (const auto& m : meshes) if (m) n += m->indexCount();
        return n;
    }

} // namespace Solace