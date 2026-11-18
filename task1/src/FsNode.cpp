#include "FsNode.h"

#include <utility>

#include "Name.h"

namespace memfs {

FsNode::FsNode(std::string name) : name_(std::move(name)) { validateName(name_); }

const std::string& FsNode::name() const noexcept { return name_; }

std::string FsNode::tree() const {
    std::string out = toString() + "\n";
    appendSubtree(out, "");
    return out;
}

void FsNode::appendSubtree(std::string& , const std::string& ) const {
   
}

Directory* FsNode::asDirectory() noexcept { return nullptr; }

const Directory* FsNode::asDirectory() const noexcept { return nullptr; }

std::ostream& operator<<(std::ostream& out, const FsNode& node) {
    return out << node.toString();
}

}  
