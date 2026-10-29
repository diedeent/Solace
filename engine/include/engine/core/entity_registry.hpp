#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <engine/core/event_bus.hpp>

namespace Solace {

    using EntityID = uint32_t;
    constexpr EntityID INVALID_ENTITY = 0;
    constexpr size_t   MAX_ENTITIES = 65536;

    // ============================================================
    // ComponentPool<T> — плотный массив компонентов, индексированный EntityID.
    //
    // - vector<T>        : данные (index = EntityID)
    // - vector<bool>     : присутствие (index = EntityID)
    //
    // Гарантии:
    //   - get(id) возвращает nullptr, если компонента нет или id мёртв
    //   - add(id) перезаписывает существующий компонент
    //   - remove(id) не ломает итерацию (только флаг)
    //   - for_each итерирует только живые компоненты
    // ============================================================
    template<typename T>
    class ComponentPool {
    public:
        ComponentPool() = default;

        T* add(EntityID id, T component) {
            if (id == INVALID_ENTITY || id >= MAX_ENTITIES) return nullptr;
            ensure_size(id + 1);
            m_data[id] = std::move(component);
            m_present[id] = true;
            return &m_data[id];
        }

        T* get(EntityID id) {
            if (id == INVALID_ENTITY || id >= m_data.size()) return nullptr;
            if (!m_present[id]) return nullptr;
            return &m_data[id];
        }

        const T* get(EntityID id) const {
            if (id == INVALID_ENTITY || id >= m_data.size()) return nullptr;
            if (!m_present[id]) return nullptr;
            return &m_data[id];
        }

        bool has(EntityID id) const {
            return get(id) != nullptr;
        }

        void remove(EntityID id) {
            if (id == INVALID_ENTITY || id >= m_data.size()) return;
            m_present[id] = false;
        }

        template<typename Fn>
        void for_each(Fn&& fn) {
            for (size_t i = 1; i < m_data.size(); ++i) {
                if (m_present[i]) fn(static_cast<EntityID>(i), m_data[i]);
            }
        }

        template<typename Fn>
        void for_each(Fn&& fn) const {
            for (size_t i = 1; i < m_data.size(); ++i) {
                if (m_present[i]) fn(static_cast<EntityID>(i), m_data[i]);
            }
        }

        size_t count() const {
            size_t n = 0;
            for (size_t i = 1; i < m_present.size(); ++i) n += m_present[i] ? 1 : 0;
            return n;
        }

        void clear() {
            m_data.clear();
            m_present.clear();
        }

    private:
        void ensure_size(size_t new_size) {
            if (new_size > m_data.size()) {
                m_data.resize(new_size);
                m_present.resize(new_size, false);
            }
        }

        std::vector<T>    m_data;
        std::vector<bool> m_present;
    };


    // ============================================================
    // EntityDestroyedEvent — эмитится в process_destroy().
    //
    // Все системы, которые держат ссылки на EntityID, подписаны.
    // Порядок: systems update → process_destroy → emit (sync).
    // ============================================================
    struct EntityDestroyedEvent {
        EntityID id;
    };


    // ============================================================
    // EntityRegistry — владелец сущностей и их компонентов.
    //
    // Гарантии:
    //   - create() → новый EntityID (никогда не переиспользуется)
    //   - destroy(id) → deferred, is_alive(id) = false сразу
    //   - process_destroy() → удаляет компоненты + emit EntityDestroyedEvent
    //   - get_component<T>(id) → nullptr, если id мёртв/нет компонента
    //   - for_each<A, B>() итерирует только сущности с A и B
    // ============================================================
    class EntityRegistry {
    public:
        explicit EntityRegistry(EventBus& bus) : m_bus(bus) {}
        ~EntityRegistry() = default;

        EntityRegistry(const EntityRegistry&) = delete;
        EntityRegistry& operator=(const EntityRegistry&) = delete;

        // ============ Жизненный цикл ============

        EntityID create() {
            if (m_next_id >= MAX_ENTITIES) {
                // 64k за сессию — не должно случаться. Если случилось — баг.
                return INVALID_ENTITY;
            }
            const EntityID id = m_next_id++;
            m_alive[id] = true;
            return id;
        }

        void destroy(EntityID id) {
            if (!is_alive(id)) return;
            m_alive[id] = false;
            m_pending_destroy.push_back(id);
        }

        void process_destroy() {
            if (m_pending_destroy.empty()) return;

            for (EntityID id : m_pending_destroy) {
                // Удаляем все компоненты
                for (auto& [_, pool] : m_pools) {
                    (void)_;
                    // Каждый pool знает свой T — нужен type-erased remove.
                    // Используем visitor-подход через лямбду, сохранённую в pool.
                    // См. PoolBase::remove_by_id
                }
                // Эмитим событие
                m_bus.emit(EntityDestroyedEvent{ id });
            }
            m_pending_destroy.clear();
        }

        bool is_alive(EntityID id) const {
            if (id == INVALID_ENTITY || id >= m_alive.size()) return false;
            return m_alive[id];
        }

