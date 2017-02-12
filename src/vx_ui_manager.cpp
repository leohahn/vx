#include "vx_ui_manager.hpp"
#include <string.h>
#include "vx_files.hpp"
#include "um.hpp"
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "vx_display.hpp"
#include "vx_shader_manager.hpp"

#define MAX_LETTERS   50
#define VERTICES_LEN (MAX_LETTERS * 6)

static glm::vec4 vertices[VERTICES_LEN];

vx::UIManager::UIManager(const char* fontname, vx::Shader* shader, vx::Display* display)
    : shader(shader)
    , fntfile(new FntFile(fontname))
{
    glGenVertexArrays(1, &this->vao);
    glGenBuffers(1, &this->fntfile_vbo);

    glBindVertexArray(this->vao);
    glBindBuffer(GL_ARRAY_BUFFER, this->fntfile_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
    glBindVertexArray(0);

    // Setup projection matrix on the shader
    glm::mat4 projection = glm::ortho(0.0f, (f32)display->width, 0.0f, (f32)display->height, -1.0f, 1.0f);
    glUseProgram(shader->program);
    glUniformMatrix4fv(glGetUniformLocation(shader->program, "projection"),
                       1, GL_FALSE, glm::value_ptr(projection));
    glUseProgram(0);
}

vx::UIManager::~UIManager()
{
    glDeleteBuffers(1, &this->fntfile_vbo);
    glDeleteVertexArrays(1, &this->vao);
    delete this->fntfile;
}

void
vx::UIManager::render_text(const char* str, glm::vec2 start, f32 scale)
{
    const u64 img_width = this->fntfile->image_width;
    const u64 img_height = this->fntfile->image_height;
    glm::vec2 cursor(start);
    u32 v = 0;
    u32 textLen = 0;
    vx::FntChar* chr;
    for (u32 i = 0; i < strlen(str); ++i)
    {
        f32 line_top_y = this->fntfile->line_height + cursor.y;

        if (str[i] == '\n')
        {
            DEBUG("New file found for: %s\n", str);
            cursor.y -= this->fntfile->line_height;
            cursor.x = start.x;
            continue;
        }
        textLen++;
        chr = &this->fntfile->characters[(i32)str[i]];

        glm::vec4 tleft;
        tleft.x = cursor.x + chr->xoffset;
        tleft.y = line_top_y - chr->yoffset;
        tleft.z = (f32)chr->x / img_width;
        tleft.w = (f32)(img_height - chr->y) / img_height;
        glm::vec4 bleft;
        bleft.x = tleft.x;
        bleft.y = tleft.y - chr->height;
        bleft.z = (f32)chr->x / img_width;
        bleft.w = (f32)(img_height - (chr->y + chr->height)) / img_height;
        glm::vec4 tright;
        tright.x = tleft.x + chr->width;
        tright.y = tleft.y;
        tright.z = (f32)(chr->x + chr->width) / img_width;
        tright.w = (f32)(img_height - chr->y) / img_height;
        glm::vec4 bright;
        bright.x = bleft.x + chr->width;
        bright.y = bleft.y;
        bright.z = (f32)(chr->x + chr->width) / img_width;
        bright.w = (f32)(img_height - (chr->y + chr->height)) / img_height;

        vertices[v++]   = tleft;
        vertices[v++] = bleft;
        vertices[v++] = bright;
        vertices[v++] = bright;
        vertices[v++] = tright;
        vertices[v++] = tleft;

        cursor.x += chr->xadvance;
    }
    // NOTE(leo): This is currently not necessary, but I will leave it here since it does
    // not hurt. If it really *does* hurt performance, it can be removed.
    /* glDisable(GL_DEPTH_TEST); */

    glBindVertexArray(this->vao);
    glBindBuffer(GL_ARRAY_BUFFER, this->fntfile_vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * v, vertices, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);

    glBindTexture(GL_TEXTURE_2D, this->fntfile->texture_id);
    glActiveTexture(GL_TEXTURE0);
    glUseProgram(this->shader->program);

    // @Speed: This is probably not so fast.
    glm::mat4 model;
    model = glm::translate(model, glm::vec3(start.x, start.y, 0.0f));
    model = glm::scale(model, glm::vec3(scale, scale, 0.0f));
    model = glm::translate(model, glm::vec3(-start.x, -start.y, 0.0f));

    glUniformMatrix4fv(this->shader->uniform_location("model"), 1, GL_FALSE, glm::value_ptr(model));
    // Mainly used for text rendering

    // Draw all of the triangles for the characters.
    glDrawArrays(GL_TRIANGLES, 0, 6 * textLen);
    glBindVertexArray(0);

    // Reenable depth testing.
    /* glEnable(GL_DEPTH_TEST); */
}
