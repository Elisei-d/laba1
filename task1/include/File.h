#pragma once

#include <cstddef>
#include <string>

#include "FsNode.h"

namespace memfs {

// Файл: элемент ФС с содержимым. Размер файла — размер содержимого в байтах.
// Содержимое скрыто (инкапсуляция) и меняется только через методы класса.
class File : public FsNode {
public:
    explicit File(std::string name, std::string content = {});

    const std::string& content() const noexcept;
    void write(std::string content);           // заменить содержимое
    void append(const std::string& data);      // дописать в конец

    std::size_t size() const override;
    std::string toString() const override;     // "report.txt (12 B)"

private:
    std::string content_;
};

}  // namespace memfs
