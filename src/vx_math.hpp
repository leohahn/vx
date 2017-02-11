#ifndef VX_MATH_H
#define VX_MATH_H

#include <stdio.h>
#include <math.h>
#include "um.hpp"
#include "glm/glm.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Vec4f
{
    f32 x, y, z, w;

    Vec4f() {}
    Vec4f(f32 x, f32 y, f32 z, f32 w): x(x), y(y), z(z), w(w) {}
};

struct Vec2f
{
    f32 x, y;

    Vec2f() {}
    Vec2f(f32 x, f32 y): x(x), y(y) {}
};

struct Vec3f
{
    f32 x, y, z;

    Vec3f() {}
    Vec3f(f32 x, f32 y, f32 z): x(x), y(y), z(z) {}

    inline void print() const
    {
        printf("x: %f, y: %f, z: %f\n", x, y, z);
    }

    Vec3f operator*(f32 k) const
    {
        return Vec3f(x*k, y*k, z*k);
    }

    Vec3f operator+(const Vec3f rhs) const
    {
        return Vec3f(x+rhs.x, y+rhs.y, z+rhs.z);
    }

    Vec3f operator-(const Vec3f rhs) const
    {
        return Vec3f(x-rhs.x, y-rhs.y, z-rhs.z);
    }

    Vec3f operator-() const
    {
        return Vec3f(-x, -y, -z);
    }
};

struct Quad3
{
    glm::vec3 p1;
    glm::vec3 p2;
    glm::vec3 p3;
    glm::vec3 p4;

    Quad3() {}
    Quad3(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4)
        : p1(p1), p2(p2), p3(p3), p4(p4) {}
};


/*
 *  Matrix Definition
 */
struct Mat4x4f
{
    union
    {
        f32 m[4][4];

        struct
        {
            f32 m00, m01, m02, m03;
            f32 m10, m11, m12, m13;
            f32 m20, m21, m22, m23;
            f32 m30, m31, m32, m33;
        };
    };

    Mat4x4f()
        : m00(1), m01(0), m02(0), m03(0)
        , m10(0), m11(1), m12(0), m13(0)
        , m20(0), m21(0), m22(1), m23(0)
        , m30(0), m31(0), m32(0), m33(1)
    {
    }
    Mat4x4f(f32 m00, f32 m10, f32 m20, f32 m30,
            f32 m01, f32 m11, f32 m21, f32 m31,
            f32 m02, f32 m12, f32 m22, f32 m32,
            f32 m03, f32 m13, f32 m23, f32 m33)
        : m00(m00), m01(m01), m02(m02), m03(m03)
        , m10(m10), m11(m11), m12(m12), m13(m13)
        , m20(m20), m21(m21), m22(m22), m23(m23)
        , m30(m30), m31(m31), m32(m32), m33(m33)
    {
    }

    Mat4x4f(f32 diagonal)
        : m00(diagonal), m01(0),        m02(0),        m03(0)
        , m10(0),        m11(diagonal), m12(0),        m13(0)
        , m20(0),        m21(0),        m22(diagonal), m23(0)
        , m30(0),        m31(0),        m32(0),        m33(diagonal)
    {}

    inline void print() const
    {
        i32 w = 8, p = 4;
        for(i32 r = 0; r < 4; r++) {
            printf("| %*.*f %*.*f %*.*f %*.*f |\n",
                   w, p, m[0][r], w, p, m[1][r], w, p, m[2][r], w, p, m[3][r]
            );
        }
    }

