#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <x86intrin.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "um.hpp"
#include "um_image.hpp"
#include "vx_camera.hpp"
#include "vx_memory.hpp"
#include "vx_shader_manager.hpp"
#include "vx_display.hpp"
#include "vx_ui_manager.hpp"
#include "vx_log_manager.hpp"
#include "vx_chunk_manager.hpp"
#include "vx_debug_counters.hpp"
#include "vx_depth_buffer_rasterizer.hpp"
#include "vx.hpp"
#include "glm/gtc/type_ptr.hpp"

void key_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mode);
GLFWwindow* create_glfw_window_and_context(const char* title, u32 width, u32 height);
void initialize_glew();
void setup_projection_matrix(GLuint program, const glm::mat4& projection);

u64 g_debugCounters[DebugCycleCount_Count] = {0};

i32
main(void)
{
    /*
     * TODO(Leo): Improve illumination:
     *   to consider: SSAO, different diffusion methods
     *   remove sun global illumination.
     * - Improve voxel rendering
     *   - FIXME: Using frustum culling
     *      - Probably implement some for of space division like octrees.
     *        http://thomasdiewald.com/blog/?p=1488
     */
    glfwInit();

    // Initialize the display where the engine will run.
    constexpr u16 SCREEN_WIDTH = 1684;
    constexpr u16 SCREEN_HEIGHT = 1050;

    vx::Display display(SCREEN_WIDTH, SCREEN_HEIGHT,
                        create_glfw_window_and_context("Voxl", SCREEN_WIDTH, SCREEN_HEIGHT));

    bool keyboard[1024] = { GLFW_RELEASE };

    glfwSetKeyCallback(display.window, key_callback);
    glfwSetWindowUserPointer(display.window, keyboard);

    initialize_glew();

    // ====================================================================
    // Allocate and Initialize Application
    // ====================================================================

    // -------------------------
    // Camera
    // -------------------------
    constexpr f32 SCREEN_RATIO = static_cast<f32>(SCREEN_WIDTH) / SCREEN_HEIGHT;
    f32 FOVY = 75.0f;
    f32 YAW = -90.0f;
    f32 PITCH = 0.0f;
    glm::vec3 CAMERA_POSITION(15.0f, 25.0f, 70.0f);
    glm::vec3 WORLD_UP(0.0f, 1.0f, 0.0f);
    f32 CAMERA_MOVE_SPEED = 0.2f;
    f32 CAMERA_TURN_SPEED = 1.0f;

    vx::Camera camera(CAMERA_POSITION, FOVY, YAW, PITCH, WORLD_UP,
                      SCREEN_RATIO, CAMERA_MOVE_SPEED, CAMERA_TURN_SPEED);

    // -------------------------
    // Shaders
    // -------------------------
    auto* shader_manager = new vx::ShaderManager();

    vx::Shader* global_shader = shader_manager->load_program("global");
    global_shader->load_uniform_location("material.ambientColor");
    global_shader->load_uniform_location("material.diffuseColor");
    global_shader->load_uniform_location("material.specularColor");
    global_shader->load_uniform_location("material.shininess");
    global_shader->load_uniform_location("cameraPosition");
    global_shader->load_uniform_location("light.position");
    global_shader->load_uniform_location("light.color");
    global_shader->load_uniform_location("model");
    global_shader->load_uniform_location("view");
    global_shader->load_uniform_location("projection");
    setup_projection_matrix(global_shader->program, camera.frustum.projection);

    vx::Shader* global_wireframe_shader = shader_manager->load_program("global_wireframe");
    global_wireframe_shader->load_uniform_location("material.ambientColor");
    global_wireframe_shader->load_uniform_location("material.diffuseColor");
    global_wireframe_shader->load_uniform_location("material.specularColor");
    global_wireframe_shader->load_uniform_location("material.shininess");
    global_wireframe_shader->load_uniform_location("model");
    global_wireframe_shader->load_uniform_location("view");
    global_wireframe_shader->load_uniform_location("projection");
    setup_projection_matrix(global_wireframe_shader->program, camera.frustum.projection);

    vx::Shader* font_shader = shader_manager->load_program("font_render");
    font_shader->load_uniform_location("textColor");
    font_shader->load_uniform_location("model");
    font_shader->load_uniform_location("projection");

    vx::Material material;
    material.ambientColor = glm::vec3(0.5f, 0.4f, 0.3f);
    material.diffuseColor = glm::vec3(0.5f, 0.4f, 0.3f);
    material.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
    material.shininess = 62.0f;
    // -------------------------
    // User Interface (TODO: Improve this)
    // -------------------------
    auto* ui_manager = new vx::UIManager("Monoid", font_shader, &display);
    // -------------------------
    // Log
    // -------------------------
    auto* log_manager = new vx::LogManager();
    // -------------------------
    // Chunks
    // -------------------------
    auto* chunk_manager = new vx::ChunkManager();
    // for (i32 x = 0; x < vx::WORLD_SIZE; x++)
    //     for (i32 y = 0; y < vx::WORLD_SIZE-5; y++)
    //         for (i32 z = 0; z < vx::WORLD_SIZE; z++)
    //         {
    //             chunk_manager->create_chunk(x, y, z, global_shader, material);
    //         }
    chunk_manager->create_chunk(0, 0, 0, global_shader, material);
    /* vx_chunk_manager_CreateChunk(chunkManager, 1, 0, 0, globalShader, material); */
    /* vx_chunk_manager_CreateChunk(chunkManager, 2, 0, 0, globalShader, material); */
    /* vx_chunk_manager_CreateChunk(chunkManager, 0, 1, 0, globalShader, material); */
    /* vx_chunk_manager_CreateChunk(chunkManager, 1, 1, 0, globalShader, material); */

    // ====================================================================
    // Create memory and enter application's main loop
    // ====================================================================
    vx::Memory memory;
    memory.display = &display;
    memory.shader_manager = shader_manager;
    memory.ui_manager = ui_manager;
    memory.log_manager = log_manager;
    memory.chunk_manager = chunk_manager;
    memory.depth_buf = new vx::DepthBufferRasterizer(display.width, display.height);
    memory.depth_buf->set_projection_matrix(camera.frustum.projection);

    // Define variables to control time
    constexpr f64 DESIRED_FPS = 60.0;
    constexpr f64 DESIRED_FRAMETIME = 1.0 / DESIRED_FPS;
    constexpr u32 MAX_STEPS = 6;
    constexpr f64 MAX_DELTA_TIME = 1.0;

    f64 new_time, total_delta, delta, frame_time;
    f64 previous_time = glfwGetTime();
    f32 fps;
    u32 loops;

    while (!glfwWindowShouldClose(display.window))
    {
        glClearColor(0.56f, 0.71f, 0.94f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        new_time = glfwGetTime();
        frame_time = new_time - previous_time;
        previous_time = new_time;

        // LOG FPS
        fps = 1 / frame_time; // used for logging
        // Update Logging on screen
        log_manager->log_fps(fps);
        log_manager->log_camera(camera.frustum.position, camera.frustum.front);

        total_delta = frame_time / DESIRED_FRAMETIME;

        loops = 0;

        /* BEGIN_TIMED_BLOCK(DebugCycleCount_MainUpdate); */
        while (total_delta > 0.0 && loops < MAX_STEPS)
        {
            delta = MIN(total_delta, MAX_DELTA_TIME);
            // ===========================================================
            // Main update function is called here
            // ===========================================================
            vx::main_update(memory, camera, keyboard, delta);
            total_delta -= delta;
            loops++;
        }
        /* END_TIMED_BLOCK(DebugCycleCount_MainUpdate); */

        // TODO: Use interpolation for smoother experience.
        // ===========================================================
        // Main render function is called here
        // ===========================================================
        // BEGIN_TIMED_BLOCK(DebugCycleCount_MainRender);
        vx::main_render(memory, camera, keyboard);
        // END_TIMED_BLOCK(DebugCycleCount_MainRender);

        glfwPollEvents();
        glfwSwapBuffers(display.window);
        /* vx_debug_counters_Dump(); */
    }
    // ====================================================================
    // Deallocate Memory and terminate program
    // ====================================================================
    glfwDestroyWindow(display.window);
    glfwTerminate();
}

void
key_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mode)
{
    UNUSED(scancode);
    UNUSED(mode);
    bool *keyboard = (bool*)glfwGetWindowUserPointer(window);
    keyboard[key] = action;
}

GLFWwindow *
create_glfw_window_and_context(const char* title, u32 width, u32 height)
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    glfwMakeContextCurrent(window);
    glViewport(0, 0, width, height);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    // Mainly used for text rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return window;
}

void
initialize_glew()
{
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Error loading Glew\n");
        abort();
    }
}

void
setup_projection_matrix(GLuint program, const glm::mat4& projection)
{
    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUseProgram(0);
}
