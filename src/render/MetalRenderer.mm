#include "render/MetalRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <numbers>
#include <string>
#include <vector>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_metal.h>

#include "game/Track.h"
#include "platform/FileSystem.h"
#include "render/ImageLoader.h"
#include "platform/WindowSDL.h"
#include "render/Math.h"
#include "render/ObjLoader.h"
#include "ui/DebugOverlay.h"

namespace sim {
namespace {

struct Vertex {
    float position[4];
    float color[4];
    float data[4];
    float tex[4];
};
static_assert(sizeof(Vertex) == 64);

struct Uniforms {
    float mvp[16];
    float model[16];
    float shadowMvp[16];
    float cameraPosition[4];
    float cameraRight[4];
    float cameraUp[4];
    float cameraForward[4];
    float renderParams[4];
    float effectParams[4];
    float contactParams[4];
};

constexpr float kMaterialDefault = 0.0F;
constexpr float kMaterialAsphalt = 1.0F;
constexpr float kMaterialGrass = 2.0F;
constexpr float kMaterialConcrete = 3.0F;
constexpr float kMaterialPaint = 4.0F;
constexpr float kMaterialCarbon = 5.0F;
constexpr float kMaterialTire = 6.0F;
constexpr float kMaterialMetal = 7.0F;
constexpr float kMaterialFence = 8.0F;
constexpr float kMaterialCrowd = 9.0F;
constexpr float kMaterialTrackPaint = 10.0F;
constexpr float kMaterialBrakeLight = 11.0F;
constexpr float kMaterialBrakeDisc = 12.0F;
constexpr float kMaterialSmoke = 13.0F;
constexpr float kMaterialSpark = 14.0F;
constexpr float kMaterialSkidmark = 15.0F;
constexpr float kMaterialVisor = 16.0F;

constexpr float kVisualFrontWheelX = 0.84F;
constexpr float kVisualRearWheelX = 0.80F;
constexpr float kVisualFrontWheelZ = 1.300F;
constexpr float kVisualRearWheelZ = -1.735F;
constexpr float kVisualWheelY = 0.34F;
constexpr float kVisualFrontWheelWidthScale = 0.72F;
constexpr float kVisualFrontWheelRadiusScale = 0.944F;

constexpr std::size_t kMaxTextVertices = DebugOverlay::kMaxLines * 128U * 6U;
constexpr std::size_t kMaxSteeringDisplayVertices = 512U;
constexpr std::size_t kMaxUiVertices = 7200U;
constexpr std::size_t kMaxSmokeVertices = 144U;
constexpr std::size_t kMaxSuspensionVertices = 900U;
constexpr std::size_t kMaxSkidmarkVertices = 3072U;
constexpr NSUInteger kWorldSampleCount = 4U;
using Glyph = std::array<unsigned char, 7>;

Glyph glyphFor(char character) {
    if (character >= 'a' && character <= 'z') {
        character = static_cast<char>(character - 'a' + 'A');
    }
    switch (character) {
        case 'A': return {14, 17, 17, 31, 17, 17, 17};
        case 'B': return {30, 17, 17, 30, 17, 17, 30};
        case 'C': return {14, 17, 16, 16, 16, 17, 14};
        case 'D': return {30, 17, 17, 17, 17, 17, 30};
        case 'E': return {31, 16, 16, 30, 16, 16, 31};
        case 'F': return {31, 16, 16, 30, 16, 16, 16};
        case 'G': return {14, 17, 16, 23, 17, 17, 15};
        case 'H': return {17, 17, 17, 31, 17, 17, 17};
        case 'I': return {14, 4, 4, 4, 4, 4, 14};
        case 'J': return {7, 2, 2, 2, 18, 18, 12};
        case 'K': return {17, 18, 20, 24, 20, 18, 17};
        case 'L': return {16, 16, 16, 16, 16, 16, 31};
        case 'M': return {17, 27, 21, 21, 17, 17, 17};
        case 'N': return {17, 25, 21, 19, 17, 17, 17};
        case 'O': return {14, 17, 17, 17, 17, 17, 14};
        case 'P': return {30, 17, 17, 30, 16, 16, 16};
        case 'Q': return {14, 17, 17, 17, 21, 18, 13};
        case 'R': return {30, 17, 17, 30, 20, 18, 17};
        case 'S': return {15, 16, 16, 14, 1, 1, 30};
        case 'T': return {31, 4, 4, 4, 4, 4, 4};
        case 'U': return {17, 17, 17, 17, 17, 17, 14};
        case 'V': return {17, 17, 17, 17, 17, 10, 4};
        case 'W': return {17, 17, 17, 21, 21, 21, 10};
        case 'X': return {17, 17, 10, 4, 10, 17, 17};
        case 'Y': return {17, 17, 10, 4, 4, 4, 4};
        case 'Z': return {31, 1, 2, 4, 8, 16, 31};
        case '0': return {14, 17, 19, 21, 25, 17, 14};
        case '1': return {4, 12, 4, 4, 4, 4, 14};
        case '2': return {14, 17, 1, 2, 4, 8, 31};
        case '3': return {30, 1, 1, 14, 1, 1, 30};
        case '4': return {2, 6, 10, 18, 31, 2, 2};
        case '5': return {31, 16, 16, 30, 1, 1, 30};
        case '6': return {14, 16, 16, 30, 17, 17, 14};
        case '7': return {31, 1, 2, 4, 8, 8, 8};
        case '8': return {14, 17, 17, 14, 17, 17, 14};
        case '9': return {14, 17, 17, 15, 1, 1, 14};
        case '.': return {0, 0, 0, 0, 0, 6, 6};
        case ',': return {0, 0, 0, 0, 6, 6, 4};
        case ':': return {0, 6, 6, 0, 6, 6, 0};
        case ';': return {0, 6, 6, 0, 6, 6, 4};
        case '-': return {0, 0, 0, 31, 0, 0, 0};
        case '+': return {0, 4, 4, 31, 4, 4, 0};
        case '/': return {1, 2, 2, 4, 8, 8, 16};
        case '\\': return {16, 8, 8, 4, 2, 2, 1};
        case '%': return {17, 2, 4, 8, 16, 17, 0};
        case '[': return {14, 8, 8, 8, 8, 8, 14};
        case ']': return {14, 2, 2, 2, 2, 2, 14};
        case '(': return {2, 4, 8, 8, 8, 4, 2};
        case ')': return {8, 4, 2, 2, 2, 4, 8};
        case '_': return {0, 0, 0, 0, 0, 0, 31};
        case '|': return {4, 4, 4, 4, 4, 4, 4};
        case '=': return {0, 0, 31, 0, 31, 0, 0};
        case '?': return {14, 17, 1, 2, 4, 0, 4};
        case '!': return {4, 4, 4, 4, 4, 0, 4};
        case '#': return {10, 31, 10, 10, 31, 10, 0};
        case '\'': return {4, 4, 2, 0, 0, 0, 0};
        default: return {};
    }
}

Vertex makeVertex(
    float x,
    float y,
    float z,
    std::array<float, 4> color,
    Vec3 normal,
    float material = kMaterialDefault,
    Vec3 tangent = {1.0F, 0.0F, 0.0F},
    float trackLateral = 0.0F) {
    const Vec3 normalized = normalize(normal);
    Vec3 tangentOrtho = subtract(tangent, {normalized.x * dot(tangent, normalized), normalized.y * dot(tangent, normalized), normalized.z * dot(tangent, normalized)});
    tangentOrtho = normalize(tangentOrtho);
    return {
        {x, y, z, 1.0F},
        {color[0], color[1], color[2], color[3]},
        {normalized.x, normalized.y, normalized.z, material},
        {tangentOrtho.x, tangentOrtho.y, tangentOrtho.z, trackLateral},
    };
}

Vertex makeEffectVertex(
    Vec3 position,
    std::array<float, 4> color,
    Vec3 normal,
    float material,
    float u,
    float v) {
    Vertex vertex = makeVertex(position.x, position.y, position.z, color, normal, material);
    vertex.tex[0] = u;
    vertex.tex[1] = v;
    vertex.tex[2] = 0.0F;
    vertex.tex[3] = 0.0F;
    return vertex;
}

Vertex makeUiVertex(float x, float y, float z, std::array<float, 4> color) {
    return {{x, y, z, 1.0F}, {color[0], color[1], color[2], color[3]}, {0.0F, 0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F, 0.0F}};
}

Vertex makeTextVertex(float x, float y, float z, std::array<float, 4> color, float u, float v) {
    return {{x, y, z, 1.0F}, {color[0], color[1], color[2], color[3]}, {u, v, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F, 0.0F}};
}

struct ClipPoint {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    float w = 1.0F;
};

ClipPoint transformPoint(const Mat4& matrix, Vec3 point) {
    return {
        matrix.values[0] * point.x + matrix.values[4] * point.y +
            matrix.values[8] * point.z + matrix.values[12],
        matrix.values[1] * point.x + matrix.values[5] * point.y +
            matrix.values[9] * point.z + matrix.values[13],
        matrix.values[2] * point.x + matrix.values[6] * point.y +
            matrix.values[10] * point.z + matrix.values[14],
        matrix.values[3] * point.x + matrix.values[7] * point.y +
            matrix.values[11] * point.z + matrix.values[15],
    };
}

void appendQuad(
    std::vector<Vertex>& vertices,
    Vec3 a,
    Vec3 b,
    Vec3 c,
    Vec3 d,
    std::array<float, 4> color,
        float material = kMaterialDefault) {
    const Vec3 normal = normalize(cross(subtract(c, a), subtract(b, a)));
    const Vec3 tangent = normalize(subtract(b, a));
    vertices.push_back(makeVertex(a.x, a.y, a.z, color, normal, material, tangent));
    vertices.push_back(makeVertex(b.x, b.y, b.z, color, normal, material, tangent));
    vertices.push_back(makeVertex(c.x, c.y, c.z, color, normal, material, tangent));
    vertices.push_back(makeVertex(a.x, a.y, a.z, color, normal, material, tangent));
    vertices.push_back(makeVertex(c.x, c.y, c.z, color, normal, material, tangent));
    vertices.push_back(makeVertex(d.x, d.y, d.z, color, normal, material, tangent));
}

void appendBox(
    std::vector<Vertex>& vertices,
    Vec3 minimum,
    Vec3 maximum,
    std::array<float, 4> color,
    float material = kMaterialDefault) {
    const Vec3 p000{minimum.x, minimum.y, minimum.z};
    const Vec3 p001{minimum.x, minimum.y, maximum.z};
    const Vec3 p010{minimum.x, maximum.y, minimum.z};
    const Vec3 p011{minimum.x, maximum.y, maximum.z};
    const Vec3 p100{maximum.x, minimum.y, minimum.z};
    const Vec3 p101{maximum.x, minimum.y, maximum.z};
    const Vec3 p110{maximum.x, maximum.y, minimum.z};
    const Vec3 p111{maximum.x, maximum.y, maximum.z};
    appendQuad(vertices, p001, p101, p111, p011, color, material);
    appendQuad(vertices, p100, p000, p010, p110, color, material);
    appendQuad(vertices, p000, p001, p011, p010, color, material);
    appendQuad(vertices, p101, p100, p110, p111, color, material);
    appendQuad(vertices, p010, p011, p111, p110, color, material);
    appendQuad(vertices, p000, p100, p101, p001, color, material);
}

void appendTaperedBox(
    std::vector<Vertex>& vertices,
    float rearZ,
    float frontZ,
    float rearHalfWidth,
    float frontHalfWidth,
    float bottomY,
    float topY,
    std::array<float, 4> color,
    float material = kMaterialDefault) {
    const Vec3 rearLeftBottom{-rearHalfWidth, bottomY, rearZ};
    const Vec3 rearRightBottom{rearHalfWidth, bottomY, rearZ};
    const Vec3 rearLeftTop{-rearHalfWidth, topY, rearZ};
    const Vec3 rearRightTop{rearHalfWidth, topY, rearZ};
    const Vec3 frontLeftBottom{-frontHalfWidth, bottomY, frontZ};
    const Vec3 frontRightBottom{frontHalfWidth, bottomY, frontZ};
    const Vec3 frontLeftTop{-frontHalfWidth, topY, frontZ};
    const Vec3 frontRightTop{frontHalfWidth, topY, frontZ};
    appendQuad(vertices, rearLeftBottom, rearRightBottom, rearRightTop, rearLeftTop, color, material);
    appendQuad(vertices, frontRightBottom, frontLeftBottom, frontLeftTop, frontRightTop, color, material);
    appendQuad(vertices, rearRightBottom, frontRightBottom, frontRightTop, rearRightTop, color, material);
    appendQuad(vertices, frontLeftBottom, rearLeftBottom, rearLeftTop, frontLeftTop, color, material);
    appendQuad(vertices, rearLeftTop, rearRightTop, frontRightTop, frontLeftTop, color, material);
    appendQuad(vertices, frontLeftBottom, frontRightBottom, rearRightBottom, rearLeftBottom, color, material);
}

void appendCylinderX(
    std::vector<Vertex>& vertices,
    Vec3 center,
    float halfWidth,
    float radiusY,
    float radiusZ,
    int segments,
    std::array<float, 4> color,
    float material = kMaterialDefault) {
    const Vec3 leftCenter{center.x - halfWidth, center.y, center.z};
    const Vec3 rightCenter{center.x + halfWidth, center.y, center.z};
    for (int segment = 0; segment < segments; ++segment) {
        const float a0 = std::numbers::pi_v<float> * 2.0F *
                         static_cast<float>(segment) / static_cast<float>(segments);
        const float a1 = std::numbers::pi_v<float> * 2.0F *
                         static_cast<float>(segment + 1) / static_cast<float>(segments);
        const Vec3 left0{
            center.x - halfWidth,
            center.y + std::cos(a0) * radiusY,
            center.z + std::sin(a0) * radiusZ,
        };
        const Vec3 right0{
            center.x + halfWidth,
            center.y + std::cos(a0) * radiusY,
            center.z + std::sin(a0) * radiusZ,
        };
        const Vec3 left1{
            center.x - halfWidth,
            center.y + std::cos(a1) * radiusY,
            center.z + std::sin(a1) * radiusZ,
        };
        const Vec3 right1{
            center.x + halfWidth,
            center.y + std::cos(a1) * radiusY,
            center.z + std::sin(a1) * radiusZ,
        };
        const Vec3 normal0{0.0F, std::cos(a0), std::sin(a0)};
        const Vec3 normal1{0.0F, std::cos(a1), std::sin(a1)};
        constexpr Vec3 treadTangent{1.0F, 0.0F, 0.0F};
        vertices.push_back(makeVertex(left0.x, left0.y, left0.z, color, normal0, material, treadTangent));
        vertices.push_back(makeVertex(right0.x, right0.y, right0.z, color, normal0, material, treadTangent));
        vertices.push_back(makeVertex(right1.x, right1.y, right1.z, color, normal1, material, treadTangent));
        vertices.push_back(makeVertex(left0.x, left0.y, left0.z, color, normal0, material, treadTangent));
        vertices.push_back(makeVertex(right1.x, right1.y, right1.z, color, normal1, material, treadTangent));
        vertices.push_back(makeVertex(left1.x, left1.y, left1.z, color, normal1, material, treadTangent));

        constexpr Vec3 leftCapNormal{-1.0F, 0.0F, 0.0F};
        constexpr Vec3 rightCapNormal{1.0F, 0.0F, 0.0F};
        constexpr Vec3 capTangent{0.0F, 1.0F, 0.0F};
        vertices.push_back(makeVertex(leftCenter.x, leftCenter.y, leftCenter.z, color, leftCapNormal, material, capTangent));
        vertices.push_back(makeVertex(left1.x, left1.y, left1.z, color, leftCapNormal, material, capTangent));
        vertices.push_back(makeVertex(left0.x, left0.y, left0.z, color, leftCapNormal, material, capTangent));
        vertices.push_back(makeVertex(rightCenter.x, rightCenter.y, rightCenter.z, color, rightCapNormal, material, capTangent));
        vertices.push_back(makeVertex(right0.x, right0.y, right0.z, color, rightCapNormal, material, capTangent));
        vertices.push_back(makeVertex(right1.x, right1.y, right1.z, color, rightCapNormal, material, capTangent));
    }
}

void appendRod(
    std::vector<Vertex>& vertices,
    Vec3 from,
    Vec3 to,
    float halfWidth,
    std::array<float, 4> color,
    float material = kMaterialDefault) {
    const Vec3 direction = subtract(to, from);
    Vec3 side = normalize({-direction.z, 0.0F, direction.x});
    if (std::abs(side.x) + std::abs(side.z) < 0.001F) {
        side = {1.0F, 0.0F, 0.0F};
    }
    const Vec3 up{0.0F, 1.0F, 0.0F};
    const auto point = [](Vec3 base, Vec3 axisA, float scaleA, Vec3 axisB, float scaleB) {
        return Vec3{
            base.x + axisA.x * scaleA + axisB.x * scaleB,
            base.y + axisA.y * scaleA + axisB.y * scaleB,
            base.z + axisA.z * scaleA + axisB.z * scaleB,
        };
    };

    const Vec3 a0 = point(from, side, -halfWidth, up, -halfWidth);
    const Vec3 a1 = point(from, side, halfWidth, up, -halfWidth);
    const Vec3 a2 = point(from, side, halfWidth, up, halfWidth);
    const Vec3 a3 = point(from, side, -halfWidth, up, halfWidth);
    const Vec3 b0 = point(to, side, -halfWidth, up, -halfWidth);
    const Vec3 b1 = point(to, side, halfWidth, up, -halfWidth);
    const Vec3 b2 = point(to, side, halfWidth, up, halfWidth);
    const Vec3 b3 = point(to, side, -halfWidth, up, halfWidth);
    appendQuad(vertices, a0, a1, b1, b0, color, material);
    appendQuad(vertices, a1, a2, b2, b1, color, material);
    appendQuad(vertices, a2, a3, b3, b2, color, material);
    appendQuad(vertices, a3, a0, b0, b3, color, material);
    appendQuad(vertices, a3, a2, a1, a0, color, material);
    appendQuad(vertices, b0, b1, b2, b3, color, material);
}

Vec3 pointAtTrackOffset(
    const TrackPose& pose,
    float lateralOffsetM,
    float verticalOffsetM = 0.0F,
    float forwardOffsetM = 0.0F) {
    float heightOffset = lateralOffsetM + pose.roadHalfWidthM;
    if (lateralOffsetM < -pose.roadHalfWidthM) {
        heightOffset = 0.0F;
    }
    const float heightY = heightOffset * std::tan(pose.bankRadians) + verticalOffsetM;
    return {
        pose.centerX + pose.rightX * lateralOffsetM + pose.tangentX * forwardOffsetM,
        heightY,
        pose.centerZ + pose.rightZ * lateralOffsetM + pose.tangentZ * forwardOffsetM,
    };
}

void appendTrackStrip(
    std::vector<Vertex>& vertices,
    const TrackPose& start,
    const TrackPose& end,
    float innerOffsetM,
    float outerOffsetM,
    float verticalOffsetM,
    std::array<float, 4> color,
    float material = kMaterialDefault,
    float innerLateralV = 0.0F,
    float outerLateralV = 0.0F) {
    const Vec3 a = pointAtTrackOffset(start, innerOffsetM, verticalOffsetM);
    const Vec3 b = pointAtTrackOffset(start, outerOffsetM, verticalOffsetM);
    const Vec3 c = pointAtTrackOffset(end, outerOffsetM, verticalOffsetM);
    const Vec3 d = pointAtTrackOffset(end, innerOffsetM, verticalOffsetM);
    const Vec3 normal = normalize(cross(subtract(c, a), subtract(b, a)));
    const Vec3 tangent = normalize(subtract(b, a));
    vertices.push_back(makeVertex(a.x, a.y, a.z, color, normal, material, tangent, innerLateralV));
    vertices.push_back(makeVertex(b.x, b.y, b.z, color, normal, material, tangent, outerLateralV));
    vertices.push_back(makeVertex(c.x, c.y, c.z, color, normal, material, tangent, outerLateralV));
    vertices.push_back(makeVertex(a.x, a.y, a.z, color, normal, material, tangent, innerLateralV));
    vertices.push_back(makeVertex(c.x, c.y, c.z, color, normal, material, tangent, outerLateralV));
    vertices.push_back(makeVertex(d.x, d.y, d.z, color, normal, material, tangent, innerLateralV));
}

void appendTrackMarker(
    std::vector<Vertex>& vertices,
    const Track& track,
    float distanceM,
    std::array<float, 4> color,
    float material = kMaterialTrackPaint) {
    const TrackPose pose = track.poseAtDistance(distanceM);
    const float halfWidth = track.roadHalfWidthM();
    constexpr float halfLength = 0.45F;
    constexpr float markerHeight = 0.055F;
    appendQuad(
        vertices,
        pointAtTrackOffset(pose, -halfWidth, markerHeight, -halfLength),
        pointAtTrackOffset(pose, halfWidth, markerHeight, -halfLength),
        pointAtTrackOffset(pose, halfWidth, markerHeight, halfLength),
        pointAtTrackOffset(pose, -halfWidth, markerHeight, halfLength),
        color,
        material);
}

void appendStartFinishChecker(std::vector<Vertex>& vertices, const Track& track) {
    const TrackPose pose = track.poseAtDistance(0.0F);
    const float halfWidth = track.roadHalfWidthM();
    constexpr int columns = 12;
    constexpr int rows = 2;
    constexpr float markerHeight = 0.06F;
    constexpr float markerLength = 1.35F;
    const float cellWidth = halfWidth * 2.0F / static_cast<float>(columns);
    const float cellLength = markerLength / static_cast<float>(rows);
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const float inner = -halfWidth + static_cast<float>(column) * cellWidth;
            const float outer = inner + cellWidth;
            const float start = -markerLength * 0.5F + static_cast<float>(row) * cellLength;
            const float end = start + cellLength;
            const bool bright = ((row + column) % 2) == 0;
            const std::array<float, 4> color =
                bright ? std::array<float, 4>{0.95F, 0.95F, 0.9F, 1.0F}
                       : std::array<float, 4>{0.03F, 0.035F, 0.04F, 1.0F};
            appendQuad(
                vertices,
                pointAtTrackOffset(pose, inner, markerHeight, start),
                pointAtTrackOffset(pose, outer, markerHeight, start),
                pointAtTrackOffset(pose, outer, markerHeight, end),
                pointAtTrackOffset(pose, inner, markerHeight, end),
                color,
                kMaterialTrackPaint);
        }
    }
}

void appendTrackOrientedBox(
    std::vector<Vertex>& vertices,
    const TrackPose& pose,
    float lateralCenterM,
    float verticalCenterM,
    float forwardHalfM,
    float lateralHalfM,
    float verticalHalfM,
    std::array<float, 4> color,
    float material = kMaterialDefault) {
    const auto corner = [&](float lateral, float vertical, float forward) {
        return pointAtTrackOffset(
            pose,
            lateralCenterM + lateral,
            verticalCenterM + vertical,
            forward);
    };
    const Vec3 p000 = corner(-lateralHalfM, -verticalHalfM, -forwardHalfM);
    const Vec3 p001 = corner(-lateralHalfM, -verticalHalfM, forwardHalfM);
    const Vec3 p010 = corner(-lateralHalfM, verticalHalfM, -forwardHalfM);
    const Vec3 p011 = corner(-lateralHalfM, verticalHalfM, forwardHalfM);
    const Vec3 p100 = corner(lateralHalfM, -verticalHalfM, -forwardHalfM);
    const Vec3 p101 = corner(lateralHalfM, -verticalHalfM, forwardHalfM);
    const Vec3 p110 = corner(lateralHalfM, verticalHalfM, -forwardHalfM);
    const Vec3 p111 = corner(lateralHalfM, verticalHalfM, forwardHalfM);
    appendQuad(vertices, p001, p101, p111, p011, color, material);
    appendQuad(vertices, p100, p000, p010, p110, color, material);
    appendQuad(vertices, p000, p001, p011, p010, color, material);
    appendQuad(vertices, p101, p100, p110, p111, color, material);
    appendQuad(vertices, p010, p011, p111, p110, color, material);
    appendQuad(vertices, p000, p100, p101, p001, color, material);
}

void appendTrackVerticalBand(
    std::vector<Vertex>& vertices,
    const TrackPose& start,
    const TrackPose& end,
    float lateralOffsetM,
    float bottomM,
    float topM,
    std::array<float, 4> color,
    float material = kMaterialDefault) {
    appendQuad(
        vertices,
        pointAtTrackOffset(start, lateralOffsetM, bottomM),
        pointAtTrackOffset(end, lateralOffsetM, bottomM),
        pointAtTrackOffset(end, lateralOffsetM, topM),
        pointAtTrackOffset(start, lateralOffsetM, topM),
        color,
        material);
}

void appendTrackDiagonalBand(
    std::vector<Vertex>& vertices,
    const TrackPose& start,
    const TrackPose& end,
    float lateralOffsetM,
    float startHeightM,
    float endHeightM,
    float thicknessM,
    std::array<float, 4> color,
    float material = kMaterialDefault) {
    appendQuad(
        vertices,
        pointAtTrackOffset(start, lateralOffsetM, startHeightM),
        pointAtTrackOffset(start, lateralOffsetM, startHeightM + thicknessM),
        pointAtTrackOffset(end, lateralOffsetM, endHeightM + thicknessM),
        pointAtTrackOffset(end, lateralOffsetM, endHeightM),
        color,
        material);
}

