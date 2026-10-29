#pragma once
#include <array>
#include <cmath>

namespace Solace {
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