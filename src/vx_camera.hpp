#ifndef VX_CAMERA_HPP
#define VX_CAMERA_HPP

#include <math.h>
#include "um_math.hpp"
#include "vx_frustum.hpp"

namespace vx
{

struct Frustum;

struct Camera
{
    enum Direction : u32
    {
        FORWARDS, BACKWARDS, LEFT, RIGHT,
    };

    f32          yaw;
    f32          pitch;
    Vec3f        upWorld;
    f32          moveSpeed;
    f32          turnSpeed;
    Frustum      frustum;

    Camera(Vec3f position, f32 fovy, f32 yaw, f32 pitch,
           Vec3f upWorld, f32 ratio, f32 znear, f32 zfar,
           f32 moveSpeed, f32 turnSpeed);

    void rotate_yaw(f32 yawOffset);
    void rotate_pitch(f32 pitchOffset);
    void move(Direction dir, f64 delta);
    Mat4x4f view_matrix() const;
};


}
#endif // VX_CAMERA_HPP
