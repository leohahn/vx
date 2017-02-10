#include "vx_camera.hpp"
#include "vx_frustum.hpp"

void
update_frustum_orientation(vx::Frustum& frustum, f32 yaw, f32 pitch, Vec3f upWorld)
{
    f32 pitchInRads = um::radians(pitch);
    f32 yawInRads = um::radians(yaw);

    Vec3f front;
    front.x = cosf(pitchInRads) * cosf(yawInRads);
    front.y = sinf(pitchInRads);
    front.z = cosf(pitchInRads) * sinf(yawInRads);
    frustum.front = um::normalize(front);
    frustum.right = um::normalize(um::cross(frustum.front, upWorld));
    frustum.up = um::normalize(um::cross(frustum.right, frustum.front));
}

void
update_frustum_points(vx::Frustum& frustum)
{
    f32 fovy_rads = um::radians(frustum.fovy);
    frustum.znearCenter = frustum.position + frustum.front * frustum.znear;
    frustum.zfarCenter = frustum.position + frustum.front * frustum.zfar;
    frustum.zfarHeight = fabs(2 * tanf(fovy_rads / 2) * frustum.zfar);
    frustum.zfarWidth = frustum.zfarHeight * frustum.ratio;
    frustum.znearHeight = fabs(2 * tanf(fovy_rads / 2) * frustum.znear);
    frustum.znearWidth = frustum.znearHeight * frustum.ratio;
}

vx::Camera::Camera(Vec3f position, f32 fovy, f32 yaw, f32 pitch, Vec3f upWorld, f32 ratio,
                   f32 znear, f32 zfar, f32 moveSpeed, f32 turnSpeed)
{
    this->yaw = yaw;
    this->pitch = pitch;
    this->upWorld = upWorld;
    this->moveSpeed = moveSpeed;
    this->turnSpeed = turnSpeed;

    vx::Frustum frustum;
    frustum.position = position;
    frustum.ratio = ratio;
    frustum.fovy = fovy;
    frustum.znear = znear;
    frustum.zfar = zfar;
    frustum.projection = um::perspective(fovy, ratio, znear, zfar);

    update_frustum_orientation(frustum, yaw, pitch, upWorld);
    update_frustum_points(frustum);

    this->frustum = frustum;
}

void
vx::Camera::rotate_yaw(f32 yawDegrees)
{
    this->yaw += yawDegrees;
    update_frustum_orientation(this->frustum, this->yaw, this->pitch, this->upWorld);
    update_frustum_points(this->frustum);
}

void
vx::Camera::rotate_pitch(f32 pitchDegrees)
{
    f32 newPitch = pitchDegrees + this->pitch;

    if ((i32)newPitch > 89 || (i32)newPitch < -89)
        return;

    this->pitch = newPitch;

    update_frustum_orientation(this->frustum, this->yaw, this->pitch, this->upWorld);
    update_frustum_points(this->frustum);
}

Mat4x4f
vx::Camera::view_matrix() const
{
    return um::look_at(this->frustum.position, this->frustum.position + this->frustum.front, this->upWorld);
}

void
vx::Camera::move(vx::Camera::Direction dir, f64 delta)
{
    switch (dir)
    {
    case vx::Camera::FORWARDS:
        this->frustum.position = this->frustum.position + this->frustum.front * (this->moveSpeed * delta);
        break;

    case vx::Camera::BACKWARDS:
        this->frustum.position = this->frustum.position + (-this->frustum.front * (this->moveSpeed * delta));
        break;

    case vx::Camera::LEFT:
        this->frustum.position = this->frustum.position + (-this->frustum.right * (this->moveSpeed * delta));
        break;

    case vx::Camera::RIGHT:
        this->frustum.position = this->frustum.position + this->frustum.right * (this->moveSpeed * delta);
        break;

    default:
        ASSERT(false);
    }

    update_frustum_points(this->frustum);
}
