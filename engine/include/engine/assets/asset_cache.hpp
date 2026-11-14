#pragma once

#include <engine/graphics/model_asset.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace Solace {

    // ============================================================
    // AssetCache — дедупликация и владение ассетами.
    //
    // - Owned by Application (DI, не singleton)
    // - get_model(path) → shared_ptr<ModelAsset>
    // - Один экземпляр на путь
    // - Thread-safety: НЕ потокобезопасен (Phase 0.7 — single-threaded)
    // ============================================================
    class AssetCache {
    public:
        AssetCache() = default;
        ~AssetCache() = default;
        AssetCache(const AssetCache&) = delete;
        AssetCache& operator=(const AssetCache&) = delete;

        // Загрузить (или получить уже загруженный) ModelAsset.
        // При ошибке возвращает nullptr и логирует.
        std::shared_ptr<ModelAsset> get_model(const std::string& path);

        // Очистить всё (между сессиями)
        void clear();

        // Debug
        size_t model_count() const { return m_models.size(); }
        std::vector<std::string> debug_model_paths() const;

        bool load_skeleton_ozz(ModelAsset& asset, const std::string& path);
        bool load_animation_ozz(ModelAsset& asset, const std::string& name, const std::string& path);

    private:
        std::unordered_map<std::string, std::shared_ptr<ModelAsset>> m_models;
    };

} // namespace Solace