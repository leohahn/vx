#include "vx.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "vx_camera.hpp"
#include "vx_shader_manager.hpp"
#include "vx_depth_buffer.hpp"
#include "vx_chunk_manager.hpp"
#include "vx_log_manager.hpp"
#include "um_math.hpp"
#include "um_image.hpp"
#include "vx_memory.hpp"
#include "vx_ui_manager.hpp"
#include "vx_debug_counters.hpp"
#include "vx_display.hpp"

void
vx::main_update(vx::Memory& mem, vx::Camera& camera, bool* keyboard, f64 delta)
{
    ASSERT(delta > 0);
    if (keyboard[GLFW_KEY_ESCAPE] == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(mem.display->window, true);
    }
    if (keyboard[GLFW_KEY_W] == GLFW_PRESS)
    {
        camera.move(vx::Camera::FORWARDS, delta);
    }
    if (keyboard[GLFW_KEY_S] == GLFW_PRESS)
    {
        camera.move(vx::Camera::BACKWARDS, delta);
    }
    if (keyboard[GLFW_KEY_A] == GLFW_PRESS)
    {
        camera.move(vx::Camera::LEFT, delta);
    }
    if (keyboard[GLFW_KEY_D] == GLFW_PRESS)
    {
        camera.move(vx::Camera::RIGHT, delta);
    }
    if (keyboard[GLFW_KEY_UP] == GLFW_PRESS)
    {
        camera.rotate_pitch(camera.turnSpeed * delta);
    }
    if (keyboard[GLFW_KEY_DOWN] == GLFW_PRESS)
    {
        camera.rotate_pitch(-camera.turnSpeed * delta);
    }
    if (keyboard[GLFW_KEY_LEFT] == GLFW_PRESS)
    {
        camera.rotate_yaw(-camera.turnSpeed * delta);
    }
    if (keyboard[GLFW_KEY_RIGHT] == GLFW_PRESS)
    {
        camera.rotate_yaw(camera.turnSpeed * delta);
    }
    if (keyboard[GLFW_KEY_P] == GLFW_PRESS)
    {
        mem.depth_buf->draw_triangle(Vec3f(0.0f, 0.0f, -1.0f),
                                      Vec3f(0.5f, 0.5f, -1.0f),
                                      Vec3f(-0.5f, -0.5f, 0.0f));
        mem.depth_buf->draw_to_file("depth_buffer.tga");
    }
}

void
vx::main_render(vx::Memory& mem, vx::Camera& camera, bool* keyboard)
{
    if (keyboard[GLFW_KEY_T] == GLFW_PRESS)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        vx::Shader* wireframe_shader = mem.shader_manager->load_program("global_wireframe");
        mem.chunk_manager->render_chunks_wireframe(camera, wireframe_shader);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    else if (keyboard[GLFW_KEY_R] == GLFW_PRESS)
    {
        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        mem.chunk_manager->render_occluders(camera, mem, keyboard);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_CULL_FACE);
    }
    else if (keyboard[GLFW_KEY_Q] == GLFW_PRESS)
    {
        glDisable(GL_CULL_FACE);
        mem.chunk_manager->render_occluders(camera, mem, keyboard);
        glEnable(GL_CULL_FACE);
    }
    else
    {
        mem.chunk_manager->render_chunks(camera, mem, keyboard);
    }

    // Render to the screen
    // TODO: Optimize this
    // TODO: Abstract the ui into something more pleasant
    /* BEGIN_TIMED_BLOCK(DebugCycleCount_RenderText); */
    mem.ui_manager->render_text(mem.log_manager->camera, Vec2f(10.0f, 60.0f), 0.5f);
    mem.ui_manager->render_text(mem.log_manager->fps, Vec2f(10.0f, 20.0f), 0.5f);
    /* END_TIMED_BLOCK(DebugCycleCount_RenderText); */
}
