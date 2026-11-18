#include "Path.h"

#include <algorithm>
#include <utility>

#include "Errors.h"
#include "Name.h"

namespace memfs {

namespace {

std::vector<std::string> parse(const std::string& text) {
    std::vector<std::string> segments;
    std::string current;
    for (char c : text) {
        if (c == '/') {
            if (!current.empty()) {
                segments.push_back(std::move(current));
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        segments.push_back(std::move(current));
    }
    for (const auto& segment : segments) {
        validateName(segment);
    }
    return segments;
}

}  

Path::Path(const std::string& text) : segments_(parse(text)) {}

Path::Path(std::vector<std::string> segments) : segments_(std::move(segments)) {}

bool Path::isRoot() const noexcept { return segments_.empty(); }

std::size_t Path::depth() const noexcept { return segments_.size(); }

const std::vector<std::string>& Path::segments() const noexcept { return segments_; }

const std::string& Path::name() const {
    if (isRoot()) {
        throw ForbiddenOperationError("у корневого пути нет имени");
    }
    return segments_.back();
}

Path Path::parent() const {
    if (isRoot()) {
        throw ForbiddenOperationError("у корневого пути нет родителя");
    }
    return Path(std::vector<std::string>(segments_.begin(), segments_.end() - 1));
}

bool Path::isAncestorOrSelfOf(const Path& other) const noexcept {
    return segments_.size() <= other.segments_.size() &&
           std::equal(segments_.begin(), segments_.end(), other.segments_.begin());
}

std::string Path::toString() const {
    if (isRoot()) {
        return "/";
    }
    std::string result;
    for (const auto& segment : segments_) {
        result += '/';
        result += segment;
    }
    return result;
}

Path operator/(const Path& lhs, const Path& rhs) {
    std::vector<std::string> segments = lhs.segments_;
    segments.insert(segments.end(), rhs.segments_.begin(), rhs.segments_.end());
    return Path(std::move(segments));
}

Path operator/(const Path& lhs, const std::string& rhs) { return lhs / Path(rhs); }

bool operator==(const Path& lhs, const Path& rhs) noexcept {
    return lhs.segments_ == rhs.segments_;
}

bool operator!=(const Path& lhs, const Path& rhs) noexcept { return !(lhs == rhs); }

std::ostream& operator<<(std::ostream& out, const Path& path) {
    return out << path.toString();
}

}  
