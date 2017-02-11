#ifndef DEPTH_BUFFER_RASTERIZER_HPP
#define DEPTH_BUFFER_RASTERIZER_HPP

#include "glm/glm.hpp"
#include "um.hpp"
#include "vx_math.hpp"
#include "vx_chunk_manager.hpp"

namespace vx
{

struct Point3
{
    i32 x, y, z;

    Point3(i32 x, i32 y, i32 z): x(x), y(y), z(z) {}
};

struct DepthBufferRasterizer
{
    DepthBufferRasterizer(i16 width, i16 height);
    ~DepthBufferRasterizer();

    void draw_triangle(const Point3& v0, const Point3& v1, const Point3& v2);
    void draw_to_image(const char* filename) const;
    void draw_occluders(Quad3* occluders[vx::FACE_COUNT]);

    void set_projection_matrix(const glm::mat4& proj);
    void set_view_matrix(const glm::mat4& view);

private:
    f32* _buf;
    i16 _width, _height;

    glm::mat4 _proj;
    glm::mat4 _view;

    void render_pixel(const Point3& p, i32 w0, i32 w1, i32 w2);
};


}

#endif // DEPTH_BUFFER_RASTERIZER_HPP
