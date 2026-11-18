// Лабораторная работа 1, задание 2, вариант 3 «История версий документа».
// Сначала запускаются автотесты, затем — демонстрация.
// Демонстрационный код пользуется только открытым контрактом VersionHistory.

#include <iostream>
#include <string>
#include <vector>

#include "HistoryTests.h"
#include "VersionHistory.h"

#ifdef _WIN32
#include <windows.h>
#endif

using vcs::VersionHistory;
using vcs::VersionId;

namespace {

std::string join(const std::vector<VersionId>& ids, const std::string& separator) {
    if (ids.empty()) return "(нет)";
    std::string result;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (i > 0) result += separator;
        result += ids[i].toString();
    }
    return result;
}

void demo() {
    std::cout << "\n================ ДЕМОНСТРАЦИЯ ================\n";
    VersionHistory history;

    // Основная ветка
    const VersionId v1 = history.createInitial("", "Пустой документ");
    const VersionId v2 = history.commit("Введение", "Добавлено введение");
    const VersionId v3 = history.commit("Введение\nГлава 1", "Глава 1");
    const VersionId v4 = history.commit("Введение\nГлава 1\nЗаключение", "Заключение");

    // Возврат к v2 и альтернативная ветка
    history.checkout(v2);
    const VersionId v5 = history.commit("Введение (ред.)", "Переписано введение");
    const VersionId v6 = history.commit("Введение (ред.)\nГлава 1 (новая)", "Новая глава 1");

    // Ещё одна ветка от v3 — третий уровень ветвления
    history.checkout(v3);
    const VersionId v7 = history.commit("Введение\nГлава 1\nПриложение", "Приложение");
    // И ветка от самой начальной версии
    const VersionId v8 = history.commitFrom(v1, "Черновик", "Черновик с нуля");

    std::cout << "\nВся история (* — текущая версия):\n" << history.toTreeString();

    std::string content = history.find(v6).content();
    for (std::size_t pos = 0; (pos = content.find('\n', pos)) != std::string::npos; pos += 3) {
        content.replace(pos, 1, "\n  ");
    }
    std::cout << "\nПоиск версии " << v6 << ": \"" << history.find(v6).message()
              << "\", содержимое:\n  " << content << "\n";

    std::cout << "\nНепосредственные потомки:\n";
    for (VersionId id : {v1, v2, v3, v4}) {
        std::cout << "  " << id << " -> " << join(history.children(id), ", ") << "\n";
    }

    std::cout << "\nВсе потомки:\n";
    for (VersionId id : {v1, v2, v5, v8}) {
        std::cout << "  " << id << " -> " << join(history.descendants(id), ", ") << "\n";
    }

    std::cout << "\nПуть от начальной версии:\n";
    for (VersionId id : {v4, v6, v7, v8, v1}) {
        std::cout << "  до " << id << ": " << join(history.pathTo(id), " -> ") << "\n";
    }

    std::cout << "\nБлижайший общий предок:\n";
    const std::vector<std::pair<VersionId, VersionId>> pairs{
        {v4, v7}, {v4, v6}, {v6, v8}, {v3, v4}, {v2, v6}, {v5, v5}};
    for (const auto& [a, b] : pairs) {
        std::cout << "  " << a << " и " << b << " -> " << history.commonAncestor(a, b) << "\n";
    }

    std::cout << "\nУдаление (правило: только версии без производных):\n";
    for (VersionId id : {v2, v8}) {
        try {
            history.remove(id);
            std::cout << "  " << id << " удалена\n";
        } catch (const vcs::HistoryError& e) {
            std::cout << "  ОТКАЗ: " << e.what() << "\n";
        }
    }
    std::cout << "\nИстория после удаления:\n" << history.toTreeString();
}

}  // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    TestRunner runner;
    runHistoryTests(runner);
    const int code = runner.summary();
    demo();
    return code;
}
