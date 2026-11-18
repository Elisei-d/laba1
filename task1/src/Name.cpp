#include "Name.h"

#include "Errors.h"

namespace memfs {

void validateName(const std::string& name) {
    if (name.empty()) {
        throw InvalidNameError("имя не может быть пустым");
    }
    if (name.find('/') != std::string::npos) {
        throw InvalidNameError("имя '" + name + "' не может содержать '/'");
    }
    if (name == "." || name == "..") {
        throw InvalidNameError("имена '.' и '..' зарезервированы");
    }
}

}  