#ifndef VX_UI_MANAGER_HPP
#define VX_UI_MANAGER_HPP

#include <GL/glew.h>
#include "glm/vec2.hpp"
#include "vx_files.hpp"

namespace vx
{

struct Shader;
struct FntFile;
struct Display;

struct UIManager
{
    Shader*   shader;
    GLuint    vao;
    GLuint    fntfile_vbo;
    FntFile*  fntfile;

    UIManager(const char *fontname, Shader* shader, Display* display);
    ~UIManager();

    void render_text(const char* str, glm::vec2 startPosition, f32 scale);
};

}

#endif // VX_UI_MANAGER_HPP