void appendCatchFencePanel(
    std::vector<Vertex>& vertices,
    const TrackPose& start,
    const TrackPose& end,
    float wallOffsetM,
    float wallHeightM,
    int segment) {
    const float fenceSide = wallOffsetM >= 0.0F ? 1.0F : -1.0F;
    const float fenceOffset = wallOffsetM + fenceSide * 0.42F;
    const float fenceBottom = wallHeightM + 0.08F;
    constexpr float fenceHeight = 4.4F;
    constexpr float railThickness = 0.045F;
    const std::array<float, 4> railColor{0.47F, 0.50F, 0.53F, 1.0F};
    const std::array<float, 4> wireColor{0.32F, 0.35F, 0.38F, 1.0F};

    appendTrackVerticalBand(
        vertices, start, end, fenceOffset, fenceBottom, fenceBottom + railThickness, railColor, kMaterialMetal);
    appendTrackVerticalBand(
        vertices, start, end, fenceOffset, fenceBottom + 1.35F, fenceBottom + 1.35F + railThickness, railColor, kMaterialMetal);
    appendTrackVerticalBand(
        vertices, start, end, fenceOffset, fenceBottom + 2.7F, fenceBottom + 2.7F + railThickness, railColor, kMaterialMetal);
    appendTrackVerticalBand(
        vertices, start, end, fenceOffset, fenceBottom + fenceHeight, fenceBottom + fenceHeight + railThickness, railColor, kMaterialMetal);

    appendTrackDiagonalBand(
        vertices, start, end, fenceOffset + 0.02F, fenceBottom + 0.15F, fenceBottom + 1.25F, 0.026F, wireColor, kMaterialFence);
    appendTrackDiagonalBand(
        vertices, start, end, fenceOffset + 0.02F, fenceBottom + 1.45F, fenceBottom + 2.55F, 0.026F, wireColor, kMaterialFence);
    appendTrackDiagonalBand(
        vertices, start, end, fenceOffset + 0.02F, fenceBottom + 2.75F, fenceBottom + 3.95F, 0.026F, wireColor, kMaterialFence);

    if (segment % 4 == 0) {
        appendTrackOrientedBox(
            vertices,
            start,
            fenceOffset,
            fenceBottom + fenceHeight * 0.5F,
            0.11F,
            0.06F,
            fenceHeight * 0.5F,
            {0.55F, 0.57F, 0.60F, 1.0F},
            kMaterialMetal);
    }
}

void appendGrandstandSection(
    std::vector<Vertex>& vertices,
    const TrackPose& pose,
    float wallOffsetM) {
    const std::array<float, 4> concrete{0.42F, 0.43F, 0.42F, 1.0F};
    const std::array<float, 4> seatA{0.18F, 0.22F, 0.28F, 1.0F};
    const std::array<float, 4> seatB{0.26F, 0.27F, 0.27F, 1.0F};
    const std::array<float, 4> rail{0.68F, 0.70F, 0.70F, 1.0F};
    const std::array<float, 4> shadow{0.20F, 0.21F, 0.22F, 1.0F};

    constexpr float sectionHalfLength = 17.0F;
    const float frontLateral = wallOffsetM + 18.0F;
    appendTrackOrientedBox(
        vertices, pose, frontLateral + 8.0F, 0.25F, sectionHalfLength, 8.6F, 0.22F, concrete, kMaterialConcrete);
    for (int row = 0; row < 7; ++row) {
        const float lateral = frontLateral + static_cast<float>(row) * 2.15F;
        const float height = 0.75F + static_cast<float>(row) * 0.45F;
        appendTrackOrientedBox(
            vertices,
            pose,
            lateral,
            height,
            sectionHalfLength,
            0.72F,
            0.08F,
            row % 2 == 0 ? seatA : seatB,
            kMaterialCrowd);
        appendTrackOrientedBox(
            vertices,
            pose,
            lateral + 0.82F,
            height - 0.22F,
            sectionHalfLength,
            0.06F,
            std::max(0.12F, height - 0.25F),
            shadow,
            kMaterialConcrete);
    }
    for (int row = 1; row < 7; ++row) {
        const float lateral = frontLateral + static_cast<float>(row) * 2.15F - 0.34F;
        const float height = 0.96F + static_cast<float>(row) * 0.45F;
        for (int column = 0; column < 11; ++column) {
            const float forward = -sectionHalfLength + 2.8F + static_cast<float>(column) * 3.1F;
            const float shade = 0.12F + 0.06F * static_cast<float>((row + column) % 4);
            const std::array<float, 4> crowd{
                0.18F + shade,
                0.20F + 0.05F * static_cast<float>((column + 2) % 3),
                0.24F + 0.05F * static_cast<float>(row % 3),
                1.0F,
            };
            appendQuad(
                vertices,
                pointAtTrackOffset(pose, lateral, height - 0.18F, forward - 0.42F),
                pointAtTrackOffset(pose, lateral, height - 0.18F, forward + 0.42F),
                pointAtTrackOffset(pose, lateral, height + 0.22F, forward + 0.42F),
                pointAtTrackOffset(pose, lateral, height + 0.22F, forward - 0.42F),
                crowd,
                kMaterialCrowd);
        }
    }
    appendTrackOrientedBox(
        vertices,
        pose,
        frontLateral + 14.8F,
        2.55F,
        sectionHalfLength,
        0.12F,
        2.35F,
        concrete,
        kMaterialConcrete);
    appendTrackOrientedBox(
        vertices,
        pose,
        frontLateral + 7.0F,
        4.15F,
        sectionHalfLength,
        7.8F,
        0.12F,
        shadow,
        kMaterialConcrete);
    appendTrackOrientedBox(
        vertices,
        pose,
        frontLateral - 1.1F,
        1.55F,
        sectionHalfLength,
        0.05F,
        1.35F,
        rail,
        kMaterialMetal);
}

void appendGrandstands(std::vector<Vertex>& vertices, const Track& track) {
    const TrackConfig& config = track.config();
    const float halfStraight = track.halfStraightLengthM();
    const float quarterCorner = std::numbers::pi_v<float> * 0.5F * config.cornerRadiusM;
    const float backStraightStart =
        halfStraight + 2.0F * quarterCorner + config.shortChuteLengthM;
    const float sectionSpacing = 42.0F;

    for (float z = -halfStraight + 95.0F; z <= halfStraight - 95.0F; z += sectionSpacing) {
        const float distance = z >= 0.0F ? z : track.lapLengthM() + z;
        appendGrandstandSection(vertices, track.poseAtDistance(distance), track.outerWallOffsetM());
    }
    for (float z = halfStraight - 95.0F; z >= -halfStraight + 95.0F; z -= sectionSpacing) {
        const float distance = backStraightStart + (halfStraight - z);
        appendGrandstandSection(vertices, track.poseAtDistance(distance), track.outerWallOffsetM());
    }
}

std::vector<Vertex> makeSkyGeometry() {
    std::vector<Vertex> vertices;
    vertices.reserve(6);
    const std::array<float, 4> white{1.0F, 1.0F, 1.0F, 1.0F};
    vertices.push_back(makeTextVertex(-1.0F, -1.0F, 0.0F, white, 0.0F, 1.0F));
    vertices.push_back(makeTextVertex(1.0F, -1.0F, 0.0F, white, 1.0F, 1.0F));
    vertices.push_back(makeTextVertex(1.0F, 1.0F, 0.0F, white, 1.0F, 0.0F));
    vertices.push_back(makeTextVertex(-1.0F, -1.0F, 0.0F, white, 0.0F, 1.0F));
    vertices.push_back(makeTextVertex(1.0F, 1.0F, 0.0F, white, 1.0F, 0.0F));
    vertices.push_back(makeTextVertex(-1.0F, 1.0F, 0.0F, white, 0.0F, 0.0F));
    return vertices;
}

std::vector<Vertex> makeGroundGeometry(const Track& track) {
    std::vector<Vertex> vertices;
    const TrackConfig& config = track.config();
    const int segments = config.renderSegments;
    vertices.reserve(static_cast<std::size_t>(segments) * 150U + 30000U);

    const float xExtent =
        config.shortChuteLengthM * 0.5F + config.cornerRadiusM +
        track.roadHalfWidthM() + config.outerWallGapM + 180.0F;
    const float zExtent =
        track.halfStraightLengthM() + config.cornerRadiusM + 180.0F;
    appendQuad(
        vertices,
        {-xExtent, -0.06F, -zExtent},
        {xExtent, -0.06F, -zExtent},
        {xExtent, -0.06F, zExtent},
        {-xExtent, -0.06F, zExtent},
        {0.055F, 0.22F, 0.085F, 1.0F},
        kMaterialGrass);

    const float halfRoad = track.roadHalfWidthM();
    const float innerApron = -halfRoad - config.apronWidthM;
    const float wallOffset = track.outerWallOffsetM();
    const float innerWallOffset = track.innerBarrierOffsetM();
    const float lapLength = track.lapLengthM();
    const auto lateralV = [&](float lateralOffsetM) {
        const float t = (lateralOffsetM - innerWallOffset) / std::max(wallOffset - innerWallOffset, 0.001F);
        return std::clamp(t * 2.0F - 1.0F, -1.0F, 1.0F);
    };
    const std::array<float, 4> roadColor{0.115F, 0.118F, 0.123F, 1.0F};
    const std::array<float, 4> apronColor{0.17F, 0.172F, 0.168F, 1.0F};
    const std::array<float, 4> shoulderColor{0.12F, 0.22F, 0.1F, 1.0F};
    const std::array<float, 4> wallColor{0.86F, 0.86F, 0.82F, 1.0F};

    for (int segment = 0; segment < segments; ++segment) {
        const float startDistance =
            lapLength * static_cast<float>(segment) / static_cast<float>(segments);
        const float endDistance =
            lapLength * static_cast<float>(segment + 1) / static_cast<float>(segments);
        const TrackPose start = track.poseAtDistance(startDistance);
        const TrackPose end = track.poseAtDistance(endDistance);

        appendTrackStrip(
            vertices,
            start,
            end,
            -halfRoad,
            halfRoad,
            0.0F,
            roadColor,
            kMaterialAsphalt,
            lateralV(-halfRoad),
            lateralV(halfRoad));
        appendTrackStrip(
            vertices,
            start,
            end,
            innerWallOffset,
            innerApron,
            -0.012F,
            shoulderColor,
            kMaterialGrass,
            lateralV(innerWallOffset),
            lateralV(innerApron));
        appendTrackStrip(
            vertices,
            start,
            end,
            innerApron,
            -halfRoad,
            0.005F,
            apronColor,
            kMaterialConcrete,
            lateralV(innerApron),
            lateralV(-halfRoad));
        appendTrackStrip(
            vertices,
            start,
            end,
            halfRoad,
            wallOffset,
            -0.01F,
            shoulderColor,
            kMaterialGrass,
            lateralV(halfRoad),
            lateralV(wallOffset));

        const Vec3 wallStart = pointAtTrackOffset(start, wallOffset, 0.0F);
        const Vec3 wallEnd = pointAtTrackOffset(end, wallOffset, 0.0F);
        appendQuad(
            vertices,
            wallStart,
            wallEnd,
            {wallEnd.x, wallEnd.y + config.outerWallHeightM, wallEnd.z},
            {wallStart.x, wallStart.y + config.outerWallHeightM, wallStart.z},
            wallColor,
            kMaterialConcrete);
        appendCatchFencePanel(vertices, start, end, wallOffset, config.outerWallHeightM, segment);

        const Vec3 innerWallStart = pointAtTrackOffset(start, innerWallOffset, 0.0F);
        const Vec3 innerWallEnd = pointAtTrackOffset(end, innerWallOffset, 0.0F);
        appendQuad(
            vertices,
            innerWallEnd,
            innerWallStart,
            {innerWallStart.x, innerWallStart.y + config.outerWallHeightM, innerWallStart.z},
            {innerWallEnd.x, innerWallEnd.y + config.outerWallHeightM, innerWallEnd.z},
            wallColor,
            kMaterialConcrete);
        appendCatchFencePanel(vertices, start, end, innerWallOffset, config.outerWallHeightM, segment);
    }

    appendStartFinishChecker(vertices, track);
    appendTrackMarker(vertices, track, lapLength * 0.25F, {0.18F, 0.65F, 0.9F, 1.0F});
    appendTrackMarker(vertices, track, lapLength * 0.5F, {0.18F, 0.65F, 0.9F, 1.0F});
    appendTrackMarker(vertices, track, lapLength * 0.75F, {0.18F, 0.65F, 0.9F, 1.0F});
    appendGrandstands(vertices, track);
    return vertices;
}

std::vector<Vertex> makeCarBodyGeometry() {
    std::vector<Vertex> vertices;
    vertices.reserve(1500);
    const std::array<float, 4> body{0.56F, 0.025F, 0.025F, 1.0F};
    const std::array<float, 4> bodyDark{0.30F, 0.018F, 0.018F, 1.0F};
    const std::array<float, 4> accent{0.86F, 0.86F, 0.80F, 1.0F};
    const std::array<float, 4> carbon{0.018F, 0.020F, 0.024F, 1.0F};
    const std::array<float, 4> arm{0.10F, 0.105F, 0.11F, 1.0F};
    const std::array<float, 4> helmet{0.78F, 0.78F, 0.72F, 1.0F};
    const std::array<float, 4> visor{0.03F, 0.07F, 0.10F, 1.0F};
    const std::array<float, 4> brakeLamp{1.0F, 0.035F, 0.02F, 1.0F};
    const std::array<float, 4> intakeShadow{0.006F, 0.008F, 0.011F, 1.0F};
    const std::array<float, 4> caliper{0.78F, 0.10F, 0.035F, 1.0F};
    const std::array<float, 4> hotMetal{0.38F, 0.36F, 0.33F, 1.0F};

    appendBox(vertices, {-0.72F, 0.10F, -1.86F}, {0.72F, 0.19F, 2.18F}, carbon, kMaterialCarbon);
    appendTaperedBox(vertices, -1.38F, 0.62F, 0.46F, 0.30F, 0.22F, 0.62F, body, kMaterialPaint);
    appendTaperedBox(vertices, 0.35F, 2.62F, 0.25F, 0.045F, 0.24F, 0.44F, body, kMaterialPaint);
    appendTaperedBox(vertices, -1.78F, -1.02F, 0.36F, 0.50F, 0.30F, 0.76F, bodyDark, kMaterialPaint);
    appendBox(vertices, {-0.78F, 0.20F, -0.98F}, {-0.34F, 0.53F, 0.28F}, bodyDark, kMaterialPaint);
    appendBox(vertices, {0.34F, 0.20F, -0.98F}, {0.78F, 0.53F, 0.28F}, bodyDark, kMaterialPaint);
    appendTaperedBox(vertices, -0.86F, -0.06F, 0.14F, 0.28F, 0.32F, 0.60F, bodyDark, kMaterialPaint);
    appendTaperedBox(vertices, -0.86F, -0.06F, 0.28F, 0.14F, 0.32F, 0.60F, bodyDark, kMaterialPaint);
    appendBox(vertices, {-0.84F, 0.34F, -0.80F}, {-0.79F, 0.50F, -0.20F}, intakeShadow, kMaterialCarbon);
    appendBox(vertices, {0.79F, 0.34F, -0.80F}, {0.84F, 0.50F, -0.20F}, intakeShadow, kMaterialCarbon);
    appendBox(vertices, {-0.86F, 0.58F, -0.80F}, {-0.35F, 0.64F, 0.04F}, carbon, kMaterialCarbon);
    appendBox(vertices, {0.35F, 0.58F, -0.80F}, {0.86F, 0.64F, 0.04F}, carbon, kMaterialCarbon);
    appendBox(vertices, {-0.80F, 0.34F, -0.70F}, {-0.55F, 0.47F, -0.18F}, carbon, kMaterialCarbon);
    appendBox(vertices, {0.55F, 0.34F, -0.70F}, {0.80F, 0.47F, -0.18F}, carbon, kMaterialCarbon);
    appendBox(vertices, {-0.09F, 0.50F, 0.72F}, {0.09F, 0.60F, 2.08F}, accent, kMaterialPaint);
    appendBox(vertices, {-0.22F, 0.53F, -1.38F}, {0.22F, 0.64F, -0.05F}, accent, kMaterialPaint);

    appendBox(vertices, {-0.33F, 0.53F, -0.58F}, {0.33F, 0.76F, 0.05F}, carbon, kMaterialCarbon);
    appendCylinderX(vertices, {0.0F, 0.77F, -0.33F}, 0.18F, 0.16F, 0.15F, 14, helmet, kMaterialPaint);
    appendBox(vertices, {-0.14F, 0.79F, -0.49F}, {0.14F, 0.88F, -0.22F}, visor, kMaterialVisor);
    appendBox(vertices, {-0.36F, 0.78F, -0.18F}, {0.36F, 0.90F, -0.08F}, carbon, kMaterialCarbon);
    appendBox(vertices, {-0.08F, 0.74F, -0.70F}, {0.08F, 1.02F, -0.52F}, carbon, kMaterialCarbon);
    appendRod(vertices, {-0.34F, 0.78F, -0.12F}, {-0.08F, 1.01F, -0.60F}, 0.025F, carbon, kMaterialCarbon);
    appendRod(vertices, {0.34F, 0.78F, -0.12F}, {0.08F, 1.01F, -0.60F}, 0.025F, carbon, kMaterialCarbon);

    appendBox(vertices, {-1.44F, 0.105F, 2.24F}, {1.44F, 0.18F, 2.48F}, carbon, kMaterialCarbon);
    appendBox(vertices, {-1.18F, 0.21F, 2.05F}, {1.18F, 0.27F, 2.20F}, accent, kMaterialPaint);
    appendBox(vertices, {-1.47F, 0.14F, 2.05F}, {-1.34F, 0.43F, 2.49F}, carbon, kMaterialCarbon);
    appendBox(vertices, {1.34F, 0.14F, 2.05F}, {1.47F, 0.43F, 2.49F}, carbon, kMaterialCarbon);
    appendBox(vertices, {-1.18F, 0.18F, 2.46F}, {-0.84F, 0.31F, 2.70F}, carbon, kMaterialCarbon);
    appendBox(vertices, {0.84F, 0.18F, 2.46F}, {1.18F, 0.31F, 2.70F}, carbon, kMaterialCarbon);
    appendBox(vertices, {-0.72F, 0.16F, 2.38F}, {-0.48F, 0.27F, 2.62F}, carbon, kMaterialCarbon);
    appendBox(vertices, {0.48F, 0.16F, 2.38F}, {0.72F, 0.27F, 2.62F}, carbon, kMaterialCarbon);
    appendBox(vertices, {-1.30F, 0.34F, 2.30F}, {-1.20F, 0.58F, 2.42F}, carbon, kMaterialCarbon);
    appendBox(vertices, {1.20F, 0.34F, 2.30F}, {1.30F, 0.58F, 2.42F}, carbon, kMaterialCarbon);
    appendBox(vertices, {-1.62F, 0.08F, 2.36F}, {-1.46F, 0.30F, 2.62F}, carbon, kMaterialCarbon);
    appendBox(vertices, {1.46F, 0.08F, 2.36F}, {1.62F, 0.30F, 2.62F}, carbon, kMaterialCarbon);
    appendBox(vertices, {-1.55F, 0.02F, 2.08F}, {-1.18F, 0.10F, 2.26F}, carbon, kMaterialCarbon);
    appendBox(vertices, {1.18F, 0.02F, 2.08F}, {1.55F, 0.10F, 2.26F}, carbon, kMaterialCarbon);
    appendBox(vertices, {-1.25F, 0.73F, -1.98F}, {1.25F, 0.84F, -1.72F}, carbon, kMaterialCarbon);
    appendBox(vertices, {-1.02F, 0.87F, -1.82F}, {1.02F, 0.93F, -1.67F}, accent, kMaterialPaint);
    appendBox(vertices, {-0.94F, 0.36F, -1.88F}, {-0.78F, 0.84F, -1.74F}, carbon, kMaterialCarbon);
    appendBox(vertices, {0.78F, 0.36F, -1.88F}, {0.94F, 0.84F, -1.74F}, carbon, kMaterialCarbon);
    appendBox(vertices, {-1.36F, 0.62F, -2.04F}, {-1.16F, 1.05F, -1.62F}, carbon, kMaterialCarbon);
    appendBox(vertices, {1.16F, 0.62F, -2.04F}, {1.36F, 1.05F, -1.62F}, carbon, kMaterialCarbon);
    appendBox(vertices, {-0.48F, 0.22F, -2.06F}, {0.48F, 0.34F, -1.84F}, carbon, kMaterialCarbon);
    appendBox(vertices, {-0.72F, 0.10F, -2.22F}, {-0.52F, 0.22F, -1.84F}, carbon, kMaterialCarbon);
    appendBox(vertices, {0.52F, 0.10F, -2.22F}, {0.72F, 0.22F, -1.84F}, carbon, kMaterialCarbon);
    appendBox(vertices, {-0.18F, 0.14F, -2.30F}, {-0.08F, 0.36F, -1.88F}, carbon, kMaterialCarbon);
    appendBox(vertices, {0.08F, 0.14F, -2.30F}, {0.18F, 0.36F, -1.88F}, carbon, kMaterialCarbon);
    appendBox(vertices, {-0.48F, 0.38F, -2.11F}, {-0.22F, 0.49F, -2.03F}, brakeLamp, kMaterialBrakeLight);
    appendBox(vertices, {0.22F, 0.38F, -2.11F}, {0.48F, 0.49F, -2.03F}, brakeLamp, kMaterialBrakeLight);
    appendRod(vertices, {-0.06F, 0.33F, -2.05F}, {-0.06F, 0.33F, -2.34F}, 0.045F, hotMetal, kMaterialMetal);
    appendRod(vertices, {0.06F, 0.33F, -2.05F}, {0.06F, 0.33F, -2.34F}, 0.045F, hotMetal, kMaterialMetal);

    appendBox(vertices, {-0.98F, 0.38F, 1.04F}, {-0.85F, 0.54F, 1.25F}, caliper, kMaterialMetal);
    appendBox(vertices, {0.85F, 0.38F, 1.04F}, {0.98F, 0.54F, 1.25F}, caliper, kMaterialMetal);
    appendBox(vertices, {-1.01F, 0.38F, -1.25F}, {-0.86F, 0.54F, -1.03F}, caliper, kMaterialMetal);
    appendBox(vertices, {0.86F, 0.38F, -1.25F}, {1.01F, 0.54F, -1.03F}, caliper, kMaterialMetal);

    appendBox(vertices, {-0.86F, 0.55F, -0.54F}, {-0.72F, 0.66F, -0.42F}, carbon, kMaterialCarbon);
    appendBox(vertices, {0.72F, 0.55F, -0.54F}, {0.86F, 0.66F, -0.42F}, carbon, kMaterialCarbon);
    appendRod(vertices, {-0.64F, 0.56F, -0.42F}, {-0.84F, 0.62F, -0.48F}, 0.018F, carbon, kMaterialCarbon);
    appendRod(vertices, {0.64F, 0.56F, -0.42F}, {0.84F, 0.62F, -0.48F}, 0.018F, carbon, kMaterialCarbon);

    appendRod(vertices, {-0.25F, 0.43F, 1.05F}, {-0.83F, 0.40F, 1.05F}, 0.025F, arm, kMaterialMetal);
    appendRod(vertices, {-0.22F, 0.34F, 1.55F}, {-0.83F, 0.34F, 1.47F}, 0.025F, arm, kMaterialMetal);
    appendRod(vertices, {-0.24F, 0.56F, 1.18F}, {-0.83F, 0.50F, 1.42F}, 0.020F, arm, kMaterialMetal);
    appendRod(vertices, {-0.20F, 0.26F, 1.18F}, {-0.83F, 0.28F, 1.03F}, 0.018F, arm, kMaterialMetal);
    appendRod(vertices, {0.25F, 0.43F, 1.05F}, {0.83F, 0.40F, 1.05F}, 0.025F, arm, kMaterialMetal);
    appendRod(vertices, {0.22F, 0.34F, 1.55F}, {0.83F, 0.34F, 1.47F}, 0.025F, arm, kMaterialMetal);
    appendRod(vertices, {0.24F, 0.56F, 1.18F}, {0.83F, 0.50F, 1.42F}, 0.020F, arm, kMaterialMetal);
    appendRod(vertices, {0.20F, 0.26F, 1.18F}, {0.83F, 0.28F, 1.03F}, 0.018F, arm, kMaterialMetal);
    appendRod(vertices, {-0.38F, 0.44F, -0.86F}, {-0.85F, 0.40F, -0.75F}, 0.025F, arm, kMaterialMetal);
    appendRod(vertices, {-0.35F, 0.34F, -1.25F}, {-0.85F, 0.34F, -1.24F}, 0.025F, arm, kMaterialMetal);
    appendRod(vertices, {-0.36F, 0.56F, -0.98F}, {-0.86F, 0.50F, -1.22F}, 0.020F, arm, kMaterialMetal);
    appendRod(vertices, {-0.34F, 0.26F, -0.98F}, {-0.86F, 0.28F, -0.76F}, 0.018F, arm, kMaterialMetal);
    appendRod(vertices, {0.38F, 0.44F, -0.86F}, {0.85F, 0.40F, -0.75F}, 0.025F, arm, kMaterialMetal);
    appendRod(vertices, {0.35F, 0.34F, -1.25F}, {0.85F, 0.34F, -1.24F}, 0.025F, arm, kMaterialMetal);
    appendRod(vertices, {0.36F, 0.56F, -0.98F}, {0.86F, 0.50F, -1.22F}, 0.020F, arm, kMaterialMetal);
    appendRod(vertices, {0.34F, 0.26F, -0.98F}, {0.86F, 0.28F, -0.76F}, 0.018F, arm, kMaterialMetal);
    return vertices;
}

