#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "Directory.h"
#include "File.h"
#include "Path.h"

namespace memfs {

// Файловая система в памяти: владеет корневым каталогом и выполняет
// операции над элементами по пути.
//
// Все операции (поиск, создание, удаление, перемещение, размер) используют
// ОДИН механизм разрешения пути — resolve()/resolveDirectory().
// Ни одна операция не содержит собственного алгоритма поиска.
//
// Правила:
//   - родительский каталог пути должен существовать;
//   - имя в целевом каталоге должно быть свободно;
//   - корень нельзя удалить и нельзя переместить;
//   - каталог нельзя переместить в себя или в собственного потомка.
// При нарушении правила операция бросает исключение и НЕ меняет состояние ФС.
class FileSystem {
public:
    // Имя корня используется только при выводе дерева; в путях корень — "/".
    explicit FileSystem(std::string rootName = "root");

    // Поиск элемента по пути на любой глубине.
    // PathNotFoundError / NotADirectoryError, если путь не разрешается.
    const FsNode& find(const Path& path) const;
    bool exists(const Path& path) const;

    // Создание. Возвращают ссылку на созданный элемент; ссылка действительна,
    // пока элемент не удалён из ФС.
    File& createFile(const Path& path, std::string content = {});
    Directory& createDirectory(const Path& path);

    // Добавление элемента ПРОИЗВОЛЬНОГО типа (наследника FsNode) в каталог.
    // Благодаря этому методу новый тип элемента (например, символьная ссылка)
    // добавляется без изменения кода FileSystem (принцип открытости/закрытости).
    FsNode& add(const Path& parentDirectory, std::unique_ptr<FsNode> node);

    void remove(const Path& path);

    // Перемещает элемент source в каталог targetDirectory, сохраняя имя.
    void move(const Path& source, const Path& targetDirectory);

    std::size_t sizeOf(const Path& path) const;

    std::string tree() const;

private:
    // Единый механизм разрешения пути.
    const FsNode& resolve(const Path& path) const;
    FsNode& resolve(const Path& path);
    Directory& resolveDirectory(const Path& path);

    // ФС единолично владеет корнем. unique_ptr (а не поле Directory) —
    // чтобы ФС можно было перемещать (std::move) без инвалидации ссылок
    // на элементы: сами узлы при перемещении ФС остаются на месте.
    // Копирование ФС запрещено автоматически (unique_ptr не копируется).
    std::unique_ptr<Directory> root_;
};

}  // namespace memfs
