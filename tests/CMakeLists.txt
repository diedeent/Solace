include(FetchContent)

# ---- Catch2 ----
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.0   # зафиксируй версию, как и советовало ревью для ozz/glad
)
FetchContent_MakeAvailable(Catch2)

# Модуль для автоматической регистрации тестов в CTest
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

# ---- Исполняемый файл тестов ----
add_executable(SolaceTests
    #test_main.cpp
    test_entity_registry.cpp
    test_event_bus.cpp
    #test_animation_state_machine.cpp
)

target_link_libraries(SolaceTests PRIVATE
    engine            # линкуемся с движком
    Catch2::Catch2WithMain
)

# Автоматически регистрирует все TEST_CASE из SolaceTests в CTest
catch_discover_tests(SolaceTests)