std::vector<Vertex> makeWheelGeometry() {
    std::vector<Vertex> vertices;
    vertices.reserve(4600);
    const std::array<float, 4> tire{0.012F, 0.012F, 0.015F, 1.0F};
    const std::array<float, 4> sidewall{0.030F, 0.030F, 0.035F, 1.0F};
    const std::array<float, 4> rim{0.56F, 0.57F, 0.56F, 1.0F};
    const std::array<float, 4> hub{0.16F, 0.17F, 0.18F, 1.0F};
    const std::array<float, 4> brake{0.23F, 0.21F, 0.19F, 1.0F};

    appendCylinderX(vertices, {0.0F, 0.0F, 0.0F}, 0.235F, 0.37F, 0.47F, 48, tire, kMaterialTire);
    appendCylinderX(vertices, {0.0F, 0.0F, 0.0F}, 0.248F, 0.285F, 0.36F, 48, sidewall, kMaterialTire);
    appendCylinderX(vertices, {-0.265F, 0.0F, 0.0F}, 0.014F, 0.155F, 0.195F, 32, rim, kMaterialMetal);
    appendCylinderX(vertices, {0.265F, 0.0F, 0.0F}, 0.014F, 0.155F, 0.195F, 32, rim, kMaterialMetal);
    appendCylinderX(vertices, {-0.282F, 0.0F, 0.0F}, 0.012F, 0.075F, 0.092F, 24, hub, kMaterialMetal);
    appendCylinderX(vertices, {0.282F, 0.0F, 0.0F}, 0.012F, 0.075F, 0.092F, 24, hub, kMaterialMetal);
    appendCylinderX(vertices, {0.0F, 0.0F, 0.0F}, 0.045F, 0.20F, 0.245F, 32, brake, kMaterialBrakeDisc);

    for (int side = -1; side <= 1; side += 2) {
        const float x = static_cast<float>(side) * 0.292F;
        for (int spoke = 0; spoke < 6; ++spoke) {
            const float angle = std::numbers::pi_v<float> * 2.0F *
                                static_cast<float>(spoke) / 6.0F;
            appendRod(
                vertices,
                {x, std::cos(angle) * 0.045F, std::sin(angle) * 0.055F},
                {x, std::cos(angle) * 0.145F, std::sin(angle) * 0.182F},
                0.012F,
                rim,
                kMaterialMetal);
        }
    }
    return vertices;
}

std::vector<Vertex> makeSteeringWheelGeometry() {
    std::vector<Vertex> vertices;
    vertices.reserve(340);
    const std::array<float, 4> rubber{0.010F, 0.011F, 0.013F, 1.0F};
    const std::array<float, 4> spoke{0.13F, 0.14F, 0.15F, 1.0F};
    const std::array<float, 4> hub{0.33F, 0.05F, 0.04F, 1.0F};

    constexpr int segments = 28;
    for (int segment = 0; segment < segments; ++segment) {
        const float a0 = std::numbers::pi_v<float> * 2.0F *
                         static_cast<float>(segment) / static_cast<float>(segments);
        const float a1 = std::numbers::pi_v<float> * 2.0F *
                         static_cast<float>(segment + 1) / static_cast<float>(segments);
        appendRod(
            vertices,
            {std::cos(a0) * 0.16F, std::sin(a0) * 0.12F, 0.0F},
            {std::cos(a1) * 0.16F, std::sin(a1) * 0.12F, 0.0F},
            0.018F,
            rubber,
            kMaterialTire);
    }
    appendRod(vertices, {0.0F, 0.0F, 0.0F}, {-0.14F, 0.0F, 0.0F}, 0.012F, spoke, kMaterialMetal);
    appendRod(vertices, {0.0F, 0.0F, 0.0F}, {0.14F, 0.0F, 0.0F}, 0.012F, spoke, kMaterialMetal);
    appendRod(vertices, {0.0F, 0.0F, 0.0F}, {0.0F, -0.10F, 0.0F}, 0.012F, spoke, kMaterialMetal);
    appendBox(vertices, {-0.035F, -0.030F, -0.018F}, {0.035F, 0.030F, 0.018F}, hub, kMaterialPaint);
    return vertices;
}

std::vector<Vertex> convertObjMesh(const ObjMesh& mesh) {
    std::vector<Vertex> vertices;
    vertices.reserve(mesh.vertices.size());
    for (const ObjMeshVertex& source : mesh.vertices) {
        vertices.push_back(makeVertex(
            source.position.x,
            source.position.y,
            source.position.z,
            source.color,
            source.normal,
            source.material,
            source.tangent));
    }
    return vertices;
}

std::vector<Vertex> loadObjGeometryOrFallback(
    const char* relativePath,
    const std::vector<Vertex>& fallback) {
    ObjMesh mesh = loadObjMesh(FileSystem::findDataFile(relativePath));
    if (mesh.vertices.empty()) {
        return fallback;
    }
    return convertObjMesh(mesh);
}

