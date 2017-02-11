#ifndef VX_FRUSTUM_HPP
#define VX_FRUSTUM_HPP

#include <stdbool.h>
#include "um.hpp"
#include "glm/vec3.hpp"
#include "glm/matrix.hpp"

namespace vx
{

struct Chunk;

struct Frustum
{
    glm::vec3 position;
    f32       ratio;
    f32       fovy;

    f32       znear;
    f32       znear_width;
    f32       znear_height;
    glm::vec3 znear_center;

    f32       zfar;
    f32       zfar_width;
    f32       zfar_height;
    glm::vec3 zfar_center;

    // Calculated attributes
    glm::vec3 front;
    glm::vec3 right;
    glm::vec3 up;
    glm::vec3 normals[6];

    glm::mat4 projection;

    bool chunk_inside(const Chunk& chunk) const;
};

}

#endif // VX_FRUSTUM_HPP
