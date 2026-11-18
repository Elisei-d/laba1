#pragma once

// ============================================================================
//  АЛГОРИТМЫ над деревом — отдельно от структуры хранения.
//
//  Все алгоритмы — шаблоны функций, параметризованные:
//   - типом дерева TreeT (подходит любой тип с операциями root(), parent(id),
//     children(id), value(id), а не только Tree<T>);
//   - изменяемым правилом (предикат / посетитель), которое передаётся
//     параметром, а не зашито внутрь алгоритма.
//  Алгоритмы не знают о документах и версиях и не обращаются к узлам
//  напрямую — только к открытому контракту дерева.
//
//  Обход реализован итеративно (явный стек), чтобы не упереться в глубину
//  стека вызовов на длинной линейной истории.
// ============================================================================

#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#include "Tree.h"

namespace tree {

// Что делать обходу после посещения узла.
enum class Visit { Continue, Stop };

// Прямой (pre-order) обход поддерева start. visitor(NodeId, depth) -> Visit.
// depth считается от start (у start глубина 0). Дети посещаются в порядке
// добавления. Возвращает true, если обход остановлен посетителем.
template <typename TreeT, typename Visitor>
bool preOrder(const TreeT& t, NodeId start, Visitor visitor) {
    std::vector<std::pair<NodeId, std::size_t>> stack{{start, 0}};
    while (!stack.empty()) {
        auto [id, depth] = stack.back();
        stack.pop_back();
        if (visitor(id, depth) == Visit::Stop) return true;
        auto kids = t.children(id);
        for (auto it = kids.rbegin(); it != kids.rend(); ++it) {
            stack.emplace_back(*it, depth + 1);
        }
    }
    return false;
}

// Все потомки узла (без него самого) в порядке прямого обхода.
template <typename TreeT>
std::vector<NodeId> descendants(const TreeT& t, NodeId id) {
    std::vector<NodeId> result;
    preOrder(t, id, [&](NodeId current, std::size_t depth) {
        if (depth > 0) result.push_back(current);
        return Visit::Continue;
    });
    return result;
}

// Первый узел (в порядке прямого обхода), значение которого удовлетворяет
// предикату. Правило поиска — параметр, алгоритм от него не зависит.
template <typename TreeT, typename Predicate>
std::optional<NodeId> findFirst(const TreeT& t, Predicate predicate) {
    if (t.empty()) return std::nullopt;
    std::optional<NodeId> found;
    preOrder(t, t.root(), [&](NodeId id, std::size_t) {
        if (predicate(t.value(id))) {
            found = id;
            return Visit::Stop;
        }
        return Visit::Continue;
    });
    return found;
}

// Все узлы, значения которых удовлетворяют предикату.
template <typename TreeT, typename Predicate>
std::vector<NodeId> findAll(const TreeT& t, Predicate predicate) {
    std::vector<NodeId> result;
    if (t.empty()) return result;
    preOrder(t, t.root(), [&](NodeId id, std::size_t) {
        if (predicate(t.value(id))) result.push_back(id);
        return Visit::Continue;
    });
    return result;
}

// Глубина узла: у корня 0.
template <typename TreeT>
std::size_t depthOf(const TreeT& t, NodeId id) {
    std::size_t depth = 0;
    for (auto p = t.parent(id); p; p = t.parent(*p)) ++depth;
    return depth;
}

// Путь от корня до узла включительно: [root, ..., id].
template <typename TreeT>
std::vector<NodeId> pathFromRoot(const TreeT& t, NodeId id) {
    std::vector<NodeId> path{id};
    for (auto p = t.parent(id); p; p = t.parent(*p)) path.push_back(*p);
    std::reverse(path.begin(), path.end());
    return path;
}

// true, если ancestor — предок node или совпадает с ним.
template <typename TreeT>
bool isAncestorOrSelf(const TreeT& t, NodeId ancestor, NodeId node) {
    std::optional<NodeId> current = node;
    for (; current; current = t.parent(*current)) {
        if (*current == ancestor) return true;
    }
    return false;
}

// Ближайший общий предок (LCA). Узел считается предком самого себя,
// поэтому LCA(a, a) = a, а LCA(предок, потомок) = предок.
// Алгоритм: выравниваем глубины, затем поднимаемся одновременно. O(h).
template <typename TreeT>
NodeId lowestCommonAncestor(const TreeT& t, NodeId a, NodeId b) {
    std::size_t depthA = depthOf(t, a);
    std::size_t depthB = depthOf(t, b);
    for (; depthA > depthB; --depthA) a = *t.parent(a);
    for (; depthB > depthA; --depthB) b = *t.parent(b);
    while (a != b) {
        a = *t.parent(a);
        b = *t.parent(b);
    }
    return a;  // в дереве с одним корнем подъём всегда сходится (инвариант I1)
}

}  // namespace tree