const char* shaderSource() {
    return R"METAL(
        #include <metal_stdlib>
        using namespace metal;

        struct Vertex {
            float4 position;
            float4 color;
            float4 data;
            float4 tex;
        };

        struct Uniforms {
            float4x4 mvp;
            float4x4 model;
            float4x4 shadowMvp;
            float4 cameraPosition;
            float4 cameraRight;
            float4 cameraUp;
            float4 cameraForward;
            float4 renderParams;
            float4 effectParams;
            float4 contactParams;
        };

        struct VertexOutput {
            float4 position [[position]];
            float4 color;
            float4 data;
            float3 normal;
            float3 tangent;
            float2 uv;
            float trackLateral;
            float3 localPosition;
            float3 worldPosition;
            float4 shadowPosition;
        };

        vertex VertexOutput vertex_main(
            const device Vertex* vertices [[buffer(0)]],
            constant Uniforms& uniforms [[buffer(1)]],
            uint vertexId [[vertex_id]]) {
            VertexOutput output;
            const Vertex inVertex = vertices[vertexId];
            const float4 worldPosition = uniforms.model * inVertex.position;
            output.position = uniforms.mvp * inVertex.position;
            output.color = inVertex.color;
            output.data = inVertex.data;
            output.normal = normalize((uniforms.model * float4(inVertex.data.xyz, 0.0)).xyz);
            output.tangent = normalize((uniforms.model * float4(inVertex.tex.xyz, 0.0)).xyz);
            output.uv = inVertex.tex.xy;
            output.trackLateral = inVertex.tex.w;
            output.localPosition = inVertex.position.xyz;
            output.worldPosition = worldPosition.xyz;
            output.shadowPosition = uniforms.shadowMvp * inVertex.position;
            return output;
        }

        vertex VertexOutput vertex_sky(
            const device Vertex* vertices [[buffer(0)]],
            constant Uniforms& uniforms [[buffer(1)]],
            uint vertexId [[vertex_id]]) {
            VertexOutput output;
            const Vertex inVertex = vertices[vertexId];
            output.position = inVertex.position;
            output.color = inVertex.color;
            output.data = inVertex.data;
            output.normal = float3(0.0, 1.0, 0.0);
            output.tangent = float3(1.0, 0.0, 0.0);
            output.uv = inVertex.tex.xy;
            output.trackLateral = 0.0;
            output.localPosition = inVertex.position.xyz;
            output.worldPosition = uniforms.cameraPosition.xyz;
            output.shadowPosition = float4(0.0);
            return output;
        }

        float hash21(float2 value) {
            return fract(sin(dot(value, float2(127.1, 311.7))) * 43758.5453);
        }

        float noise21(float2 value) {
            const float2 cell = floor(value);
            const float2 local = fract(value);
            const float a = hash21(cell);
            const float b = hash21(cell + float2(1.0, 0.0));
            const float c = hash21(cell + float2(0.0, 1.0));
            const float d = hash21(cell + float2(1.0, 1.0));
            const float2 blend = local * local * (3.0 - 2.0 * local);
            return mix(mix(a, b, blend.x), mix(c, d, blend.x), blend.y);
        }

        float3 skyRadiance(float3 ray, float timeSeconds) {
            const float3 sun = normalize(float3(-0.42, 0.82, -0.38));
            const float upAmount = clamp(ray.y * 0.5 + 0.5, 0.0, 1.0);
            const float horizon = pow(1.0 - upAmount, 2.35);
            const float groundBlend = smoothstep(-0.18, 0.08, ray.y);
            float3 color = mix(float3(0.070, 0.095, 0.105), float3(0.70, 0.84, 1.04), groundBlend);
            color = mix(color, float3(0.070, 0.19, 0.46), pow(upAmount, 0.72) * groundBlend);
            color += float3(1.08, 0.70, 0.38) * horizon * 0.42;

            const float sunDot = max(dot(ray, sun), 0.0);
            color += float3(10.0, 6.9, 3.1) * pow(sunDot, 920.0);
            color += float3(1.75, 1.05, 0.54) * pow(sunDot, 19.0) * 0.58;
            color += float3(0.18, 0.28, 0.42) * pow(max(dot(ray, float3(-sun.x, sun.y * 0.25, -sun.z)), 0.0), 2.2);

            const float cloudHeight = max(ray.y, 0.075);
            float2 cloudUv = ray.xz / cloudHeight * 0.18 + float2(timeSeconds * 0.006, timeSeconds * 0.0025);
            float cloud = noise21(cloudUv);
            cloud += noise21(cloudUv * 2.15 + 17.3) * 0.52;
            cloud += noise21(cloudUv * 4.20 + 43.1) * 0.24;
            const float cloudMask =
                smoothstep(0.76, 1.18, cloud) *
                smoothstep(0.04, 0.34, ray.y) *
                (1.0 - smoothstep(0.78, 1.0, ray.y));
            const float cirrus =
                smoothstep(0.64, 0.88, noise21(ray.xz * 1.7 + float2(timeSeconds * -0.010, 31.2))) *
                smoothstep(0.30, 0.76, ray.y);
            const float sunBacklight = pow(sunDot, 5.0) * 0.52;
            color = mix(color, color + float3(1.02, 1.00, 0.92) * (0.34 + sunBacklight), cloudMask * 0.58);
            color = mix(color, color + float3(0.78, 0.84, 0.90) * 0.16, cirrus * 0.35);
            return max(color, float3(0.0));
        }

        float2 environmentUv(float3 ray) {
            const float3 direction = normalize(ray);
            const float u = atan2(direction.z, direction.x) / 6.2831853 + 0.5;
            const float v = acos(clamp(direction.y, -1.0, 1.0)) / 3.14159265;
            return float2(fract(u), clamp(v, 0.0, 1.0));
        }

        float3 environmentRadiance(
            texture2d<float> skyboxTexture,
            sampler textureSampler,
            float3 ray,
            float timeSeconds) {
            const float3 procedural = skyRadiance(ray, timeSeconds);
            const float3 skybox = skyboxTexture.sample(textureSampler, environmentUv(ray)).rgb * 1.28;
            return mix(procedural, skybox, 0.72);
        }

        float3 acesToneMap(float3 value) {
            value = max(value, float3(0.0));
            const float a = 2.51;
            const float b = 0.03;
            const float c = 2.43;
            const float d = 0.59;
            const float e = 0.14;
            return clamp((value * (a * value + b)) / (value * (c * value + d) + e), 0.0, 1.0);
        }

        float3 lensGrade(float3 hdrColor, float2 screenUv, float strength) {
            float3 color = acesToneMap(hdrColor);
            const float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
            color = mix(float3(luminance), color, 1.075);
            color = max(color - float3(0.004), float3(0.0));
            color = color * (1.0 + color * 0.040);
            color = pow(color, float3(1.0 / 2.2));
            const float2 centered = screenUv - 0.5;
            const float edge = clamp(dot(centered, centered) * 2.0, 0.0, 1.0);
            const float vignette = clamp(1.0 - edge * 0.24 - edge * edge * 0.10, 0.66, 1.0);
            color *= vignette;
            color.r += edge * 0.008 * strength;
            color.g *= 1.0 - edge * 0.004 * strength;
            color.b -= edge * 0.006 * strength;
            return clamp(color, 0.0, 1.0);
        }

        float boxMask(float2 value, float2 center, float2 halfSize, float softness) {
            const float2 delta = abs(value - center) - halfSize;
            const float outside = length(max(delta, float2(0.0)));
            const float inside = min(max(delta.x, delta.y), 0.0);
            return 1.0 - smoothstep(0.0, softness, outside + inside);
        }

        float digit4Mask(float2 uv) {
            float result = 0.0;
            result = max(result, boxMask(uv, float2(-0.43, 0.06), float2(0.08, 0.46), 0.035));
            result = max(result, boxMask(uv, float2(0.40, 0.00), float2(0.08, 0.75), 0.035));
            result = max(result, boxMask(uv, float2(0.00, 0.02), float2(0.46, 0.08), 0.035));
            result = max(result, boxMask(uv, float2(-0.08, 0.43), float2(0.08, 0.34), 0.035));
            return result;
        }

        float digit5Mask(float2 uv) {
            float result = 0.0;
            result = max(result, boxMask(uv, float2(0.00, 0.67), float2(0.48, 0.08), 0.035));
            result = max(result, boxMask(uv, float2(-0.40, 0.30), float2(0.08, 0.34), 0.035));
            result = max(result, boxMask(uv, float2(0.00, 0.03), float2(0.48, 0.08), 0.035));
            result = max(result, boxMask(uv, float2(0.40, -0.34), float2(0.08, 0.34), 0.035));
            result = max(result, boxMask(uv, float2(0.00, -0.67), float2(0.48, 0.08), 0.035));
            return result;
        }

        fragment float4 fragment_sky(
            VertexOutput input [[stage_in]],
            constant Uniforms& uniforms [[buffer(1)]],
            texture2d<float> skyboxTexture [[texture(0)]],
            sampler materialSampler [[sampler(0)]]) {
            const float2 uv = clamp(input.data.xy, 0.0, 1.0);
            const float aspect = uniforms.renderParams.x / max(uniforms.renderParams.y, 1.0);
            const float2 ndc = float2(uv.x * 2.0 - 1.0, (1.0 - uv.y) * 2.0 - 1.0);
            const float3 ray = normalize(
                uniforms.cameraForward.xyz +
                uniforms.cameraRight.xyz * ndc.x * aspect * 0.72 +
                uniforms.cameraUp.xyz * ndc.y * 0.72);
            const float3 color = environmentRadiance(skyboxTexture, materialSampler, ray, uniforms.renderParams.z);
            return float4(color, 1.0);
        }

        float2 materialUv(float material, float3 worldPosition, float3 localPosition) {
            if (material > 4.5 && material < 5.5) {
                return localPosition.xz * 2.8 + localPosition.yy * 0.35;
            }
            if (material > 2.5 && material < 3.5) {
                return float2(worldPosition.x * 0.08, worldPosition.z * 0.08 + worldPosition.y * 0.35);
            }
            if (material > 1.5 && material < 2.5) {
                return worldPosition.xz * 0.052;
            }
            return worldPosition.xz * 0.075;
        }

        float3 normalMapped(
            texture2d<float> normalTexture,
            sampler textureSampler,
            float2 uv,
            float3 geometricNormal,
            float3 vertexTangent,
            float strength) {
            const float3 tangentNormal = normalTexture.sample(textureSampler, uv).xyz * 2.0 - 1.0;
            const float3 n = normalize(geometricNormal);
            const float3 t = normalize(vertexTangent - n * dot(vertexTangent, n));
            const float3 b = normalize(cross(n, t));
            const float3 mapped = normalize(t * tangentNormal.x + b * tangentNormal.y + n * tangentNormal.z);
            return normalize(mix(n, mapped, strength));
        }

        float ggxDistribution(float nDotH, float roughness) {
            const float alpha = max(roughness * roughness, 0.002);
            const float alpha2 = alpha * alpha;
            const float denom = nDotH * nDotH * (alpha2 - 1.0) + 1.0;
            return alpha2 / max(3.14159265 * denom * denom, 0.0001);
        }

        float smithSchlickGgx(float nDotV, float roughness) {
            const float r = roughness + 1.0;
            const float k = (r * r) * 0.125;
            return nDotV / max(nDotV * (1.0 - k) + k, 0.0001);
        }

        float smithGeometry(float nDotV, float nDotL, float roughness) {
            return smithSchlickGgx(nDotV, roughness) * smithSchlickGgx(nDotL, roughness);
        }

        float3 fresnelSchlick(float cosTheta, float3 f0) {
            const float f = pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
            return f0 + (float3(1.0) - f0) * f;
        }

        float shadowVisibility(
            depth2d<float> shadowTexture,
            sampler shadowSampler,
            float4 shadowPosition,
            float bias,
            float configuredTexelSize) {
            const float3 projected = shadowPosition.xyz / max(shadowPosition.w, 0.0001);
            const float2 uv = projected.xy * 0.5 + 0.5;
            if (uv.x < 0.001 || uv.x > 0.999 || uv.y < 0.001 || uv.y > 0.999 ||
                projected.z < 0.0 || projected.z > 1.0) {
                return 1.0;
            }
            const float shadowTexelSize = max(configuredTexelSize, 1.0 / 8192.0);
            const float2 texel = float2(shadowTexelSize, shadowTexelSize);
            const float2 poisson[8] = {
                float2(-0.94201624, -0.39906216),
                float2( 0.94558609, -0.76890725),
                float2(-0.09418410, -0.92938870),
                float2( 0.34495938,  0.29387760),
                float2(-0.91588581,  0.45771432),
                float2(-0.38277543,  0.27676845),
                float2( 0.97484398,  0.75648379),
                float2( 0.44323325, -0.97511554),
            };
            float visibility = 0.0;
            for (int tap = 0; tap < 8; ++tap) {
                visibility += shadowTexture.sample_compare(
                    shadowSampler,
                    uv + poisson[tap] * texel * 3.35,
                    projected.z - bias);
            }
            return visibility * 0.125;
        }

        float bandMask(float value, float center, float halfWidth, float softness) {
            return 1.0 - smoothstep(halfWidth, halfWidth + softness, abs(value - center));
        }

        fragment float4 fragment_world(
            VertexOutput input [[stage_in]],
            constant Uniforms& uniforms [[buffer(1)]],
            texture2d<float> asphaltAlbedo [[texture(0)]],
            texture2d<float> asphaltNormal [[texture(1)]],
            texture2d<float> concreteAlbedo [[texture(2)]],
            texture2d<float> concreteNormal [[texture(3)]],
            texture2d<float> grassAlbedo [[texture(4)]],
            texture2d<float> grassNormal [[texture(5)]],
            texture2d<float> carbonAlbedo [[texture(6)]],
            texture2d<float> carbonNormal [[texture(7)]],
            depth2d<float> shadowTexture [[texture(8)]],
            texture2d<float> smokeTexture [[texture(9)]],
            texture2d<float> asphaltRoughness [[texture(10)]],
            texture2d<float> asphaltHeight [[texture(11)]],
            texture2d<float> grassRoughness [[texture(12)]],
            texture2d<float> grassHeight [[texture(13)]],
            texture2d<float> carbonRoughness [[texture(14)]],
            texture2d<float> skyboxTexture [[texture(15)]],
            texture2d<float> concreteRoughness [[texture(16)]],
            sampler materialSampler [[sampler(0)]],
            sampler shadowSampler [[sampler(1)]]) {
            float3 normal = normalize(input.normal);
            const float3 sun = normalize(float3(-0.42, 0.82, -0.38));
            const float3 viewDir = normalize(uniforms.cameraPosition.xyz - input.worldPosition);
            const float3 halfDir = normalize(sun + viewDir);
            const float material = input.data.w;
            const float2 groundUv = input.worldPosition.xz;
            const float2 screenUv = clamp(input.position.xy / max(uniforms.renderParams.xy, float2(1.0)), 0.0, 1.0);
            float3 base = input.color.rgb;
            float roughDiffuse = 1.0;
            float roughness = 0.62;
            float metallic = 0.0;
            float specularStrength = 0.020;
            float specularPower = 24.0;
            float clearcoatStrength = 0.0;
            float clearcoatPower = 96.0;
            float clearcoatRoughness = 0.16;
            float3 emission = float3(0.0);
            float materialAlphaScale = 1.0;
            float materialOcclusion = 1.0;
            float environmentReflectionScale = 1.0;
            const float2 uv = materialUv(material, input.worldPosition, input.localPosition);

            if (material > 0.5 && material < 1.5) {
                const float3 tex = asphaltAlbedo.sample(materialSampler, uv).rgb;
                const float heightSample = asphaltHeight.sample(materialSampler, uv).r;
                const float roughnessSample = asphaltRoughness.sample(materialSampler, uv).r;
                base = mix(base, tex, 0.86);
                normal = normalMapped(asphaltNormal, materialSampler, uv, normal, input.tangent, 0.92);
                const float groove = exp(-pow(input.trackLateral - (-0.30), 2.0) / 0.12);
                const float rubberLane = exp(-pow(input.trackLateral - 0.05, 2.0) / 0.38);
                const float aggregate = noise21(groundUv * 12.0) * 0.08 + noise21(groundUv * 46.0) * 0.035;
                const float oilSheen = smoothstep(0.70, 0.96, noise21(groundUv * 0.42 + float2(13.7, 4.1))) * rubberLane;
                const float whiteLine = bandMask(input.trackLateral, 0.84, 0.010, 0.018);
                const float yellowLine = bandMask(input.trackLateral, -0.36, 0.010, 0.018);
                const float expansionJoint =
                    bandMask(fract(input.worldPosition.z * 0.021 + 0.15), 0.50, 0.010, 0.030) * 0.18;
                base *= 0.90 + heightSample * 0.18;
                base = mix(base, base * 0.40, groove * 0.80);
                base = mix(base, base * (0.74 + aggregate), rubberLane * 0.42);
                base += float3(0.018, 0.017, 0.014) * aggregate;
                base = mix(base, base * 0.72, expansionJoint);
                base = mix(base, float3(0.92, 0.91, 0.82), whiteLine * 0.82);
                base = mix(base, float3(0.95, 0.76, 0.22), yellowLine * 0.80);
                roughDiffuse = 0.92;
                roughness = clamp(mix(0.82 - oilSheen * 0.22 + rubberLane * 0.05, roughnessSample, 0.62), 0.50, 0.94);
                metallic = 0.0;
                specularStrength = 0.030 + oilSheen * 0.055;
                specularPower = 26.0 + oilSheen * 18.0;
                materialOcclusion = 0.88 + heightSample * 0.14;
                environmentReflectionScale = 0.55 + oilSheen * 0.38;
            } else if (material > 1.5 && material < 2.5) {
                const float heightSample = grassHeight.sample(materialSampler, uv).r;
                const float roughnessSample = grassRoughness.sample(materialSampler, uv).r;
                base = mix(base, grassAlbedo.sample(materialSampler, uv).rgb, 0.88);
                normal = normalMapped(grassNormal, materialSampler, uv, normal, input.tangent, 0.78);
                const float bladeRows = 0.5 + 0.5 * sin((input.worldPosition.x + input.worldPosition.z * 0.31) * 18.0);
                const float mowing = 0.5 + 0.5 * sin((input.worldPosition.x - input.worldPosition.z * 0.18) * 0.85);
                const float clumps = noise21(groundUv * 3.6) * 0.22 + noise21(groundUv * 18.0) * 0.10;
                base *= 0.72 + bladeRows * 0.12 + mowing * 0.12 + clumps + heightSample * 0.12;
                base = mix(base, float3(0.080, 0.165, 0.050), smoothstep(0.64, 0.96, clumps) * 0.22);
                normal = normalize(normal + input.tangent * ((bladeRows - 0.5) * 0.040 + (heightSample - 0.5) * 0.018));
                roughness = clamp(mix(0.94, roughnessSample, 0.58), 0.76, 0.98);
                specularStrength = 0.006;
                specularPower = 10.0;
                materialOcclusion = 0.88 + heightSample * 0.10;
            } else if (material > 2.5 && material < 3.5) {
                const float roughnessSample = concreteRoughness.sample(materialSampler, uv).r;
                base = mix(base, concreteAlbedo.sample(materialSampler, uv).rgb, 0.82);
                normal = normalMapped(concreteNormal, materialSampler, uv, normal, input.tangent, 0.66);
                const float drip = smoothstep(0.40, 0.88, noise21(float2(input.worldPosition.z * 0.035, input.worldPosition.y * 1.2)));
                const float seam = bandMask(fract(input.worldPosition.z * 0.080), 0.50, 0.018, 0.030);
                base *= 0.88 + noise21(input.worldPosition.zy * 3.0) * 0.16;
                base = mix(base, base * 0.70, drip * smoothstep(0.18, 0.82, input.localPosition.y) * 0.30);
                base = mix(base, base * 0.62, seam * 0.32);
                roughness = clamp(mix(0.86, roughnessSample, 0.55), 0.60, 0.94);
                specularStrength = 0.012;
                specularPower = 14.0;
                materialOcclusion = 0.96;
            } else if (material > 3.5 && material < 4.5) {
                const float clearcoat = noise21(groundUv * 8.0 + input.worldPosition.y);
                const float pearlFlake = noise21(input.localPosition.xz * 62.0 + input.localPosition.yy * 13.0);
                const float pearlEdge = pow(clamp(1.0 - max(dot(normal, viewDir), 0.0), 0.0, 1.0), 2.2);
                base *= 0.92 + clearcoat * 0.07 + pearlFlake * 0.025;
                base += float3(0.055, 0.040, 0.085) * pearlEdge * (0.22 + pearlFlake * 0.16);
                const float3 local = input.localPosition;
                const float topInfluence = smoothstep(0.18, 0.48, local.y);
                const float centerStripe =
                    (1.0 - smoothstep(0.060, 0.110, abs(local.x))) *
                    smoothstep(-1.95, -1.15, local.z) *
                    (1.0 - smoothstep(2.42, 2.72, local.z));
                const float blueKnife =
                    (1.0 - smoothstep(0.155, 0.215, abs(abs(local.x) - 0.19))) *
                    smoothstep(-1.55, -0.85, local.z) *
                    (1.0 - smoothstep(2.05, 2.45, local.z));
                base = mix(base, float3(0.96, 0.94, 0.86), centerStripe * topInfluence * 0.86);
                base = mix(base, float3(0.05, 0.16, 0.34), blueKnife * topInfluence * 0.78);

                const float panel = boxMask(local.xz, float2(0.0, -0.42), float2(0.48, 0.42), 0.055) *
                                    smoothstep(0.30, 0.58, local.y);
                const float digit4 = digit4Mask(float2((local.x + 0.18) / 0.18, (local.z + 0.42) / 0.38));
                const float digit5 = digit5Mask(float2((local.x - 0.18) / 0.18, (local.z + 0.42) / 0.38));
                base = mix(base, float3(0.92, 0.90, 0.80), panel * 0.72);
                base = mix(base, float3(0.025, 0.030, 0.038), max(digit4, digit5) * panel);
                const float panelGap =
                    max(
                        bandMask(fract((local.z + 2.50) * 1.25), 0.50, 0.012, 0.028),
                        bandMask(abs(local.x), 0.48, 0.018, 0.035)) * smoothstep(0.12, 0.54, local.y);
                const float lowerCavity =
                    (1.0 - smoothstep(-0.08, 0.34, local.y)) *
                    smoothstep(0.42, 0.88, abs(local.x)) *
                    smoothstep(-2.36, -0.84, local.z);
                base = mix(base, base * 0.54, panelGap * 0.22);
                normal = normalize(normal + input.tangent * ((pearlFlake - 0.5) * 0.012));
                materialOcclusion = 1.0 - lowerCavity * 0.30 - panelGap * 0.08;
                roughness = 0.145;
                metallic = 0.48;
                specularStrength = 0.42;
                specularPower = 58.0;
                clearcoatStrength = 1.08;
                clearcoatPower = 260.0;
                clearcoatRoughness = 0.024;
                environmentReflectionScale = 1.22;
            } else if (material > 4.5 && material < 5.5) {
                const float roughnessSample = carbonRoughness.sample(materialSampler, uv).r;
                base = mix(base, carbonAlbedo.sample(materialSampler, uv).rgb, 0.92);
                normal = normalMapped(carbonNormal, materialSampler, uv, normal, input.tangent, 0.86);
                const float weaveA = 0.5 + 0.5 * sin(input.localPosition.x * 80.0 + input.localPosition.z * 12.0);
                const float weaveB = 0.5 + 0.5 * sin(input.localPosition.z * 92.0 - input.localPosition.x * 10.0);
                const float fiber = weaveA * weaveB;
                base *= 0.80 + fiber * 0.26;
                normal = normalize(normal + input.tangent * ((weaveA - weaveB) * 0.018));
                roughness = clamp(mix(0.24, roughnessSample, 0.70), 0.17, 0.55);
                specularStrength = 0.30;
                specularPower = 68.0;
                clearcoatStrength = 0.34;
                clearcoatRoughness = 0.088;
                materialOcclusion = 0.92 + fiber * 0.06;
                environmentReflectionScale = 0.95;
            } else if (material > 5.5 && material < 6.5) {
                const float scuff = noise21(input.worldPosition.yz * 12.0 + input.worldPosition.x);
                const float warm = noise21(input.worldPosition.zy * 2.4);
                const float micro = noise21(input.localPosition.yz * 65.0 + input.localPosition.xx * 17.0);
                const float circumGroove = smoothstep(0.88, 0.985, abs(sin(input.localPosition.z * 38.0)));
                const float shoulderRib = smoothstep(0.18, 0.24, abs(input.localPosition.x));
                const float centerRib = 1.0 - smoothstep(0.020, 0.055, abs(input.localPosition.x));
                const float sidewall = smoothstep(0.17, 0.23, abs(input.localPosition.x));
                const float angle = atan2(input.localPosition.y, input.localPosition.z);
                const float letterRepeat = fract((angle + 3.14159265) / 6.2831853 * 18.0);
                const float2 letterUv = float2(letterRepeat * 2.0 - 1.0, (abs(input.localPosition.x) - 0.205) / 0.035);
                const float letterBars =
                    max(
                        boxMask(letterUv, float2(-0.45, 0.0), float2(0.12, 0.72), 0.055),
                        max(
                            boxMask(letterUv, float2(0.08, 0.54), float2(0.46, 0.10), 0.055),
                            boxMask(letterUv, float2(0.18, -0.48), float2(0.36, 0.10), 0.055)));
                const float lettering = sidewall * letterBars * smoothstep(0.02, 0.18, abs(sin(angle * 9.0)));
                base *= 0.56 + scuff * 0.18 + warm * 0.08 + micro * 0.08;
                base *= 1.0 - circumGroove * 0.16 - shoulderRib * 0.10 + centerRib * 0.035;
                base = mix(base, float3(0.52, 0.52, 0.48), lettering * 0.42);
                normal = normalize(normal + input.tangent * ((circumGroove - 0.45) * 0.045 + (micro - 0.5) * 0.018));
                roughness = clamp(0.72 + micro * 0.16 + circumGroove * 0.06 - lettering * 0.10, 0.56, 0.94);
                specularStrength = 0.035;
                specularPower = 16.0;
                materialOcclusion = 0.88 + sidewall * 0.08;
            } else if (material > 6.5 && material < 7.5) {
                const float brushed = noise21(float2(input.worldPosition.x * 4.5, input.worldPosition.y * 14.0));
                base *= 0.84 + brushed * 0.14;
                normal = normalize(normal + input.tangent * ((brushed - 0.5) * 0.010));
                roughness = 0.18;
                metallic = 1.0;
                specularStrength = 0.52;
                specularPower = 88.0;
                clearcoatStrength = 0.16;
                environmentReflectionScale = 1.08;
            } else if (material > 7.5 && material < 8.5) {
                base *= 0.74 + noise21(groundUv * 5.5) * 0.16;
                roughness = 0.40;
                metallic = 0.78;
                specularStrength = 0.20;
                specularPower = 34.0;
            } else if (material > 8.5 && material < 9.5) {
                const float row = noise21(floor(groundUv * 0.65));
                const float fleck = noise21(groundUv * 7.0);
                base *= 0.70 + row * 0.30 + fleck * 0.16;
                roughness = 0.88;
                specularStrength = 0.01;
            } else if (material > 9.5 && material < 10.5) {
                const float wear = noise21(groundUv * 6.5);
                base *= 0.82 + wear * 0.18;
                base = mix(base, float3(0.92, 0.92, 0.84), 0.06);
                roughness = 0.82;
                specularStrength = 0.01;
            } else if (material > 10.5 && material < 11.5) {
                const float brake = clamp(uniforms.effectParams.x, 0.0, 1.0);
                base *= 0.22 + brake * 0.95;
                emission += float3(4.8, 0.16, 0.06) * (0.10 + brake * 1.25);
                roughness = 0.30;
                specularStrength = 0.05;
                specularPower = 36.0;
            } else if (material > 11.5 && material < 12.5) {
                roughness = 0.18;
                metallic = 1.0;
                specularStrength = 0.24;
                specularPower = 44.0;
            } else if (material > 12.5 && material < 13.5) {
                const float4 puff = smokeTexture.sample(materialSampler, clamp(input.uv, 0.0, 1.0));
                const float turbulence = noise21(input.worldPosition.xz * 1.8 + uniforms.renderParams.z * 0.35);
                base = mix(float3(0.34, 0.35, 0.34), float3(0.66, 0.66, 0.62), puff.r * 0.75 + turbulence * 0.25);
                materialAlphaScale = puff.a * (0.82 + turbulence * 0.18);
                roughDiffuse = 0.66;
                roughness = 0.90;
                specularStrength = 0.0;
                clearcoatStrength = 0.0;
            } else if (material > 13.5 && material < 14.5) {
                const float hotCore = noise21(input.localPosition.xy * 9.0 + uniforms.renderParams.z * 8.0);
                base = mix(float3(1.0, 0.16, 0.02), float3(1.0, 0.85, 0.20), hotCore);
                emission += float3(3.2, 1.0, 0.12);
                roughness = 0.50;
                specularStrength = 0.0;
                clearcoatStrength = 0.0;
            } else if (material > 14.5 && material < 15.5) {
                const float streak = noise21(input.worldPosition.xz * 5.0);
                const float feather = smoothstep(0.38, 0.50, abs(input.uv.x - 0.5));
                const float endFade = smoothstep(0.00, 0.18, input.uv.y) * (1.0 - smoothstep(0.82, 1.0, input.uv.y));
                base = mix(float3(0.010, 0.011, 0.012), float3(0.045, 0.042, 0.038), streak * 0.32);
                materialAlphaScale = (1.0 - feather) * endFade * (0.72 + streak * 0.18);
                roughDiffuse = 0.88;
                roughness = 0.86;
                specularStrength = 0.0;
                clearcoatStrength = 0.0;
            } else if (material > 15.5 && material < 16.5) {
                const float glassNoise = noise21(input.localPosition.xz * 18.0 + uniforms.renderParams.z * 0.05);
                base = mix(float3(0.005, 0.040, 0.065), float3(0.040, 0.115, 0.160), glassNoise);
                roughDiffuse = 0.28;
                roughness = 0.052;
                specularStrength = 0.42;
                specularPower = 116.0;
                clearcoatStrength = 1.12;
                clearcoatPower = 220.0;
                clearcoatRoughness = 0.018;
                environmentReflectionScale = 1.18;
            }

            float contactShadow = 0.0;
            if ((material > 0.5 && material < 1.5) ||
                (material > 1.5 && material < 2.5) ||
                (material > 2.5 && material < 3.5)) {
                const float2 carDelta = input.worldPosition.xz - uniforms.contactParams.xy;
                const float yaw = uniforms.contactParams.z;
                const float2 carRight = float2(cos(yaw), -sin(yaw));
                const float2 carForward = float2(sin(yaw), cos(yaw));
                const float2 carLocal = float2(dot(carDelta, carRight), dot(carDelta, carForward));
                const float chassis =
                    1.0 - smoothstep(0.72, 1.10, length(float2(carLocal.x / 0.58, (carLocal.y + 0.02) / 2.18)));
                const float floorTray =
                    1.0 - smoothstep(0.82, 1.25, length(float2(carLocal.x / 0.98, carLocal.y / 2.56)));
                float wheelShadow = 0.0;
                const float2 wheelCenters[4] = {
                    float2(-0.84,  1.30),
                    float2( 0.84,  1.30),
                    float2(-0.80, -1.735),
                    float2( 0.80, -1.735),
                };
                for (int wheel = 0; wheel < 4; ++wheel) {
                    const float2 wheelDelta = carLocal - wheelCenters[wheel];
                    wheelShadow = max(
                        wheelShadow,
                        1.0 - smoothstep(0.72, 1.10, length(float2(wheelDelta.x / 0.34, wheelDelta.y / 0.52))));
                }
                contactShadow =
                    clamp(max(max(chassis * 0.78, floorTray * 0.46), wheelShadow) *
                              clamp(uniforms.contactParams.w, 0.0, 1.0),
                          0.0,
                          1.0);
                base *= 1.0 - contactShadow * 0.42;
            }

            const float shadow = shadowVisibility(
                shadowTexture,
                shadowSampler,
                input.shadowPosition,
                0.0018,
                uniforms.effectParams.y);
            const float nDotL = max(dot(normal, sun), 0.0);
            const float nDotV = max(dot(normal, viewDir), 0.001);
            const float nDotH = max(dot(normal, halfDir), 0.0);
            const float vDotH = max(dot(viewDir, halfDir), 0.0);
            const float directVisibility = mix(0.18, 1.0, shadow) * (1.0 - contactShadow * 0.12);
            const float wrap = max(dot(normal, sun) * 0.5 + 0.5, 0.0);
            const float skyAmount = max(normal.y, 0.0);
            const float horizonAmount = clamp(1.0 - abs(normal.y), 0.0, 1.0);
            const float occlusion = clamp(materialOcclusion * (1.0 - contactShadow * 0.48), 0.34, 1.0);
            const float3 ambientDirection = normalize(normal + float3(0.0, 0.92, 0.0));
            const float3 environmentAmbient =
                environmentRadiance(skyboxTexture, materialSampler, ambientDirection, uniforms.renderParams.z);
            const float3 skyAmbientColor =
                mix(
                    mix(float3(0.105, 0.120, 0.125), float3(0.34, 0.43, 0.57), pow(skyAmount, 0.65)),
                    environmentAmbient * 0.45,
                    0.62);
            const float3 horizonBounce =
                environmentRadiance(skyboxTexture, materialSampler, normalize(float3(viewDir.x, 0.08, viewDir.z)), uniforms.renderParams.z) *
                horizonAmount * 0.055;
            const float ambientStrength = 0.31 + wrap * 0.055 + skyAmount * 0.12;
            const float3 f0 = mix(float3(0.04), clamp(base, 0.0, 1.0), metallic);
            const float distribution = ggxDistribution(nDotH, roughness);
            const float geometry = smithGeometry(nDotV, nDotL, roughness);
            const float3 fresnel = fresnelSchlick(vDotH, f0);
            const float3 specularBrdf =
                distribution * geometry * fresnel / max(4.0 * nDotV * nDotL, 0.001);
            const float3 diffuseBrdf =
                (float3(1.0) - fresnel) * (1.0 - metallic) * base * roughDiffuse / 3.14159265;
            const float clearcoatDistribution = ggxDistribution(nDotH, clearcoatRoughness);
            const float clearcoatGeometry = smithGeometry(nDotV, nDotL, clearcoatRoughness);
            const float clearcoatFresnel =
                (0.04 + 0.96 * pow(clamp(1.0 - vDotH, 0.0, 1.0), 5.0)) * clearcoatStrength;
            const float clearcoatSpecular =
                clearcoatDistribution * clearcoatGeometry * clearcoatFresnel /
                max(4.0 * nDotV * nDotL, 0.001);
            const float3 directLight =
                (diffuseBrdf + specularBrdf * specularStrength * 2.8 + clearcoatSpecular) *
                nDotL * directVisibility * 3.85;
            float3 clearcoatReflection = float3(0.0);
            if ((material > 3.5 && material < 4.5) ||
                (material > 4.5 && material < 5.5) ||
                (material > 15.5 && material < 16.5)) {
                const float3 reflectDir = reflect(-viewDir, normal);
                float3 skyReflection =
                    environmentRadiance(skyboxTexture, materialSampler, reflectDir, uniforms.renderParams.z);
                if (reflectDir.y < 0.0) {
                    const float groundMix = smoothstep(0.0, 0.70, -reflectDir.y);
                    const float3 darkGround = float3(0.022, 0.022, 0.020);
                    skyReflection = mix(skyReflection, darkGround, groundMix);
                }
                const float fresnel =
                    0.04 + 0.96 * pow(1.0 - max(dot(normal, viewDir), 0.0), 5.0);
                clearcoatReflection =
                    skyReflection * fresnel * clearcoatStrength * environmentReflectionScale *
                    (material > 3.5 && material < 4.5 ? 1.32 : 1.06);
            }
            const float3 ambientLight =
                base * (skyAmbientColor * ambientStrength + horizonBounce) * roughDiffuse * occlusion;
            float3 color = ambientLight + directLight + clearcoatReflection * occlusion + emission;

            const float distanceFromCamera = distance(input.worldPosition, uniforms.cameraPosition.xyz);
            const float fog = smoothstep(300.0, 1400.0, distanceFromCamera);
            const float3 viewRay = normalize(input.worldPosition - uniforms.cameraPosition.xyz);
            const float3 fogColor =
                environmentRadiance(skyboxTexture, materialSampler, viewRay, uniforms.renderParams.z);
            color = mix(color, fogColor, fog * 0.46);
            float alpha = input.color.a * materialAlphaScale;
            const float ghostAlpha = clamp(uniforms.effectParams.w, 0.0, 1.0);
            if (ghostAlpha > 0.01) {
                color = mix(float3(0.18, 0.62, 1.0), color, 0.45) + float3(0.02, 0.08, 0.18);
                alpha *= ghostAlpha;
            }
            return float4(color, alpha);
        }

        fragment float4 fragment_ui(
            VertexOutput input [[stage_in]],
            constant Uniforms& uniforms [[buffer(1)]],
            texture2d<float> hdrTexture [[texture(0)]],
            sampler linearSampler [[sampler(0)]]) {
            const float2 uv = clamp(input.position.xy / max(uniforms.renderParams.xy, float2(1.0)), 0.0, 1.0);
            const float2 texel = 1.0 / max(uniforms.renderParams.xy, float2(1.0));
            float3 blurred = float3(0.0);
            float weight = 0.0;
            for (int y = -2; y <= 2; ++y) {
                for (int x = -2; x <= 2; ++x) {
                    const float2 offset = float2(x, y) * texel * 5.5;
                    const float tapWeight = 1.0 / (1.0 + length(float2(x, y)));
                    blurred += hdrTexture.sample(linearSampler, clamp(uv + offset, 0.0, 1.0)).rgb * tapWeight;
                    weight += tapWeight;
                }
            }
            blurred /= max(weight, 0.001);
            const float2 refractNoise = float2(
                noise21(uv * float2(220.0, 157.0) + uniforms.renderParams.z * 0.12),
                noise21(uv * float2(141.0, 263.0) - uniforms.renderParams.z * 0.10)) - 0.5;
            const float3 refracted =
                hdrTexture.sample(linearSampler, clamp(uv + refractNoise * texel * 7.0, 0.0, 1.0)).rgb;
            const float3 glassScene = lensGrade(mix(blurred, refracted, 0.24), uv, 0.35);
            const float alpha = input.color.a;
            const float3 color =
                mix(glassScene * 0.78 + input.color.rgb * 0.48, input.color.rgb, clamp(alpha * 0.52, 0.0, 0.85));
            return float4(color, alpha);
        }

        fragment float4 fragment_text(
            VertexOutput input [[stage_in]],
            texture2d<float> fontTexture [[texture(0)]],
            sampler fontSampler [[sampler(0)]]) {
            const float coverage = fontTexture.sample(fontSampler, input.data.xy).r;
            const float alpha = smoothstep(0.42, 0.58, coverage);
            return float4(input.color.rgb, input.color.a * alpha);
        }

        fragment float4 fragment_bloom(
            VertexOutput input [[stage_in]],
            constant Uniforms& uniforms [[buffer(1)]],
            texture2d<float> hdrTexture [[texture(0)]],
            sampler linearSampler [[sampler(0)]]) {
            const float2 uv = clamp(input.data.xy, 0.0, 1.0);
            const float2 texel = 1.0 / max(uniforms.renderParams.xy, float2(1.0));
            float3 sum = float3(0.0);
            float weight = 0.0;
            for (int y = -3; y <= 3; ++y) {
                for (int x = -3; x <= 3; ++x) {
                    const float2 offset = float2(x, y) * texel * 3.0;
                    const float w = 1.0 / (1.0 + length(float2(x, y)));
                    const float3 c = hdrTexture.sample(linearSampler, uv + offset).rgb;
                    const float luma = dot(c, float3(0.2126, 0.7152, 0.0722));
                    sum += max(c - 0.88, float3(0.0)) * smoothstep(0.66, 2.20, luma) * w;
                    weight += w;
                }
            }
            return float4(sum / max(weight, 0.001), 1.0);
        }

        fragment float4 fragment_post(
            VertexOutput input [[stage_in]],
            constant Uniforms& uniforms [[buffer(1)]],
            texture2d<float> hdrTexture [[texture(0)]],
            texture2d<float> bloomTexture [[texture(1)]],
            texture2d<float> bloomQuarterTexture [[texture(2)]],
            texture2d<float> bloomEighthTexture [[texture(3)]],
            sampler linearSampler [[sampler(0)]]) {
            const float2 uv = clamp(input.data.xy, 0.0, 1.0);
            const float speedBlur = clamp(uniforms.effectParams.x, 0.0, 1.0);
            const float2 sampleUv = uv;
            const float2 center = float2(0.50, 0.60);
            const float2 radial = sampleUv - center;
            float3 color = hdrTexture.sample(linearSampler, sampleUv).rgb;
            const float2 texel = 1.0 / max(uniforms.renderParams.xy, float2(1.0));
            const float3 neighborAverage =
                (hdrTexture.sample(linearSampler, clamp(sampleUv + float2(texel.x, 0.0), 0.0, 1.0)).rgb +
                 hdrTexture.sample(linearSampler, clamp(sampleUv - float2(texel.x, 0.0), 0.0, 1.0)).rgb +
                 hdrTexture.sample(linearSampler, clamp(sampleUv + float2(0.0, texel.y), 0.0, 1.0)).rgb +
                 hdrTexture.sample(linearSampler, clamp(sampleUv - float2(0.0, texel.y), 0.0, 1.0)).rgb) * 0.25;
            color += (color - neighborAverage) * 0.10;
            float blurWeight = 1.0;
            for (int tap = 1; tap <= 5; ++tap) {
                const float t = float(tap) / 5.0;
                const float centerProtection = smoothstep(0.30, 0.72, length(radial));
                const float2 blurUv = sampleUv - radial * speedBlur * centerProtection * t * 0.012;
                color += hdrTexture.sample(linearSampler, blurUv).rgb * 0.045;
                blurWeight += 0.045;
            }
            color /= blurWeight;
            color += bloomTexture.sample(linearSampler, uv).rgb * 0.86;
            color += bloomQuarterTexture.sample(linearSampler, uv).rgb * 0.72;
            color += bloomEighthTexture.sample(linearSampler, uv).rgb * 0.46;
            return float4(lensGrade(color, uv, 1.0), 1.0);
        }
    )METAL";
}

void copyUniforms(
    Uniforms& uniforms,
    const Mat4& mvp,
    const Mat4& model,
    const Mat4& shadowMvp,
    Vec3 cameraPosition) {
    std::memcpy(uniforms.mvp, mvp.values.data(), sizeof(uniforms.mvp));
    std::memcpy(uniforms.model, model.values.data(), sizeof(uniforms.model));
    std::memcpy(uniforms.shadowMvp, shadowMvp.values.data(), sizeof(uniforms.shadowMvp));
    uniforms.cameraPosition[0] = cameraPosition.x;
    uniforms.cameraPosition[1] = cameraPosition.y;
    uniforms.cameraPosition[2] = cameraPosition.z;
    uniforms.cameraPosition[3] = 1.0F;
}

}  // namespace

struct MetalRenderer::Impl {
    SDL_MetalView metalView = nullptr;
    CAMetalLayer* layer = nil;
    id<MTLDevice> device = nil;
    id<MTLCommandQueue> commandQueue = nil;
    id<MTLRenderPipelineState> skyPipeline = nil;
    id<MTLRenderPipelineState> worldPipeline = nil;
    id<MTLRenderPipelineState> fxPipeline = nil;
    id<MTLRenderPipelineState> uiPipeline = nil;
    id<MTLRenderPipelineState> textPipeline = nil;
    id<MTLRenderPipelineState> worldTextPipeline = nil;
    id<MTLRenderPipelineState> shadowPipeline = nil;
    id<MTLRenderPipelineState> bloomPipeline = nil;
    id<MTLRenderPipelineState> postPipeline = nil;
    id<MTLDepthStencilState> worldDepthState = nil;
    id<MTLDepthStencilState> skyDepthState = nil;
    id<MTLDepthStencilState> fxDepthState = nil;
    id<MTLDepthStencilState> textDepthState = nil;
    id<MTLDepthStencilState> shadowDepthState = nil;
    id<MTLBuffer> skyBuffer = nil;
    id<MTLBuffer> groundBuffer = nil;
    id<MTLBuffer> carBodyBuffer = nil;
    id<MTLBuffer> wheelBuffer = nil;
    id<MTLBuffer> steeringWheelBuffer = nil;
    id<MTLBuffer> suspensionBuffer = nil;
    id<MTLBuffer> smokeBuffer = nil;
    id<MTLBuffer> skidmarkBuffer = nil;
    id<MTLBuffer> uiBuffer = nil;
    id<MTLBuffer> textBuffer = nil;
    id<MTLBuffer> steeringDisplayBuffer = nil;
    id<MTLTexture> fontTexture = nil;
    id<MTLTexture> depthTexture = nil;
    id<MTLTexture> msaaDepthTexture = nil;
    id<MTLTexture> hdrTexture = nil;
    id<MTLTexture> msaaHdrTexture = nil;
    id<MTLTexture> bloomTexture = nil;
    id<MTLTexture> bloomQuarterTexture = nil;
    id<MTLTexture> bloomEighthTexture = nil;
    id<MTLTexture> shadowDepthTexture = nil;
    id<MTLTexture> asphaltAlbedoTexture = nil;
    id<MTLTexture> asphaltNormalTexture = nil;
    id<MTLTexture> asphaltRoughnessTexture = nil;
    id<MTLTexture> asphaltHeightTexture = nil;
    id<MTLTexture> concreteAlbedoTexture = nil;
    id<MTLTexture> concreteNormalTexture = nil;
    id<MTLTexture> concreteRoughnessTexture = nil;
    id<MTLTexture> grassAlbedoTexture = nil;
    id<MTLTexture> grassNormalTexture = nil;
    id<MTLTexture> grassRoughnessTexture = nil;
    id<MTLTexture> grassHeightTexture = nil;
    id<MTLTexture> carbonAlbedoTexture = nil;
    id<MTLTexture> carbonNormalTexture = nil;
    id<MTLTexture> carbonRoughnessTexture = nil;
    id<MTLTexture> skyboxTexture = nil;
    id<MTLTexture> smokeTexture = nil;
    id<MTLSamplerState> fontSampler = nil;
    id<MTLSamplerState> materialSampler = nil;
    id<MTLSamplerState> shadowSampler = nil;
    MTLRenderPassDescriptor* passDescriptor = nil;
    MTLRenderPassDescriptor* bloomPassDescriptor = nil;
    MTLRenderPassDescriptor* bloomQuarterPassDescriptor = nil;
    MTLRenderPassDescriptor* bloomEighthPassDescriptor = nil;
    MTLRenderPassDescriptor* finalPassDescriptor = nil;
    MTLRenderPassDescriptor* shadowPassDescriptor = nil;
    std::array<Vertex, kMaxUiVertices> uiVertices{};
    std::array<Vertex, kMaxTextVertices> textVertices{};
    std::array<Vertex, kMaxSteeringDisplayVertices> steeringDisplayVertices{};
    std::array<Vertex, kMaxSkidmarkVertices> skidmarkVertices{};
    NSUInteger skyVertexCount = 0;
    NSUInteger groundVertexCount = 0;
    NSUInteger carBodyVertexCount = 0;
    NSUInteger wheelVertexCount = 0;
    NSUInteger steeringWheelVertexCount = 0;
    NSUInteger suspensionVertexCount = 0;
    NSUInteger skidmarkVertexCount = 0;
    int drawableWidth = 0;
    int drawableHeight = 0;
    float renderScale = 1.0F;
    int shadowMapSize = 2048;
    float elapsedSeconds = 0.0F;
    float cameraTrauma = 0.0F;
    bool cameraInitialized = false;
    Uint64 previousRenderTicks = 0;
    Vec3 cameraEye{};
    Vec3 cameraTarget{};
    int previousCameraMode = -1;
    Vec3 lastSkidmarkPosition{};
    bool hasLastSkidmark = false;
    std::string error;

