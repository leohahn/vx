#ifndef VX_FILES_H
#define VX_FILES_H

#include <stdlib.h>
#include <GL/glew.h>
#include "um.hpp"

namespace vx
{

struct FntChar
{
    u32     id;
    u32     x;
    u32     y;
    u32     width;
    u32     height;
    i32     xoffset;
    i32     yoffset;
    u32     xadvance;
};

struct FntFile
{
    FntChar      characters[128];
    size_t       num_characters;
    GLuint       texture_id;
    u64          image_width;
    u64          image_height;
    u64          line_height;

    FntFile(const char *fontname);

private:
    void load_texture_from_png(const char* fontname);
};

}
#endif // VX_FILES_H
