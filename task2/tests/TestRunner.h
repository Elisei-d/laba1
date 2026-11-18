#pragma once

#include <exception>
#include <iostream>
#include <string>
#include <typeinfo>
#include <utility>

// Минимальный тестовый фреймворк без внешних зависимостей.
// check       — проверка условия;
// expectThrow — проверка, что код бросает исключение ОЖИДАЕМОГО типа.
class TestRunner {
public:
    void section(const std::string& name) { std::cout << "\n[" << name << "]\n"; }

    void check(bool condition, const std::string& description) {
        record(condition, description, "");
    }

    template <typename Exception, typename Action>
    void expectThrow(Action&& action, const std::string& description) {
        try {
            std::forward<Action>(action)();
        } catch (const Exception& e) {
            record(true, description, std::string("-> ") + e.what());
            return;
        } catch (const std::exception& e) {
            record(false, description, std::string("брошено другое исключение: ") + e.what());
            return;
        } catch (...) {
            record(false, description, "брошено неизвестное исключение");
            return;
        }
        record(false, description, "исключение не брошено");
    }

    template <typename Action>
    void expectNoThrow(Action&& action, const std::string& description) {
        try {
            std::forward<Action>(action)();
            record(true, description, "");
        } catch (const std::exception& e) {
            record(false, description, std::string("неожиданное исключение: ") + e.what());
        }
    }

    // Итог; возвращает код завершения программы (0 — всё прошло).
    int summary() const {
        std::cout << "\n==== Тесты: пройдено " << passed_ << " из " << (passed_ + failed_)
                  << (failed_ == 0 ? " — OK" : " — ЕСТЬ ОШИБКИ") << " ====\n";
        return failed_ == 0 ? 0 : 1;
    }

    bool allPassed() const { return failed_ == 0; }

private:
    void record(bool ok, const std::string& description, const std::string& details) {
        (ok ? passed_ : failed_)++;
        std::cout << (ok ? "  [OK]   " : "  [FAIL] ") << description;
        if (!details.empty()) {
            std::cout << "  " << details;
        }
        std::cout << '\n';
    }

    int passed_ = 0;
    int failed_ = 0;
};