    bool createPipelines() {
        NSError* compileError = nil;
        NSString* source = [NSString stringWithUTF8String:shaderSource()];
        id<MTLLibrary> library = [device newLibraryWithSource:source options:nil error:&compileError];
        if (library == nil) {
            error = compileError != nil ? compileError.localizedDescription.UTF8String : "Unknown shader compile error";
            return false;
        }

        id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertex_main"];
        id<MTLFunction> skyVertexFunction = [library newFunctionWithName:@"vertex_sky"];
        id<MTLFunction> skyFragment = [library newFunctionWithName:@"fragment_sky"];
        id<MTLFunction> worldFragment = [library newFunctionWithName:@"fragment_world"];
        id<MTLFunction> uiFragment = [library newFunctionWithName:@"fragment_ui"];
        id<MTLFunction> textFragment = [library newFunctionWithName:@"fragment_text"];
        id<MTLFunction> bloomFragment = [library newFunctionWithName:@"fragment_bloom"];
        id<MTLFunction> postFragment = [library newFunctionWithName:@"fragment_post"];

        MTLRenderPipelineDescriptor* skyDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        skyDescriptor.vertexFunction = skyVertexFunction;
        skyDescriptor.fragmentFunction = skyFragment;
        skyDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA16Float;
        skyDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        skyDescriptor.rasterSampleCount = kWorldSampleCount;
        NSError* pipelineError = nil;
        skyPipeline = [device newRenderPipelineStateWithDescriptor:skyDescriptor error:&pipelineError];
        if (skyPipeline == nil) {
            error = pipelineError != nil ? pipelineError.localizedDescription.UTF8String : "Could not create sky pipeline";
            return false;
        }

        MTLRenderPipelineDescriptor* worldDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        worldDescriptor.vertexFunction = vertexFunction;
        worldDescriptor.fragmentFunction = worldFragment;
        worldDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA16Float;
        worldDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        worldDescriptor.rasterSampleCount = kWorldSampleCount;
        worldPipeline = [device newRenderPipelineStateWithDescriptor:worldDescriptor error:&pipelineError];
        if (worldPipeline == nil) {
            error = pipelineError != nil ? pipelineError.localizedDescription.UTF8String : "Could not create world pipeline";
            return false;
        }

        MTLRenderPipelineDescriptor* fxDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        fxDescriptor.vertexFunction = vertexFunction;
        fxDescriptor.fragmentFunction = worldFragment;
        fxDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA16Float;
        fxDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        fxDescriptor.rasterSampleCount = kWorldSampleCount;
        fxDescriptor.colorAttachments[0].blendingEnabled = YES;
        fxDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        fxDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        fxDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        fxDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        fxPipeline = [device newRenderPipelineStateWithDescriptor:fxDescriptor error:&pipelineError];
        if (fxPipeline == nil) {
            error = pipelineError != nil ? pipelineError.localizedDescription.UTF8String : "Could not create FX pipeline";
            return false;
        }

        MTLRenderPipelineDescriptor* uiDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        uiDescriptor.vertexFunction = vertexFunction;
        uiDescriptor.fragmentFunction = uiFragment;
        uiDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        uiDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        uiDescriptor.colorAttachments[0].blendingEnabled = YES;
        uiDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        uiDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        uiDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        uiDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        uiPipeline = [device newRenderPipelineStateWithDescriptor:uiDescriptor error:&pipelineError];
        if (uiPipeline == nil) {
            error = pipelineError != nil ? pipelineError.localizedDescription.UTF8String : "Could not create UI pipeline";
            return false;
        }

        MTLRenderPipelineDescriptor* textDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        textDescriptor.vertexFunction = vertexFunction;
        textDescriptor.fragmentFunction = textFragment;
        textDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        textDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        textDescriptor.colorAttachments[0].blendingEnabled = YES;
        textDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        textDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        textDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        textDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        textPipeline = [device newRenderPipelineStateWithDescriptor:textDescriptor error:&pipelineError];
        if (textPipeline == nil) {
            error = pipelineError != nil ? pipelineError.localizedDescription.UTF8String : "Could not create text pipeline";
            return false;
        }

        MTLRenderPipelineDescriptor* worldTextDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        worldTextDescriptor.vertexFunction = vertexFunction;
        worldTextDescriptor.fragmentFunction = textFragment;
        worldTextDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA16Float;
        worldTextDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        worldTextDescriptor.rasterSampleCount = kWorldSampleCount;
        worldTextDescriptor.colorAttachments[0].blendingEnabled = YES;
        worldTextDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        worldTextDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        worldTextDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        worldTextDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        worldTextPipeline = [device newRenderPipelineStateWithDescriptor:worldTextDescriptor error:&pipelineError];
        if (worldTextPipeline == nil) {
            error = pipelineError != nil ? pipelineError.localizedDescription.UTF8String : "Could not create world text pipeline";
            return false;
        }

        MTLRenderPipelineDescriptor* shadowDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        shadowDescriptor.vertexFunction = vertexFunction;
        shadowDescriptor.fragmentFunction = nil;
        shadowDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        shadowPipeline = [device newRenderPipelineStateWithDescriptor:shadowDescriptor error:&pipelineError];
        if (shadowPipeline == nil) {
            error = pipelineError != nil ? pipelineError.localizedDescription.UTF8String : "Could not create shadow pipeline";
            return false;
        }

        MTLRenderPipelineDescriptor* bloomDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        bloomDescriptor.vertexFunction = skyVertexFunction;
        bloomDescriptor.fragmentFunction = bloomFragment;
        bloomDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA16Float;
        bloomPipeline = [device newRenderPipelineStateWithDescriptor:bloomDescriptor error:&pipelineError];
        if (bloomPipeline == nil) {
            error = pipelineError != nil ? pipelineError.localizedDescription.UTF8String : "Could not create bloom pipeline";
            return false;
        }

        MTLRenderPipelineDescriptor* postDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        postDescriptor.vertexFunction = skyVertexFunction;
        postDescriptor.fragmentFunction = postFragment;
        postDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        postPipeline = [device newRenderPipelineStateWithDescriptor:postDescriptor error:&pipelineError];
        if (postPipeline == nil) {
            error = pipelineError != nil ? pipelineError.localizedDescription.UTF8String : "Could not create post pipeline";
            return false;
        }

        MTLDepthStencilDescriptor* worldDepthDescriptor = [[MTLDepthStencilDescriptor alloc] init];
        worldDepthDescriptor.depthCompareFunction = MTLCompareFunctionLess;
        worldDepthDescriptor.depthWriteEnabled = YES;
        worldDepthState = [device newDepthStencilStateWithDescriptor:worldDepthDescriptor];

        MTLDepthStencilDescriptor* skyDepthDescriptor = [[MTLDepthStencilDescriptor alloc] init];
        skyDepthDescriptor.depthCompareFunction = MTLCompareFunctionAlways;
        skyDepthDescriptor.depthWriteEnabled = NO;
        skyDepthState = [device newDepthStencilStateWithDescriptor:skyDepthDescriptor];

        MTLDepthStencilDescriptor* fxDepthDescriptor = [[MTLDepthStencilDescriptor alloc] init];
        fxDepthDescriptor.depthCompareFunction = MTLCompareFunctionLess;
        fxDepthDescriptor.depthWriteEnabled = NO;
        fxDepthState = [device newDepthStencilStateWithDescriptor:fxDepthDescriptor];

        MTLDepthStencilDescriptor* textDepthDescriptor = [[MTLDepthStencilDescriptor alloc] init];
        textDepthDescriptor.depthCompareFunction = MTLCompareFunctionAlways;
        textDepthDescriptor.depthWriteEnabled = NO;
        textDepthState = [device newDepthStencilStateWithDescriptor:textDepthDescriptor];

        MTLDepthStencilDescriptor* shadowDepthDescriptor = [[MTLDepthStencilDescriptor alloc] init];
        shadowDepthDescriptor.depthCompareFunction = MTLCompareFunctionLess;
        shadowDepthDescriptor.depthWriteEnabled = YES;
        shadowDepthState = [device newDepthStencilStateWithDescriptor:shadowDepthDescriptor];
        return true;
    }

    bool createGeometry(const Track& track) {
        const std::vector<Vertex> sky = makeSkyGeometry();
        skyVertexCount = sky.size();
        skyBuffer = [device newBufferWithBytes:sky.data()
                                        length:sky.size() * sizeof(Vertex)
                                       options:MTLResourceStorageModeShared];

        const std::vector<Vertex> ground = makeGroundGeometry(track);
        groundVertexCount = ground.size();
        groundBuffer = [device newBufferWithBytes:ground.data()
                                          length:ground.size() * sizeof(Vertex)
                                         options:MTLResourceStorageModeShared];

        const std::vector<Vertex> carBody =
            loadObjGeometryOrFallback("assets/meshes/car.obj", makeCarBodyGeometry());
        carBodyVertexCount = carBody.size();
        carBodyBuffer = [device newBufferWithBytes:carBody.data()
                                            length:carBody.size() * sizeof(Vertex)
                                           options:MTLResourceStorageModeShared];

        const std::vector<Vertex> wheel =
            loadObjGeometryOrFallback("assets/meshes/wheel.obj", makeWheelGeometry());
        wheelVertexCount = wheel.size();
        wheelBuffer = [device newBufferWithBytes:wheel.data()
                                          length:wheel.size() * sizeof(Vertex)
                                         options:MTLResourceStorageModeShared];

        const std::vector<Vertex> steeringWheel =
            loadObjGeometryOrFallback("assets/meshes/steering_wheel.obj", makeSteeringWheelGeometry());
        steeringWheelVertexCount = steeringWheel.size();
        steeringWheelBuffer = [device newBufferWithBytes:steeringWheel.data()
                                                  length:steeringWheel.size() * sizeof(Vertex)
                                                 options:MTLResourceStorageModeShared];

        suspensionBuffer = [device newBufferWithLength:kMaxSuspensionVertices * sizeof(Vertex)
                                               options:MTLResourceStorageModeShared];
        smokeBuffer = [device newBufferWithLength:kMaxSmokeVertices * sizeof(Vertex)
                                          options:MTLResourceStorageModeShared];
        skidmarkBuffer = [device newBufferWithLength:skidmarkVertices.size() * sizeof(Vertex)
                                             options:MTLResourceStorageModeShared];
        uiBuffer = [device newBufferWithLength:uiVertices.size() * sizeof(Vertex)
                                       options:MTLResourceStorageModeShared];
        textBuffer = [device newBufferWithLength:textVertices.size() * sizeof(Vertex)
                                         options:MTLResourceStorageModeShared];
        steeringDisplayBuffer = [device newBufferWithLength:steeringDisplayVertices.size() * sizeof(Vertex)
                                                     options:MTLResourceStorageModeShared];
        if (skyBuffer == nil || groundBuffer == nil || carBodyBuffer == nil || wheelBuffer == nil ||
            steeringWheelBuffer == nil || suspensionBuffer == nil ||
            smokeBuffer == nil || skidmarkBuffer == nil || uiBuffer == nil || textBuffer == nil ||
            steeringDisplayBuffer == nil) {
            error = "Could not allocate Metal geometry buffers";
            return false;
        }
        return true;
    }

    bool createFont() {
        constexpr int cellSize = 16;
        constexpr int atlasWidth = cellSize * 16;
        constexpr int atlasHeight = cellSize * 8;
        std::array<unsigned char, atlasWidth * atlasHeight> pixels{};
        for (int character = 0; character < 128; ++character) {
            const Glyph glyph = glyphFor(static_cast<char>(character));
            const int originX = (character % 16) * cellSize;
            const int originY = (character / 16) * cellSize;
            std::array<unsigned char, cellSize * cellSize> coverage{};
            bool hasInk = false;
            for (int row = 0; row < cellSize; ++row) {
                for (int column = 0; column < cellSize; ++column) {
                    const int glyphColumn = (column - 3) / 2;
                    const int glyphRow = (row - 1) / 2;
                    const bool insideGlyph =
                        glyphColumn >= 0 && glyphColumn < 5 && glyphRow >= 0 && glyphRow < 7 &&
                        (glyph[glyphRow] & (1U << (4 - glyphColumn))) != 0;
                    coverage[row * cellSize + column] = insideGlyph ? 1U : 0U;
                    hasInk = hasInk || insideGlyph;
                }
            }

            for (int row = 0; row < cellSize; ++row) {
                for (int column = 0; column < cellSize; ++column) {
                    if (!hasInk) {
                        pixels[(originY + row) * atlasWidth + originX + column] = 0;
                        continue;
                    }
                    const bool inside = coverage[row * cellSize + column] != 0;
                    float bestDistance = 32.0F;
                    for (int sampleY = 0; sampleY < cellSize; ++sampleY) {
                        for (int sampleX = 0; sampleX < cellSize; ++sampleX) {
                            const bool sampleInside = coverage[sampleY * cellSize + sampleX] != 0;
                            if (sampleInside == inside) {
                                continue;
                            }
                            const float dx = static_cast<float>(column - sampleX);
                            const float dy = static_cast<float>(row - sampleY);
                            bestDistance = std::min(bestDistance, std::sqrt(dx * dx + dy * dy));
                        }
                    }
                    const float signedDistance = inside ? bestDistance : -bestDistance;
                    const float encoded = std::clamp(0.5F + signedDistance / 6.0F, 0.0F, 1.0F);
                    pixels[(originY + row) * atlasWidth + originX + column] =
                        static_cast<unsigned char>(encoded * 255.0F);
                }
            }
        }

        MTLTextureDescriptor* descriptor =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm
                                                               width:atlasWidth
                                                              height:atlasHeight
                                                           mipmapped:NO];
        descriptor.usage = MTLTextureUsageShaderRead;
        fontTexture = [device newTextureWithDescriptor:descriptor];
        if (fontTexture == nil) {
            error = "Could not create debug font texture";
            return false;
        }
        [fontTexture replaceRegion:MTLRegionMake2D(0, 0, atlasWidth, atlasHeight)
                       mipmapLevel:0
                         withBytes:pixels.data()
                       bytesPerRow:atlasWidth];

