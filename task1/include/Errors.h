#pragma once

#include <stdexcept>
#include <string>

namespace memfs {

// Иерархия исключений файловой системы.
// Наследование здесь выражает отношение «является»: любая ошибка ФС
// является std::runtime_error, а «путь не найден» является ошибкой ФС.
// Благодаря этому вызывающий код может ловить как конкретную ошибку,
// так и все ошибки ФС сразу (catch (const FsError&)).
class FsError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Недопустимое имя элемента или сегмент пути ("", "a/b", ".", "..").
class InvalidNameError : public FsError {
public:
    using FsError::FsError;
};

// Элемент по указанному пути не существует.
class PathNotFoundError : public FsError {
public:
    using FsError::FsError;
};

// Путь проходит через элемент, который не является каталогом.
class NotADirectoryError : public FsError {
public:
    using FsError::FsError;
};

// В целевом каталоге уже есть элемент с таким именем.
class AlreadyExistsError : public FsError {
public:
    using FsError::FsError;
};

// Операция запрещена правилами ФС (удаление/перемещение корня,
// перемещение каталога внутрь самого себя и т.п.).
class ForbiddenOperationError : public FsError {
public:
    using FsError::FsError;
};

}  // namespace memfs
