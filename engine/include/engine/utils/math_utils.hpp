#pragma once
#include <array>
#include <cmath>

namespace Solace {

    struct Vec3 {
        float x = 0.0f, y = 0.0f, z = 0.0f;

        Vec3() = default;
        Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
        explicit Vec3(float s) : x(s), y(s), z(s) {}

        Vec3 operator+(const Vec3& o) const { return { x + o.x, y + o.y, z + o.z }; }
        Vec3 operator-(const Vec3& o) const { return { x - o.x, y - o.y, z - o.z }; }
        Vec3 operator*(float s)      const { return { x * s, y * s, z * s }; }

        Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
        Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
        Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

        float length_sqr() const { return x * x + y * y + z * z; }
        float length()     const { return std::sqrt(length_sqr()); }
        Vec3  normalized() const {
            const float len = length();
            return len > 1e-6f ? (*this) * (1.0f / len) : Vec3{};
        }
    };

    using Mat4 = std::array<float, 16>;

    Mat4 mat4Identity();
    Mat4 mat4Translate(float x, float y, float z);
    Mat4 mat4Scale(float x, float y, float z);
    Mat4 mat4Multiply(const Mat4& a, const Mat4& b);

    Mat4 mat4Perspective(float fovYRad, float aspect, float nearZ, float farZ);

    Mat4 mat4LookAt(float ex, float ey, float ez,
        float cx, float cy, float cz,
        float ux, float uy, float uz);

} // namespace Solace 