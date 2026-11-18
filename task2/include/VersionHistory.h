#pragma once

// ============================================================================
//  ПРЕДМЕТНАЯ ЛОГИКА: история версий документа.
//
//  Использует обобщённое дерево tree::Tree<Version> и обобщённые алгоритмы,
//  обращаясь к ним только через открытый контракт (NodeId), без доступа к
//  внутреннему устройству узлов.
//
//  «Текущая версия» — версия, на основе которой по умолчанию создаётся новая.
//  checkout() позволяет вернуться к любой версии и продолжить работу с неё —
//  так история ветвится.
//
//  ПРАВИЛО УДАЛЕНИЯ (единое): удалить можно только версию, у которой нет
//  производных версий. Правило выражено в модели: у дерева есть только
//  removeLeaf(), а нарушение даёт отдельный тип ошибки VersionHasDerivedError.
//  Если удаляется текущая версия, текущей становится её родитель.
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "Tree.h"
#include "Version.h"

namespace vcs {

class HistoryError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
class VersionNotFoundError : public HistoryError {
public:
    using HistoryError::HistoryError;
};
class EmptyHistoryError : public HistoryError {
public:
    using HistoryError::HistoryError;
};
class InitialVersionExistsError : public HistoryError {
public:
    using HistoryError::HistoryError;
};
class VersionHasDerivedError : public HistoryError {
public:
    using HistoryError::HistoryError;
};

class VersionHistory {
public:
    // Создать начальную версию (корень истории). Делает её текущей.
    VersionId createInitial(std::string content, std::string message);

    // Новая версия на основе ТЕКУЩЕЙ. Новая версия становится текущей.
    VersionId commit(std::string content, std::string message);

    // Новая версия на основе ЛЮБОЙ существующей. Новая становится текущей.
    VersionId commitFrom(VersionId base, std::string content, std::string message);

    // Вернуться к существующей версии, чтобы продолжить работу с неё.
    void checkout(VersionId id);

    VersionId current() const;                  // EmptyHistoryError, если пусто
    VersionId initial() const;                  // EmptyHistoryError, если пусто
    bool empty() const noexcept;
    std::size_t size() const noexcept;
    bool contains(VersionId id) const;

    const Version& find(VersionId id) const;    // VersionNotFoundError
    std::optional<VersionId> parentOf(VersionId id) const;
    std::vector<VersionId> children(VersionId id) const;     // непосредственные
    std::vector<VersionId> descendants(VersionId id) const;  // все, в pre-order
    std::vector<VersionId> pathTo(VersionId id) const;       // [initial, ..., id]
    VersionId commonAncestor(VersionId a, VersionId b) const;

    void remove(VersionId id);

    // Вся история в древовидном виде; текущая версия помечена '*'.
    std::string toTreeString() const;

    // Проверка инвариантов истории (для тестов).
    bool checkInvariants() const;

private:
    tree::NodeId nodeOf(VersionId id) const;
    std::vector<VersionId> toVersionIds(const std::vector<tree::NodeId>& nodes) const;
    VersionId allocateId() noexcept;

    tree::Tree<Version> tree_;
    std::optional<VersionId> current_;
    std::uint32_t nextNumber_ = 1;  // номера не переиспользуются после удаления
};

}  // namespace vcs
