#include "vx_camera.hpp"
#include "vx_frustum.hpp"
#include "glm/gtc/matrix_transform.hpp"

void
update_frustum_orientation(vx::Frustum& frustum, f32 yaw, f32 pitch, glm::vec3 up_world)
{
    f32 pitch_rads = glm::radians(pitch);
    f32 yaw_rads = glm::radians(yaw);

    glm::vec3 front;
    front.x = cosf(pitch_rads) * cosf(yaw_rads);
    front.y = sinf(pitch_rads);
    front.z = cosf(pitch_rads) * sinf(yaw_rads);

    frustum.front = glm::normalize(front);
    frustum.right = glm::normalize(glm::cross(frustum.front, up_world));
    frustum.up = glm::normalize(glm::cross(frustum.right, frustum.front));
}

void
update_frustum_points(vx::Frustum& frustum)
{
    f32 fovy_rads = glm::radians(frustum.fovy);
    frustum.znear_center = frustum.position + frustum.front * frustum.znear;
    frustum.zfar_center  = frustum.position + frustum.front * frustum.zfar;
    frustum.zfar_height  = fabs(2 * tanf(fovy_rads / 2) * frustum.zfar);
    frustum.zfar_width   = frustum.zfar_height * frustum.ratio;
    frustum.znear_height = fabs(2 * tanf(fovy_rads / 2) * frustum.znear);
    frustum.znear_width  = frustum.znear_height * frustum.ratio;
}

vx::Camera::Camera(glm::vec3 position, f32 fovy, f32 yaw, f32 pitch, glm::vec3 up_world, f32 ratio,
                   f32 move_speed, f32 turn_speed)
    : yaw(yaw)
    , pitch(pitch)
    , up_world(up_world)
    , move_speed(move_speed)
    , turn_speed(turn_speed)
{
    frustum.position = position;
    frustum.ratio = ratio;
    frustum.fovy = fovy;
    frustum.znear = ZNEAR;
    frustum.zfar = ZFAR;
    frustum.projection = glm::perspective(glm::radians(fovy), ratio, ZNEAR, ZFAR);

    update_frustum_orientation(frustum, yaw, pitch, up_world);
    update_frustum_points(frustum);
}

void
vx::Camera::rotate_yaw(f32 yaw_degrees)
{
    this->yaw += yaw_degrees;
    update_frustum_orientation(this->frustum, this->yaw, this->pitch, this->up_world);
    update_frustum_points(this->frustum);
}

void
vx::Camera::rotate_pitch(f32 pitch_degrees)
{
    f32 new_pitch = pitch_degrees + this->pitch;

    if ((i32)new_pitch > 89 || (i32)new_pitch < -89)
        return;

    this->pitch = new_pitch;

    update_frustum_orientation(this->frustum, this->yaw, this->pitch, this->up_world);
    update_frustum_points(this->frustum);
}

glm::mat4
vx::Camera::view_matrix() const
{
    return glm::lookAt(frustum.position, frustum.position + frustum.front, frustum.up);
}

void
vx::Camera::move(vx::Camera::Direction dir, f64 delta)
{
    f32 offset = move_speed * delta;
    switch (dir)
    {
    case vx::Camera::FORWARDS:
        frustum.position += frustum.front * offset;
        break;

    case vx::Camera::BACKWARDS:
        frustum.position -= frustum.front * offset;
        break;

    case vx::Camera::LEFT:
        frustum.position -= frustum.right * offset;
        break;

    case vx::Camera::RIGHT:
        frustum.position += frustum.right * offset;
        break;

    default:
        ASSERT(false);
    }

    update_frustum_points(frustum);
}
