#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

namespace memfs {

// Путь в файловой системе — иммутабельный ОБЪЕКТ-ЗНАЧЕНИЕ.
//
// Путь — это последовательность имён от корня ФС. Пустая последовательность
// означает корневой каталог. Путь всегда «абсолютный» относительно корня
// и всегда нормализован: пустых сегментов, "." и ".." в нём нет.
// Поэтому два пути равны тогда и только тогда, когда равны их сегменты.
//
// Иммутабельность:
//   - поле segments_ объявлено const, поэтому присваивание Path = Path
//     невозможно (оно неявно удалено), а изменить объект после создания нельзя;
//   - все методы константные;
//   - operator/ не меняет операнды, а возвращает НОВЫЙ путь.
// Копирование разрешено: копия значения — это то же значение.
class Path {
public:
    // Корневой путь "/".
    Path() = default;

    // Разбор строки вида "/home/docs", "home/docs" или "home//docs/".
    // Пустые сегменты пропускаются, остальные проверяются validateName.
    // explicit: строка не должна молча превращаться в путь — поэтому
    // для Path / std::string есть отдельная перегрузка оператора.
    explicit Path(const std::string& text);

    bool isRoot() const noexcept;
    std::size_t depth() const noexcept;
    const std::vector<std::string>& segments() const noexcept;

    // Имя последнего элемента пути. Для корня — ForbiddenOperationError.
    const std::string& name() const;

    // Путь к родительскому каталогу. Для корня — ForbiddenOperationError.
    Path parent() const;

    // true, если *this совпадает с other или является его предком.
    // Используется, чтобы запретить перемещение каталога внутрь себя/потомка.
    bool isAncestorOrSelfOf(const Path& other) const noexcept;

    // Текстовое представление: "/" для корня, "/home/docs" и т.д.
    std::string toString() const;

    // Конкатенация путей. Возвращает новый объект, операнды не меняются.
    friend Path operator/(const Path& lhs, const Path& rhs);
    // Перегрузка для строки (специальный, ad-hoc полиморфизм):
    // строка разбирается как относительный путь, т.е. "a/b" тоже допустимо.
    friend Path operator/(const Path& lhs, const std::string& rhs);

    friend bool operator==(const Path& lhs, const Path& rhs) noexcept;
    friend bool operator!=(const Path& lhs, const Path& rhs) noexcept;

private:
    explicit Path(std::vector<std::string> segments);

    const std::vector<std::string> segments_;
};

std::ostream& operator<<(std::ostream& out, const Path& path);

}  // namespace memfs
