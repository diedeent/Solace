# Solace Engine & Game Framework

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![SDL3](https://img.shields.io/badge/SDL-3.x-1E5A8A.svg)](https://wiki.libsdl.org/SDL3)
[![OpenGL](https://img.shields.io/badge/OpenGL-4.3%20Core-5586A4.svg)](https://www.khronos.org/opengl/)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)]()
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](./LICENSE)

[Архитектура](#архитектурная-философия) •
[Технологии](#технологический-стек) •
[Документация](#навигационный-каталог-документации) •
[Дорожная карта](#фаза-0-фундамент-архитектуры-phase-0--foundation) •
[Сборка](#инструкции-по-сборке) •
[Git Workflow](#регламент-разработки)

## Описание проекта

**Solace** — это кастомный движок и кроссплатформенный фреймворк, разрабатываемый на языке C++20 с использованием графического API OpenGL 4.3 Core. Архитектура системы сфокусирована на обеспечении высокой производительности, строгом соблюдении слоёв ответственности, полном отказе от глобальных состояний (Dependency Injection) и отложенной мутации ECS-компонентов.

Целью проекта является создание высокопроизводительного, масштабируемого архитектурного фундамента для инди-разработки с отзывчивым управлением и глубокой боевой системой.

> \[!NOTE\]
> Движок использует собственную архитектуру без использования тяжеловесных сторонних ECS-фреймворков. Все компоненты представляют собой плоские POD-структуры данных, а логика полностью изолирована внутри систем слоя 4.

## Навигационный каталог документации

Для обеспечения высокого качества кодовой базы в репозитории поддерживается распределённый комплект спецификаций и регламентов:

| Документ | Содержимое и назначение |
| :--- | :--- |
| [`ENGINEERING_HANDBOOK.md`](./docs/ENGINEERING_HANDBOOK.md) | Архитектурные правила (Hard Rules), иерархия слоев, DoD, реестр антипаттернов и диагностика. |
| [`DEVELOPER_GUIDE.md`](./docs/DEVELOPER_GUIDE.md) | Практические стандарты C++20, RAII, управление памятью и регламент релизов. |
| [`GIT_GUIDE.md`](./docs/GIT_GUIDE.md) | Правила Git Flow, Conventional Commits, шаблоны PR, конфигурация CI/CD и Git LFS. |

## Архитектурная философия

Приоритеты проектирования кодовой базы распределены следующим образом:
1. **Работоспособность** — гарантированная корректность сборки и исполнения.
2. **Читаемость** — прозрачность архитектурных решений, обеспечивающая быстрое погружение новых участников.
3. **Элегантность** — следование принципам SOLID, стандартам RAII и паттернам проектирования.

> \[!IMPORTANT\]
> **Hard Rule:** В кодовой базе действует строгое правило направленности зависимостей — **только сверху вниз**. Модуль слоя $N$ может включать заголовки только слоёв $0 \dots N-1$. Нарушение правила приводит к авто-отклонению Pull Request на этапе проверки CMake-целей.

### Иерархия слоёв (Layering Standard)

Система разделена на 7 независимых уровней (от 0 до 6), где каждый слой может зависеть только от нижележащих:

```
graph TD
    subgraph Layer_6 [Layer 6: Game Specific]
        G6[Game, GameplayScene, PlayerController]
    end

    subgraph Layer_5 [Layer 5: Orchestration]
        O5[Application, SceneManager, UILayer]
    end

    subgraph Layer_4 [Layer 4: Systems]
        S4[MovementSystem, DamageSystem, HitboxSystem, RenderSystem]
    end

    subgraph Layer_3 [Layer 3: Data Structures]
        D3[EntityRegistry, ComponentPools, AssetCache, Scenes]
    end

    subgraph Layer_2 [Layer 2: Platform Wrappers]
        P2[Window, ShaderWrapper, MeshWrapper, TextureWrapper]
    end

    subgraph Layer_1 [Layer 1: Core Utilities]
        C1[EventBus, GameplayClock, ConfigCache, Logger, Math]
    end

    subgraph Layer_0 [Layer 0: Platform / Third-Party]
        P0[SDL3, OpenGL 4.3, Assimp, ozz-animation, miniaudio]
    end

    Layer_6 --> Layer_5
    Layer_5 --> Layer_4
    Layer_4 --> Layer_3
    Layer_3 --> Layer_2
    Layer_2 --> Layer_1
    Layer_1 --> Layer_0
```

---

### Поток данных при обработке событий (Event-Driven Dataflow)

Системы одного слоя не знают о существовании друг друга. Связь между ними осуществляется изолированно через асинхронную или синхронную шину `EventBus`:

```
sequenceDiagram
    autonumber
    participant CombatSystem as CombatSystem (Layer 4)
    participant EventBus as EventBus (Layer 1)
    participant HealthSystem as HealthSystem (Layer 4)
    participant AudioSystem as AudioSystem (Layer 4)
    participant VFXSystem as VFXSystem (Layer 4)

    CombatSystem->>EventBus: emit(DamageEvent)
    par Изменение состояния
        EventBus->>HealthSystem: notify(DamageEvent)
        HealthSystem->>HealthSystem: Применение урона, расчет HP
    and Воспроизведение звука
        EventBus->>AudioSystem: notify(DamageEvent)
        AudioSystem->>AudioSystem: Воспроизведение 3D-звука удара
    and Визуализация
        EventBus->>VFXSystem: notify(DamageEvent)
        VFXSystem->>VFXSystem: Генерация частиц (Sparks)
    end
```

## Технологический стек

| Категория | Подсистема | Технология / Спецификация | 
| ----- | ----- | ----- | 
| **Язык** | Ядро и логика | **C++20** (ISO/IEC 14882:2020) | 
| **Графика** | Рендеринг | **OpenGL 4.3 Core Profile** (SSBO, Instancing) | 
| **Окно и ввод** | Platform Layer | **SDL3** (Simple DirectMedia Layer 3) | 
| **Анимация** | Скелетные анимации | **ozz-animation** (Crossfade, Blending, Skinning) | 
| **3D Модели** | Импорт ресурсов | **Assimp** (Формат `.glb` / `.gltf`) | 
| **Аудио** | Позиционируемый звук | **miniaudio** / **SoundPool** | 
| **Сборка** | Мета-система | **CMake 3.25+**, **Ninja Build** | 
| **Пакеты** | Зависимости | **vcpkg** (Manifest mode) | 
| **CI / CD** | Проверки | **GitHub Actions** (Clang-Format, GCC, MSVC) | 

---

## Дорожная карта и вехи

Разработка разбита на последовательные фазы. Актуальный статус выполнения фиксируется в чек-листах ниже:

<details>
<summary><b>Фаза 0: Фундамент архитектуры (Phase 0 — Foundation)</b> [Нажмите для просмотра]</summary>

- [ ] Реализация типа-независимой шины событий (`EventBus`) без статических глобальных состояний.
- [ ] Разделение системного и игрового времени (`Clock Separation` для корректной работы паузы и HitStop).
- [ ] Создание реестра сущностей (`EntityRegistry`) с плоскими массивами и отложенным удалением (`Deferred Destroy`).
- [ ] Менеджер конфигураций (`ConfigCache`) с типизированными структурами и поддержкой горячей перезагрузки (`F5`).
- [ ] Управление жизненным циклом сцен (`Scene Lifecycle`).
- [ ] Интеграция отладочного графического интерфейса (`Debug Overlay / ImGui` по клавише `F1`).
- [ ] Определение базовых POD-структур для компонентов данных.

</details>

<details>
<summary><b>Фаза 1: Боевой вертикальный срез (Phase 1 — Combat Vertical Slice)</b> [Нажмите для просмотра]</summary>

- [ ] Машина состояний анимаций (`AnimationStateMachine`) с поддержкой кроссфейдов.
- [ ] Подсистема анимационных событий (`AnimationEvents`) для триггеров звуков и частиц.
- [ ] Пространственная физика столкновений (`Capsule Sweep Collision` и сетка разбиения).
- [ ] Централизованная система расчета урона (`DamageSystem`).
- [ ] Привязка хитбоксов к костям скелета (`Bone-attached Hitboxes`).
- [ ] Реализация эффектов остановки времени (`HitStop`) и процедурной тряски камеры.
- [ ] Позиционируемая аудиосистема.
- [ ] Инстансированная система частиц для визуальных эффектов.

</details>

<details>
<summary><b>Фазы 2–4: Искусственный интеллект, Мир и Полировка</b> [Нажмите для просмотра]</summary>

- [ ] Разработка ИИ противников (FSM, алгоритм поиска пути A*).
- [ ] Процедурная генерация террейна и стриминг чанков.
- [ ] Оптимизация производительности (LOD, Frustum Culling).
- [ ] Подсистема сохранения и загрузки игры.

</details>

---

## Инструкции по сборке

> \[!TIP\]
> Для сборки рекомендуется использовать генератор **Ninja** и пресеты **CMakePresets.json**. Это гарантирует идентичность параметров конфигурации на всех платформах.

### Предварительные требования

1. **CMake** $\ge$ 3.25
2. **C++20 компилятор:** MSVC (Visual Studio 2022 v17.0+), GCC 12+ или Clang 15+
3. **vcpkg:** установлена переменная окружения `VCPKG_ROOT`

### Порядок сборки через пресеты

```bash
# 1. Клонирование репозитория вместе с подмодулями
git clone https://github.com/diedeent/solace.git
cd solace

# 2. Конфигурация и сборка под Windows (MSVC)
cmake --preset windows-vs-debug
cmake --build --preset windows-vs-debug

# 3. Конфигурация и сборка под Linux (GCC / Clang)
cmake --preset linux-debug
cmake --build --preset linux-debug
```

Бинарные файлы генерируются в директории `out/build/<preset-name>/bin/`. Запуск осуществляется из корневого каталога репозитория для правильного разрешения путей к ресурсам `assets/`.

## Регламент разработки

Прямые коммиты в ветки `main` и `dev` запрещены. В проекте применяется строгий регламент **Git Flow**:

1. Создайте рабочую ветку от актуального состояния `dev`:

   ```bash
   git checkout dev
   git pull origin dev
   git checkout -b feature/event-bus-filtering
   ```

2. Оформляйте коммиты строго по стандарту **Conventional Commits**:
   `feat(event_bus): add type-erased handler storage`

3. Убедитесь в отсутствии предупреждений компилятора (`-Wall -Wextra -Werror`).

4. Откройте Pull Request в ветку `dev`. Проверьте выполнение условий чек-листа в [`GIT_GUIDE.md`](./docs/GIT_GUIDE.md).

## Лицензирование

Программное обеспечение распространяется под лицензией **MIT**. Полный текст доступен в файле [`LICENSE`](./LICENSE).