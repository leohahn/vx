#include "vx_log_manager.hpp"
#include <stdio.h>
#include <string.h>

#define FPS_BUFFER_SIZE 20
#define CAMERA_BUFFER_SIZE 200

vx::LogManager::LogManager()
{
    this->fps = new char[FPS_BUFFER_SIZE]();
    this->camera = new char[CAMERA_BUFFER_SIZE]();
}

vx::LogManager::~LogManager()
{
    delete[] this->fps;
    delete[] this->camera;
}

void
vx::LogManager::log_fps(f32 fps)
{
    memset(this->fps, 0, sizeof(char) * FPS_BUFFER_SIZE);
    snprintf(this->fps, FPS_BUFFER_SIZE-1, "FPS: %.2f", fps);
}

void
vx::LogManager::log_camera(glm::vec3 position, glm::vec3 front)
{
    memset(this->camera, 0, sizeof(char) * CAMERA_BUFFER_SIZE);
    /* snprintf(manager->camera, CAMERA_BUFFER_SIZE, */
    /*          "Camera:\n" */
    /*          "position: x = %.2f, y = %.2f, z = %.2f\n" */
    /*          "front: x = %.2f, y = %.2f, z = %.2f", */
    /*          position.x, position.y, position.z, */
    /*          front.x, front.y, front.z); */
    snprintf(this->camera, CAMERA_BUFFER_SIZE-1,
             "Camera: x = %.2f, y = %.2f, z = %.2f (front: %.2f, %.2f, %.2f)",
             position.x, position.y, position.z, front.x, front.y, front.z);
    /* snprintf(manager->camera, CAMERA_BUFFER_SIZE, */
    /*          "Camera: x = %.2f, y = %.2f, z = %.2f iiiiiiii", */
    /*          position.x, position.y, position.z); */
}
