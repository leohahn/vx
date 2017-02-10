#ifndef VX_MATERIAL_HPP
#define VX_MATERIAL_HPP

#include "um.hpp"
#include "um_math.hpp"

namespace vx
{

struct Material
{
    Vec3f ambientColor;
    Vec3f diffuseColor;
    Vec3f specularColor;
    f32   shininess;
};

}

#endif // VX_MATERIAL_HPP
