#pragma once

#include <iostream>
#include <string_view>

namespace Solace {
    class Logger {
    public:
        // Информационное сообщение (Зеленый цвет)
        template<typename... Args>
        static void info(Args&&... args) {
            std::cout << "\033[32m[INFO]\033[0m ";
            print(std::forward<Args>(args)...);
        }

        // Предупреждение (Желтый цвет)
        template<typename... Args>
        static void warning(Args&&... args) {
            std::cout << "\033[33m[WARNING]\033[0m ";
            print(std::forward<Args>(args)...);
        }

        // Критическая ошибка (Красный цвет)
        template<typename... Args>
        static void error(Args&&... args) {
            std::cerr << "\033[31m[ERROR]\033[0m ";
            print(std::forward<Args>(args)...);
        }

        // Отладочное сообщение (Голубой/Циан цвет)
        template<typename... Args>
        static void debug(Args&&... args) {
            std::cout << "\033[36m[DEBUG]\033[0m ";
            print(std::forward<Args>(args)...);
        }

    private:
        // Вспомогательная функция для склеивания любых аргументов в одну строчку (фишка C++17/C++20)
        template<typename... Args>
        static void print(Args&&... args) {
            (std::cout << ... << args);
            std::cout << "\n";
        }
    };
}