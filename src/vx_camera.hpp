#ifndef VX_CAMERA_HPP
#define VX_CAMERA_HPP

#include <math.h>
#include "vx_frustum.hpp"
#include "glm/fwd.hpp"

namespace vx
{

struct Frustum;

struct Camera
{
    static constexpr f32 ZNEAR = 0.1f;
    static constexpr f32 ZFAR = 600.0f;

    enum Direction : u32
    {
        FORWARDS, BACKWARDS, LEFT, RIGHT,
    };

    f32          yaw;
    f32          pitch;
    glm::vec3    up_world;
    f32          move_speed;
    f32          turn_speed;
    Frustum      frustum;

    Camera(glm::vec3 position, f32 fovy, f32 yaw, f32 pitch,
           glm::vec3 up_world, f32 ratio, f32 move_speed, f32 turn_speed);

    void rotate_yaw(f32 yawOffset);
    void rotate_pitch(f32 pitchOffset);
    void move(Direction dir, f64 delta);
    glm::mat4 view_matrix() const;
};


}
#endif // VX_CAMERA_HPP
