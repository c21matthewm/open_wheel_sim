#pragma once

#include <array>
#include <cmath>

namespace sim {

struct Vec3 {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

struct Mat4 {
    std::array<float, 16> values{};

    static Mat4 identity() {
        Mat4 result;
        result.values[0] = 1.0F;
        result.values[5] = 1.0F;
        result.values[10] = 1.0F;
        result.values[15] = 1.0F;
        return result;
    }
};

inline Vec3 subtract(Vec3 lhs, Vec3 rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

inline float dot(Vec3 lhs, Vec3 rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

inline Vec3 cross(Vec3 lhs, Vec3 rhs) {
    return {
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x,
    };
}

inline Vec3 normalize(Vec3 value) {
    const float length = std::sqrt(dot(value, value));
    return length > 0.00001F
               ? Vec3{value.x / length, value.y / length, value.z / length}
               : Vec3{0.0F, 0.0F, 1.0F};
}

inline Mat4 multiply(const Mat4& lhs, const Mat4& rhs) {
    Mat4 result;
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0F;
            for (int index = 0; index < 4; ++index) {
                sum += lhs.values[index * 4 + row] * rhs.values[column * 4 + index];
            }
            result.values[column * 4 + row] = sum;
        }
    }
    return result;
}

inline Mat4 translation(float x, float y, float z) {
    Mat4 result = Mat4::identity();
    result.values[12] = x;
    result.values[13] = y;
    result.values[14] = z;
    return result;
}

inline Mat4 rotationX(float radians) {
    Mat4 result = Mat4::identity();
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    result.values[5] = cosine;
    result.values[6] = sine;
    result.values[9] = -sine;
    result.values[10] = cosine;
    return result;
}

inline Mat4 rotationY(float radians) {
    Mat4 result = Mat4::identity();
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    result.values[0] = cosine;
    result.values[2] = -sine;
    result.values[8] = sine;
    result.values[10] = cosine;
    return result;
}

inline Mat4 rotationZ(float radians) {
    Mat4 result = Mat4::identity();
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    result.values[0] = cosine;
    result.values[1] = sine;
    result.values[4] = -sine;
    result.values[5] = cosine;
    return result;
}

inline Mat4 scale(float x, float y, float z) {
    Mat4 result = Mat4::identity();
    result.values[0] = x;
    result.values[5] = y;
    result.values[10] = z;
    return result;
}

inline Mat4 perspectiveLeftHanded(float verticalFovRadians, float aspect, float nearPlane, float farPlane) {
    Mat4 result;
    const float focalLength = 1.0F / std::tan(verticalFovRadians * 0.5F);
    result.values[0] = focalLength / aspect;
    result.values[5] = focalLength;
    result.values[10] = farPlane / (farPlane - nearPlane);
    result.values[11] = 1.0F;
    result.values[14] = -(nearPlane * farPlane) / (farPlane - nearPlane);
    return result;
}

inline Mat4 lookAtLeftHanded(Vec3 eye, Vec3 target, Vec3 up) {
    const Vec3 forward = normalize(subtract(target, eye));
    const Vec3 right = normalize(cross(up, forward));
    const Vec3 cameraUp = cross(forward, right);

    Mat4 result = Mat4::identity();
    result.values[0] = right.x;
    result.values[4] = right.y;
    result.values[8] = right.z;
    result.values[12] = -dot(right, eye);
    result.values[1] = cameraUp.x;
    result.values[5] = cameraUp.y;
    result.values[9] = cameraUp.z;
    result.values[13] = -dot(cameraUp, eye);
    result.values[2] = forward.x;
    result.values[6] = forward.y;
    result.values[10] = forward.z;
    result.values[14] = -dot(forward, eye);
    return result;
}

inline Mat4 orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
    Mat4 result = Mat4::identity();
    result.values[0] = 2.0F / (right - left);
    result.values[5] = 2.0F / (top - bottom);
    result.values[10] = 1.0F / (farPlane - nearPlane);
    result.values[12] = -(right + left) / (right - left);
    result.values[13] = -(top + bottom) / (top - bottom);
    result.values[14] = -nearPlane / (farPlane - nearPlane);
    return result;
}

}  // namespace sim
