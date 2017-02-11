#ifndef VX_MATERIAL_HPP
#define VX_MATERIAL_HPP

#include "um.hpp"
#include "vx_math.hpp"
#include "glm/vec3.hpp"

namespace vx
{

struct Material
{
    glm::vec3 ambientColor;
    glm::vec3 diffuseColor;
    glm::vec3 specularColor;
    f32   shininess;
};

}

#endif // VX_MATERIAL_HPP
