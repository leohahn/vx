#ifndef VX_FRUSTUM_HPP
#define VX_FRUSTUM_HPP

#include <stdbool.h>
#include "um.hpp"
#include "um_math.hpp"

namespace vx
{

struct Chunk;

struct Frustum
{
    Vec3f   position;
    f32     ratio;
    f32     fovy;

    f32     znear;
    f32     znearWidth;
    f32     znearHeight;
    Vec3f   znearCenter;

    f32     zfar;
    f32     zfarWidth;
    f32     zfarHeight;
    Vec3f   zfarCenter;


    Vec3f   front;
    Vec3f   right;
    Vec3f   up;
    Vec3f   normals[6];

    Mat4x4f projection;

    bool chunk_inside(const Chunk& chunk) const;
};

}

#endif // VX_FRUSTUM_HPP
