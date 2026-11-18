#include "FileSystemTests.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "Errors.h"
#include "FileSystem.h"

using namespace memfs;

// Проверки на этапе компиляции: модель запрещает то, что должна запрещать.
static_assert(!std::is_copy_assignable<Path>::value, "Path иммутабелен: присваивание запрещено");
static_assert(std::is_copy_constructible<Path>::value, "Path — значение: копировать можно");
static_assert(!std::is_copy_constructible<FsNode>::value, "элемент ФС — объект, копировать нельзя");
static_assert(std::is_abstract<FsNode>::value, "FsNode — абстрактный тип");
static_assert(!std::is_convertible<std::string, Path>::value,
              "строка не превращается в Path неявно (explicit-конструктор)");

namespace {

// ---------------------------------------------------------------------------
// Новый тип элемента, объявленный ВНЕ библиотеки. Демонстрирует принцип
// открытости/закрытости: ни FsNode, ни Directory, ни FileSystem не менялись,
// а символьная ссылка работает в дереве, в подсчёте размера и в путях.
// ---------------------------------------------------------------------------
class Symlink : public FsNode {
public:
    Symlink(std::string name, std::string target)
        : FsNode(std::move(name)), target_(Path(target)) {}
    std::size_t size() const override { return 0; }  // ссылка не занимает места файлов
    std::string toString() const override { return name() + " -> " + target_.toString(); }

private:
    Path target_;
};

void testPath(TestRunner& t) {
    t.section("Path: создание и нормализация");
    Path root;
    t.check(root.isRoot() && root.depth() == 0, "Path() — корень, глубина 0");
    t.check(root.toString() == "/", "корень печатается как '/'");
    t.check(Path("") == root && Path("/") == root && Path("///") == root,
            "\"\", \"/\" и \"///\" — тоже корень");
    Path docs("/home/docs");
    t.check(docs.depth() == 2 && docs.segments()[0] == "home" && docs.segments()[1] == "docs",
            "'/home/docs' разбирается на 2 сегмента");
    t.check(Path("home//docs/") == docs, "лишние '/' игнорируются: 'home//docs/' == '/home/docs'");
    t.check(docs.toString() == "/home/docs", "toString() -> '/home/docs'");
    t.expectThrow<InvalidNameError>([] { Path("/home/./docs"); }, "сегмент '.' запрещён");
    t.expectThrow<InvalidNameError>([] { Path("/home/.."); }, "сегмент '..' запрещён");

    t.section("Path: операторы / == !=");
    Path home{"home"};
    Path documents = home / "documents";
    Path report = documents / "report.txt";
    t.check(report.toString() == "/home/documents/report.txt", "Path / string / string");
    t.check(home / Path("a/b") == Path("/home/a/b"), "Path / Path");
    t.check(home / "a/b" == Path("/home/a/b"), "Path / \"a/b\" — строка разбирается как путь");
    t.check(home / root == home && root / home == home, "корень — нейтральный элемент для /");
    t.check(home.toString() == "/home" && documents.toString() == "/home/documents",
            "operator/ не изменяет исходные объекты (иммутабельность)");
    t.check(Path("/a/b") == Path("a/b"), "== для равных путей");
    t.check(Path("/a/b") != Path("/a/c") && Path("/a") != Path("/a/b"), "!= для разных путей");
    t.check(!(Path("/a") != Path("a")), "!= согласован с ==");
    t.expectThrow<InvalidNameError>([&] { (void)(home / ".."); }, "Path / \"..\" -> InvalidNameError");

    t.section("Path: name, parent, isAncestorOrSelfOf");
    t.check(report.name() == "report.txt", "name() — последний сегмент");
    t.check(report.parent() == documents && Path("/a").parent().isRoot(), "parent()");
    t.expectThrow<ForbiddenOperationError>([&] { (void)root.name(); }, "у корня нет имени");
    t.expectThrow<ForbiddenOperationError>([&] { (void)root.parent(); }, "у корня нет родителя");
    t.check(home.isAncestorOrSelfOf(report), "'/home' — предок '/home/documents/report.txt'");
    t.check(home.isAncestorOrSelfOf(home), "путь — «предок или сам» для себя");
    t.check(root.isAncestorOrSelfOf(report), "корень — предок любого пути");
    t.check(!report.isAncestorOrSelfOf(home), "потомок не является предком");
    t.check(!Path("/a/b").isAncestorOrSelfOf(Path("/a/bc")),
            "'/a/b' НЕ предок '/a/bc' (сравнение по сегментам, а не по строке)");
}

void testFileAndDirectory(TestRunner& t) {
    t.section("File");
    File empty("empty.txt");
    t.check(empty.size() == 0 && empty.content().empty(), "пустой файл имеет размер 0");
    File file("a.txt", "hello");
    t.check(file.size() == 5, "размер файла = размер содержимого");
    file.append(", world");
    t.check(file.size() == 12 && file.content() == "hello, world", "append увеличивает размер");
    file.write("x");
    t.check(file.size() == 1, "write заменяет содержимое");
    t.check(file.toString() == "a.txt (1 B)", "toString файла");
    t.expectThrow<InvalidNameError>([] { File(""); }, "пустое имя файла запрещено");
    t.expectThrow<InvalidNameError>([] { File("a/b"); }, "имя с '/' запрещено");
    t.expectThrow<InvalidNameError>([] { Directory(".."); }, "имя '..' запрещено");

    t.section("Directory");
    Directory dir("dir");
    t.check(dir.size() == 0 && dir.childCount() == 0, "пустой каталог имеет размер 0");
    dir.add(std::make_unique<File>("a.txt", "12345"));
    auto& sub = static_cast<Directory&>(dir.add(std::make_unique<Directory>("sub")));
    sub.add(std::make_unique<File>("b.txt", "123"));
    sub.add(std::make_unique<Directory>("emptySub"));
    t.check(dir.size() == 8, "размер каталога = сумма файлов, включая вложенные (5 + 3)");
    t.check(dir.find("a.txt") != nullptr && dir.find("sub") == &sub, "find по имени");
    t.check(dir.find("b.txt") == nullptr, "find ищет только непосредственных потомков");
    t.check(dir.find("nope") == nullptr && !dir.contains("nope"), "find отсутствующего -> nullptr");
    t.check((dir.childNames() == std::vector<std::string>{"a.txt", "sub"}),
            "childNames в порядке добавления");
    t.expectThrow<AlreadyExistsError>([&] { dir.add(std::make_unique<File>("a.txt")); },
                                      "дубликат имени файла запрещён");
    t.expectThrow<AlreadyExistsError>([&] { dir.add(std::make_unique<Directory>("a.txt")); },
                                      "каталог с именем существующего файла запрещён");
    t.expectThrow<std::invalid_argument>([&] { dir.add(nullptr); }, "добавление nullptr запрещено");
    t.check(dir.childCount() == 2, "после неудачных add состояние не изменилось");
    t.check(dir.asDirectory() == &dir && File("f").asDirectory() == nullptr,
            "asDirectory: каталог — да, файл — нет");

    const std::string expectedTree =
        "dir/ (8 B)\n"
        "|-- a.txt (5 B)\n"
        "`-- sub/ (3 B)\n"
        "    |-- b.txt (3 B)\n"
        "    `-- emptySub/ (0 B)\n";
    t.check(dir.tree() == expectedTree, "вывод каталога в виде дерева");

    auto extracted = dir.extract("sub");
    t.check(extracted != nullptr && extracted->size() == 3 && !dir.contains("sub"),
            "extract передаёт владение поддеревом");
    t.check(dir.extract("sub") == nullptr, "повторный extract -> nullptr");
    t.check(dir.remove("a.txt") && !dir.remove("a.txt"), "remove: true, затем false");
    t.check(dir.size() == 0, "после удаления каталог пуст");

    t.section("Полиморфизм по подтипу");
    std::vector<std::unique_ptr<FsNode>> nodes;
    nodes.push_back(std::make_unique<File>("f", "abc"));
    nodes.push_back(std::move(extracted));
    nodes.push_back(std::make_unique<Symlink>("link", "/f"));
    std::size_t total = 0;
    for (const auto& node : nodes) total += node->size();
    t.check(total == 6, "size() через базовый тип: 3 (файл) + 3 (каталог) + 0 (ссылка)");
    t.check(nodes[2]->toString() == "link -> /f", "toString() нового типа через базовый тип");
}

FileSystem makeSample() {
    FileSystem fs;
    fs.createDirectory(Path("/home"));
    fs.createDirectory(Path("/home/docs"));
    fs.createFile(Path("/home/docs/report.txt"), "quarterly report");  // 16
    fs.createFile(Path("/home/docs/notes.txt"), "todo");               // 4
    fs.createDirectory(Path("/home/music"));
    fs.createFile(Path("/home/music/song.mp3"), std::string(100, 'x'));  // 100
    fs.createDirectory(Path("/tmp"));
    return fs;
}

void testFileSystemCreateFind(TestRunner& t) {
    t.section("FileSystem: создание и поиск");
    FileSystem fs = makeSample();
    t.check(fs.find(Path("/home/docs/report.txt")).size() == 16, "поиск файла на глубине 3");
    t.check(fs.find(Path("/")).name() == "root", "поиск корня");
    t.check(fs.exists(Path("/home/music")), "exists для каталога");
    t.check(!fs.exists(Path("/home/videos")), "exists для отсутствующего -> false");
    t.check(!fs.exists(Path("/home/docs/report.txt/x")), "exists «сквозь файл» -> false");
    t.expectThrow<PathNotFoundError>([&] { fs.find(Path("/home/videos")); },
                                     "find отсутствующего -> PathNotFoundError");
    t.expectThrow<NotADirectoryError>([&] { fs.find(Path("/home/docs/report.txt/x")); },
                                      "путь через файл -> NotADirectoryError");
    t.expectThrow<PathNotFoundError>([&] { fs.createFile(Path("/no/such/dir/f.txt")); },
                                     "создание при отсутствующем родителе запрещено");
    t.expectThrow<NotADirectoryError>([&] { fs.createFile(Path("/home/docs/report.txt/f")); },
                                      "создание внутри файла запрещено");
    t.expectThrow<AlreadyExistsError>([&] { fs.createFile(Path("/home/docs/report.txt")); },
                                      "повторное создание файла запрещено");
    t.expectThrow<AlreadyExistsError>([&] { fs.createDirectory(Path("/home/docs/notes.txt")); },
                                      "каталог с именем существующего файла запрещён");
    t.expectThrow<ForbiddenOperationError>([&] { fs.createDirectory(Path("/")); },
                                           "создать корень повторно нельзя");
    t.expectThrow<ForbiddenOperationError>([&] { fs.createFile(Path("/")); },
                                           "создать файл на месте корня нельзя");
    File& created = fs.createFile(Path("/tmp/new.txt"));
    created.write("12");
    t.check(fs.sizeOf(Path("/tmp/new.txt")) == 2, "ссылка на созданный файл позволяет его менять");

    t.section("FileSystem: произвольная глубина");
    FileSystem deep;
    std::string text;  // Path иммутабелен (присваивания нет), поэтому копим строку
    for (int i = 0; i < 200; ++i) {
        text += "/d" + std::to_string(i);
        deep.createDirectory(Path(text));
    }
    const Path path(text);
    deep.createFile(path / "leaf.txt", "abc");
    t.check(deep.find(path / "leaf.txt").size() == 3, "поиск файла на глубине 201");
    t.check(deep.sizeOf(Path("/d0")) == 3, "размер верхнего каталога учитывает файл на дне");
}

void testFileSystemRemove(TestRunner& t) {
    t.section("FileSystem: удаление");
    FileSystem fs = makeSample();
    t.check(fs.sizeOf(Path("/")) == 120, "исходный размер корня = 120");
    fs.remove(Path("/home/docs/notes.txt"));
    t.check(!fs.exists(Path("/home/docs/notes.txt")), "файл удалён");
    t.check(fs.sizeOf(Path("/home")) == 116, "размер родителя уменьшился");
    fs.remove(Path("/home/music"));
    t.check(!fs.exists(Path("/home/music/song.mp3")), "каталог удалён вместе с поддеревом");
    t.check(fs.sizeOf(Path("/")) == 16, "размер корня пересчитан");
    t.expectThrow<ForbiddenOperationError>([&] { fs.remove(Path("/")); },
                                           "удаление корня запрещено");
    t.expectThrow<PathNotFoundError>([&] { fs.remove(Path("/home/music")); },
                                     "удаление отсутствующего -> PathNotFoundError");
    t.expectThrow<NotADirectoryError>([&] { fs.remove(Path("/home/docs/report.txt/x")); },
                                      "удаление по пути через файл -> NotADirectoryError");
}

void testFileSystemMove(TestRunner& t) {
    t.section("FileSystem: перемещение");
    FileSystem fs = makeSample();
    fs.move(Path("/home/docs/notes.txt"), Path("/tmp"));
    t.check(!fs.exists(Path("/home/docs/notes.txt")) && fs.exists(Path("/tmp/notes.txt")),
            "файл перемещён, имя сохранено");
    t.check(fs.sizeOf(Path("/home/docs")) == 16 && fs.sizeOf(Path("/tmp")) == 4,
            "размеры каталогов после перемещения пересчитаны");

    fs.move(Path("/home/music"), Path("/tmp"));
    t.check(fs.find(Path("/tmp/music/song.mp3")).size() == 100,
            "каталог перемещён вместе с поддеревом");

    t.expectThrow<ForbiddenOperationError>([&] { fs.move(Path("/"), Path("/tmp")); },
                                           "перемещение корня запрещено");
    t.expectThrow<ForbiddenOperationError>([&] { fs.move(Path("/home"), Path("/home")); },
                                           "каталог нельзя переместить в самого себя");
    t.expectThrow<ForbiddenOperationError>([&] { fs.move(Path("/home"), Path("/home/docs")); },
                                           "каталог нельзя переместить в своего потомка");
    fs.createDirectory(Path("/home/docs/deep"));
    t.expectThrow<ForbiddenOperationError>(
        [&] { fs.move(Path("/home"), Path("/home/docs/deep")); },
        "каталог нельзя переместить в потомка на любой глубине");
    t.check(fs.exists(Path("/home/docs/deep")), "после запрещённого move структура не изменилась");

    fs.createFile(Path("/home/notes.txt"), "other");
    t.expectThrow<AlreadyExistsError>([&] { fs.move(Path("/tmp/notes.txt"), Path("/home")); },
                                      "конфликт имён в целевом каталоге");
    t.check(fs.find(Path("/tmp/notes.txt")).size() == 4 &&
                fs.find(Path("/home/notes.txt")).size() == 5,
            "при конфликте оба файла остались на месте (уникальность имён сохранена)");
    t.expectThrow<AlreadyExistsError>([&] { fs.move(Path("/tmp/notes.txt"), Path("/tmp")); },
                                      "перемещение в тот же каталог — конфликт с самим собой");
    t.expectThrow<PathNotFoundError>([&] { fs.move(Path("/nothing"), Path("/tmp")); },
                                     "перемещение отсутствующего элемента");
    t.expectThrow<PathNotFoundError>([&] { fs.move(Path("/tmp/notes.txt"), Path("/nowhere")); },
                                     "перемещение в отсутствующий каталог");
    t.expectThrow<NotADirectoryError>(
        [&] { fs.move(Path("/tmp/notes.txt"), Path("/home/docs/report.txt")); },
        "перемещение «внутрь файла» запрещено");

    fs.createDirectory(Path("/a"));
    fs.createDirectory(Path("/a/b"));
    fs.createDirectory(Path("/a/bc"));
    t.expectNoThrow([&] { fs.move(Path("/a/b"), Path("/a/bc")); },
                    "'/a/b' можно переместить в '/a/bc' (это не потомок)");
    t.check(fs.exists(Path("/a/bc/b")), "результат перемещения '/a/b' -> '/a/bc'");
}

void testFileSystemTreeAndExtensibility(TestRunner& t) {
    t.section("FileSystem: вывод дерева");
    FileSystem fs = makeSample();
    const std::string expected =
        "root/ (120 B)\n"
        "|-- home/ (120 B)\n"
        "|   |-- docs/ (20 B)\n"
        "|   |   |-- report.txt (16 B)\n"
        "|   |   `-- notes.txt (4 B)\n"
        "|   `-- music/ (100 B)\n"
        "|       `-- song.mp3 (100 B)\n"
        "`-- tmp/ (0 B)\n";
    t.check(fs.tree() == expected, "дерево ФС совпадает с ожидаемым");
    t.check(FileSystem("empty").tree() == "empty/ (0 B)\n", "дерево пустой ФС");

    t.section("Расширяемость (OCP): новый тип элемента без изменения библиотеки");
    fs.add(Path("/tmp"), std::make_unique<Symlink>("docs-link", "/home/docs"));
    t.check(fs.exists(Path("/tmp/docs-link")), "Symlink добавлен через общий контракт");
    t.check(fs.sizeOf(Path("/tmp")) == 0, "размер ссылки учтён полиморфно");
    t.check(fs.tree().find("`-- docs-link -> /home/docs") != std::string::npos,
            "Symlink выводится в дереве");
    t.expectThrow<NotADirectoryError>([&] { fs.find(Path("/tmp/docs-link/report.txt")); },
                                      "в Symlink нельзя «зайти» — он не каталог");
    t.expectThrow<PathNotFoundError>(
        [&] { fs.add(Path("/missing"), std::make_unique<File>("x")); },
        "add в отсутствующий каталог");

    t.section("Владение: перемещение самой ФС");
    const FsNode& report = fs.find(Path("/home/docs/report.txt"));
    FileSystem moved = std::move(fs);
    t.check(&moved.find(Path("/home/docs/report.txt")) == &report,
            "после std::move(FileSystem) ссылки на элементы остаются валидными");
}

}  // namespace

void runFileSystemTests(TestRunner& t) {
    testPath(t);
    testFileAndDirectory(t);
    testFileSystemCreateFind(t);
    testFileSystemRemove(t);
    testFileSystemMove(t);
    testFileSystemTreeAndExtensibility(t);
}
