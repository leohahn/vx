#ifndef VX_SHADER_MANAGER_HPP
#define VX_SHADER_MANAGER_HPP

#include "GL/glew.h"

#ifndef SHADERS_PATH
#define SHADERS_PATH "resources/shaders/"
#endif

namespace vx
{

struct StringHashmap;

struct Shader
{
    char*              name;
    GLuint             program;
    StringHashmap*     locations;

    void load_uniform_location(const char* uniform_name);
    GLuint uniform_location(const char* uniform_name) const;
};

struct ShaderManager
{
    StringHashmap* shaders;

    ShaderManager();
    ~ShaderManager();

    Shader* load_program(const char* shader_name);
};

}
#endif // VX_SHADER_MANAGER_HPP
