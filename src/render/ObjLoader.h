#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "render/Math.h"

namespace sim {

struct ObjMeshVertex {
    Vec3 position{};
    Vec3 normal{0.0F, 1.0F, 0.0F};
    Vec3 tangent{1.0F, 0.0F, 0.0F};
    std::array<float, 4> color{0.8F, 0.8F, 0.8F, 1.0F};
    float material = 0.0F;
};

struct ObjMesh {
    std::vector<ObjMeshVertex> vertices;
};

namespace detail {

struct ObjIndex {
    int position = 0;
    int normal = 0;
};

inline std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

inline int resolveObjIndex(int index, std::size_t count) {
    if (index > 0) {
        return index - 1;
    }
    if (index < 0) {
        return static_cast<int>(count) + index;
    }
    return -1;
}

inline ObjIndex parseFaceIndex(const std::string& token) {
    ObjIndex result;
    std::size_t firstSlash = token.find('/');
    std::size_t secondSlash = firstSlash == std::string::npos
                                  ? std::string::npos
                                  : token.find('/', firstSlash + 1);
    const std::string positionToken = firstSlash == std::string::npos
                                          ? token
                                          : token.substr(0, firstSlash);
    if (!positionToken.empty()) {
        result.position = std::stoi(positionToken);
    }
    if (secondSlash != std::string::npos && secondSlash + 1 < token.size()) {
        result.normal = std::stoi(token.substr(secondSlash + 1));
    }
    return result;
}

inline float materialForName(const std::string& rawName) {
    const std::string name = lowercase(rawName);
    if (name.find("asphalt") != std::string::npos) {
        return 1.0F;
    }
    if (name.find("grass") != std::string::npos) {
        return 2.0F;
    }
    if (name.find("concrete") != std::string::npos) {
        return 3.0F;
    }
    if (name.find("carbon") != std::string::npos ||
        name.find("floor") != std::string::npos ||
        name.find("wing") != std::string::npos ||
        name.find("splitter") != std::string::npos ||
        name.find("diffuser") != std::string::npos) {
        return 5.0F;
    }
    if (name.find("tire") != std::string::npos ||
        name.find("tyre") != std::string::npos ||
        name.find("rubber") != std::string::npos) {
        return 6.0F;
    }
    if (name.find("brake_disc") != std::string::npos ||
        name.find("disc") != std::string::npos) {
        return 12.0F;
    }
    if (name.find("brake_light") != std::string::npos) {
        return 11.0F;
    }
    if (name.find("visor") != std::string::npos ||
        name.find("glass") != std::string::npos) {
        return 16.0F;
    }
    if (name.find("metal") != std::string::npos ||
        name.find("rim") != std::string::npos ||
        name.find("hub") != std::string::npos ||
        name.find("spoke") != std::string::npos ||
        name.find("rod") != std::string::npos ||
        name.find("arm") != std::string::npos ||
        name.find("caliper") != std::string::npos ||
        name.find("suspension") != std::string::npos) {
        return 7.0F;
    }
    return 4.0F;
}

inline std::array<float, 4> colorForMaterial(float material, const std::string& rawName) {
    const std::string name = lowercase(rawName);
    if (material > 3.5F && material < 4.5F) {
        if (name.find("accent") != std::string::npos || name.find("stripe") != std::string::npos) {
            return {0.86F, 0.86F, 0.80F, 1.0F};
        }
        if (name.find("dark") != std::string::npos) {
            return {0.30F, 0.018F, 0.018F, 1.0F};
        }
        return {0.56F, 0.025F, 0.025F, 1.0F};
    }
    if (material > 4.5F && material < 5.5F) {
        return {0.018F, 0.020F, 0.024F, 1.0F};
    }
    if (material > 5.5F && material < 6.5F) {
        return {0.012F, 0.012F, 0.015F, 1.0F};
    }
    if (material > 6.5F && material < 7.5F) {
        if (name.find("caliper") != std::string::npos) {
            return {0.78F, 0.10F, 0.035F, 1.0F};
        }
        if (name.find("blue") != std::string::npos) {
            return {0.08F, 0.28F, 0.86F, 1.0F};
        }
        if (name.find("green") != std::string::npos) {
            return {0.10F, 0.70F, 0.24F, 1.0F};
        }
        if (name.find("yellow") != std::string::npos) {
            return {0.95F, 0.72F, 0.12F, 1.0F};
        }
        return {0.52F, 0.53F, 0.52F, 1.0F};
    }
    if (material > 10.5F && material < 11.5F) {
        return {1.0F, 0.035F, 0.02F, 1.0F};
    }
    if (material > 11.5F && material < 12.5F) {
        return {0.24F, 0.22F, 0.20F, 1.0F};
    }
    if (material > 15.5F && material < 16.5F) {
        return {0.015F, 0.055F, 0.085F, 1.0F};
    }
    return {0.8F, 0.8F, 0.8F, 1.0F};
}

}  // namespace detail

inline ObjMesh loadObjMesh(const std::string& path) {
    ObjMesh mesh;
    std::ifstream file(path);
    if (!file) {
        return mesh;
    }

    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::string currentMaterial = "paint";
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream stream(line);
        std::string tag;
        stream >> tag;
        if (tag == "v") {
            Vec3 position;
            stream >> position.x >> position.y >> position.z;
            positions.push_back(position);
        } else if (tag == "vn") {
            Vec3 normal;
            stream >> normal.x >> normal.y >> normal.z;
            normals.push_back(normalize(normal));
        } else if (tag == "usemtl" || tag == "g" || tag == "o") {
            std::string name;
            stream >> name;
            if (!name.empty()) {
                currentMaterial = name;
            }
        } else if (tag == "f") {
            std::vector<detail::ObjIndex> face;
            std::string token;
            while (stream >> token) {
                try {
                    face.push_back(detail::parseFaceIndex(token));
                } catch (...) {
                    face.clear();
                    break;
                }
            }
            if (face.size() < 3) {
                continue;
            }
            const float material = detail::materialForName(currentMaterial);
            const std::array<float, 4> color = detail::colorForMaterial(material, currentMaterial);
            const auto emitTriangle = [&](detail::ObjIndex ia, detail::ObjIndex ib, detail::ObjIndex ic) {
                const int pa = detail::resolveObjIndex(ia.position, positions.size());
                const int pb = detail::resolveObjIndex(ib.position, positions.size());
                const int pc = detail::resolveObjIndex(ic.position, positions.size());
                if (pa < 0 || pb < 0 || pc < 0 ||
                    pa >= static_cast<int>(positions.size()) ||
                    pb >= static_cast<int>(positions.size()) ||
                    pc >= static_cast<int>(positions.size())) {
                    return;
                }
                const Vec3 a = positions[static_cast<std::size_t>(pa)];
                const Vec3 b = positions[static_cast<std::size_t>(pb)];
                const Vec3 c = positions[static_cast<std::size_t>(pc)];
                const Vec3 faceNormal = normalize(cross(subtract(c, a), subtract(b, a)));
                const Vec3 tangent = normalize(subtract(b, a));
                const detail::ObjIndex indices[] = {ia, ib, ic};
                const Vec3 points[] = {a, b, c};
                for (int index = 0; index < 3; ++index) {
                    const int normalIndex = detail::resolveObjIndex(indices[index].normal, normals.size());
                    const Vec3 normal = normalIndex >= 0 && normalIndex < static_cast<int>(normals.size())
                                            ? normals[static_cast<std::size_t>(normalIndex)]
                                            : faceNormal;
                    mesh.vertices.push_back({points[index], normal, tangent, color, material});
                }
            };
            for (std::size_t index = 1; index + 1 < face.size(); ++index) {
                emitTriangle(face[0], face[index], face[index + 1]);
            }
        }
    }
    return mesh;
}

}  // namespace sim
