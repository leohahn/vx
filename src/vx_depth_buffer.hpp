#ifndef VX_DEPTH_BUFFER_HPP
#define VX_DEPTH_BUFFER_HPP

#include "vx_math.hpp"
#include "um.hpp"

namespace vx
{

struct DepthBuffer
{
    f32*    data;
    u16     width;
    u16     height;

    DepthBuffer(u16 width, u16 height);
    ~DepthBuffer();

    void draw_triangle(Vec3f p1, Vec3f p2, Vec3f p3);
    void draw_occluders(Quad3* occluders, u16 numOccluders);

    //@Temporary
    void draw_to_file(const char* filepath);

};

}

#endif // VX_DEPTH_BUFFER_HPP
