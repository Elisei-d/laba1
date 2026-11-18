#include "VersionHistory.h"

#include <utility>

#include "TreeAlgorithms.h"
#include "TreeRender.h"

namespace vcs {

VersionId VersionHistory::allocateId() noexcept { return VersionId(nextNumber_++); }

tree::NodeId VersionHistory::nodeOf(VersionId id) const {
    // Правило поиска передаётся обобщённому алгоритму параметром (лямбда).
    auto node = tree::findFirst(tree_, [id](const Version& v) { return v.id() == id; });
    if (!node) {
        throw VersionNotFoundError("версия " + id.toString() + " не найдена");
    }
    return *node;
}

std::vector<VersionId> VersionHistory::toVersionIds(const std::vector<tree::NodeId>& nodes) const {
    std::vector<VersionId> result;
    result.reserve(nodes.size());
    for (tree::NodeId node : nodes) result.push_back(tree_.value(node).id());
    return result;
}

VersionId VersionHistory::createInitial(std::string content, std::string message) {
    if (!tree_.empty()) {
        throw InitialVersionExistsError("начальная версия уже существует");
    }
    const VersionId id = allocateId();
    tree_.createRoot(Version(id, std::move(content), std::move(message)));
    current_ = id;
    return id;
}

VersionId VersionHistory::commit(std::string content, std::string message) {
    return commitFrom(current(), std::move(content), std::move(message));
}

VersionId VersionHistory::commitFrom(VersionId base, std::string content, std::string message) {
    const tree::NodeId parent = nodeOf(base);  // бросит, если base нет
    const VersionId id = allocateId();
    tree_.addChild(parent, Version(id, std::move(content), std::move(message)));
    current_ = id;
    return id;
}

void VersionHistory::checkout(VersionId id) {
    nodeOf(id);
    current_ = id;
}

VersionId VersionHistory::current() const {
    if (!current_) throw EmptyHistoryError("история пуста: нет текущей версии");
    return *current_;
}

VersionId VersionHistory::initial() const {
    if (tree_.empty()) throw EmptyHistoryError("история пуста: нет начальной версии");
    return tree_.value(tree_.root()).id();
}

bool VersionHistory::empty() const noexcept { return tree_.empty(); }

std::size_t VersionHistory::size() const noexcept { return tree_.size(); }

bool VersionHistory::contains(VersionId id) const {
    return tree::findFirst(tree_, [id](const Version& v) { return v.id() == id; }).has_value();
}

const Version& VersionHistory::find(VersionId id) const { return tree_.value(nodeOf(id)); }

std::optional<VersionId> VersionHistory::parentOf(VersionId id) const {
    auto parent = tree_.parent(nodeOf(id));
    if (!parent) return std::nullopt;
    return tree_.value(*parent).id();
}

std::vector<VersionId> VersionHistory::children(VersionId id) const {
    return toVersionIds(tree_.children(nodeOf(id)));
}

std::vector<VersionId> VersionHistory::descendants(VersionId id) const {
    return toVersionIds(tree::descendants(tree_, nodeOf(id)));
}

std::vector<VersionId> VersionHistory::pathTo(VersionId id) const {
    return toVersionIds(tree::pathFromRoot(tree_, nodeOf(id)));
}

VersionId VersionHistory::commonAncestor(VersionId a, VersionId b) const {
    const tree::NodeId lca = tree::lowestCommonAncestor(tree_, nodeOf(a), nodeOf(b));
    return tree_.value(lca).id();
}

void VersionHistory::remove(VersionId id) {
    const tree::NodeId node = nodeOf(id);
    if (!tree_.isLeaf(node)) {
        throw VersionHasDerivedError("версию " + id.toString() +
                                     " нельзя удалить: от неё происходят другие версии");
    }
    const std::optional<tree::NodeId> parent = tree_.parent(node);
    tree_.removeLeaf(node);
    if (current_ && *current_ == id) {
        current_ = parent ? std::optional<VersionId>(tree_.value(*parent).id()) : std::nullopt;
    }
}

std::string VersionHistory::toTreeString() const {
    return tree::renderTree(tree_, [this](tree::NodeId, const Version& v) {
        std::string line = v.id().toString() + " \"" + v.message() + "\"";
        if (current_ && *current_ == v.id()) line += "  *";
        return line;
    });
}

bool VersionHistory::checkInvariants() const {
    if (!tree_.checkInvariants()) return false;
    if (tree_.empty()) return !current_.has_value();
    if (!current_ || !contains(*current_)) return false;  // текущая версия существует
    // id версий уникальны и меньше следующего выдаваемого номера
    auto all = tree::findAll(tree_, [](const Version&) { return true; });
    std::vector<bool> seen(nextNumber_, false);
    for (tree::NodeId node : all) {
        const std::uint32_t n = tree_.value(node).id().number();
        if (n == 0 || n >= nextNumber_ || seen[n]) return false;
        seen[n] = true;
    }
    return true;
}

}  // namespace vcs
