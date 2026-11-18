#pragma once

// ============================================================================
//  ПРЕДМЕТНАЯ ОБЛАСТЬ: версия документа.
// ============================================================================

#include <cstdint>
#include <utility>
#include <ostream>
#include <string>

namespace vcs {

// Идентификатор версии — иммутабельное значение ("v1", "v2", ...).
// Выдаётся историей; не путать с tree::NodeId — это разные уровни:
// VersionId — понятие предметной области, NodeId — деталь хранения.
class VersionId {
public:
    explicit VersionId(std::uint32_t number) noexcept : number_(number) {}

    std::uint32_t number() const noexcept { return number_; }
    std::string toString() const { return "v" + std::to_string(number_); }

    friend bool operator==(VersionId a, VersionId b) noexcept { return a.number_ == b.number_; }
    friend bool operator!=(VersionId a, VersionId b) noexcept { return !(a == b); }
    friend bool operator<(VersionId a, VersionId b) noexcept { return a.number_ < b.number_; }
    friend std::ostream& operator<<(std::ostream& out, VersionId id) {
        return out << id.toString();
    }

private:
    std::uint32_t number_;
};

// Версия — иммутабельный снимок документа: id, содержимое и описание.
// Изменить существующую версию нельзя — можно только создать новую.
class Version {
public:
    Version(VersionId id, std::string content, std::string message)
        : id_(id), content_(std::move(content)), message_(std::move(message)) {}

    VersionId id() const noexcept { return id_; }
    const std::string& content() const noexcept { return content_; }
    const std::string& message() const noexcept { return message_; }

private:
    VersionId id_;
    std::string content_;
    std::string message_;
};

}  // namespace vcs
