#include "File.h"

#include <utility>

namespace memfs {

File::File(std::string name, std::string content)
    : FsNode(std::move(name)), content_(std::move(content)) {}

const std::string& File::content() const noexcept { return content_; }

void File::write(std::string content) { content_ = std::move(content); }

void File::append(const std::string& data) { content_ += data; }

std::size_t File::size() const { return content_.size(); }

std::string File::toString() const {
    return name() + " (" + std::to_string(size()) + " B)";
}

}  
