#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "FsNode.h"

namespace memfs {

// Каталог: элемент ФС, содержащий другие элементы (в том числе каталоги).
//
// Модель владения: каталог ЕДИНОЛИЧНО владеет своими потомками
// (std::unique_ptr). Удаление каталога автоматически удаляет всё поддерево.
// Элемент не может одновременно лежать в двух каталогах — unique_ptr нельзя
// скопировать, а перенос между каталогами — это явная передача владения
// (extract + add).
//
// Инвариант: имена непосредственных потомков уникальны.
class Directory : public FsNode {
public:
    explicit Directory(std::string name);

    // Добавить элемент. Каталог становится его владельцем.
    // nullptr -> std::invalid_argument, повтор имени -> AlreadyExistsError.
    FsNode& add(std::unique_ptr<FsNode> node);

    // Удалить непосредственного потомка. false, если такого нет.
    bool remove(const std::string& name);

    // Изъять непосредственного потомка, передав владение вызывающему.
    // nullptr, если потомка с таким именем нет.
    std::unique_ptr<FsNode> extract(const std::string& name);

    // Найти непосредственного потомка по имени. nullptr, если нет.
    FsNode* find(const std::string& name) noexcept;
    const FsNode* find(const std::string& name) const noexcept;

    bool contains(const std::string& name) const noexcept;
    std::size_t childCount() const noexcept;
    std::vector<std::string> childNames() const;   // в порядке добавления

    // Суммарный размер всех файлов поддерева (рекурсивно, через полиморфный size()).
    std::size_t size() const override;
    std::string toString() const override;         // "docs/ (40 B)"

    void appendSubtree(std::string& out, const std::string& prefix) const override;

    Directory* asDirectory() noexcept override;
    const Directory* asDirectory() const noexcept override;

private:
    using Children = std::vector<std::unique_ptr<FsNode>>;

    Children::iterator position(const std::string& name) noexcept;
    Children::const_iterator position(const std::string& name) const noexcept;

    Children children_;
};

}  // namespace memfs
