#include "HistoryTests.h"

#include <string>
#include <type_traits>
#include <vector>

#include "Tree.h"
#include "TreeAlgorithms.h"
#include "TreeRender.h"
#include "VersionHistory.h"

using tree::NodeId;
using tree::Tree;
using vcs::VersionHistory;
using vcs::VersionId;

static_assert(!std::is_copy_constructible<Tree<int>>::value, "дерево не копируется");
static_assert(std::is_nothrow_move_constructible<Tree<int>>::value, "дерево перемещается");
static_assert(!std::is_constructible<NodeId, std::uint64_t>::value,
              "NodeId нельзя создать снаружи дерева");

namespace {

using Ids = std::vector<VersionId>;

Ids ids(std::initializer_list<std::uint32_t> numbers) {
    Ids result;
    for (auto n : numbers) result.emplace_back(n);
    return result;
}

// ------------------------- структура Tree<T> -------------------------------

void testTreeStructure(TestRunner& t) {
    t.section("Tree<int>: создание");
    Tree<int> tr;
    t.check(tr.empty() && tr.size() == 0 && tr.checkInvariants(), "новое дерево пусто");
    t.expectThrow<tree::EmptyTreeError>([&] { tr.root(); }, "root() пустого дерева");
    NodeId r = tr.createRoot(1);
    t.check(!tr.empty() && tr.size() == 1 && tr.root() == r, "создан корень");
    t.expectThrow<tree::RootExistsError>([&] { tr.createRoot(2); }, "второй корень запрещён (I1)");
    NodeId a = tr.addChild(r, 10);
    NodeId b = tr.addChild(r, 20);
    NodeId a1 = tr.addChild(a, 11);
    t.check(a != b && b != a1 && a != r, "id узлов уникальны");
    t.check(tr.value(a1) == 11, "value() возвращает данные узла");
    t.check(!tr.parent(r).has_value(), "у корня нет родителя (I2)");
    t.check(tr.parent(a1) == a && tr.parent(a) == r, "у остальных ровно один родитель (I2)");
    t.check((tr.children(r) == std::vector<NodeId>{a, b}), "children в порядке добавления");
    t.check(tr.children(b).empty() && tr.isLeaf(b) && !tr.isLeaf(a), "isLeaf");
    t.check(tr.checkInvariants(), "инварианты соблюдены после вставок");

    t.section("Tree<int>: удаление (только лист)");
    t.expectThrow<tree::NodeHasChildrenError>([&] { tr.removeLeaf(a); },
                                              "удалить узел с потомками нельзя");
    t.check(tr.size() == 4 && tr.contains(a1), "после отказа дерево не изменилось");
    tr.removeLeaf(a1);
    t.check(!tr.contains(a1) && tr.size() == 3 && tr.isLeaf(a), "лист удалён");
    t.expectThrow<tree::UnknownNodeError>([&] { tr.value(a1); },
                                          "обращение по устаревшему id — исключение, а не UB");
    t.expectThrow<tree::UnknownNodeError>([&] { tr.addChild(a1, 0); },
                                          "добавить потомка к удалённому узлу нельзя");
    NodeId c = tr.addChild(r, 30);
    t.check(c != a1 && c.value() > a1.value(), "id не переиспользуются (I4)");
    t.check(tr.checkInvariants(), "инварианты соблюдены после удалений");
    t.expectThrow<tree::NodeHasChildrenError>([&] { tr.removeLeaf(r); },
                                              "корень с потомками удалить нельзя");
    tr.removeLeaf(a);
    tr.removeLeaf(b);
    tr.removeLeaf(c);
    tr.removeLeaf(r);
    t.check(tr.empty() && tr.size() == 0 && tr.checkInvariants(),
            "удаление единственного узла делает дерево пустым");
    NodeId r2 = tr.createRoot(5);
    t.check(tr.root() == r2 && r2 != r, "в опустевшем дереве можно создать новый корень");

    t.section("Tree<T>: параметрический полиморфизм и владение");
    Tree<std::string> words;
    NodeId w = words.createRoot("корень");
    words.addChild(w, "лист");
    t.check(words.value(words.children(w)[0]) == "лист", "Tree<std::string> работает так же");
    Tree<std::string> moved = std::move(words);
    t.check(moved.size() == 2 && moved.root() == w && words.empty(),
            "перемещение дерева сохраняет узлы и id");
    Tree<int> other;
    other.createRoot(0);
    t.expectThrow<tree::UnknownNodeError>([&] { other.value(a); },
                                          "id из другого дерева не принимается");
}

// ------------------------- алгоритмы ---------------------------------------

void testTreeAlgorithms(TestRunner& t) {
    // Дерево для тестов:
    //   1 -> {2, 3};  2 -> {4, 5};  5 -> {7};  3 -> {6}
    Tree<int> tr;
    NodeId n1 = tr.createRoot(1);
    NodeId n2 = tr.addChild(n1, 2);
    NodeId n3 = tr.addChild(n1, 3);
    NodeId n4 = tr.addChild(n2, 4);
    NodeId n5 = tr.addChild(n2, 5);
    NodeId n6 = tr.addChild(n3, 6);
    NodeId n7 = tr.addChild(n5, 7);

    t.section("Алгоритмы: обходы и поиск");
    std::vector<int> order;
    tree::preOrder(tr, n1, [&](NodeId id, std::size_t) {
        order.push_back(tr.value(id));
        return tree::Visit::Continue;
    });
    t.check((order == std::vector<int>{1, 2, 4, 5, 7, 3, 6}), "прямой обход: 1 2 4 5 7 3 6");
    t.check((tree::descendants(tr, n2) == std::vector<NodeId>{n4, n5, n7}), "потомки узла 2");
    t.check(tree::descendants(tr, n7).empty(), "у листа нет потомков");
    t.check(tree::descendants(tr, n1).size() == 6, "потомки корня — все, кроме него");
    t.check(tree::findFirst(tr, [](int v) { return v > 4; }) == n5,
            "findFirst с предикатом «> 4» -> первый в обходе (5)");
    t.check(!tree::findFirst(tr, [](int v) { return v > 100; }).has_value(),
            "findFirst без совпадений -> nullopt");
    t.check(tree::findAll(tr, [](int v) { return v % 2 == 0; }).size() == 3,
            "findAll с предикатом «чётное» -> 2, 4, 6");
    t.check(!tree::findFirst(Tree<int>{}, [](int) { return true; }).has_value(),
            "findFirst по пустому дереву");
    int visited = 0;
    bool stopped = tree::preOrder(tr, n1, [&](NodeId, std::size_t) {
        return ++visited == 3 ? tree::Visit::Stop : tree::Visit::Continue;
    });
    t.check(stopped && visited == 3, "посетитель может остановить обход");

    t.section("Алгоритмы: путь и общий предок");
    t.check((tree::pathFromRoot(tr, n7) == std::vector<NodeId>{n1, n2, n5, n7}),
            "путь от корня до 7: 1 2 5 7");
    t.check((tree::pathFromRoot(tr, n1) == std::vector<NodeId>{n1}), "путь до корня — сам корень");
    t.check(tree::depthOf(tr, n7) == 3 && tree::depthOf(tr, n1) == 0, "глубина");
    t.check(tree::lowestCommonAncestor(tr, n4, n7) == n2, "LCA(4, 7) = 2 (разная глубина)");
    t.check(tree::lowestCommonAncestor(tr, n7, n6) == n1, "LCA(7, 6) = 1 (разные ветви)");
    t.check(tree::lowestCommonAncestor(tr, n2, n7) == n2, "LCA(предок, потомок) = предок");
    t.check(tree::lowestCommonAncestor(tr, n6, n6) == n6, "LCA(x, x) = x");
    t.check(tree::lowestCommonAncestor(tr, n1, n4) == n1, "LCA(корень, x) = корень");
    t.check(tree::isAncestorOrSelf(tr, n2, n7) && !tree::isAncestorOrSelf(tr, n7, n2),
            "isAncestorOrSelf");

    t.section("Вывод дерева");
    const std::string expected =
        "1\n"
        "|-- 2\n"
        "|   |-- 4\n"
        "|   `-- 5\n"
        "|       `-- 7\n"
        "`-- 3\n"
        "    `-- 6\n";
    t.check(tree::renderTree(tr, [](NodeId, int v) { return std::to_string(v); }) == expected,
            "renderTree с пользовательским форматированием");
    t.check(tree::renderTree(Tree<int>{}, [](NodeId, int) { return ""; }) == "(пусто)\n",
            "вывод пустого дерева");

    t.section("Глубокое дерево (100 000 уровней)");
    {
        Tree<int> deep;
        NodeId root = deep.createRoot(0);
        NodeId last = root;
        for (int i = 1; i < 100000; ++i) last = deep.addChild(last, i);
        t.check(tree::pathFromRoot(deep, last).size() == 100000, "путь длиной 100 000");
        t.check(tree::lowestCommonAncestor(deep, last, root) == root, "LCA на глубине 100 000");
        t.check(tree::descendants(deep, root).size() == 99999, "итеративный обход не падает");
        t.check(deep.checkInvariants(), "инварианты глубокого дерева");
    }  // деструктор итеративный — стек не переполняется
    t.check(true, "глубокое дерево уничтожено без переполнения стека");
}

// ------------------------- история версий ----------------------------------

void testHistory(TestRunner& t) {
    t.section("VersionId / Version");
    t.check(VersionId(3).toString() == "v3", "VersionId печатается как v3");
    t.check(VersionId(1) == VersionId(1) && VersionId(1) != VersionId(2), "== и !=");
    t.check(VersionId(1) < VersionId(2), "<");

    t.section("История: пустая");
    VersionHistory h;
    t.check(h.empty() && h.size() == 0 && h.checkInvariants(), "новая история пуста");
    t.expectThrow<vcs::EmptyHistoryError>([&] { h.current(); }, "current() пустой истории");
    t.expectThrow<vcs::EmptyHistoryError>([&] { h.commit("x", "x"); },
                                          "commit без начальной версии");
    t.expectThrow<vcs::VersionNotFoundError>([&] { h.find(VersionId(1)); },
                                             "find в пустой истории");

    t.section("История: создание и ветвление");
    VersionId v1 = h.createInitial("", "init");
    t.expectThrow<vcs::InitialVersionExistsError>([&] { h.createInitial("", "again"); },
                                                  "вторая начальная версия запрещена");
    VersionId v2 = h.commit("A", "A");
    VersionId v3 = h.commit("AB", "B");
    t.check(h.current() == v3 && h.parentOf(v3) == v2, "commit строится от текущей версии");
    h.checkout(v2);
    VersionId v4 = h.commit("AC", "C");
    t.check(h.parentOf(v4) == v2 && h.current() == v4, "после checkout(v2) ветка растёт от v2");
    VersionId v5 = h.commitFrom(v1, "Z", "Z");
    t.check(h.parentOf(v5) == v1, "commitFrom от произвольной версии");
    VersionId v6 = h.commit("Z2", "Z2");
    t.check(h.size() == 6 && h.checkInvariants(), "6 версий, инварианты соблюдены");
    t.check(h.initial() == v1 && !h.parentOf(v1).has_value(), "у начальной версии нет родителя");
    t.check(h.find(v3).content() == "AB" && h.find(v3).message() == "B",
            "find возвращает содержимое версии");
    t.expectThrow<vcs::VersionNotFoundError>([&] { h.find(VersionId(99)); }, "find отсутствующей");
    t.expectThrow<vcs::VersionNotFoundError>([&] { h.checkout(VersionId(99)); },
                                             "checkout отсутствующей");
    t.expectThrow<vcs::VersionNotFoundError>([&] { h.commitFrom(VersionId(99), "", ""); },
                                             "commitFrom от отсутствующей");
    t.check(h.size() == 6, "после ошибок история не изменилась");

    t.section("История: запросы");
    t.check(h.children(v1) == Ids{v2, v5}, "непосредственные потомки v1: v2, v5");
    t.check(h.children(v3).empty(), "у v3 нет производных");
    t.check(h.descendants(v2) == Ids{v3, v4}, "все потомки v2: v3, v4");
    t.check(h.descendants(v1) == ids({2, 3, 4, 5, 6}), "все потомки v1 — вся история кроме неё");
    t.check(h.pathTo(v4) == Ids{v1, v2, v4}, "путь до v4: v1 -> v2 -> v4");
    t.check(h.pathTo(v1) == Ids{v1}, "путь до начальной: [v1]");
    t.check(h.commonAncestor(v3, v4) == v2, "общий предок v3 и v4 — v2");
    t.check(h.commonAncestor(v3, v6) == v1, "общий предок v3 и v6 — v1");
    t.check(h.commonAncestor(v2, v4) == v2, "общий предок предка и потомка — предок");
    t.check(h.commonAncestor(v6, v6) == v6, "общий предок версии с собой — она сама");
    t.expectThrow<vcs::VersionNotFoundError>([&] { h.commonAncestor(v1, VersionId(42)); },
                                             "общий предок с несуществующей версией");

    t.section("История: вывод");
    const std::string expected =
        "v1 \"init\"\n"
        "|-- v2 \"A\"\n"
        "|   |-- v3 \"B\"\n"
        "|   `-- v4 \"C\"\n"
        "`-- v5 \"Z\"\n"
        "    `-- v6 \"Z2\"  *\n";
    t.check(h.toTreeString() == expected, "дерево истории, текущая версия помечена *");

    t.section("История: удаление");
    t.expectThrow<vcs::VersionHasDerivedError>([&] { h.remove(v2); },
                                               "версию с производными удалить нельзя");
    t.expectThrow<vcs::VersionHasDerivedError>([&] { h.remove(v1); },
                                               "начальную версию с производными удалить нельзя");
    h.remove(v6);
    t.check(!h.contains(v6) && h.current() == v5, "удалена текущая -> текущей стал родитель");
    h.remove(v3);
    t.check(h.children(v2) == Ids{v4} && h.current() == v5, "удаление нетекущей не меняет current");
    VersionId v7 = h.commit("Z3", "Z3");
    t.check(v7 == VersionId(7), "номера удалённых версий не переиспользуются");
    t.expectThrow<vcs::VersionNotFoundError>([&] { h.remove(v6); }, "повторное удаление");
    t.check(h.checkInvariants(), "инварианты после удалений");

    VersionHistory single;
    VersionId only = single.createInitial("x", "x");
    single.remove(only);
    t.check(single.empty() && single.checkInvariants(), "удаление единственной версии -> пусто");
    t.expectThrow<vcs::EmptyHistoryError>([&] { single.current(); }, "current() после этого");
    t.check(single.createInitial("y", "y") == VersionId(2), "можно начать историю заново");
}

}  // namespace

void runHistoryTests(TestRunner& t) {
    testTreeStructure(t);
    testTreeAlgorithms(t);
    testHistory(t);
}
