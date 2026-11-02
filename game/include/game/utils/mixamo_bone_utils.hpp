#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>

#include <ozz/animation/runtime/skeleton.h>

namespace App {

    inline std::vector<std::string> to_string_vector(ozz::span<const char* const> names) {
        std::vector<std::string> out;
        out.reserve(names.size());
        for (const char* n : names) {
            out.emplace_back(n ? n : "");
        }
        return out;
    }

    inline std::string normalize_mixamo_bone_name(std::string name) {
        const auto colon = name.find(':');
        if (colon != std::string::npos) {
            name = name.substr(colon + 1);
        }

        const auto pipe = name.find('|');
        if (pipe != std::string::npos) {
            name = name.substr(pipe + 1);
        }

        // Убираем суффиксы вроде "_001" (4 символа)
        while (name.size() >= 4 &&
            name[name.size() - 4] == '_' &&
            std::isdigit(static_cast<unsigned char>(name[name.size() - 3])) &&
            std::isdigit(static_cast<unsigned char>(name[name.size() - 2])) &&
            std::isdigit(static_cast<unsigned char>(name[name.size() - 1])))
        {
            name.resize(name.size() - 4);
        }

        // Убираем суффиксы вроде "_01" (3 символа)
        while (name.size() >= 3 &&
            name[name.size() - 3] == '_' &&
            std::isdigit(static_cast<unsigned char>(name[name.size() - 2])) &&
            std::isdigit(static_cast<unsigned char>(name[name.size() - 1])))
        {
            name.resize(name.size() - 3);
        }

        while (!name.empty() && std::isspace(static_cast<unsigned char>(name.front()))) {
            name.erase(name.begin());
        }
        while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back()))) {
            name.pop_back();
        }

        std::transform(name.begin(), name.end(), name.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        return name;
    }

    inline std::vector<int> build_mixamo_bone_map(
        const std::vector<std::string>& assimp_bone_names,
        ozz::span<const char* const> ozz_joint_names)
    {
        std::unordered_map<std::string, int> ozz_index_by_name;
        ozz_index_by_name.reserve(ozz_joint_names.size());

        for (size_t i = 0; i < ozz_joint_names.size(); ++i) {
            const std::string key = normalize_mixamo_bone_name(ozz_joint_names[i] ? ozz_joint_names[i] : "");
            ozz_index_by_name[key] = static_cast<int>(i);
        }

        std::vector<int> bone_map(assimp_bone_names.size(), -1);

        for (size_t i = 0; i < assimp_bone_names.size(); ++i) {
            const std::string key = normalize_mixamo_bone_name(assimp_bone_names[i]);
            auto it = ozz_index_by_name.find(key);
            if (it != ozz_index_by_name.end()) {
                bone_map[i] = it->second;
            }
        }

        return bone_map;
    }

    inline bool validate_bone_map(
        const std::vector<std::string>& assimp_bone_names,
        ozz::span<const char* const> ozz_joint_names,
        const std::vector<int>& bone_map)
    {
        if (assimp_bone_names.empty() || ozz_joint_names.empty()) return false;
        if (assimp_bone_names.size() != bone_map.size()) return false;

        int mapped = 0;
        for (int idx : bone_map) {
            if (idx >= 0) ++mapped;
        }

        // Достаточно, чтобы смапилась хотя бы половина костей
        if (mapped < (assimp_bone_names.size() / 2)) {
            return false;
        }

        return true;
    }

} // namespace App