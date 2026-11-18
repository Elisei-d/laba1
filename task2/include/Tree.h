#pragma once

// ============================================================================
//  СТРУКТУРА ХРАНЕНИЯ: обобщённое корневое дерево Tree<T>.
//
//  Ничего не знает о документах и версиях: T — произвольные данные узла.
//
//  ИНВАРИАНТЫ (корректное состояние дерева):
//   I1. Дерево либо пусто, либо имеет ровно один корень.
//   I2. У корня нет родителя; у любого другого узла ровно один родитель.
//   I3. Ни один узел не является своим предком (циклов нет).
//   I4. Идентификаторы узлов уникальны и не переиспользуются после удаления.
//   I5. Индекс id -> узел содержит ровно те узлы, что достижимы из корня.
//
//  КАК ИНВАРИАНТЫ ОБЕСПЕЧИВАЮТСЯ:
//   - узел можно создать только двумя способами: createRoot() (только для
//     пустого дерева -> I1) и addChild(существующий родитель) (-> I2);
//   - нет операций «сменить родителя» / «присоединить готовый узел», поэтому
//     новый узел всегда лист, и цикл образоваться не может (-> I3);
//   - id выдаёт само дерево из монотонного счётчика (-> I4);
//   - индекс обновляется в тех же методах, что меняют структуру (-> I5);
//   - удалять можно только лист (removeLeaf), поэтому удаление не «отрывает»
//     поддерево и не оставляет в индексе висячих указателей.
//   checkInvariants() проверяет I1–I5 явно (используется в тестах).
//
//  МОДЕЛЬ ВЛАДЕНИЯ:
//   - дерево владеет корнем через std::unique_ptr;
//   - каждый узел владеет своими детьми через std::unique_ptr;
//   - Node::parent — НЕвладеющий сырой указатель «вверх». Он обоснован:
//     родитель всегда живёт дольше ребёнка (ребёнок — часть родителя),
//     поэтому указатель не может стать висячим; shared_ptr/weak_ptr здесь
//     дали бы лишние накладные расходы без пользы;
//   - index_ хранит невладеющие указатели для поиска за O(1);
//   - наружу узлы НЕ выдаются: пользователь работает только с NodeId.
//     Устаревший id (удалённого узла) безопасно распознаётся -> исключение,
//     а не неопределённое поведение, как было бы с указателем.
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace tree {

template <typename T>
class Tree;

// Непрозрачный идентификатор узла — иммутабельное значение.
// Создать его может только дерево (конструктор закрыт), поэтому «придумать»
// id несуществующего узла снаружи нельзя.
class NodeId {
public:
    std::uint64_t value() const noexcept { return value_; }

    friend bool operator==(NodeId a, NodeId b) noexcept { return a.value_ == b.value_; }
    friend bool operator!=(NodeId a, NodeId b) noexcept { return !(a == b); }
    friend bool operator<(NodeId a, NodeId b) noexcept { return a.value_ < b.value_; }
    friend std::ostream& operator<<(std::ostream& out, NodeId id) {
        return out << "#" << id.value_;
    }

private:
    template <typename T>
    friend class Tree;
    explicit NodeId(std::uint64_t value) noexcept : value_(value) {}

    std::uint64_t value_;
};

// Ошибки структуры дерева.
class TreeError : public std::logic_error {
public:
    using std::logic_error::logic_error;
};
class UnknownNodeError : public TreeError {   // id не принадлежит дереву / узел удалён
public:
    using TreeError::TreeError;
};
class EmptyTreeError : public TreeError {     // операция требует непустого дерева
public:
    using TreeError::TreeError;
};
class RootExistsError : public TreeError {    // второй корень нарушил бы I1
public:
    using TreeError::TreeError;
};
class NodeHasChildrenError : public TreeError {  // удаляется не лист
public:
    using TreeError::TreeError;
};

}  // namespace tree

// Специализация шаблона std::hash для NodeId — нужна unordered_map.
namespace std {
template <>
struct hash<tree::NodeId> {
    std::size_t operator()(tree::NodeId id) const noexcept {
        return std::hash<std::uint64_t>{}(id.value());
    }
};
}  // namespace std

namespace tree {

template <typename T>
class Tree {
public:
    using value_type = T;

    Tree() = default;
    ~Tree() { clear(); }

    // Копирование запрещено (узлы — объекты с идентичностью, глубокая копия
    // породила бы вопрос «тот же id или новый?»). Перемещение разрешено.
    Tree(const Tree&) = delete;
    Tree& operator=(const Tree&) = delete;
    Tree(Tree&& other) noexcept { swap(other); }
    Tree& operator=(Tree&& other) noexcept {
        if (this != &other) {
            clear();
            swap(other);
        }
        return *this;
    }

