#include "platform/FileSystem.h"

#include <filesystem>

#include <SDL3/SDL.h>

namespace sim::FileSystem {

std::string findDataFile(const std::string& relativePath) {
    namespace fs = std::filesystem;

    const fs::path current = fs::current_path() / relativePath;
    if (fs::exists(current)) {
        return current.string();
    }

    if (const char* base = SDL_GetBasePath(); base != nullptr) {
        const fs::path executableBase(base);
        const fs::path besideExecutable = executableBase / relativePath;
        if (fs::exists(besideExecutable)) {
            return besideExecutable.string();
        }

        const fs::path bundleResource = executableBase / ".." / "Resources" / relativePath;
        if (fs::exists(bundleResource)) {
            return fs::weakly_canonical(bundleResource).string();
        }
    }

    return relativePath;
}

}  // namespace sim::FileSystem
