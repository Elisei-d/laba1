#include "FileSystem.h"

#include <utility>

#include "Errors.h"

namespace memfs {

FileSystem::FileSystem(std::string rootName)
    : root_(std::make_unique<Directory>(std::move(rootName))) {}



const FsNode& FileSystem::resolve(const Path& path) const {
    const FsNode* current = root_.get();
    std::string walked;  
    for (const auto& segment : path.segments()) {
        const Directory* directory = current->asDirectory();
        if (directory == nullptr) {
            throw NotADirectoryError("'" + walked + "' не является каталогом");
        }
        current = directory->find(segment);
        walked += "/" + segment;
        if (current == nullptr) {
            throw PathNotFoundError("путь '" + walked + "' не найден");
        }
    }
    return *current;
}

FsNode& FileSystem::resolve(const Path& path) {
    return const_cast<FsNode&>(static_cast<const FileSystem&>(*this).resolve(path));
}

Directory& FileSystem::resolveDirectory(const Path& path) {
    Directory* directory = resolve(path).asDirectory();
    if (directory == nullptr) {
        throw NotADirectoryError("'" + path.toString() + "' не является каталогом");
    }
    return *directory;
}


const FsNode& FileSystem::find(const Path& path) const { return resolve(path); }

bool FileSystem::exists(const Path& path) const {
    try {
        resolve(path);
        return true;
    } catch (const PathNotFoundError&) {
        return false;
    } catch (const NotADirectoryError&) {
        return false;
    }
}

FsNode& FileSystem::add(const Path& parentDirectory, std::unique_ptr<FsNode> node) {
    return resolveDirectory(parentDirectory).add(std::move(node));
}

File& FileSystem::createFile(const Path& path, std::string content) {
    if (path.isRoot()) {
        throw ForbiddenOperationError("нельзя создать файл на месте корня");
    }
    auto file = std::make_unique<File>(path.name(), std::move(content));
    File& ref = *file;
    add(path.parent(), std::move(file));
    return ref;
}

Directory& FileSystem::createDirectory(const Path& path) {
    if (path.isRoot()) {
        throw ForbiddenOperationError("корневой каталог уже существует");
    }
    auto directory = std::make_unique<Directory>(path.name());
    Directory& ref = *directory;
    add(path.parent(), std::move(directory));
    return ref;
}

void FileSystem::remove(const Path& path) {
    if (path.isRoot()) {
        throw ForbiddenOperationError("удаление корневого каталога запрещено");
    }
    resolve(path);  
    resolveDirectory(path.parent()).remove(path.name());
}

void FileSystem::move(const Path& source, const Path& targetDirectory) {
    
    if (source.isRoot()) {
        throw ForbiddenOperationError("перемещение корневого каталога запрещено");
    }
    resolve(source);
    Directory& target = resolveDirectory(targetDirectory);
    if (source.isAncestorOrSelfOf(targetDirectory)) {
        throw ForbiddenOperationError("нельзя переместить '" + source.toString() +
                                      "' внутрь самого себя или своего потомка '" +
                                      targetDirectory.toString() + "'");
    }
    if (target.contains(source.name())) {
        throw AlreadyExistsError("в каталоге '" + targetDirectory.toString() +
                                 "' уже есть элемент '" + source.name() + "'");
    }
    Directory& from = resolveDirectory(source.parent());
    target.add(from.extract(source.name()));
}

std::size_t FileSystem::sizeOf(const Path& path) const { return resolve(path).size(); }

std::string FileSystem::tree() const { return root_->tree(); }

}  
