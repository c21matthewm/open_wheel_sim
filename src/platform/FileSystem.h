#pragma once

#include <string>

namespace sim::FileSystem {

[[nodiscard]] std::string findDataFile(const std::string& relativePath);

}  // namespace sim::FileSystem