    bool empty() const noexcept { return root_ == nullptr; }
    std::size_t size() const noexcept { return index_.size(); }
    bool contains(NodeId id) const { return index_.count(id) != 0; }

    NodeId root() const {
        if (empty()) throw EmptyTreeError("дерево пусто");
        return root_->id;
    }

    NodeId createRoot(T value) {
        if (!empty()) throw RootExistsError("у дерева уже есть корень");
        root_ = std::make_unique<Node>(allocateId(), std::move(value), nullptr);
        index_.emplace(root_->id, root_.get());
        return root_->id;
    }

    NodeId addChild(NodeId parentId, T value) {
        Node& parent = node(parentId);
        auto child = std::make_unique<Node>(allocateId(), std::move(value), &parent);
        Node* raw = child.get();
        index_.emplace(raw->id, raw);  // сначала индекс: если бросит — дерево не изменилось
        try {
            parent.children.push_back(std::move(child));
        } catch (...) {
            index_.erase(raw->id);
            throw;
        }
        return raw->id;
    }

    const T& value(NodeId id) const { return node(id).value; }

    std::optional<NodeId> parent(NodeId id) const {
        const Node& n = node(id);
        if (n.parent == nullptr) return std::nullopt;
        return n.parent->id;
    }

    std::vector<NodeId> children(NodeId id) const {
        const Node& n = node(id);
        std::vector<NodeId> result;
        result.reserve(n.children.size());
        for (const auto& child : n.children) result.push_back(child->id);
        return result;
    }

    bool isLeaf(NodeId id) const { return node(id).children.empty(); }

    // ЕДИНОЕ ПРАВИЛО УДАЛЕНИЯ: удалять можно только лист.
    // Узел с потомками -> NodeHasChildrenError, дерево не меняется.
    // Удаление единственного узла (корня-листа) делает дерево пустым.
    void removeLeaf(NodeId id) {
        Node& n = node(id);
        if (!n.children.empty()) {
            throw NodeHasChildrenError("узел " + std::to_string(id.value()) +
                                       " имеет потомков и не может быть удалён");
        }
        index_.erase(id);
        if (n.parent == nullptr) {
            root_.reset();
            return;
        }
        auto& siblings = n.parent->children;
        for (auto it = siblings.begin(); it != siblings.end(); ++it) {
            if (it->get() == &n) {
                siblings.erase(it);
                return;
            }
        }
    }

    // Явная проверка инвариантов I1–I5 (обход без рекурсии).
    bool checkInvariants() const {
        if (empty()) return index_.empty();
        if (root_->parent != nullptr) return false;                       // I2 для корня
        std::unordered_set<std::uint64_t> seen;
        std::vector<const Node*> stack{root_.get()};
        while (!stack.empty()) {
            const Node* n = stack.back();
            stack.pop_back();
            if (!seen.insert(n->id.value()).second) return false;         // I3/I4: повтор
            if (n->id.value() >= nextId_) return false;                   // I4
            auto it = index_.find(n->id);
            if (it == index_.end() || it->second != n) return false;      // I5
            for (const auto& child : n->children) {
                if (!child || child->parent != n) return false;           // I2
                stack.push_back(child.get());
            }
        }
        return seen.size() == index_.size();                              // I5
    }

private:
    struct Node {
        Node(NodeId nodeId, T nodeValue, Node* parentNode)
            : id(nodeId), value(std::move(nodeValue)), parent(parentNode) {}

        const NodeId id;
        const T value;                              // данные узла не меняются
        Node* parent;                               // НЕ владеет (см. шапку)
        std::vector<std::unique_ptr<Node>> children;  // владеет
    };

    Node& node(NodeId id) const {
        auto it = index_.find(id);
        if (it == index_.end()) {
            throw UnknownNodeError("узел " + std::to_string(id.value()) + " не найден в дереве");
        }
        return *it->second;
    }

    NodeId allocateId() noexcept { return NodeId(nextId_++); }

    // Разрушение без рекурсии: у вырожденного дерева («длинная история»)
    // глубина может быть 10^5+, и рекурсивные деструкторы unique_ptr
    // переполнили бы стек.
    void clear() noexcept {
        std::vector<std::unique_ptr<Node>> pending;
        if (root_) pending.push_back(std::move(root_));
        while (!pending.empty()) {
            std::unique_ptr<Node> current = std::move(pending.back());
            pending.pop_back();
            for (auto& child : current->children) pending.push_back(std::move(child));
            // current уничтожается здесь, его дети уже перенесены в pending
        }
        index_.clear();
    }

    void swap(Tree& other) noexcept {
        std::swap(root_, other.root_);
        std::swap(index_, other.index_);
        std::swap(nextId_, other.nextId_);
    }

    std::unique_ptr<Node> root_;
    std::unordered_map<NodeId, Node*> index_;
    std::uint64_t nextId_ = 1;
};

}  // namespace tree
