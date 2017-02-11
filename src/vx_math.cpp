#include "vx_math.hpp"

namespace um
{

Mat4x4f
look_at(const Vec3f from, const Vec3f to, const Vec3f up_world)
{
    Vec3f forward = um::normalize(from - to);
    Vec3f right = um::normalize(cross(up_world, forward));
    Vec3f up  = um::cross(forward, right);

    return Mat4x4f(right.x,   right.y,   right.z,   -um::dot(from,   right),
                   up.x,      up.y,      up.z,      -um::dot(from,      up),
                   forward.x, forward.y, forward.z, -um::dot(from, forward),
                   0.0f,      0.0f,      0.0f,      1.0f);
    // return Mat4x4f(right.x, up.x, forward.x, -um::dot(from,   right),
    //                right.y, up.y, forward.y, -um::dot(from,      up),
    //                right.z, up.z, forward.z, -um::dot(from, forward),
    //                0.0f,      0.0f,      0.0f,      1.0f);
}

Mat4x4f
perspective(f32 fovy_deg, f32 aspect_ratio, f32 znear, f32 zfar)
{
    f32 fovyRad = fovy_deg / 180 * M_PI;
    f32 f = 1.0f / tanf(fovyRad / 2.0f);
    f32 ar = aspect_ratio;

    return Mat4x4f(
        f / ar,           0,                0,                         0,
        0,                f,                0,                         0,
        0,                0,               (zfar+znear)/(znear-zfar),  (2*zfar*znear)/(znear-zfar),
        0,                0,               -1,                         0
    );
}

Mat4x4f
orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 znear, f32 zfar)
{
    f32 l = left, r = right, b = bottom, t = top; //, n = front, f = back;
    f32 n = znear;
    f32 f = zfar;
    f32 tx = -(r + l) / (r - l);
    f32 ty = -(t + b) / (t - b);
    f32 tz = -(f + n) / (f - n);
    return Mat4x4f(
        2.0f / (r - l),  0.0f,            0.0f,            tx,
        0.0f,            2.0f / (t - b),  0.0f,            ty,
        0.0f,            0.0f,           -2.0f / (f - n),  tz,
        0.0f,            0.0f,            0.0f,            1.0f
    );
}

inline Mat4x4f
make_rotation(const f32 angleInRads, const Vec3f axis)
{
    Vec3f ax = um::normalize(axis);
    f32 s = sinf(angleInRads);
    f32 c = cosf(angleInRads);

    return Mat4x4f(
        c+pow(ax.x, 2)*(1.0f - c), ax.x*ax.y*(1.0f-c)-ax.z*s, ax.x*ax.z*(1.0f-c)+ax.y*s,     0.0f,
        ax.y*ax.x*(1.0f-c)+ax.z*s, c+pow(ax.y, 2)*(1.0f - c), ax.y*ax.z*(1.0f - c)-ax.x * s, 0.0f,
        ax.z*ax.x*(1.0f-c)-ax.y*s, ax.z*ax.y*(1.0f-c)+ax.x*s, c + pow(ax.z, 2)*(1.0f-c),     0.0f,
        0.0f,                      0.0f,                      0.0f,                          1.0f);
}
}