        MTLSamplerDescriptor* samplerDescriptor = [[MTLSamplerDescriptor alloc] init];
        samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
        samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
        samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
        samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
        fontSampler = [device newSamplerStateWithDescriptor:samplerDescriptor];
        return fontSampler != nil;
    }

    id<MTLTexture> createTextureFromFile(
        const char* relativePath,
        std::array<unsigned char, 4> fallbackRgba) {
        LoadedImage image = loadImageRgba8(FileSystem::findDataFile(relativePath));
        int width = image.width;
        int height = image.height;
        std::vector<unsigned char> fallback;
        const unsigned char* bytes = nullptr;
        if (width > 0 && height > 0 && !image.rgba.empty()) {
            bytes = image.rgba.data();
        } else {
            width = 4;
            height = 4;
            fallback.resize(static_cast<std::size_t>(width * height * 4));
            for (int index = 0; index < width * height; ++index) {
                fallback[static_cast<std::size_t>(index) * 4U + 0U] = fallbackRgba[0];
                fallback[static_cast<std::size_t>(index) * 4U + 1U] = fallbackRgba[1];
                fallback[static_cast<std::size_t>(index) * 4U + 2U] = fallbackRgba[2];
                fallback[static_cast<std::size_t>(index) * 4U + 3U] = fallbackRgba[3];
            }
            bytes = fallback.data();
        }

        MTLTextureDescriptor* descriptor =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                               width:static_cast<NSUInteger>(width)
                                                              height:static_cast<NSUInteger>(height)
                                                           mipmapped:YES];
        descriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
        id<MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
        if (texture != nil) {
            [texture replaceRegion:MTLRegionMake2D(0, 0, static_cast<NSUInteger>(width), static_cast<NSUInteger>(height))
                        mipmapLevel:0
                          withBytes:bytes
                        bytesPerRow:static_cast<NSUInteger>(width) * 4U];
            id<MTLCommandBuffer> mipCommandBuffer = [commandQueue commandBuffer];
            id<MTLBlitCommandEncoder> blitEncoder = [mipCommandBuffer blitCommandEncoder];
            if (blitEncoder != nil) {
                [blitEncoder generateMipmapsForTexture:texture];
                [blitEncoder endEncoding];
                [mipCommandBuffer commit];
                [mipCommandBuffer waitUntilCompleted];
            }
        }
        return texture;
    }

    bool createMaterialTextures() {
        asphaltAlbedoTexture = createTextureFromFile(
            "assets/textures/asphalt_albedo.webp", {58, 58, 60, 255});
        asphaltNormalTexture = createTextureFromFile(
            "assets/textures/asphalt_normal.webp", {128, 128, 255, 255});
        asphaltRoughnessTexture = createTextureFromFile(
            "assets/textures/asphalt_roughness.webp", {210, 210, 210, 255});
        asphaltHeightTexture = createTextureFromFile(
            "assets/textures/asphalt_height.webp", {128, 128, 128, 255});
        concreteAlbedoTexture = createTextureFromFile(
            "assets/textures/concrete_albedo.webp", {146, 144, 136, 255});
        concreteNormalTexture = createTextureFromFile(
            "assets/textures/concrete_normal.webp", {128, 128, 255, 255});
        concreteRoughnessTexture = createTextureFromFile(
            "assets/textures/concrete_roughness.webp", {215, 215, 215, 255});
        grassAlbedoTexture = createTextureFromFile(
            "assets/textures/grass_albedo.webp", {62, 116, 52, 255});
        grassNormalTexture = createTextureFromFile(
            "assets/textures/grass_normal.webp", {128, 128, 255, 255});
        grassRoughnessTexture = createTextureFromFile(
            "assets/textures/grass_roughness.webp", {232, 232, 232, 255});
        grassHeightTexture = createTextureFromFile(
            "assets/textures/grass_height.webp", {128, 128, 128, 255});
        carbonAlbedoTexture = createTextureFromFile(
            "assets/textures/carbon_albedo.webp", {20, 22, 26, 255});
        carbonNormalTexture = createTextureFromFile(
            "assets/textures/carbon_normal.webp", {128, 128, 255, 255});
        carbonRoughnessTexture = createTextureFromFile(
            "assets/textures/carbon_roughness.webp", {72, 72, 72, 255});
        skyboxTexture = createTextureFromFile(
            "assets/textures/skybox_ims_evening.webp", {96, 138, 190, 255});
        smokeTexture = createTextureFromFile(
            "assets/textures/smoke_puff.webp", {180, 180, 174, 0});

        MTLSamplerDescriptor* materialDescriptor = [[MTLSamplerDescriptor alloc] init];
        materialDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
        materialDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
        materialDescriptor.mipFilter = MTLSamplerMipFilterLinear;
        materialDescriptor.maxAnisotropy = 12;
        materialDescriptor.sAddressMode = MTLSamplerAddressModeRepeat;
        materialDescriptor.tAddressMode = MTLSamplerAddressModeRepeat;
        materialSampler = [device newSamplerStateWithDescriptor:materialDescriptor];

        MTLSamplerDescriptor* shadowDescriptor = [[MTLSamplerDescriptor alloc] init];
        shadowDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
        shadowDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
        shadowDescriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
        shadowDescriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
        shadowDescriptor.compareFunction = MTLCompareFunctionLessEqual;
        shadowSampler = [device newSamplerStateWithDescriptor:shadowDescriptor];

        return asphaltAlbedoTexture != nil && asphaltNormalTexture != nil &&
               asphaltRoughnessTexture != nil && asphaltHeightTexture != nil &&
               concreteAlbedoTexture != nil && concreteNormalTexture != nil &&
               concreteRoughnessTexture != nil &&
               grassAlbedoTexture != nil && grassNormalTexture != nil &&
               grassRoughnessTexture != nil && grassHeightTexture != nil &&
               carbonAlbedoTexture != nil && carbonNormalTexture != nil &&
               carbonRoughnessTexture != nil && skyboxTexture != nil &&
               smokeTexture != nil && materialSampler != nil && shadowSampler != nil;
    }

    void resizeIfNeeded(int windowPixelWidth, int windowPixelHeight) {
        const int width = std::max(1, static_cast<int>(static_cast<float>(windowPixelWidth) * renderScale));
        const int height = std::max(1, static_cast<int>(static_cast<float>(windowPixelHeight) * renderScale));
        if (width == drawableWidth && height == drawableHeight && depthTexture != nil &&
            msaaDepthTexture != nil && hdrTexture != nil && msaaHdrTexture != nil &&
            bloomTexture != nil && bloomQuarterTexture != nil && bloomEighthTexture != nil) {
            return;
        }
        drawableWidth = width;
        drawableHeight = height;
        layer.drawableSize = CGSizeMake(width, height);

        MTLTextureDescriptor* depthDescriptor =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                               width:width
                                                              height:height
                                                           mipmapped:NO];
        depthDescriptor.usage = MTLTextureUsageRenderTarget;
        depthDescriptor.storageMode = MTLStorageModePrivate;
        depthTexture = [device newTextureWithDescriptor:depthDescriptor];

        MTLTextureDescriptor* msaaDepthDescriptor =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                               width:width
                                                              height:height
                                                           mipmapped:NO];
        msaaDepthDescriptor.textureType = MTLTextureType2DMultisample;
        msaaDepthDescriptor.sampleCount = kWorldSampleCount;
        msaaDepthDescriptor.usage = MTLTextureUsageRenderTarget;
        msaaDepthDescriptor.storageMode = MTLStorageModePrivate;
        msaaDepthTexture = [device newTextureWithDescriptor:msaaDepthDescriptor];

        MTLTextureDescriptor* hdrDescriptor =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                                                               width:width
                                                              height:height
                                                           mipmapped:NO];
        hdrDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        hdrDescriptor.storageMode = MTLStorageModePrivate;
        hdrTexture = [device newTextureWithDescriptor:hdrDescriptor];

        MTLTextureDescriptor* msaaHdrDescriptor =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                                                               width:width
                                                              height:height
                                                           mipmapped:NO];
        msaaHdrDescriptor.textureType = MTLTextureType2DMultisample;
        msaaHdrDescriptor.sampleCount = kWorldSampleCount;
        msaaHdrDescriptor.usage = MTLTextureUsageRenderTarget;
        msaaHdrDescriptor.storageMode = MTLStorageModePrivate;
        msaaHdrTexture = [device newTextureWithDescriptor:msaaHdrDescriptor];

        MTLTextureDescriptor* bloomDescriptor =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                                                               width:std::max(1, width / 2)
                                                              height:std::max(1, height / 2)
                                                           mipmapped:NO];
        bloomDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        bloomDescriptor.storageMode = MTLStorageModePrivate;
        bloomTexture = [device newTextureWithDescriptor:bloomDescriptor];

        MTLTextureDescriptor* bloomQuarterDescriptor =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                                                               width:std::max(1, width / 4)
                                                              height:std::max(1, height / 4)
                                                           mipmapped:NO];
        bloomQuarterDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        bloomQuarterDescriptor.storageMode = MTLStorageModePrivate;
        bloomQuarterTexture = [device newTextureWithDescriptor:bloomQuarterDescriptor];

        MTLTextureDescriptor* bloomEighthDescriptor =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                                                               width:std::max(1, width / 8)
                                                              height:std::max(1, height / 8)
                                                           mipmapped:NO];
        bloomEighthDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        bloomEighthDescriptor.storageMode = MTLStorageModePrivate;
        bloomEighthTexture = [device newTextureWithDescriptor:bloomEighthDescriptor];

        if (shadowDepthTexture == nil) {
            MTLTextureDescriptor* shadowDescriptor =
                [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                                   width:static_cast<NSUInteger>(shadowMapSize)
                                                                  height:static_cast<NSUInteger>(shadowMapSize)
                                                               mipmapped:NO];
            shadowDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
            shadowDescriptor.storageMode = MTLStorageModePrivate;
            shadowDepthTexture = [device newTextureWithDescriptor:shadowDescriptor];
        }
    }

    std::size_t buildSuspensionVertices(
        const RenderScene& scene,
        const Mat4& vehicleFrameWorld,
        const Mat4& chassisWorld) {
        std::vector<Vertex> vertices;
        vertices.reserve(kMaxSuspensionVertices);
        const auto worldPoint = [](const Mat4& transform, Vec3 local) {
            const ClipPoint point = transformPoint(transform, local);
            return Vec3{point.x, point.y, point.z};
        };
        const auto appendSuspensionRod = [&](Vec3 chassisLocal, Vec3 hubLocal, float halfWidth) {
            if (vertices.size() + 36U > kMaxSuspensionVertices) {
                return;
            }
            appendRod(
                vertices,
                worldPoint(chassisWorld, chassisLocal),
                worldPoint(vehicleFrameWorld, hubLocal),
                halfWidth,
                {0.050F, 0.056F, 0.060F, 1.0F},
                kMaterialMetal);
        };
        const auto appendCorner = [&](float side, float hubTravel, float z, bool front) {
            const float innerX = side * (front ? 0.30F : 0.36F);
            const float hubX = side * (front ? kVisualFrontWheelX : kVisualRearWheelX);
            const float hubY = kVisualWheelY + hubTravel;
            const float forwardBias = front ? 1.0F : -1.0F;
            appendSuspensionRod({innerX, 0.47F, z + 0.24F * forwardBias}, {hubX, hubY + 0.12F, z + 0.15F * forwardBias}, 0.015F);
            appendSuspensionRod({innerX, 0.47F, z - 0.16F * forwardBias}, {hubX, hubY + 0.12F, z - 0.12F * forwardBias}, 0.015F);
            appendSuspensionRod({innerX, 0.28F, z + 0.26F * forwardBias}, {hubX, hubY - 0.11F, z + 0.16F * forwardBias}, 0.017F);
            appendSuspensionRod({innerX, 0.28F, z - 0.18F * forwardBias}, {hubX, hubY - 0.11F, z - 0.13F * forwardBias}, 0.017F);
            appendSuspensionRod({side * 0.20F, 0.57F, z - 0.03F * forwardBias}, {hubX, hubY + 0.04F, z}, 0.014F);
        };
        appendCorner(-1.0F, scene.vehicle.frontLeftWheelHubTravelM, kVisualFrontWheelZ, true);
        appendCorner(1.0F, scene.vehicle.frontRightWheelHubTravelM, kVisualFrontWheelZ, true);
        appendCorner(-1.0F, scene.vehicle.rearLeftWheelHubTravelM, kVisualRearWheelZ, false);
        appendCorner(1.0F, scene.vehicle.rearRightWheelHubTravelM, kVisualRearWheelZ, false);

        suspensionVertexCount = static_cast<NSUInteger>(std::min<std::size_t>(vertices.size(), kMaxSuspensionVertices));
        if (suspensionVertexCount > 0) {
            std::memcpy(suspensionBuffer.contents, vertices.data(), suspensionVertexCount * sizeof(Vertex));
        }
        return suspensionVertexCount;
    }

    std::size_t buildSmokeVertices(
        const RenderScene& scene,
        Vec3 carForward,
        Vec3 carRight,
        Vec3 cameraRight,
        Vec3 cameraUp) {
        const float rearUsage = std::max(scene.vehicle.rearLeftTireUsage, scene.vehicle.rearRightTireUsage);
        const float rearSlip = std::max(std::abs(scene.vehicle.rearLeftLongitudinalSlip),
                                        std::abs(scene.vehicle.rearRightLongitudinalSlip));
        float intensity =
            std::clamp((rearUsage - 0.90F) * 2.2F + (rearSlip - 0.80F) * 0.30F, 0.0F, 1.0F);
        if (scene.surface == TrackSurface::Grass) {
            intensity = std::max(intensity, std::clamp(scene.vehicle.speedMps / 38.0F, 0.0F, 1.0F) * 0.65F);
        } else if (scene.surface == TrackSurface::Asphalt) {
            intensity *= 0.42F;
        }
        const float sparkIntensity =
            std::clamp(scene.vehicle.floorStrikeIntensity * std::clamp(scene.vehicle.speedMps / 24.0F, 0.0F, 1.0F), 0.0F, 1.0F);
        if (intensity < 0.035F && sparkIntensity < 0.05F) {
            return 0;
        }

        std::array<Vertex, kMaxSmokeVertices> vertices{};
        std::size_t count = 0;
        const auto appendSmokeQuad = [&](Vec3 center, float width, float height, float alpha) {
            if (count + 6 > vertices.size()) {
                return;
            }
            const Vec3 xAxis{cameraRight.x * width, cameraRight.y * width, cameraRight.z * width};
            const Vec3 yAxis{cameraUp.x * height, cameraUp.y * height, cameraUp.z * height};
            const Vec3 p0{center.x - xAxis.x - yAxis.x, center.y - xAxis.y - yAxis.y, center.z - xAxis.z - yAxis.z};
            const Vec3 p1{center.x + xAxis.x - yAxis.x, center.y + xAxis.y - yAxis.y, center.z + xAxis.z - yAxis.z};
            const Vec3 p2{center.x + xAxis.x + yAxis.x, center.y + xAxis.y + yAxis.y, center.z + xAxis.z + yAxis.z};
            const Vec3 p3{center.x - xAxis.x + yAxis.x, center.y - xAxis.y + yAxis.y, center.z - xAxis.z + yAxis.z};
            const std::array<float, 4> color =
                scene.surface == TrackSurface::Grass
                    ? std::array<float, 4>{0.56F, 0.48F, 0.33F, alpha}
                    : std::array<float, 4>{0.48F, 0.49F, 0.47F, alpha};
            constexpr Vec3 normal{0.0F, 1.0F, 0.0F};
            vertices[count++] = makeEffectVertex(p0, color, normal, kMaterialSmoke, 0.0F, 1.0F);
            vertices[count++] = makeEffectVertex(p1, color, normal, kMaterialSmoke, 1.0F, 1.0F);
            vertices[count++] = makeEffectVertex(p2, color, normal, kMaterialSmoke, 1.0F, 0.0F);
            vertices[count++] = makeEffectVertex(p0, color, normal, kMaterialSmoke, 0.0F, 1.0F);
            vertices[count++] = makeEffectVertex(p2, color, normal, kMaterialSmoke, 1.0F, 0.0F);
            vertices[count++] = makeEffectVertex(p3, color, normal, kMaterialSmoke, 0.0F, 0.0F);
        };
        const auto appendSparkQuad = [&](Vec3 center, float sideOffset, float length, float alpha) {
            if (count + 6 > vertices.size()) {
                return;
            }
            const float jitter =
                std::sin(elapsedSeconds * 71.0F + sideOffset * 13.0F) * 0.18F;
            const Vec3 side{
                carRight.x * (0.018F + jitter * 0.010F),
                0.0F,
                carRight.z * (0.018F + jitter * 0.010F),
            };
            const Vec3 along{
                -carForward.x * length + carRight.x * sideOffset * 0.18F,
                0.010F + sparkIntensity * 0.020F,
                -carForward.z * length + carRight.z * sideOffset * 0.18F,
            };
            const Vec3 p0{center.x - side.x, center.y, center.z - side.z};
            const Vec3 p1{center.x + side.x, center.y, center.z + side.z};
            const Vec3 p2{center.x + side.x + along.x, center.y + along.y, center.z + side.z + along.z};
            const Vec3 p3{center.x - side.x + along.x, center.y + along.y, center.z - side.z + along.z};
            constexpr Vec3 normal{0.0F, 1.0F, 0.0F};
            const std::array<float, 4> color{1.0F, 0.52F, 0.10F, alpha};
            vertices[count++] = makeEffectVertex(p0, color, normal, kMaterialSpark, 0.0F, 1.0F);
            vertices[count++] = makeEffectVertex(p1, color, normal, kMaterialSpark, 1.0F, 1.0F);
            vertices[count++] = makeEffectVertex(p2, color, normal, kMaterialSpark, 1.0F, 0.0F);
            vertices[count++] = makeEffectVertex(p0, color, normal, kMaterialSpark, 0.0F, 1.0F);
            vertices[count++] = makeEffectVertex(p2, color, normal, kMaterialSpark, 1.0F, 0.0F);
            vertices[count++] = makeEffectVertex(p3, color, normal, kMaterialSpark, 0.0F, 0.0F);
        };

        if (intensity >= 0.035F) {
            for (int side = -1; side <= 1; side += 2) {
                for (int puff = 0; puff < 3; ++puff) {
                    const float drift = static_cast<float>(puff) * 0.46F;
                    const float wobble = std::sin(elapsedSeconds * 4.2F + static_cast<float>(side * 3 + puff)) * 0.08F;
                    const Vec3 center{
                        scene.vehicle.positionX + carRight.x * static_cast<float>(side) * (kVisualRearWheelX + wobble) -
                            carForward.x * (std::abs(kVisualRearWheelZ) + 0.18F + drift),
                        scene.vehicleHeightM + 0.32F + static_cast<float>(puff) * 0.13F,
                        scene.vehicle.positionZ + carRight.z * static_cast<float>(side) * (kVisualRearWheelX + wobble) -
                            carForward.z * (std::abs(kVisualRearWheelZ) + 0.18F + drift),
                    };
                    appendSmokeQuad(
                        center,
                        (0.18F + static_cast<float>(puff) * 0.10F) * intensity,
                        (0.13F + static_cast<float>(puff) * 0.08F) * intensity,
                        intensity * (0.22F - static_cast<float>(puff) * 0.045F));
                }
            }
        }

        if (sparkIntensity >= 0.05F) {
            for (int spark = 0; spark < 7; ++spark) {
                const float sideOffset =
                    (static_cast<float>(spark) - 3.0F) * 0.13F +
                    std::sin(elapsedSeconds * 37.0F + static_cast<float>(spark)) * 0.04F;
                const Vec3 center{
                    scene.vehicle.positionX + carRight.x * sideOffset - carForward.x * (0.24F + 0.07F * static_cast<float>(spark % 3)),
                    scene.vehicleHeightM + 0.035F,
                    scene.vehicle.positionZ + carRight.z * sideOffset - carForward.z * (0.24F + 0.07F * static_cast<float>(spark % 3)),
                };
                appendSparkQuad(
                    center,
                    sideOffset,
                    (0.30F + 0.08F * static_cast<float>(spark % 3)) * sparkIntensity,
                    sparkIntensity * (0.18F + 0.035F * static_cast<float>(spark % 2)));
            }
        }

        if (count > 0) {
            std::memcpy(smokeBuffer.contents, vertices.data(), count * sizeof(Vertex));
        }
        return count;
    }

    std::size_t updateSkidmarkVertices(
        const RenderScene& scene,
        Vec3 carForward,
        Vec3 carRight) {
        const float rearUsage = std::max(scene.vehicle.rearLeftTireUsage, scene.vehicle.rearRightTireUsage);
        const float rearSlip = std::max(std::abs(scene.vehicle.rearLeftLongitudinalSlip),
                                        std::abs(scene.vehicle.rearRightLongitudinalSlip));
        const float intensity =
            std::clamp((rearUsage - 0.94F) * 3.0F + (rearSlip - 0.72F) * 0.22F, 0.0F, 1.0F);
        if (scene.surface != TrackSurface::Asphalt || scene.vehicle.speedMps < 7.5F || intensity < 0.16F) {
            return skidmarkVertexCount;
        }

        const Vec3 currentPosition{scene.vehicle.positionX, scene.vehicleHeightM, scene.vehicle.positionZ};
        if (hasLastSkidmark) {
            const Vec3 delta = subtract(currentPosition, lastSkidmarkPosition);
            if (dot(delta, delta) < 0.95F * 0.95F) {
                return skidmarkVertexCount;
            }
        }
        if (skidmarkVertexCount + 12U > skidmarkVertices.size()) {
            return skidmarkVertexCount;
        }

        const auto appendSkidQuad = [&](int side) {
            const float sideScale = static_cast<float>(side);
            const float width = 0.145F;
            const float length = 0.58F + intensity * 0.34F;
            const Vec3 center{
                scene.vehicle.positionX + carRight.x * sideScale * kVisualRearWheelX +
                    carForward.x * kVisualRearWheelZ,
                scene.vehicleHeightM + 0.010F,
                scene.vehicle.positionZ + carRight.z * sideScale * kVisualRearWheelX +
                    carForward.z * kVisualRearWheelZ,
            };
            const Vec3 across{carRight.x * width, 0.0F, carRight.z * width};
            const Vec3 along{carForward.x * length, 0.0F, carForward.z * length};
            const Vec3 p0{center.x - across.x - along.x, center.y, center.z - across.z - along.z};
            const Vec3 p1{center.x + across.x - along.x, center.y, center.z + across.z - along.z};
            const Vec3 p2{center.x + across.x + along.x, center.y, center.z + across.z + along.z};
            const Vec3 p3{center.x - across.x + along.x, center.y, center.z - across.z + along.z};
            const std::array<float, 4> color{0.018F, 0.016F, 0.014F, 0.18F + intensity * 0.28F};
            constexpr Vec3 normal{0.0F, 1.0F, 0.0F};
            skidmarkVertices[skidmarkVertexCount++] =
                makeEffectVertex(p0, color, normal, kMaterialSkidmark, 0.0F, 1.0F);
            skidmarkVertices[skidmarkVertexCount++] =
                makeEffectVertex(p1, color, normal, kMaterialSkidmark, 1.0F, 1.0F);
            skidmarkVertices[skidmarkVertexCount++] =
                makeEffectVertex(p2, color, normal, kMaterialSkidmark, 1.0F, 0.0F);
            skidmarkVertices[skidmarkVertexCount++] =
                makeEffectVertex(p0, color, normal, kMaterialSkidmark, 0.0F, 1.0F);
            skidmarkVertices[skidmarkVertexCount++] =
                makeEffectVertex(p2, color, normal, kMaterialSkidmark, 1.0F, 0.0F);
            skidmarkVertices[skidmarkVertexCount++] =
                makeEffectVertex(p3, color, normal, kMaterialSkidmark, 0.0F, 0.0F);
        };

        appendSkidQuad(-1);
        appendSkidQuad(1);
        lastSkidmarkPosition = currentPosition;
        hasLastSkidmark = true;
        std::memcpy(skidmarkBuffer.contents, skidmarkVertices.data(), skidmarkVertexCount * sizeof(Vertex));
        return skidmarkVertexCount;
    }

    std::size_t buildUiVertices(const RenderScene& scene, const DebugOverlay& overlay) {
        std::size_t count = 0;
        const auto appendScreenQuad = [&](float x, float y, float width, float height, std::array<float, 4> color) {
            if (count + 6 > uiVertices.size()) {
                return;
            }
            uiVertices[count++] = makeUiVertex(x, y, 0.0F, color);
            uiVertices[count++] = makeUiVertex(x + width, y, 0.0F, color);
            uiVertices[count++] = makeUiVertex(x + width, y + height, 0.0F, color);
            uiVertices[count++] = makeUiVertex(x, y, 0.0F, color);
            uiVertices[count++] = makeUiVertex(x + width, y + height, 0.0F, color);
            uiVertices[count++] = makeUiVertex(x, y + height, 0.0F, color);
        };
        const auto appendScreenTriangle = [&](Vec3 a, Vec3 b, Vec3 c, std::array<float, 4> color) {
            if (count + 3 > uiVertices.size()) {
                return;
            }
            uiVertices[count++] = makeUiVertex(a.x, a.y, 0.0F, color);
            uiVertices[count++] = makeUiVertex(b.x, b.y, 0.0F, color);
            uiVertices[count++] = makeUiVertex(c.x, c.y, 0.0F, color);
        };
        const auto appendRoundedRect = [&](float x, float y, float width, float height, float radius, std::array<float, 4> color) {
            constexpr int kCornerSegments = 6;
            constexpr int kPointCount = kCornerSegments * 4 + 4;
            if (count + kPointCount * 3 > uiVertices.size()) {
                return;
            }
            std::array<Vec3, kPointCount> points{};
            int pointCount = 0;
            const float r = std::min(radius, std::min(width, height) * 0.5F);
            const auto addCorner = [&](float cx, float cy, float startAngle) {
                for (int segment = 0; segment <= kCornerSegments; ++segment) {
                    const float a = startAngle + static_cast<float>(segment) *
                                                 (std::numbers::pi_v<float> * 0.5F) /
                                                 static_cast<float>(kCornerSegments);
                    points[pointCount++] = {cx + std::cos(a) * r, cy + std::sin(a) * r, 0.0F};
                }
            };
            addCorner(x + width - r, y + r, -std::numbers::pi_v<float> * 0.5F);
            addCorner(x + width - r, y + height - r, 0.0F);
            addCorner(x + r, y + height - r, std::numbers::pi_v<float> * 0.5F);
            addCorner(x + r, y + r, std::numbers::pi_v<float>);
            const Vec3 center{x + width * 0.5F, y + height * 0.5F, 0.0F};
            for (int index = 0; index < pointCount; ++index) {
                appendScreenTriangle(center, points[index], points[(index + 1) % pointCount], color);
            }
        };
        const auto appendArcSegment = [&](float centerX,
                                          float centerY,
                                          float innerRadius,
                                          float outerRadius,
                                          float startAngle,
                                          float endAngle,
                                          std::array<float, 4> color) {
            constexpr int kArcSegments = 3;
            for (int segment = 0; segment < kArcSegments; ++segment) {
                if (count + 6 > uiVertices.size()) {
                    return;
                }
                const float a0 = startAngle + (endAngle - startAngle) *
                                                  static_cast<float>(segment) /
                                                  static_cast<float>(kArcSegments);
                const float a1 = startAngle + (endAngle - startAngle) *
                                                  static_cast<float>(segment + 1) /
                                                  static_cast<float>(kArcSegments);
                const Vec3 p0{centerX + std::cos(a0) * innerRadius, centerY + std::sin(a0) * innerRadius, 0.0F};
                const Vec3 p1{centerX + std::cos(a0) * outerRadius, centerY + std::sin(a0) * outerRadius, 0.0F};
                const Vec3 p2{centerX + std::cos(a1) * outerRadius, centerY + std::sin(a1) * outerRadius, 0.0F};
                const Vec3 p3{centerX + std::cos(a1) * innerRadius, centerY + std::sin(a1) * innerRadius, 0.0F};
                appendScreenTriangle(p0, p1, p2, color);
                appendScreenTriangle(p0, p2, p3, color);
            }
        };

        const float scale = renderScale;
        const std::array<float, 4> panel{0.006F, 0.010F, 0.016F, 0.70F};
        const std::array<float, 4> panelDeep{0.002F, 0.004F, 0.008F, 0.78F};
        const std::array<float, 4> panelEdge{0.20F, 0.78F, 0.96F, 0.44F};
        const std::array<float, 4> glass{0.11F, 0.17F, 0.22F, 0.25F};
        const std::array<float, 4> glow{0.18F, 0.72F, 1.0F, 0.17F};
        const std::array<float, 4> barBack{0.020F, 0.026F, 0.036F, 0.78F};
        const std::array<float, 4> speedFill{0.30F, 0.78F, 1.0F, 0.82F};
        const std::array<float, 4> throttleFill{0.23F, 1.0F, 0.48F, 0.76F};
        const std::array<float, 4> brakeFill{1.0F, 0.20F, 0.12F, 0.78F};
        const float hudWidth = std::min(870.0F * scale, static_cast<float>(drawableWidth) - 24.0F * scale);
        const float hudHeight = 126.0F * scale;
        const float hudX = static_cast<float>(drawableWidth) * 0.5F - hudWidth * 0.5F;
        const float hudY = static_cast<float>(drawableHeight) - hudHeight - 12.0F * scale;
        const float centerX = hudX + hudWidth * 0.5F;
        const float centerY = hudY + 106.0F * scale;
        const float rpmRatio =
            std::clamp((scene.vehicle.rpm - 3000.0F) / (12000.0F - 3000.0F), 0.0F, 1.0F);
        const float speedMph = scene.vehicle.speedMps * 2.2369363F;
        const float speedRatio = std::clamp(speedMph / 240.0F, 0.0F, 1.0F);
        const float throttleRatio = std::clamp(scene.throttleInput, 0.0F, 1.0F);
        const float brakeRatio = std::clamp(scene.brakeInput, 0.0F, 1.0F);

        appendRoundedRect(
            hudX - 10.0F * scale,
            hudY - 10.0F * scale,
            hudWidth + 20.0F * scale,
            hudHeight + 20.0F * scale,
            22.0F * scale,
            glow);
        appendRoundedRect(hudX, hudY, hudWidth, hudHeight, 18.0F * scale, panel);
        appendRoundedRect(
            hudX + 3.0F * scale,
            hudY + 3.0F * scale,
            hudWidth - 6.0F * scale,
            34.0F * scale,
            15.0F * scale,
            glass);
        appendScreenQuad(hudX + 22.0F * scale, hudY + 8.0F * scale, hudWidth - 44.0F * scale, 1.2F * scale, panelEdge);
        appendScreenQuad(hudX + 22.0F * scale, hudY + 38.0F * scale, hudWidth - 44.0F * scale, 1.0F * scale, {0.95F, 0.78F, 0.30F, 0.20F});

        const float ledWidth = std::min(28.0F * scale, (hudWidth - 120.0F * scale) / 16.0F);
        const float ledGap = 5.0F * scale;
        const float ledTotal = ledWidth * 14.0F + ledGap * 13.0F;
        const float ledX = centerX - ledTotal * 0.5F;
        const float ledY = hudY - 23.0F * scale;
        const float ledFlash =
            scene.vehicle.rpm > 11450.0F ? (std::sin(elapsedSeconds * 36.0F) * 0.5F + 0.5F) : 0.0F;
        for (int led = 0; led < 14; ++led) {
            const float threshold = 0.48F + static_cast<float>(led) * 0.036F;
            const bool lit = rpmRatio >= threshold;
            std::array<float, 4> ledColor = {0.020F, 0.026F, 0.030F, 0.72F};
            if (lit) {
                if (led >= 11) {
                    ledColor = {1.0F, 0.05F + ledFlash * 0.30F, 0.02F, 0.96F};
                } else if (led >= 7) {
                    ledColor = {1.0F, 0.76F, 0.10F, 0.92F};
                } else {
                    ledColor = {0.14F, 1.0F, 0.44F, 0.86F};
                }
            }
            appendRoundedRect(
                ledX + static_cast<float>(led) * (ledWidth + ledGap),
                ledY,
                ledWidth,
                9.0F * scale,
                3.5F * scale,
                ledColor);
        }

        appendRoundedRect(centerX - 57.0F * scale, hudY + 27.0F * scale, 114.0F * scale, 78.0F * scale, 18.0F * scale, panelDeep);
        appendRoundedRect(centerX - 61.0F * scale, hudY + 23.0F * scale, 122.0F * scale, 86.0F * scale, 20.0F * scale, {1.0F, 0.80F, 0.26F, 0.12F});
        appendScreenQuad(centerX - 36.0F * scale, hudY + 91.0F * scale, 72.0F * scale, 1.4F * scale, {1.0F, 0.88F, 0.40F, 0.52F});

        constexpr int kRpmSegments = 54;
        const float arcStart = std::numbers::pi_v<float> * 1.04F;
        const float arcEnd = std::numbers::pi_v<float> * 1.96F;
        for (int segment = 0; segment < kRpmSegments; ++segment) {
            const float segmentT0 = static_cast<float>(segment) / static_cast<float>(kRpmSegments);
            const float segmentT1 = static_cast<float>(segment + 1) / static_cast<float>(kRpmSegments);
            const float a0 = arcStart + (arcEnd - arcStart) * segmentT0;
            const float a1 = arcStart + (arcEnd - arcStart) * segmentT1 - 0.004F;
            std::array<float, 4> color = {0.026F, 0.034F, 0.042F, 0.72F};
            if (segmentT0 <= rpmRatio) {
                if (segmentT0 > 0.86F) {
                    color = {1.0F, 0.08F + ledFlash * 0.20F, 0.04F, 0.95F};
                } else if (segmentT0 > 0.66F) {
                    color = {1.0F, 0.72F, 0.12F, 0.90F};
                } else {
                    color = {0.20F, 1.0F, 0.46F, 0.82F};
                }
            }
            appendArcSegment(centerX, centerY, 108.0F * scale, 120.0F * scale, a0, a1, color);
        }

        const float leftX = hudX + 34.0F * scale;
        const float rightX = hudX + hudWidth - 250.0F * scale;
        appendRoundedRect(leftX, hudY + 52.0F * scale, 202.0F * scale, 11.0F * scale, 5.5F * scale, barBack);
        appendRoundedRect(leftX, hudY + 52.0F * scale, 202.0F * scale * speedRatio, 11.0F * scale, 5.5F * scale, speedFill);
        appendRoundedRect(leftX, hudY + 84.0F * scale, 86.0F * scale, 10.0F * scale, 5.0F * scale, barBack);
        appendRoundedRect(leftX, hudY + 84.0F * scale, 86.0F * scale * brakeRatio, 10.0F * scale, 5.0F * scale, brakeFill);
        appendRoundedRect(leftX + 108.0F * scale, hudY + 84.0F * scale, 106.0F * scale, 10.0F * scale, 5.0F * scale, barBack);
        appendRoundedRect(leftX + 108.0F * scale, hudY + 84.0F * scale, 106.0F * scale * throttleRatio, 10.0F * scale, 5.0F * scale, throttleFill);

        const std::array<float, 4> tireBack{0.020F, 0.026F, 0.032F, 0.76F};
        const std::array<float, 4> tireCool{0.26F, 0.72F, 1.0F, 0.82F};
        const std::array<float, 4> tireWarm{0.25F, 1.0F, 0.48F, 0.82F};
        const std::array<float, 4> tireHot{1.0F, 0.30F, 0.12F, 0.88F};
        const std::array<float, 4> usageWarn{1.0F, 0.78F, 0.16F, 0.85F};
        const float temps[4] = {
            scene.vehicle.frontLeftTireTemperatureC,
            scene.vehicle.frontRightTireTemperatureC,
            scene.vehicle.rearLeftTireTemperatureC,
            scene.vehicle.rearRightTireTemperatureC,
        };
        const float usages[4] = {
            scene.vehicle.frontLeftTireUsage,
            scene.vehicle.frontRightTireUsage,
            scene.vehicle.rearLeftTireUsage,
            scene.vehicle.rearRightTireUsage,
        };
        for (int tire = 0; tire < 4; ++tire) {
            const float column = static_cast<float>(tire % 2);
            const float row = static_cast<float>(tire / 2);
            const float x = rightX + column * 62.0F * scale;
            const float y = hudY + 57.0F * scale + row * 32.0F * scale;
            const float tempRatio = std::clamp((temps[tire] - 45.0F) / 100.0F, 0.0F, 1.0F);
            const float usageRatio = std::clamp(usages[tire], 0.0F, 1.0F);
            const std::array<float, 4> tireColor =
                temps[tire] > 122.0F ? tireHot : (temps[tire] < 70.0F ? tireCool : tireWarm);
            appendRoundedRect(x, y, 46.0F * scale, 18.0F * scale, 5.0F * scale, tireBack);
            appendRoundedRect(x + 3.0F * scale, y + 3.0F * scale, 40.0F * scale * tempRatio, 5.0F * scale, 2.5F * scale, tireColor);
            appendRoundedRect(x + 3.0F * scale, y + 10.0F * scale, 40.0F * scale * usageRatio, 4.0F * scale, 2.0F * scale,
                              usageRatio > 0.90F ? usageWarn : speedFill);
        }

        if (scene.wallContact) {
            appendRoundedRect(centerX - 144.0F * scale, hudY + 5.0F * scale, 288.0F * scale, 13.0F * scale, 6.5F * scale,
                              {1.0F, 0.12F, 0.04F, 0.34F + ledFlash * 0.30F});
        }

        if (overlay.visible() && overlay.lineCount() > 0) {
            const float mainWidth = std::min(static_cast<float>(drawableWidth) - 12.0F * scale, 850.0F * scale);
            appendScreenQuad(6.0F * scale, 6.0F * scale, mainWidth, 140.0F * scale, panel);
            appendScreenQuad(6.0F * scale, 6.0F * scale, mainWidth, 2.0F * scale, panelEdge);
            appendScreenQuad(6.0F * scale, 144.0F * scale, mainWidth, 2.0F * scale, panelEdge);
        }

        if (overlay.visible() && overlay.lineCount() > 12) {
            const float diagnosticsX =
                drawableWidth >= static_cast<int>(1150.0F * scale) ? static_cast<float>(drawableWidth) * 0.47F
                                                                    : 6.0F * scale;
            const float diagnosticsY =
                drawableWidth >= static_cast<int>(1150.0F * scale) ? 42.0F * scale : 158.0F * scale;
            const float diagnosticsWidth =
                std::max(220.0F * scale, static_cast<float>(drawableWidth) - diagnosticsX - 8.0F * scale);
            const float diagnosticsHeight =
                std::min(static_cast<float>(drawableHeight) - diagnosticsY - 8.0F * scale, 390.0F * scale);
            appendScreenQuad(diagnosticsX - 6.0F * scale, diagnosticsY - 6.0F * scale,
                             diagnosticsWidth, diagnosticsHeight, panel);
            appendScreenQuad(diagnosticsX - 6.0F * scale, diagnosticsY - 6.0F * scale,
                             diagnosticsWidth, 2.0F * scale, panelEdge);
        }

        std::memcpy(uiBuffer.contents, uiVertices.data(), count * sizeof(Vertex));
        return count;
    }

    std::size_t buildSteeringDisplayVertices(const RenderScene& scene) {
        constexpr float cellSize = 16.0F;
        constexpr float atlasWidth = cellSize * 16.0F;
        constexpr float atlasHeight = cellSize * 8.0F;
        constexpr float zOffset = 0.034F;
        std::size_t count = 0;

        const auto appendString = [&](const char* text,
                                      float x,
                                      float y,
                                      float size,
                                      std::array<float, 4> color) {
            const float glyphWidth = size * 0.62F;
            for (const char* cursor = text; *cursor != '\0'; ++cursor) {
                const char character = *cursor;
                if (count + 6 > steeringDisplayVertices.size()) {
                    return;
                }
                if (character != ' ') {
                    const unsigned char code = static_cast<unsigned char>(character) & 0x7F;
                    const float originX = static_cast<float>(code % 16) * cellSize;
                    const float originY = static_cast<float>(code / 16) * cellSize;
                    const float u0 = (originX + 0.5F) / atlasWidth;
                    const float v0 = (originY + 0.5F) / atlasHeight;
                    const float u1 = (originX + cellSize - 0.5F) / atlasWidth;
                    const float v1 = (originY + cellSize - 0.5F) / atlasHeight;
                    steeringDisplayVertices[count++] = makeTextVertex(x, y, zOffset, color, u0, v0);
                    steeringDisplayVertices[count++] = makeTextVertex(x + glyphWidth, y, zOffset, color, u1, v0);
                    steeringDisplayVertices[count++] = makeTextVertex(x + glyphWidth, y - size, zOffset, color, u1, v1);
                    steeringDisplayVertices[count++] = makeTextVertex(x, y, zOffset, color, u0, v0);
                    steeringDisplayVertices[count++] = makeTextVertex(x + glyphWidth, y - size, zOffset, color, u1, v1);
                    steeringDisplayVertices[count++] = makeTextVertex(x, y - size, zOffset, color, u0, v1);
                }
                x += glyphWidth + size * 0.08F;
            }
        };

        const auto formatLapTime = [](float seconds, char* output, std::size_t outputSize) {
            if (seconds <= 0.0F) {
                std::snprintf(output, outputSize, "--:--.-");
                return;
            }
            const int minutes = static_cast<int>(seconds / 60.0F);
            const float remainder = seconds - static_cast<float>(minutes) * 60.0F;
            std::snprintf(output, outputSize, "%d:%04.1f", minutes, remainder);
        };

        char gearText[12]{};
        char speedText[18]{};
        char rpmText[20]{};
        char lapTime[16]{};
        char lapText[24]{};
        char deltaText[24]{};
        const int speedMph = static_cast<int>(std::round(scene.vehicle.speedMps * 2.2369363F));
        std::snprintf(gearText, sizeof(gearText), "G%d", std::max(0, scene.vehicle.gear));
        std::snprintf(speedText, sizeof(speedText), "%d MPH", std::max(0, speedMph));
        std::snprintf(rpmText, sizeof(rpmText), "%.0f RPM", std::max(0.0F, scene.vehicle.rpm));
        formatLapTime(scene.currentLapSeconds, lapTime, sizeof(lapTime));
        std::snprintf(lapText, sizeof(lapText), "LAP %s", lapTime);
        if (scene.bestLapSeconds > 0.0F) {
            std::snprintf(deltaText, sizeof(deltaText), "DELTA %+.1f", scene.lapDeltaSeconds);
        } else {
            std::snprintf(deltaText, sizeof(deltaText), "DELTA --");
        }

        appendString(gearText, -0.096F, 0.030F, 0.044F, {1.00F, 0.82F, 0.24F, 0.94F});
        appendString(speedText, -0.018F, 0.026F, 0.017F, {0.42F, 0.92F, 1.00F, 0.88F});
        appendString(rpmText, -0.018F, 0.004F, 0.013F, {0.38F, 1.00F, 0.48F, 0.82F});
        appendString(lapText, -0.096F, -0.018F, 0.012F, {0.90F, 0.94F, 0.98F, 0.80F});
        appendString(
            deltaText,
            -0.096F,
            -0.035F,
            0.012F,
            scene.bestLapSeconds > 0.0F && scene.lapDeltaSeconds < 0.0F
                ? std::array<float, 4>{0.30F, 1.00F, 0.48F, 0.82F}
                : std::array<float, 4>{1.00F, 0.62F, 0.26F, 0.82F});

        if (count > 0) {
            std::memcpy(steeringDisplayBuffer.contents, steeringDisplayVertices.data(), count * sizeof(Vertex));
        }
        return count;
    }

    std::size_t buildTextVertices(const DebugOverlay& overlay) {
        constexpr float cellSize = 16.0F;
        constexpr float atlasWidth = cellSize * 16.0F;
        constexpr float atlasHeight = cellSize * 8.0F;
        std::size_t count = 0;
        for (int lineIndex = 0; lineIndex < overlay.lineCount(); ++lineIndex) {
            const OverlayLine& line = overlay.line(lineIndex);
            float x = line.x * renderScale;
            const float y = line.y * renderScale;
            const float size = 8.0F * line.scale * renderScale;
            const std::array<float, 4> color{
                line.color.red, line.color.green, line.color.blue, line.color.alpha};

            for (char character : line.text) {
                if (character == '\0') {
                    break;
                }
                if (count + 6 > textVertices.size()) {
                    return count;
                }
                const unsigned char code = static_cast<unsigned char>(character) & 0x7F;
                if (character != ' ') {
                    const float originX = static_cast<float>(code % 16) * cellSize;
                    const float originY = static_cast<float>(code / 16) * cellSize;
                    const float u0 = (originX + 0.5F) / atlasWidth;
                    const float v0 = (originY + 0.5F) / atlasHeight;
                    const float u1 = (originX + cellSize - 0.5F) / atlasWidth;
                    const float v1 = (originY + cellSize - 0.5F) / atlasHeight;
                    textVertices[count++] = makeTextVertex(x, y, 0.0F, color, u0, v0);
                    textVertices[count++] = makeTextVertex(x + size, y, 0.0F, color, u1, v0);
                    textVertices[count++] = makeTextVertex(x + size, y + size, 0.0F, color, u1, v1);
                    textVertices[count++] = makeTextVertex(x, y, 0.0F, color, u0, v0);
                    textVertices[count++] = makeTextVertex(x + size, y + size, 0.0F, color, u1, v1);
                    textVertices[count++] = makeTextVertex(x, y + size, 0.0F, color, u0, v1);
                }
                x += size;
            }
        }
        std::memcpy(textBuffer.contents, textVertices.data(), count * sizeof(Vertex));
        return count;
    }
};

