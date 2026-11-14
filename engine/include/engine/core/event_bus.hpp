#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <queue>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifndef NDEBUG
#include <cassert>
#endif

namespace Solace {

    // 0 зарезервирован как INVALID
    using SubscriptionID = uint64_t;
    constexpr SubscriptionID INVALID_SUBSCRIPTION = 0;

    class ScopedSubscriptions;

    // ============================================================
    // EventBus — type-erased, DI-driven pub/sub
    //
    // Жизненный цикл: Application владеет EventBus, передаёт по &
    // в Scene, Scene — в системы через конструктор.
    //
    // Гарантии:
    //   - Нет static/singleton
    //   - emit = sync, queue = deferred (FIFO, в конце кадра)
    //   - unsubscribe безопасен во время emit (snapshot)
    //   - ID не переиспользуются даже после clear()
    //   - ScopedSubscriptions автоматически отписывают при уничтожении
    //   - ~EventBus инвалидирует все ScopedSubscriptions
    // ============================================================
    class EventBus {
    public:
        EventBus() = default;
        ~EventBus();

        EventBus(const EventBus&) = delete;
        EventBus& operator=(const EventBus&) = delete;
        EventBus(EventBus&&) = delete;              // ссылки в Scoped не выживут move
        EventBus& operator=(EventBus&&) = delete;

        template<typename T>
        using EventCallback = std::function<void(const T&)>;

        // -------- Подписка / отписка --------

        template<typename T>
        SubscriptionID subscribe(EventCallback<T> callback) {
            const auto typeId = std::type_index(typeid(T));
            const SubscriptionID id = m_nextSubscriptionID++;

            ErasedHandler erased = [cb = std::move(callback)](const void* data) {
                cb(*static_cast<const T*>(data));
                };

            m_subscribers[typeId].push_back(Subscription{ id, std::move(erased) });
            m_idToType.emplace(id, typeId);
            return id;
        }

        void unsubscribe(SubscriptionID id) {
            auto it = m_idToType.find(id);
            if (it == m_idToType.end()) return;

            const std::type_index typeId = it->second;
            auto bucketIt = m_subscribers.find(typeId);
            if (bucketIt != m_subscribers.end()) {
                auto& list = bucketIt->second;
                list.erase(
                    std::remove_if(list.begin(), list.end(),
                        [id](const Subscription& s) { return s.id == id; }),
                    list.end()
                );
            }
            m_idToType.erase(it);
        }

        // -------- Эмиссия --------

        template<typename T>
        void emit(const T& event) {
#ifndef NDEBUG
            // Guard от рекурсии: emit внутри emit допустим только для
            // РАЗНЫХ типов. Защита от бесконечного цикла — на совести
            // подписчика (см. Handbook 1.3).
            assert(!m_inEmitOfSameType(typeid(T)) &&
                "emit<SameType> called re-entrantly — infinite loop risk");
            m_emitStack.push_back(typeid(T));
#endif

            const auto typeId = std::type_index(typeid(T));
            auto it = m_subscribers.find(typeId);
            if (it != m_subscribers.end()) {
                // Snapshot: если handler вызывает subscribe/unsubscribe,
                // итерация не сломается.
                auto snapshot = it->second;
                for (const auto& sub : snapshot) {
                    sub.handler(&event);
                }
            }

#ifndef NDEBUG
            m_emitStack.pop_back();
#endif
        }

        template<typename T>
        void queue(T event) {
            // Захватываем event по значению в замыкание — тип сохраняется.
            m_queue.push([this, ev = std::move(event)]() mutable {
                emit(ev);
                });
        }

        void processQueue() {
            // Snapshot + swap: если во время обработки кто-то queue'ит новое,
            // оно попадёт в СЛЕДУЮЩИЙ кадр (не в этот).
            auto pending = std::move(m_queue);
            m_queue = {};

            while (!pending.empty()) {
                pending.front()();
                pending.pop();
            }
        }

        // -------- Управление --------

        void clear() {
            m_subscribers.clear();
            m_idToType.clear();
            m_queue = {};
            // m_nextSubscriptionID НЕ сбрасываем — иначе коллизия ID
            // с живыми ScopedSubscriptions.
        }

