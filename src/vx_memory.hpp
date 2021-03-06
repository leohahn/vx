#ifndef VX_MEMORY_HPP
#define VX_MEMORY_HPP

namespace vx
{

struct ShaderManager;
struct UIManager;
struct LogManager;
struct ChunkManager;
struct Display;
struct DepthBufferRasterizer;


struct Memory
{
    ShaderManager*          shader_manager;
    UIManager*              ui_manager;
    LogManager*             log_manager;
    ChunkManager*           chunk_manager;
    Display*                display;
    DepthBufferRasterizer*  depth_buf;
};

}
#endif // VX_MEMORY_HPP