MetalRenderer::MetalRenderer() : impl_(std::make_unique<Impl>()) {}

MetalRenderer::~MetalRenderer() {
    shutdown();
}

bool MetalRenderer::initialize(
    WindowSDL& window,
    bool vsync,
    float renderScale,
    int shadowMapSize,
    const Track& track) {
    impl_->metalView = SDL_Metal_CreateView(window.nativeHandle());
    if (impl_->metalView == nullptr) {
        impl_->error = SDL_GetError();
        return false;
    }

    impl_->layer = (__bridge CAMetalLayer*)SDL_Metal_GetLayer(impl_->metalView);
    impl_->device = MTLCreateSystemDefaultDevice();
    if (impl_->layer == nil || impl_->device == nil) {
        impl_->error = "Metal is not available on this Mac";
        return false;
    }
    impl_->layer.device = impl_->device;
    impl_->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    impl_->layer.framebufferOnly = YES;
    impl_->layer.displaySyncEnabled = vsync;
    impl_->renderScale = std::clamp(renderScale, 0.5F, 1.0F);
    impl_->shadowMapSize = std::clamp(shadowMapSize, 1024, 4096);
    impl_->commandQueue = [impl_->device newCommandQueue];
    impl_->passDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    impl_->bloomPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    impl_->bloomQuarterPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    impl_->bloomEighthPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    impl_->finalPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    impl_->shadowPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    if (impl_->commandQueue == nil || impl_->passDescriptor == nil ||
        impl_->bloomPassDescriptor == nil || impl_->bloomQuarterPassDescriptor == nil ||
        impl_->bloomEighthPassDescriptor == nil || impl_->finalPassDescriptor == nil ||
        impl_->shadowPassDescriptor == nil) {
        impl_->error = "Could not create Metal command objects";
        return false;
    }
    return impl_->createPipelines() && impl_->createGeometry(track) &&
           impl_->createFont() && impl_->createMaterialTextures();
}

void MetalRenderer::shutdown() {
    if (!impl_) {
        return;
    }
    impl_->depthTexture = nil;
    impl_->msaaDepthTexture = nil;
    impl_->hdrTexture = nil;
    impl_->msaaHdrTexture = nil;
    impl_->bloomTexture = nil;
    impl_->bloomQuarterTexture = nil;
    impl_->bloomEighthTexture = nil;
    impl_->shadowDepthTexture = nil;
    impl_->asphaltAlbedoTexture = nil;
    impl_->asphaltNormalTexture = nil;
    impl_->asphaltRoughnessTexture = nil;
    impl_->asphaltHeightTexture = nil;
    impl_->concreteAlbedoTexture = nil;
    impl_->concreteNormalTexture = nil;
    impl_->concreteRoughnessTexture = nil;
    impl_->grassAlbedoTexture = nil;
    impl_->grassNormalTexture = nil;
    impl_->grassRoughnessTexture = nil;
    impl_->grassHeightTexture = nil;
    impl_->carbonAlbedoTexture = nil;
    impl_->carbonNormalTexture = nil;
    impl_->carbonRoughnessTexture = nil;
    impl_->skyboxTexture = nil;
    impl_->smokeTexture = nil;
    impl_->fontTexture = nil;
    impl_->steeringDisplayBuffer = nil;
    impl_->textBuffer = nil;
    impl_->uiBuffer = nil;
    impl_->skidmarkBuffer = nil;
    impl_->smokeBuffer = nil;
    impl_->suspensionBuffer = nil;
    impl_->steeringWheelBuffer = nil;
    impl_->wheelBuffer = nil;
    impl_->carBodyBuffer = nil;
    impl_->groundBuffer = nil;
    impl_->skyBuffer = nil;
    impl_->fontSampler = nil;
    impl_->materialSampler = nil;
    impl_->shadowSampler = nil;
    impl_->skyPipeline = nil;
    impl_->worldPipeline = nil;
    impl_->fxPipeline = nil;
    impl_->uiPipeline = nil;
    impl_->textPipeline = nil;
    impl_->worldTextPipeline = nil;
    impl_->shadowPipeline = nil;
    impl_->bloomPipeline = nil;
    impl_->postPipeline = nil;
    impl_->worldDepthState = nil;
    impl_->skyDepthState = nil;
    impl_->fxDepthState = nil;
    impl_->textDepthState = nil;
    impl_->shadowDepthState = nil;
    impl_->commandQueue = nil;
    impl_->passDescriptor = nil;
    impl_->bloomPassDescriptor = nil;
    impl_->bloomQuarterPassDescriptor = nil;
    impl_->bloomEighthPassDescriptor = nil;
    impl_->finalPassDescriptor = nil;
    impl_->shadowPassDescriptor = nil;
    impl_->layer = nil;
    impl_->device = nil;
    if (impl_->metalView != nullptr) {
        SDL_Metal_DestroyView(impl_->metalView);
        impl_->metalView = nullptr;
    }
}

