

#include <iostream>

#include "Errors.h"
#include "FileSystem.h"
#include "FileSystemTests.h"

#ifdef _WIN32
#include <windows.h>
#endif

using namespace memfs;

namespace {


template <typename Action>
void attempt(const std::string& title, Action action) {
    std::cout << "> " << title << "\n";
    try {
        action();
        std::cout << "  успешно\n";
    } catch (const FsError& e) {
        std::cout << "  ОТКАЗ: " << e.what() << "\n";
    }
}

void demo() {
    std::cout << "\n================ ДЕМОНСТРАЦИЯ ================\n";
    FileSystem fs;

    const Path home{"home"};
    const Path documents = home / "documents";
    const Path report = documents / "report.txt";
    const Path tmp{"tmp"};

    fs.createDirectory(home);
    fs.createDirectory(documents);
    fs.createFile(report, "Отчёт за квартал");
    fs.createFile(documents / "plan.txt", "1. Сдать лабу");
    fs.createDirectory(home / "photos");
    fs.createFile(home / "photos" / "cat.jpg", std::string(2048, '#'));
    fs.createDirectory(tmp);

    std::cout << "\nИсходное дерево:\n" << fs.tree();
    std::cout << "\nfind(" << report << ") = " << fs.find(report) << "\n";
    std::cout << "sizeOf(" << home << ") = " << fs.sizeOf(home) << " B\n\n";

    attempt("создать " + (tmp / "x" / "y.txt").toString() + " (нет родителя)",
            [&] { fs.createFile(tmp / "x" / "y.txt"); });
    attempt("создать " + report.toString() + " повторно",
            [&] { fs.createFile(report); });
    attempt("удалить корень", [&] { fs.remove(Path("/")); });
    attempt("переместить корень в /tmp", [&] { fs.move(Path("/"), tmp); });
    attempt("переместить /home в /home/documents",
            [&] { fs.move(home, documents); });
    attempt("переместить " + report.toString() + " в /tmp", [&] { fs.move(report, tmp); });
    attempt("переместить /home/photos в /tmp", [&] { fs.move(home / "photos", tmp); });
    attempt("удалить /home/documents/plan.txt", [&] { fs.remove(documents / "plan.txt"); });

    std::cout << "\nДерево после операций:\n" << fs.tree();
}

} 

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);  
#endif
    TestRunner runner;
    runFileSystemTests(runner);
    const int code = runner.summary();
    demo();
    return code;
}
