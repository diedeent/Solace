#include <engine/utils/math_utils.hpp>
#include <cmath>

namespace Solace {

    Mat4 mat4Identity() {
        return {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
    }

    Mat4 mat4Translate(float x, float y, float z) {
        Mat4 m = mat4Identity();
        m[12] = x;
        m[13] = y;
        m[14] = z;
        return m;
    }

    Mat4 mat4Scale(float x, float y, float z) {
        Mat4 m = { 0 };
        m[0] = x;
        m[5] = y;
        m[10] = z;
        m[15] = 1.0f;
        return m;
    }

    Mat4 mat4Multiply(const Mat4& a, const Mat4& b) {
        Mat4 r{};
        for (int c = 0; c < 4; ++c) {
            for (int row = 0; row < 4; ++row) {
                r[c * 4 + row] =
                    a[0 * 4 + row] * b[c * 4 + 0] +
                    a[1 * 4 + row] * b[c * 4 + 1] +
                    a[2 * 4 + row] * b[c * 4 + 2] +
                    a[3 * 4 + row] * b[c * 4 + 3];
            }
        }
        return r;
    }

    Mat4 mat4Perspective(float fovYRad, float aspect, float nearZ, float farZ) {
        const float t = 1.0f / std::tan(fovYRad * 0.5f);
        Mat4 m{};
        m[0] = t / aspect;
        m[5] = t;
        m[10] = (farZ + nearZ) / (nearZ - farZ);
        m[11] = -1.0f;
        m[14] = (2.0f * farZ * nearZ) / (nearZ - farZ);
        return m;
    }

    Mat4 mat4LookAt(float ex, float ey, float ez,
        float cx, float cy, float cz,
        float ux, float uy, float uz)
    {
        // (Forward / Z-axis)
        float zx = ex - cx, zy = ey - cy, zz = ez - cz;
        float len = std::sqrt(zx * zx + zy * zy + zz * zz);
        if (len > 0.0f) { zx /= len; zy /= len; zz /= len; }

        // (Right / X-axis) = Up x Forward
        float xx = uy * zz - uz * zy;
        float xy = uz * zx - ux * zz;
        float xz = ux * zy - uy * zx;
        len = std::sqrt(xx * xx + xy * xy + xz * xz);
        if (len > 0.0f) { xx /= len; xy /= len; xz /= len; }

        // (True Up / Y-axis) = Forward x Right
        float yx = zy * xz - zz * xy;
        float yy = zz * xx - zx * xz;
        float yz = zx * xy - zy * xx;

        Mat4 m{};
        // (X-axis / Right)
        m[0] = xx;  m[1] = yx;  m[2] = zx;  m[3] = 0.0f;
        // (Y-axis / Up)
        m[4] = xy;  m[5] = yy;  m[6] = zy;  m[7] = 0.0f;
        // (Z-axis / Forward)
        m[8] = xz;  m[9] = yz;  m[10] = zz; m[11] = 0.0f;
        // (Position / Translation)
        m[12] = -(xx * ex + xy * ey + xz * ez);
        m[13] = -(yx * ex + yy * ey + yz * ez);
        m[14] = -(zx * ex + zy * ey + zz * ez);
        m[15] = 1.0f;

        return m;
    }

} // namespace Solace 