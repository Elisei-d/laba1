#pragma once

// ============================================================================
//  ВЫВОД: текстовое представление дерева — отдельно от структуры и алгоритмов.
//  Как именно печатать значение узла, решает вызывающий код (formatter).
// ============================================================================

#include <string>
#include <utility>
#include <vector>

#include "Tree.h"

namespace tree {

// formatter(NodeId, const T&) -> std::string — подпись одного узла.
// Итеративно, чтобы глубокие деревья не переполняли стек.
template <typename TreeT, typename Formatter>
std::string renderTree(const TreeT& t, Formatter formatter) {
    if (t.empty()) return "(пусто)\n";

    struct Frame {
        NodeId id;
        std::string prefix;  // отступ для потомков этого узла
        std::string line;    // начало строки самого узла
    };

    std::string out;
    std::vector<Frame> stack{{t.root(), "", ""}};
    while (!stack.empty()) {
        Frame frame = std::move(stack.back());
        stack.pop_back();
        out += frame.line + formatter(frame.id, t.value(frame.id)) + "\n";

        auto kids = t.children(frame.id);
        for (std::size_t i = kids.size(); i-- > 0;) {  // в стек — в обратном порядке
            const bool last = (i + 1 == kids.size());
            stack.push_back({kids[i], frame.prefix + (last ? "    " : "|   "),
                             frame.prefix + (last ? "`-- " : "|-- ")});
        }
    }
    return out;
}

}  // namespace tree