        template<typename T>
        bool has_subscribers() const {
            auto it = m_subscribers.find(std::type_index(typeid(T)));
            return it != m_subscribers.end() && !it->second.empty();
        }

        // Debug
        struct Stats {
            size_t total_subscribers = 0;
            size_t unique_event_types = 0;
            size_t queued_events = 0;
        };
        Stats debug_stats() const {
            Stats s;
            s.unique_event_types = m_subscribers.size();
            s.queued_events = m_queue.size();
            for (const auto& [_, list] : m_subscribers) s.total_subscribers += list.size();
            return s;
        }

    private:
        friend class ScopedSubscriptions;

        using ErasedHandler = std::function<void(const void*)>;

        struct Subscription {
            SubscriptionID id;
            ErasedHandler handler;
        };

        std::unordered_map<std::type_index, std::vector<Subscription>> m_subscribers;
        std::unordered_map<SubscriptionID, std::type_index> m_idToType;
        std::queue<std::function<void()>> m_queue;
        SubscriptionID m_nextSubscriptionID = 1;  // 0 = INVALID

        // Список живых ScopedSubscriptions для инвалидации при ~EventBus
        std::unordered_set<ScopedSubscriptions*> m_scoped;

        void invalidate_all_scoped();

#ifndef NDEBUG
        std::vector<std::type_index> m_emitStack;
        bool m_inEmitOfSameType(const std::type_info& t) const {
            const auto idx = std::type_index(t);
            return std::find(m_emitStack.begin(), m_emitStack.end(), idx) != m_emitStack.end();
        }
#endif
    };


    // ============================================================
    // ScopedSubscriptions — RAII обёртка над набором подписок.
    //
    // Гарантии:
    //   - При уничтожении отписывает всё, что подписала
    //   - Если EventBus уничтожен раньше — безопасно (m_bus = nullptr)
    //   - Move-safe: перенос владения
    // ============================================================
    class ScopedSubscriptions {
    public:
        explicit ScopedSubscriptions(EventBus& bus) : m_bus(&bus) {
            bus.m_scoped.insert(this);
        }

        ~ScopedSubscriptions() {
            clear();
            if (m_bus) m_bus->m_scoped.erase(this);
        }

        ScopedSubscriptions(const ScopedSubscriptions&) = delete;
        ScopedSubscriptions& operator=(const ScopedSubscriptions&) = delete;

        ScopedSubscriptions(ScopedSubscriptions&& other) noexcept
            : m_bus(other.m_bus), m_ids(std::move(other.m_ids)) {
            if (m_bus) {
                m_bus->m_scoped.erase(&other);
                m_bus->m_scoped.insert(this);
            }
            other.m_bus = nullptr;
            other.m_ids.clear();
        }

        ScopedSubscriptions& operator=(ScopedSubscriptions&& other) noexcept {
            if (this != &other) {
                clear();
                if (m_bus) m_bus->m_scoped.erase(this);

                m_bus = other.m_bus;
                m_ids = std::move(other.m_ids);

                if (m_bus) {
                    m_bus->m_scoped.erase(&other);
                    m_bus->m_scoped.insert(this);
                }
                other.m_bus = nullptr;
                other.m_ids.clear();
            }
            return *this;
        }

        template<typename T>
        void subscribe(EventBus::EventCallback<T> callback) {
            if (!m_bus) return;
            m_ids.push_back(m_bus->subscribe<T>(std::move(callback)));
        }

        void clear() {
            if (!m_bus) {
                m_ids.clear();
                return;
            }
            for (auto id : m_ids) m_bus->unsubscribe(id);
            m_ids.clear();
        }

    private:
        friend class EventBus;
        EventBus* m_bus = nullptr;
        std::vector<SubscriptionID> m_ids;
    };

    inline EventBus::~EventBus() {
        invalidate_all_scoped();
    }

    inline void EventBus::invalidate_all_scoped() {
        for (auto* scoped : m_scoped) {
            scoped->m_bus = nullptr;
            scoped->m_ids.clear();
        }
        m_scoped.clear();
    }

} // namespace Solace