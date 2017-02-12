#ifndef DEPTH_BUFFER_RASTERIZER_HPP
#define DEPTH_BUFFER_RASTERIZER_HPP

#include "glm/fwd.hpp"
#include "um.hpp"
#include "vx_math.hpp"
#include "vx_chunk_manager.hpp"

namespace vx
{

struct Point3
{
    i32 x, y;
    f32 z;

    Point3() {}
    Point3(i32 x, i32 y, f32 z): x(x), y(y), z(z) {}

    static Point3 from_vec(const glm::vec4& v);
    static Point3 from_vec(const glm::vec3& v);
};

struct DepthBufferRasterizer
{
    DepthBufferRasterizer(i16 width, i16 height);
    ~DepthBufferRasterizer();

    void draw_triangle(Point3 unsorted_v0, Point3 unsorted_v1, Point3 unsorted_v2);
    void draw_to_image(const char* filename) const;
    void draw_occluders(const Frustum& frustum, Quad3* occluders[vx::FACE_COUNT]);

    void set_projection_matrix(const glm::mat4& proj);
    void set_view_matrix(const glm::mat4& view);
    void clear_buffer();

private:
    f32* _buf;
    u16 _width, _height;

    glm::mat4 _proj;
    glm::mat4 _view;

    void render_pixel(const Point3& p, i32 w0, i32 w1, i32 w2);
};


}

#endif // DEPTH_BUFFER_RASTERIZER_HPP
