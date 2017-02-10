#ifndef VX_LOG_MANAGER_HPP
#define VX_LOG_MANAGER_HPP

#include <stdlib.h>
#include "um.hpp"

struct Vec3f;

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
    void log_camera(Vec3f position, Vec3f front);
};

}

#endif // VX_LOG_MANAGER_HPP
