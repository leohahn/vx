#ifndef VX_LOG_MANAGER_HPP
#define VX_LOG_MANAGER_HPP

#include <stdlib.h>
#include "um.hpp"
#include "glm/glm.hpp"

namespace vx
{

// struct RenderLog
// {
//     u32 numChunks;
//     u32 numBlocks;
//     u32 numVertices;
// };

struct LogManager
{
    char *fps;
    char *camera;

    LogManager();
    ~LogManager();

    void log_fps(f32 fps);
    void log_camera(glm::vec3 position, glm::vec3 front);
};

}

#endif // VX_LOG_MANAGER_HPP