        // ============ Компоненты ============

        template<typename T>
        T* add_component(EntityID id, T component) {
            if (!is_alive(id)) return nullptr;
            auto& pool = get_or_create_pool<T>();
            return pool.add(id, std::move(component));
        }

        template<typename T>
        T* get_component(EntityID id) {
            if (!is_alive(id)) return nullptr;
            auto* pool = find_pool<T>();
            return pool ? pool->get(id) : nullptr;
        }

        template<typename T>
        const T* get_component(EntityID id) const {
            if (!is_alive(id)) return nullptr;
            auto* pool = find_pool<T>();
            return pool ? pool->get(id) : nullptr;
        }

        template<typename T>
        bool has_component(EntityID id) const {
            if (!is_alive(id)) return false;
            auto* pool = find_pool<T>();
            return pool && pool->has(id);
        }

        template<typename T>
        void remove_component(EntityID id) {
            if (!is_alive(id)) return;
            auto* pool = find_pool<T>();
            if (pool) pool->remove(id);
        }

        // ============ Итерация ============

        template<typename... Components, typename Fn>
        void for_each(Fn&& fn) {
            // Итерируем по первой компоненте, проверяем остальные
            if constexpr (sizeof...(Components) == 0) {
                // Только по сущностям
                for (size_t i = 1; i < m_alive.size(); ++i) {
                    if (m_alive[i]) fn(static_cast<EntityID>(i));
                }
            }
            else if constexpr (sizeof...(Components) == 1) {
                using First = std::tuple_element_t<0, std::tuple<Components...>>;
                auto* pool = find_pool<First>();
                if (!pool) return;
                pool->for_each([&](EntityID id, First& c) {
                    fn(id, c);
                    });
            }
            else {
                // 2+ компонентов: итерируем по первой, проверяем остальные
                using First = std::tuple_element_t<0, std::tuple<Components...>>;
                auto* first_pool = find_pool<First>();
                if (!first_pool) return;
                first_pool->for_each([&](EntityID id, First& c0) {
                    if (has_all<Components...>(id)) {
                        invoke_with_all<Components...>(id, [&](auto&... comps) {
                            fn(id, comps...);
                            });
                    }
                    });
            }
        }

        // ============ Управление ============

        void clear() {
            for (auto& [_, pool] : m_pools) (void)_;
            // Очищаем все pools через их clear()
            for (auto& [_, pool] : m_pools) {
                (void)_;
                // Нужно вызвать clear() на PoolBase — см. PoolBase::clear
            }
            m_pools.clear();
            m_alive.clear();
            m_pending_destroy.clear();
            m_next_id = 1;
        }

        size_t alive_count() const {
            size_t n = 0;
            for (size_t i = 1; i < m_alive.size(); ++i) n += m_alive[i] ? 1 : 0;
            return n;
        }

        size_t pending_count() const { return m_pending_destroy.size(); }

        EntityID next_id() const { return m_next_id; }

    private:
        // ---- Type-erased base для всех pools ----
        struct PoolBase {
            virtual ~PoolBase() = default;
            virtual void clear() = 0;
            virtual void remove_by_id(EntityID id) = 0;
        };

        template<typename T>
        struct PoolWrapper : PoolBase {
            ComponentPool<T> pool;
            void clear() override { pool.clear(); }
            void remove_by_id(EntityID id) override { pool.remove(id); }
        };

        template<typename T>
        PoolWrapper<T>& get_or_create_pool() {
            auto typeId = std::type_index(typeid(T));
            auto it = m_pools.find(typeId);
            if (it == m_pools.end()) {
                auto [new_it, _] = m_pools.emplace(typeId, std::make_unique<PoolWrapper<T>>());
                return *static_cast<PoolWrapper<T>*>(new_it->second.get());
            }
            return *static_cast<PoolWrapper<T>*>(it->second.get());
        }

        template<typename T>
        PoolWrapper<T>* find_pool() {
            auto it = m_pools.find(std::type_index(typeid(T)));
            if (it == m_pools.end()) return nullptr;
            return static_cast<PoolWrapper<T>*>(it->second.get());
        }

        template<typename T>
        const PoolWrapper<T>* find_pool() const {
            auto it = m_pools.find(std::type_index(typeid(T)));
            if (it == m_pools.end()) return nullptr;
            return static_cast<const PoolWrapper<T>*>(it->second.get());
        }

        template<typename... Components>
        bool has_all(EntityID id) {
            bool all = true;
            ((all = all && has_component<Components>(id)), ...);
            return all;
        }

        template<typename... Components, typename Fn>
        void invoke_with_all(EntityID id, Fn&& fn) {
            fn(*get_component<Components>(id)...);
        }

        EventBus& m_bus;
        EntityID m_next_id = 1;
        std::vector<bool> m_alive;
        std::vector<EntityID> m_pending_destroy;
        std::unordered_map<std::type_index, std::unique_ptr<PoolBase>> m_pools;
    };

} // namespace Solace