#include "Directory.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "Errors.h"

namespace memfs {

Directory::Directory(std::string name) : FsNode(std::move(name)) {}

Directory::Children::iterator Directory::position(const std::string& name) noexcept {
    return std::find_if(children_.begin(), children_.end(),
                        [&name](const auto& child) { return child->name() == name; });
}

Directory::Children::const_iterator Directory::position(const std::string& name) const noexcept {
    return std::find_if(children_.begin(), children_.end(),
                        [&name](const auto& child) { return child->name() == name; });
}

FsNode& Directory::add(std::unique_ptr<FsNode> node) {
    if (!node) {
        throw std::invalid_argument("нельзя добавить пустой элемент");
    }
    if (contains(node->name())) {
        throw AlreadyExistsError("в каталоге '" + name() + "' уже есть элемент '" +
                                 node->name() + "'");
    }
    children_.push_back(std::move(node));
    return *children_.back();
}

bool Directory::remove(const std::string& name) { return extract(name) != nullptr; }

std::unique_ptr<FsNode> Directory::extract(const std::string& name) {
    auto it = position(name);
    if (it == children_.end()) {
        return nullptr;
    }
    std::unique_ptr<FsNode> node = std::move(*it);
    children_.erase(it);
    return node;
}

FsNode* Directory::find(const std::string& name) noexcept {
    auto it = position(name);
    return it == children_.end() ? nullptr : it->get();
}

const FsNode* Directory::find(const std::string& name) const noexcept {
    auto it = position(name);
    return it == children_.end() ? nullptr : it->get();
}

bool Directory::contains(const std::string& name) const noexcept {
    return position(name) != children_.end();
}

std::size_t Directory::childCount() const noexcept { return children_.size(); }

std::vector<std::string> Directory::childNames() const {
    std::vector<std::string> names;
    names.reserve(children_.size());
    for (const auto& child : children_) {
        names.push_back(child->name());
    }
    return names;
}

std::size_t Directory::size() const {
    std::size_t total = 0;
    for (const auto& child : children_) {
        total += child->size();  // полиморфный вызов: файл, каталог или любой новый тип
    }
    return total;
}

std::string Directory::toString() const {
    return name() + "/ (" + std::to_string(size()) + " B)";
}

void Directory::appendSubtree(std::string& out, const std::string& prefix) const {
    for (std::size_t i = 0; i < children_.size(); ++i) {
        const bool last = (i + 1 == children_.size());
        out += prefix + (last ? "`-- " : "|-- ") + children_[i]->toString() + "\n";
        children_[i]->appendSubtree(out, prefix + (last ? "    " : "|   "));
    }
}

Directory* Directory::asDirectory() noexcept { return this; }

const Directory* Directory::asDirectory() const noexcept { return this; }

}  