    Mat4x4f operator*(const Mat4x4f& rhs) const
    {
        Mat4x4f res;
        res.m00 = m00 * rhs.m00 + m10 * rhs.m01 + m20 * rhs.m02 + m30 * rhs.m03;
        res.m10 = m00 * rhs.m10 + m10 * rhs.m11 + m20 * rhs.m12 + m30 * rhs.m13;
        res.m20 = m00 * rhs.m20 + m10 * rhs.m21 + m20 * rhs.m22 + m30 * rhs.m23;
        res.m30 = m00 * rhs.m30 + m10 * rhs.m31 + m20 * rhs.m32 + m30 * rhs.m33;
        res.m01 = m01 * rhs.m00 + m11 * rhs.m01 + m21 * rhs.m02 + m31 * rhs.m03;
        res.m11 = m01 * rhs.m10 + m11 * rhs.m11 + m21 * rhs.m12 + m31 * rhs.m13;
        res.m21 = m01 * rhs.m20 + m11 * rhs.m21 + m21 * rhs.m22 + m31 * rhs.m23;
        res.m31 = m01 * rhs.m30 + m11 * rhs.m31 + m21 * rhs.m32 + m31 * rhs.m33;
        res.m02 = m02 * rhs.m00 + m12 * rhs.m01 + m22 * rhs.m02 + m32 * rhs.m03;
        res.m12 = m02 * rhs.m10 + m12 * rhs.m11 + m22 * rhs.m12 + m32 * rhs.m13;
        res.m22 = m02 * rhs.m20 + m12 * rhs.m21 + m22 * rhs.m22 + m32 * rhs.m23;
        res.m32 = m02 * rhs.m30 + m12 * rhs.m31 + m22 * rhs.m32 + m32 * rhs.m33;
        res.m03 = m03 * rhs.m00 + m13 * rhs.m01 + m23 * rhs.m02 + m33 * rhs.m03;
        res.m13 = m03 * rhs.m10 + m13 * rhs.m11 + m23 * rhs.m12 + m33 * rhs.m13;
        res.m23 = m03 * rhs.m20 + m13 * rhs.m21 + m23 * rhs.m22 + m33 * rhs.m23;
        res.m33 = m03 * rhs.m30 + m13 * rhs.m31 + m23 * rhs.m32 + m33 * rhs.m33;
        return res;
    }

    Vec3f operator*(const Vec3f vec)
    {
        return Vec3f((vec.x * m00) + (vec.y * m10) + (vec.z * m20),
                     (vec.x * m01) + (vec.y * m11) + (vec.z * m21),
                     (vec.x * m02) + (vec.y * m12) + (vec.z * m22));
    }

};

namespace um
{

inline f32 radians(f32 Angle) { return Angle * M_PI / 180.0f; }

inline Vec3f
normalize(const Vec3f vec)
{
    f32 length = sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
    ASSERT(length >= 0);
    return Vec3f(vec.x/length, vec.y/length, vec.z/length);
}

inline f32
dot(const Vec3f lhs, const Vec3f rhs)
{
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

inline Vec3f
cross(const Vec3f lhs, const Vec3f rhs)
{
    return Vec3f(
        (lhs.y * rhs.z) - (lhs.z * rhs.y),
        (lhs.z * rhs.x) - (lhs.x * rhs.z),
        (lhs.x * rhs.y) - (lhs.y * rhs.x)
    );
}

inline Mat4x4f
make_rotation_x(f32 angleInRads)
{
    f32 s = sinf(angleInRads);
    f32 c = cosf(angleInRads);
    return Mat4x4f(
        1,  0,  0,  0,
        0,  c, -s,  0,
        0,  s,  c,  0,
        0,  0,  0,  1);
}

inline Mat4x4f
make_rotation_y(f32 angleInRads)
{
    f32 s = sinf(angleInRads);
    f32 c = cosf(angleInRads);
    return Mat4x4f(
        c,  0,  s,  0,
        0,  1,  0,  0,
        -s,  0,  c,  0,
        0,  0,  0,  1);
}

inline Mat4x4f
make_rotation_z(f32 angleInRads)
{
    f32 s = sinf(angleInRads);
    f32 c = cosf(angleInRads);
    return Mat4x4f(
        c,  -s,  0,  0,
        s,   c,  0,  0,
        0,   0,  1,  0,
        0,   0,  0,  1);
}

inline Mat4x4f
make_translation(const Mat4x4f& matrix, const Vec3f translation)
{
    Mat4x4f res = matrix;
    res.m30 = translation.x;
    res.m31 = translation.y;
    res.m32 = translation.z;
    return res;
}

inline Mat4x4f
make_scale(const Mat4x4f& matrix, const Vec3f scalingVector)
{
    Mat4x4f m = matrix;
    m.m00 = matrix.m00 * scalingVector.x;
    m.m11 = matrix.m11 * scalingVector.y;
    m.m22 = matrix.m22 * scalingVector.z;
    return m;
}

Mat4x4f look_at(const Vec3f from, const Vec3f to, const Vec3f up);
Mat4x4f perspective(f32 fovy_deg, f32 aspect_ratio, f32 znear, f32 zfar);
Mat4x4f orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 znear, f32 zfar);
Mat4x4f make_rotation(const f32 angleInRads, const Vec3f axis);

}
#endif // VX_MATH_H
