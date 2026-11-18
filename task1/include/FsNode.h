#pragma once

#include <cstddef>
#include <ostream>
#include <string>

namespace memfs {

class Directory;

// Абстрактный элемент файловой системы (паттерн «Компоновщик», Composite).
//
// Это ОБЪЕКТ С ИДЕНТИЧНОСТЬЮ, а не значение: два разных файла с одинаковым
// именем и содержимым — всё равно два разных файла. Поэтому копирование
// запрещено — иначе легко случайно продублировать поддерево.
//
// Общий контракт любого элемента:
//   - name()     — имя;
//   - size()     — занимаемый размер (у каждого типа своё правило);
//   - toString() — текстовое представление.
class FsNode {
public:
    explicit FsNode(std::string name);
    virtual ~FsNode() = default;

    FsNode(const FsNode&) = delete;
    FsNode& operator=(const FsNode&) = delete;

    const std::string& name() const noexcept;

    virtual std::size_t size() const = 0;
    virtual std::string toString() const = 0;

    // Дерево элемента в текстовом виде. Невиртуальный метод (идиома NVI):
    // формат «рамки» дерева задан здесь один раз, а наследники только
    // дописывают свою часть через appendSubtree.
    std::string tree() const;

    // Дописывает в out строки потомков элемента с отступом prefix.
    // У «листа» (файла) потомков нет, поэтому по умолчанию ничего не делает.
    // Каталог переопределяет метод и рекурсивно выводит своё содержимое.
    virtual void appendSubtree(std::string& out, const std::string& prefix) const;

    // Запрос возможности «содержать элементы». Возвращает nullptr для элементов,
    // в которые нельзя «зайти» по пути. Каталог возвращает себя.
    // Это не проверка конкретного типа (if (type == FILE) ...): FileSystem не
    // знает, какие ещё бывают элементы, и новый тип элемента (ссылка, архив…)
    // добавляется без изменения FileSystem — достаточно унаследоваться от FsNode.
    virtual Directory* asDirectory() noexcept;
    virtual const Directory* asDirectory() const noexcept;

private:
    std::string name_;
};

std::ostream& operator<<(std::ostream& out, const FsNode& node);

}  // namespace memfs
