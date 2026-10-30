#pragma once

#include <engine/core/event_bus.hpp>
#include <engine/core/logger.hpp>

#include <nlohmann/json.hpp>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace Solace {

    struct ConfigReloadRequestEvent {};
    struct ConfigReloadedEvent { bool success = false; };

    class ConfigCache {
    public:
        ConfigCache() = default;
        ~ConfigCache() = default;
        ConfigCache(const ConfigCache&) = delete;
        ConfigCache& operator=(const ConfigCache&) = delete;

        template<typename T>
        using Parser = std::function<bool(const nlohmann::json&, T&)>;

        template<typename T>
        void register_section(std::string name, Parser<T> parser) {
            auto section = std::make_unique<Section<T>>();
            section->parser = std::move(parser);
            m_type_to_name[std::type_index(typeid(T))] = name;
            m_sections.emplace(std::move(name), std::move(section));
        }

        bool load(const std::string& root_dir) {
            m_root_dir = root_dir;
            return reload();
        }

        bool reload() {
            if (m_root_dir.empty()) {
                Logger::warning("ConfigCache: reload() before load()");
                return false;
            }

            bool all_ok = true;
            for (auto& [name, section] : m_sections) {
                if (!reload_section(name, *section)) all_ok = false;
            }

            Logger::info(all_ok ? "ConfigCache: all sections reloaded"
                : "ConfigCache: some sections failed");
            return all_ok;
        }

        template<typename T>
        const T& get() const {
            const auto* section = find_section<T>();
            assert(section && "ConfigCache::get<T>() — type not registered");
            return section->data;
        }

        template<typename T>
        bool has() const { return find_section<T>() != nullptr; }

    private:
        struct SectionBase {
            virtual ~SectionBase() = default;
            virtual bool parse_from_json(const nlohmann::json& j) = 0;
        };

        template<typename T>
        struct Section : SectionBase {
            T data;
            Parser<T> parser;

            bool parse_from_json(const nlohmann::json& j) override {
                T temp = data;  // откат при ошибке парсинга
                try {
                    if (parser(j, temp)) {
                        data = std::move(temp);
                        return true;
                    }
                }
                catch (const std::exception& e) {
                    Logger::error("ConfigCache: parse exception: ", e.what());
                }
                return false;  // data остаётся прежней
            }
        };

        template<typename T>
        const Section<T>* find_section() const {
            auto it = m_type_to_name.find(std::type_index(typeid(T)));
            if (it == m_type_to_name.end()) return nullptr;
            auto sec_it = m_sections.find(it->second);
            if (sec_it == m_sections.end()) return nullptr;
            return static_cast<const Section<T>*>(sec_it->second.get());
        }

        bool reload_section(const std::string& name, SectionBase& section) {
            const auto file = std::filesystem::path(m_root_dir) / (name + ".json");

            std::ifstream in(file);
            if (!in) {
                Logger::warning("ConfigCache: not found: ", file.string(), " (keeping old)");
                return false;
            }

            try {
                nlohmann::json j;
                in >> j;
                return section.parse_from_json(j);
            }
            catch (const std::exception& e) {
                Logger::error("ConfigCache: JSON error in ", file.string(), ": ", e.what());
                return false;
            }
        }

        std::string m_root_dir;
        std::unordered_map<std::string, std::unique_ptr<SectionBase>> m_sections;
        std::unordered_map<std::type_index, std::string> m_type_to_name;
    };

} // namespace Solace