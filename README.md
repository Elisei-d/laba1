# Лабораторная работа 1 — вариант 3

| | Задание | Папка | Исполняемый файл |
|---|---|---|---|
| 1 | Принципы ООП. Перегрузка операторов — «Файлы и каталоги» | `task1/` | `filesystem` |
| 2 | Рекурсивные структуры — «История версий документа» | `task2/` | `history` |

Вариант: 17 mod 5 + 1 = **3**.

## Сборка

Требуется компилятор с поддержкой C++17 (GCC ≥ 8, Clang ≥ 7, MSVC 2019+).

### CMake (любая ОС, в том числе CLion / Visual Studio)

```bash
cd task1            # или task2
cmake -S . -B build
cmake --build build
./build/filesystem  # или ./build/history (в Windows: build\Debug\filesystem.exe)
```

### Без CMake

```bash
cd task1
g++ -std=c++17 -Wall -Wextra -pedantic -Iinclude -Itests src/*.cpp tests/*.cpp -o filesystem
./filesystem

cd ../task2
g++ -std=c++17 -Wall -Wextra -pedantic -Iinclude -Itests src/*.cpp tests/*.cpp -o history
./history
```

Программа сначала запускает автотесты (печатает `[OK]`/`[FAIL]` по каждому
и итог), затем показывает демонстрацию. Код возврата 0 означает, что все тесты прошли.

Код проверен с `-Wall -Wextra -pedantic -Werror` в GCC 13 и Clang, а также с
AddressSanitizer и UBSan: предупреждений, утечек и неопределённого поведения нет.

## Структура

```
task1/
  include/  Errors.h  Name.h  Path.h  FsNode.h  File.h  Directory.h  FileSystem.h
  src/      *.cpp, main.cpp (тесты + демонстрация)
  tests/    TestRunner.h, FileSystemTests.cpp
task2/
  include/  Tree.h            — структура хранения (шаблон Tree<T>)
            TreeAlgorithms.h  — обобщённые алгоритмы (обход, путь, LCA, поиск)
            TreeRender.h      — вывод дерева
            Version.h         — предметная область: VersionId, Version
            VersionHistory.h  — предметная логика истории
  src/      VersionHistory.cpp, main.cpp (тесты + демонстрация)
  tests/    TestRunner.h, HistoryTests.cpp
```

Подготовка к защите — в файле `ЗАЩИТА.md`.