bool MetalRenderer::render(const RenderScene& scene, const DebugOverlay& overlay) {
    @autoreleasepool {
        impl_->resizeIfNeeded(scene.pixelWidth, scene.pixelHeight);
        id<CAMetalDrawable> drawable = [impl_->layer nextDrawable];
        if (drawable == nil) {
            return true;
        }
        if (impl_->depthTexture == nil || impl_->msaaDepthTexture == nil ||
            impl_->hdrTexture == nil || impl_->msaaHdrTexture == nil ||
            impl_->bloomTexture == nil || impl_->bloomQuarterTexture == nil ||
            impl_->bloomEighthTexture == nil || impl_->shadowDepthTexture == nil) {
            impl_->error = "Could not create the Metal render targets";
            return false;
        }

        id<MTLCommandBuffer> commandBuffer = [impl_->commandQueue commandBuffer];
        if (commandBuffer == nil) {
            impl_->error = "Could not create Metal command buffer";
            return false;
        }

        const Uint64 nowTicks = SDL_GetTicks();
        if (impl_->previousRenderTicks == 0) {
            impl_->previousRenderTicks = nowTicks;
        }
        const float frameDeltaSeconds =
            std::clamp(static_cast<float>(nowTicks - impl_->previousRenderTicks) * 0.001F, 1.0F / 240.0F, 0.05F);
        impl_->previousRenderTicks = nowTicks;
        impl_->elapsedSeconds += frameDeltaSeconds;

        const float brakeSignal = std::clamp(scene.brakeInput, 0.0F, 1.0F);
        if (scene.wallContact) {
            impl_->cameraTrauma = std::max(impl_->cameraTrauma, 1.0F);
        }
        impl_->cameraTrauma = std::max(0.0F, impl_->cameraTrauma - frameDeltaSeconds * 1.65F);

        const float forwardX = std::sin(scene.vehicle.yawRadians);
        const float forwardZ = std::cos(scene.vehicle.yawRadians);
        const Vec3 carForward{forwardX, 0.0F, forwardZ};
        const Vec3 carRight{forwardZ, 0.0F, -forwardX};
        const Mat4 vehicleFrameWorld =
            multiply(
                multiply(
                    translation(
                        scene.vehicle.positionX,
                        scene.vehicleHeightM,
                        scene.vehicle.positionZ),
                    rotationY(scene.vehicle.yawRadians)),
                rotationZ(scene.vehicleBankRadians));
        const Mat4 chassisWorld =
            multiply(
                vehicleFrameWorld,
                multiply(
                    multiply(
                        translation(0.0F, scene.vehicle.chassisHeaveM, 0.0F),
                        rotationX(scene.vehicle.chassisPitchRadians)),
                    rotationZ(scene.vehicle.chassisRollRadians)));
        const Mat4 carWorld = chassisWorld;
        const auto worldFromChassisLocal = [&](Vec3 local) {
            const ClipPoint point = transformPoint(chassisWorld, local);
            return Vec3{point.x, point.y, point.z};
        };
        const Vec3 chassisUp{
            chassisWorld.values[4],
            chassisWorld.values[5],
            chassisWorld.values[6],
        };
        const float speedFactor = std::clamp(scene.vehicle.speedMps / 75.0F, 0.0F, 1.0F);
        const float tireUse = std::max(
            std::max(scene.vehicle.frontLeftTireUsage, scene.vehicle.frontRightTireUsage),
            std::max(scene.vehicle.rearLeftTireUsage, scene.vehicle.rearRightTireUsage));
        const bool cockpitCamera = scene.cameraMode == 1;
        const Vec3 desiredEye =
            cockpitCamera
                ? worldFromChassisLocal({0.0F, 1.08F, -0.46F})
                : Vec3{
                      scene.vehicle.positionX - carForward.x * 10.2F,
                      scene.vehicleHeightM + scene.vehicle.chassisHeaveM + 3.15F,
                      scene.vehicle.positionZ - carForward.z * 10.2F,
                  };
        const Vec3 desiredTarget =
            cockpitCamera
                ? worldFromChassisLocal({0.0F, 0.90F, 60.0F})
                : Vec3{
                      scene.vehicle.positionX + carForward.x * 5.4F,
                      scene.vehicleHeightM + scene.vehicle.chassisHeaveM + 0.72F,
                      scene.vehicle.positionZ + carForward.z * 5.4F,
                  };
        if (!impl_->cameraInitialized ||
            cockpitCamera ||
            impl_->previousCameraMode != scene.cameraMode) {
            impl_->cameraEye = desiredEye;
            impl_->cameraTarget = desiredTarget;
            impl_->cameraInitialized = true;
        } else {
            impl_->cameraEye = desiredEye;
            impl_->cameraTarget = desiredTarget;
        }
        impl_->previousCameraMode = scene.cameraMode;

        const bool onAsphalt = scene.surface == TrackSurface::Asphalt;
        const bool onGrass = scene.surface == TrackSurface::Grass;
        const float cleanSurfaceBuzz = onAsphalt ? 0.0012F : (onGrass ? 0.030F : 0.010F);
        const float rpmBuzz =
            std::clamp((scene.vehicle.rpm - 1200.0F) / 9600.0F, 0.0F, 1.0F) *
            (onAsphalt ? 0.0008F : 0.012F);
        const float speedBuzz = speedFactor * (onAsphalt ? 0.0005F : (onGrass ? 0.040F : 0.010F));
        const float slipBuzz =
            std::clamp(tireUse - 0.86F, 0.0F, 0.55F) * (onAsphalt ? 0.035F : 0.18F);
        const float shakeAmount =
            cockpitCamera
                ? 0.0F
                : cleanSurfaceBuzz + rpmBuzz + speedBuzz + slipBuzz + impl_->cameraTrauma * 0.26F;
        const float t = impl_->elapsedSeconds;
        const Vec3 shake{
            carRight.x * std::sin(t * 43.0F + scene.vehicle.rpm * 0.0023F) * shakeAmount +
                carForward.x * std::sin(t * 31.0F) * shakeAmount * 0.22F,
            std::sin(t * 57.0F + scene.vehicle.speedMps * 0.31F) * shakeAmount * 0.34F,
            carRight.z * std::sin(t * 43.0F + scene.vehicle.rpm * 0.0023F) * shakeAmount +
                carForward.z * std::sin(t * 31.0F) * shakeAmount * 0.22F,
        };
        const Vec3 eye{
            impl_->cameraEye.x + shake.x,
            impl_->cameraEye.y + shake.y,
            impl_->cameraEye.z + shake.z,
        };
        const Vec3 target{
            impl_->cameraTarget.x + shake.x * 0.18F,
            impl_->cameraTarget.y + shake.y * 0.08F,
            impl_->cameraTarget.z + shake.z * 0.18F,
        };
        const float cameraRoll =
            cockpitCamera
                ? 0.0F
                : scene.vehicleBankRadians * 0.34F +
                      std::clamp(scene.vehicle.lateralG, -2.4F, 2.4F) * 0.045F -
                      std::clamp(scene.vehicle.longitudinalG, -1.8F, 1.8F) * 0.022F;
        const Vec3 rolledUp =
            cockpitCamera
                ? normalize(chassisUp)
                : normalize({
                      carRight.x * std::sin(cameraRoll),
                      std::cos(cameraRoll),
                      carRight.z * std::sin(cameraRoll),
                  });
        const Vec3 cameraForward = normalize(subtract(target, eye));
        const Vec3 cameraRight = normalize(cross(rolledUp, cameraForward));
        const Vec3 cameraUp = cross(cameraForward, cameraRight);
        const Mat4 view = lookAtLeftHanded(eye, target, cameraUp);
        const float aspect =
            static_cast<float>(impl_->drawableWidth) / static_cast<float>(impl_->drawableHeight);
        const float fovDegrees = cockpitCamera ? 55.0F : 50.0F;
        const Mat4 projection =
            perspectiveLeftHanded(fovDegrees * std::numbers::pi_v<float> / 180.0F, aspect, 0.1F, 3200.0F);
        const Mat4 viewProjection = multiply(projection, view);

        Uniforms uniforms{};
        uniforms.cameraRight[0] = cameraRight.x;
        uniforms.cameraRight[1] = cameraRight.y;
        uniforms.cameraRight[2] = cameraRight.z;
        uniforms.cameraRight[3] = 0.0F;
        uniforms.cameraUp[0] = cameraUp.x;
        uniforms.cameraUp[1] = cameraUp.y;
        uniforms.cameraUp[2] = cameraUp.z;
        uniforms.cameraUp[3] = 0.0F;
        uniforms.cameraForward[0] = cameraForward.x;
        uniforms.cameraForward[1] = cameraForward.y;
        uniforms.cameraForward[2] = cameraForward.z;
        uniforms.cameraForward[3] = 0.0F;
        uniforms.renderParams[0] = static_cast<float>(impl_->drawableWidth);
        uniforms.renderParams[1] = static_cast<float>(impl_->drawableHeight);
        uniforms.renderParams[2] = impl_->elapsedSeconds;
        uniforms.renderParams[3] = aspect;
        uniforms.effectParams[0] = brakeSignal;
        uniforms.effectParams[1] = 1.0F / static_cast<float>(impl_->shadowMapSize);
        uniforms.effectParams[2] = 0.0F;
        uniforms.effectParams[3] = 0.0F;
        uniforms.contactParams[0] = scene.vehicle.positionX;
        uniforms.contactParams[1] = scene.vehicle.positionZ;
        uniforms.contactParams[2] = scene.vehicle.yawRadians;
        uniforms.contactParams[3] = 1.0F;
        const Mat4 identity = Mat4::identity();

        const Mat4 steeringWheelWorld =
            multiply(
                carWorld,
                multiply(
                    multiply(
                        multiply(
                            translation(0.0F, 0.51F, 0.46F),
                            rotationX(-0.25F)),
                        scale(0.82F, 0.82F, 0.82F)),
                    rotationZ(-scene.vehicle.steeringAngleRadians * 6.0F)));

        const Vec3 sunDirection = normalize({-0.42F, 0.82F, -0.38F});
        const Vec3 shadowCenter{
            scene.vehicle.positionX + carForward.x * 3.5F,
            scene.vehicleHeightM + 2.0F,
            scene.vehicle.positionZ + carForward.z * 3.5F,
        };
        constexpr float kShadowLightDistanceM = 150.0F;
        constexpr float kShadowFrustumExtentM = 82.0F;
        const Vec3 lightEye{
            shadowCenter.x - sunDirection.x * kShadowLightDistanceM,
            shadowCenter.y - sunDirection.y * kShadowLightDistanceM + 52.0F,
            shadowCenter.z - sunDirection.z * kShadowLightDistanceM,
        };
        const Mat4 lightView = lookAtLeftHanded(lightEye, shadowCenter, {0.0F, 1.0F, 0.0F});
        const Mat4 lightProjection = orthographic(
            -kShadowFrustumExtentM,
            kShadowFrustumExtentM,
            -kShadowFrustumExtentM,
            kShadowFrustumExtentM,
            0.1F,
            260.0F);
        const Mat4 shadowViewProjection = multiply(lightProjection, lightView);

        const std::size_t suspensionVertexCount =
            impl_->buildSuspensionVertices(scene, vehicleFrameWorld, chassisWorld);

        const auto wheelLocalTransform = [&](
                                             float localX,
                                             float localY,
                                             float localZ,
                                             bool frontWheel,
                                             float normalLoadN,
                                             float wheelRotationRadians,
                                             float hubTravelM) {
            const float compression = std::clamp((normalLoadN - 2500.0F) / 9500.0F, 0.0F, 0.14F);
            const float widthScale = frontWheel ? kVisualFrontWheelWidthScale : 1.0F;
            const float radiusScale = frontWheel ? kVisualFrontWheelRadiusScale : 1.0F;
            return multiply(
                multiply(
                    multiply(
                        translation(localX, localY + hubTravelM, localZ),
                        rotationY(frontWheel ? scene.vehicle.steeringAngleRadians : 0.0F)),
                    rotationX(wheelRotationRadians)),
                scale(widthScale, radiusScale * (1.0F - compression), radiusScale * (1.0F + compression * 0.16F)));
        };

        impl_->shadowPassDescriptor.depthAttachment.texture = impl_->shadowDepthTexture;
        impl_->shadowPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
        impl_->shadowPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
        impl_->shadowPassDescriptor.depthAttachment.clearDepth = 1.0;
        id<MTLRenderCommandEncoder> shadowEncoder =
            [commandBuffer renderCommandEncoderWithDescriptor:impl_->shadowPassDescriptor];
        if (shadowEncoder == nil) {
            impl_->error = "Could not create Metal shadow encoder";
            return false;
        }
        const double shadowViewportSize = static_cast<double>(impl_->shadowMapSize);
        [shadowEncoder setViewport:MTLViewport{0.0, 0.0, shadowViewportSize, shadowViewportSize, 0.0, 1.0}];
        [shadowEncoder setRenderPipelineState:impl_->shadowPipeline];
        [shadowEncoder setDepthStencilState:impl_->shadowDepthState];
        const auto drawShadowMesh = [&](id<MTLBuffer> buffer, NSUInteger vertexCount, const Mat4& model) {
            Uniforms shadowUniforms = uniforms;
            const Mat4 shadowMvp = multiply(shadowViewProjection, model);
            copyUniforms(shadowUniforms, shadowMvp, model, shadowMvp, lightEye);
            [shadowEncoder setVertexBuffer:buffer offset:0 atIndex:0];
            [shadowEncoder setVertexBytes:&shadowUniforms length:sizeof(shadowUniforms) atIndex:1];
            [shadowEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:vertexCount];
        };
        drawShadowMesh(impl_->groundBuffer, impl_->groundVertexCount, identity);
        drawShadowMesh(impl_->carBodyBuffer, impl_->carBodyVertexCount, carWorld);
        drawShadowMesh(impl_->steeringWheelBuffer, impl_->steeringWheelVertexCount, steeringWheelWorld);
        if (suspensionVertexCount > 0) {
            drawShadowMesh(impl_->suspensionBuffer, static_cast<NSUInteger>(suspensionVertexCount), identity);
        }
        const auto drawShadowWheel = [&](
                                         float localX,
                                         float localY,
                                         float localZ,
                                         bool frontWheel,
                                         float normalLoadN,
                                         float wheelRotationRadians,
                                         float hubTravelM) {
            drawShadowMesh(
                impl_->wheelBuffer,
                impl_->wheelVertexCount,
                multiply(vehicleFrameWorld, wheelLocalTransform(localX, localY, localZ, frontWheel, normalLoadN, wheelRotationRadians, hubTravelM)));
        };
        drawShadowWheel(-kVisualFrontWheelX, kVisualWheelY, kVisualFrontWheelZ, true, scene.vehicle.frontLeftNormalLoadN, scene.vehicle.frontLeftWheelRotationRadians, scene.vehicle.frontLeftWheelHubTravelM);
        drawShadowWheel(kVisualFrontWheelX, kVisualWheelY, kVisualFrontWheelZ, true, scene.vehicle.frontRightNormalLoadN, scene.vehicle.frontRightWheelRotationRadians, scene.vehicle.frontRightWheelHubTravelM);
        drawShadowWheel(-kVisualRearWheelX, kVisualWheelY, kVisualRearWheelZ, false, scene.vehicle.rearLeftNormalLoadN, scene.vehicle.rearLeftWheelRotationRadians, scene.vehicle.rearLeftWheelHubTravelM);
        drawShadowWheel(kVisualRearWheelX, kVisualWheelY, kVisualRearWheelZ, false, scene.vehicle.rearRightNormalLoadN, scene.vehicle.rearRightWheelRotationRadians, scene.vehicle.rearRightWheelHubTravelM);
        [shadowEncoder endEncoding];

        impl_->passDescriptor.colorAttachments[0].texture = impl_->msaaHdrTexture;
        impl_->passDescriptor.colorAttachments[0].resolveTexture = impl_->hdrTexture;
        impl_->passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        impl_->passDescriptor.colorAttachments[0].storeAction = MTLStoreActionMultisampleResolve;
        impl_->passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
        impl_->passDescriptor.depthAttachment.texture = impl_->msaaDepthTexture;
        impl_->passDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
        impl_->passDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
        impl_->passDescriptor.depthAttachment.clearDepth = 1.0;
        id<MTLRenderCommandEncoder> encoder =
            [commandBuffer renderCommandEncoderWithDescriptor:impl_->passDescriptor];
        if (encoder == nil) {
            impl_->error = "Could not create Metal scene encoder";
            return false;
        }
        [encoder setViewport:MTLViewport{
                                 0.0,
                                 0.0,
                                 static_cast<double>(impl_->drawableWidth),
                                 static_cast<double>(impl_->drawableHeight),
                                 0.0,
                                 1.0,
                             }];
        const auto bindMaterialTextures = [&](id<MTLRenderCommandEncoder> activeEncoder) {
            [activeEncoder setFragmentTexture:impl_->asphaltAlbedoTexture atIndex:0];
            [activeEncoder setFragmentTexture:impl_->asphaltNormalTexture atIndex:1];
            [activeEncoder setFragmentTexture:impl_->concreteAlbedoTexture atIndex:2];
            [activeEncoder setFragmentTexture:impl_->concreteNormalTexture atIndex:3];
            [activeEncoder setFragmentTexture:impl_->grassAlbedoTexture atIndex:4];
            [activeEncoder setFragmentTexture:impl_->grassNormalTexture atIndex:5];
            [activeEncoder setFragmentTexture:impl_->carbonAlbedoTexture atIndex:6];
            [activeEncoder setFragmentTexture:impl_->carbonNormalTexture atIndex:7];
            [activeEncoder setFragmentTexture:impl_->shadowDepthTexture atIndex:8];
            [activeEncoder setFragmentTexture:impl_->smokeTexture atIndex:9];
            [activeEncoder setFragmentTexture:impl_->asphaltRoughnessTexture atIndex:10];
            [activeEncoder setFragmentTexture:impl_->asphaltHeightTexture atIndex:11];
            [activeEncoder setFragmentTexture:impl_->grassRoughnessTexture atIndex:12];
            [activeEncoder setFragmentTexture:impl_->grassHeightTexture atIndex:13];
            [activeEncoder setFragmentTexture:impl_->carbonRoughnessTexture atIndex:14];
            [activeEncoder setFragmentTexture:impl_->skyboxTexture atIndex:15];
            [activeEncoder setFragmentTexture:impl_->concreteRoughnessTexture atIndex:16];
            [activeEncoder setFragmentSamplerState:impl_->materialSampler atIndex:0];
            [activeEncoder setFragmentSamplerState:impl_->shadowSampler atIndex:1];
        };

        [encoder setRenderPipelineState:impl_->skyPipeline];
        [encoder setDepthStencilState:impl_->skyDepthState];
        copyUniforms(uniforms, identity, identity, identity, eye);
        [encoder setVertexBuffer:impl_->skyBuffer offset:0 atIndex:0];
        [encoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];
        [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:1];
        [encoder setFragmentTexture:impl_->skyboxTexture atIndex:0];
        [encoder setFragmentSamplerState:impl_->materialSampler atIndex:0];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:impl_->skyVertexCount];

        [encoder setRenderPipelineState:impl_->worldPipeline];
        [encoder setDepthStencilState:impl_->worldDepthState];
        bindMaterialTextures(encoder);

        copyUniforms(uniforms, viewProjection, identity, shadowViewProjection, eye);
        [encoder setVertexBuffer:impl_->groundBuffer offset:0 atIndex:0];
        [encoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];
        [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:1];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:impl_->groundVertexCount];

        copyUniforms(uniforms, multiply(viewProjection, carWorld), carWorld, multiply(shadowViewProjection, carWorld), eye);
        [encoder setVertexBuffer:impl_->carBodyBuffer offset:0 atIndex:0];
        [encoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];
        [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:1];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:impl_->carBodyVertexCount];

        if (suspensionVertexCount > 0) {
            copyUniforms(uniforms, viewProjection, identity, shadowViewProjection, eye);
            [encoder setVertexBuffer:impl_->suspensionBuffer offset:0 atIndex:0];
            [encoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];
            [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:1];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                        vertexStart:0
                        vertexCount:static_cast<NSUInteger>(suspensionVertexCount)];
        }

        copyUniforms(
            uniforms,
            multiply(viewProjection, steeringWheelWorld),
            steeringWheelWorld,
            multiply(shadowViewProjection, steeringWheelWorld),
            eye);
        [encoder setVertexBuffer:impl_->steeringWheelBuffer offset:0 atIndex:0];
        [encoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];
        [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:1];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                    vertexStart:0
                    vertexCount:impl_->steeringWheelVertexCount];

        const std::size_t steeringDisplayVertexCount = impl_->buildSteeringDisplayVertices(scene);
        if (steeringDisplayVertexCount > 0) {
            copyUniforms(
                uniforms,
                multiply(viewProjection, steeringWheelWorld),
                steeringWheelWorld,
                multiply(shadowViewProjection, steeringWheelWorld),
                eye);
            [encoder setRenderPipelineState:impl_->worldTextPipeline];
            [encoder setDepthStencilState:impl_->textDepthState];
            [encoder setVertexBuffer:impl_->steeringDisplayBuffer offset:0 atIndex:0];
            [encoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];
            [encoder setFragmentTexture:impl_->fontTexture atIndex:0];
            [encoder setFragmentSamplerState:impl_->fontSampler atIndex:0];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                        vertexStart:0
                        vertexCount:static_cast<NSUInteger>(steeringDisplayVertexCount)];
            [encoder setRenderPipelineState:impl_->worldPipeline];
            [encoder setDepthStencilState:impl_->worldDepthState];
            bindMaterialTextures(encoder);
        }

        const auto drawWheel = [&](
                                   float localX,
                                   float localY,
                                   float localZ,
                                   bool frontWheel,
                                   float normalLoadN,
                                   float wheelRotationRadians,
                                   float hubTravelM) {
            const Mat4 localWheel =
                wheelLocalTransform(localX, localY, localZ, frontWheel, normalLoadN, wheelRotationRadians, hubTravelM);
            const Mat4 wheelWorld = multiply(vehicleFrameWorld, localWheel);
            copyUniforms(
                uniforms,
                multiply(viewProjection, wheelWorld),
                wheelWorld,
                multiply(shadowViewProjection, wheelWorld),
                eye);
            [encoder setVertexBuffer:impl_->wheelBuffer offset:0 atIndex:0];
            [encoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];
            [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:1];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                        vertexStart:0
                        vertexCount:impl_->wheelVertexCount];
        };
        drawWheel(-kVisualFrontWheelX, kVisualWheelY, kVisualFrontWheelZ, true, scene.vehicle.frontLeftNormalLoadN, scene.vehicle.frontLeftWheelRotationRadians, scene.vehicle.frontLeftWheelHubTravelM);
        drawWheel(kVisualFrontWheelX, kVisualWheelY, kVisualFrontWheelZ, true, scene.vehicle.frontRightNormalLoadN, scene.vehicle.frontRightWheelRotationRadians, scene.vehicle.frontRightWheelHubTravelM);
        drawWheel(-kVisualRearWheelX, kVisualWheelY, kVisualRearWheelZ, false, scene.vehicle.rearLeftNormalLoadN, scene.vehicle.rearLeftWheelRotationRadians, scene.vehicle.rearLeftWheelHubTravelM);
        drawWheel(kVisualRearWheelX, kVisualWheelY, kVisualRearWheelZ, false, scene.vehicle.rearRightNormalLoadN, scene.vehicle.rearRightWheelRotationRadians, scene.vehicle.rearRightWheelHubTravelM);

        if (scene.ghost.available) {
            Uniforms ghostUniforms = uniforms;
            ghostUniforms.effectParams[0] = 0.0F;
            ghostUniforms.effectParams[1] = uniforms.effectParams[1];
            ghostUniforms.effectParams[2] = 0.0F;
            ghostUniforms.effectParams[3] = 0.32F;
            const Mat4 ghostWorld =
                multiply(
                    multiply(
                        translation(scene.ghost.positionX, scene.ghost.heightM, scene.ghost.positionZ),
                        rotationY(scene.ghost.yawRadians)),
                    rotationZ(scene.ghost.bankRadians));
            [encoder setRenderPipelineState:impl_->fxPipeline];
            [encoder setDepthStencilState:impl_->fxDepthState];
            bindMaterialTextures(encoder);
            copyUniforms(
                ghostUniforms,
                multiply(viewProjection, ghostWorld),
                ghostWorld,
                multiply(shadowViewProjection, ghostWorld),
                eye);
            [encoder setVertexBuffer:impl_->carBodyBuffer offset:0 atIndex:0];
            [encoder setVertexBytes:&ghostUniforms length:sizeof(ghostUniforms) atIndex:1];
            [encoder setFragmentBytes:&ghostUniforms length:sizeof(ghostUniforms) atIndex:1];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                        vertexStart:0
                        vertexCount:impl_->carBodyVertexCount];

            const auto drawGhostWheel = [&](float localX, float localY, float localZ, bool frontWheel) {
                const Mat4 localWheel =
                    multiply(
                        multiply(
                            translation(localX, localY, localZ),
                            rotationY(frontWheel ? 0.08F : 0.0F)),
                        scale(
                            frontWheel ? kVisualFrontWheelWidthScale : 1.0F,
                            frontWheel ? kVisualFrontWheelRadiusScale : 1.0F,
                            frontWheel ? kVisualFrontWheelRadiusScale : 1.0F));
                const Mat4 wheelWorld = multiply(ghostWorld, localWheel);
                copyUniforms(
                    ghostUniforms,
                    multiply(viewProjection, wheelWorld),
                    wheelWorld,
                    multiply(shadowViewProjection, wheelWorld),
                    eye);
                [encoder setVertexBuffer:impl_->wheelBuffer offset:0 atIndex:0];
                [encoder setVertexBytes:&ghostUniforms length:sizeof(ghostUniforms) atIndex:1];
                [encoder setFragmentBytes:&ghostUniforms length:sizeof(ghostUniforms) atIndex:1];
                [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                            vertexStart:0
                            vertexCount:impl_->wheelVertexCount];
            };
            drawGhostWheel(-kVisualFrontWheelX, kVisualWheelY, kVisualFrontWheelZ, true);
            drawGhostWheel(kVisualFrontWheelX, kVisualWheelY, kVisualFrontWheelZ, true);
            drawGhostWheel(-kVisualRearWheelX, kVisualWheelY, kVisualRearWheelZ, false);
            drawGhostWheel(kVisualRearWheelX, kVisualWheelY, kVisualRearWheelZ, false);

            [encoder setRenderPipelineState:impl_->worldPipeline];
            [encoder setDepthStencilState:impl_->worldDepthState];
            bindMaterialTextures(encoder);
        }

        const std::size_t skidmarkVertexCount = impl_->updateSkidmarkVertices(scene, carForward, carRight);
        const std::size_t smokeVertexCount =
            impl_->buildSmokeVertices(scene, carForward, carRight, cameraRight, cameraUp);
        if (smokeVertexCount > 0 || skidmarkVertexCount > 0) {
            [encoder setRenderPipelineState:impl_->fxPipeline];
            [encoder setDepthStencilState:impl_->fxDepthState];
            bindMaterialTextures(encoder);
        }
        if (skidmarkVertexCount > 0) {
            copyUniforms(uniforms, viewProjection, identity, shadowViewProjection, eye);
            [encoder setVertexBuffer:impl_->skidmarkBuffer offset:0 atIndex:0];
            [encoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];
            [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:1];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                        vertexStart:0
                        vertexCount:static_cast<NSUInteger>(skidmarkVertexCount)];
        }
        if (smokeVertexCount > 0) {
            copyUniforms(uniforms, viewProjection, identity, shadowViewProjection, eye);
            [encoder setVertexBuffer:impl_->smokeBuffer offset:0 atIndex:0];
            [encoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];
            [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:1];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                        vertexStart:0
                        vertexCount:static_cast<NSUInteger>(smokeVertexCount)];
        }

        [encoder endEncoding];

        Uniforms postUniforms = uniforms;
        copyUniforms(postUniforms, identity, identity, identity, eye);
        const auto runBloomPass =
            [&](MTLRenderPassDescriptor* descriptor,
                id<MTLTexture> target,
                id<MTLTexture> source,
                int targetWidth,
                int targetHeight,
                int sourceWidth,
                int sourceHeight,
                const char* label) -> bool {
            descriptor.colorAttachments[0].texture = target;
            descriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
            descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
            descriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
            id<MTLRenderCommandEncoder> bloomEncoder =
                [commandBuffer renderCommandEncoderWithDescriptor:descriptor];
            if (bloomEncoder == nil) {
                impl_->error = label;
                return false;
            }
            [bloomEncoder setViewport:MTLViewport{
                                          0.0,
                                          0.0,
                                          static_cast<double>(std::max(1, targetWidth)),
                                          static_cast<double>(std::max(1, targetHeight)),
                                          0.0,
                                          1.0,
                                      }];
            Uniforms bloomUniforms = postUniforms;
            bloomUniforms.renderParams[0] = static_cast<float>(std::max(1, sourceWidth));
            bloomUniforms.renderParams[1] = static_cast<float>(std::max(1, sourceHeight));
            [bloomEncoder setRenderPipelineState:impl_->bloomPipeline];
            [bloomEncoder setVertexBuffer:impl_->skyBuffer offset:0 atIndex:0];
            [bloomEncoder setVertexBytes:&bloomUniforms length:sizeof(bloomUniforms) atIndex:1];
            [bloomEncoder setFragmentBytes:&bloomUniforms length:sizeof(bloomUniforms) atIndex:1];
            [bloomEncoder setFragmentTexture:source atIndex:0];
            [bloomEncoder setFragmentSamplerState:impl_->fontSampler atIndex:0];
            [bloomEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:impl_->skyVertexCount];
            [bloomEncoder endEncoding];
            return true;
        };
        if (!runBloomPass(
                impl_->bloomPassDescriptor,
                impl_->bloomTexture,
                impl_->hdrTexture,
                std::max(1, impl_->drawableWidth / 2),
                std::max(1, impl_->drawableHeight / 2),
                impl_->drawableWidth,
                impl_->drawableHeight,
                "Could not create Metal half-resolution bloom encoder")) {
            return false;
        }
        if (!runBloomPass(
                impl_->bloomQuarterPassDescriptor,
                impl_->bloomQuarterTexture,
                impl_->bloomTexture,
                std::max(1, impl_->drawableWidth / 4),
                std::max(1, impl_->drawableHeight / 4),
                std::max(1, impl_->drawableWidth / 2),
                std::max(1, impl_->drawableHeight / 2),
                "Could not create Metal quarter-resolution bloom encoder")) {
            return false;
        }
        if (!runBloomPass(
                impl_->bloomEighthPassDescriptor,
                impl_->bloomEighthTexture,
                impl_->bloomQuarterTexture,
                std::max(1, impl_->drawableWidth / 8),
                std::max(1, impl_->drawableHeight / 8),
                std::max(1, impl_->drawableWidth / 4),
                std::max(1, impl_->drawableHeight / 4),
                "Could not create Metal eighth-resolution bloom encoder")) {
            return false;
        }

        impl_->finalPassDescriptor.colorAttachments[0].texture = drawable.texture;
        impl_->finalPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        impl_->finalPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        impl_->finalPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
        impl_->finalPassDescriptor.depthAttachment.texture = impl_->depthTexture;
        impl_->finalPassDescriptor.depthAttachment.loadAction = MTLLoadActionDontCare;
        impl_->finalPassDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
        id<MTLRenderCommandEncoder> finalEncoder =
            [commandBuffer renderCommandEncoderWithDescriptor:impl_->finalPassDescriptor];
        if (finalEncoder == nil) {
            impl_->error = "Could not create Metal final encoder";
            return false;
        }
        [finalEncoder setViewport:MTLViewport{
                                     0.0,
                                     0.0,
                                     static_cast<double>(impl_->drawableWidth),
                                     static_cast<double>(impl_->drawableHeight),
                                     0.0,
                                     1.0,
                                 }];
        copyUniforms(postUniforms, identity, identity, identity, eye);
        postUniforms.effectParams[0] =
            std::pow(std::clamp(scene.vehicle.speedMps / 110.0F, 0.0F, 1.0F), 1.35F) * 0.42F;
        postUniforms.effectParams[1] = 0.0F;
        postUniforms.effectParams[2] = 0.0F;
        postUniforms.effectParams[3] = 0.0F;
        [finalEncoder setRenderPipelineState:impl_->postPipeline];
        [finalEncoder setVertexBuffer:impl_->skyBuffer offset:0 atIndex:0];
        [finalEncoder setVertexBytes:&postUniforms length:sizeof(postUniforms) atIndex:1];
        [finalEncoder setFragmentBytes:&postUniforms length:sizeof(postUniforms) atIndex:1];
        [finalEncoder setFragmentTexture:impl_->hdrTexture atIndex:0];
        [finalEncoder setFragmentTexture:impl_->bloomTexture atIndex:1];
        [finalEncoder setFragmentTexture:impl_->bloomQuarterTexture atIndex:2];
        [finalEncoder setFragmentTexture:impl_->bloomEighthTexture atIndex:3];
        [finalEncoder setFragmentSamplerState:impl_->fontSampler atIndex:0];
        [finalEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:impl_->skyVertexCount];

        const Mat4 textProjection = orthographic(
            0.0F,
            static_cast<float>(impl_->drawableWidth),
            static_cast<float>(impl_->drawableHeight),
            0.0F,
            0.0F,
            1.0F);
        const std::size_t uiVertexCount = impl_->buildUiVertices(scene, overlay);
        if (uiVertexCount > 0) {
            copyUniforms(uniforms, textProjection, identity, identity, eye);
            [finalEncoder setRenderPipelineState:impl_->uiPipeline];
            [finalEncoder setDepthStencilState:impl_->textDepthState];
            [finalEncoder setVertexBuffer:impl_->uiBuffer offset:0 atIndex:0];
            [finalEncoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];
            [finalEncoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:1];
            [finalEncoder setFragmentTexture:impl_->hdrTexture atIndex:0];
            [finalEncoder setFragmentSamplerState:impl_->fontSampler atIndex:0];
            [finalEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                        vertexStart:0
                        vertexCount:static_cast<NSUInteger>(uiVertexCount)];
        }

        const std::size_t textVertexCount = impl_->buildTextVertices(overlay);
        if (textVertexCount > 0) {
            copyUniforms(uniforms, textProjection, identity, identity, eye);
            [finalEncoder setRenderPipelineState:impl_->textPipeline];
            [finalEncoder setDepthStencilState:impl_->textDepthState];
            [finalEncoder setVertexBuffer:impl_->textBuffer offset:0 atIndex:0];
            [finalEncoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];
            [finalEncoder setFragmentTexture:impl_->fontTexture atIndex:0];
            [finalEncoder setFragmentSamplerState:impl_->fontSampler atIndex:0];
            [finalEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                        vertexStart:0
                        vertexCount:static_cast<NSUInteger>(textVertexCount)];
        }

        [finalEncoder endEncoding];
        [commandBuffer presentDrawable:drawable];
        [commandBuffer commit];
        return true;
    }
}

const std::string& MetalRenderer::error() const {
    return impl_->error;
}

}  // namespace sim
