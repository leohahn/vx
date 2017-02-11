#include "vx_depth_buffer_rasterizer.hpp"
#include <stdio.h>
#include "um_image.hpp"

bool
is_top_or_left_edge(const vx::Point3& a, const vx::Point3& b)
{
    // This assumes the triangle is defined in counter clockwise order.
    //       left edge               top edge
    return (a.y > b.y) || ((a.y == b.y) && (a.x > b.x));
}

i32
orient2d(const vx::Point3& a, const vx::Point3& b, const vx::Point3& c)
{
    return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
}

vx::DepthBufferRasterizer::DepthBufferRasterizer(i16 width, i16 height)
    : _width(width)
    , _height(height)
{
    _buf = new f32[width * height];
}

vx::DepthBufferRasterizer::~DepthBufferRasterizer()
{
    delete[] _buf;
}

void
vx::DepthBufferRasterizer::draw_to_image(const char* filename) const
{
    um::TGAImageGray image(_width, _height);

    for (u16 j = 0; j < _height; j++)
        for (u16 i = 0; i < _width; i++)
        {
            u32 index = (j * _width) + i;
            u8 depth = MAX(MIN(255, _buf[index]), 0);
            image.set(i, j, depth);
        }

    image.write_to_file(filename);
}

void
vx::DepthBufferRasterizer::draw_occluders(Quad3* occluders[vx::FACE_COUNT])
{
    // using vec3 = glm::vec3;
    // using vec4 = glm::vec4;
    // using mat4 = glm::mat4;
    // @Speed, this is probably not optimal, compute this only one time per frame.

    for (u32 i = 0; i < vx::FACE_COUNT; i++)
    {
        if (occluders[i] == nullptr) continue;

        // Apply perspective divide
        // screen_space.x = camera_space.x / camera_space.z
        // screen_space.y = camera_space.y / camera_space.z

        // vec3 transformed_p1 = _view * vec4(occluders[i]->p1, 1.0f);
        // vec3 transformed_p2 = _view * vec4(occluders[i]->p2, 1.0f);
        // vec3 transformed_p3 = _view * vec4(occluders[i]->p3, 1.0f);
        // vec3 transformed_p4 = _view * vec4(occluders[i]->p4, 1.0f);

        if (i == vx::FACE_FRONT)
        {
            // printf("FACE FRONT\n");
            // printf("world_p1: x = %.2f y = %.2f z = %.2f\n",
            //        occluders[i]->p1.x, occluders[i]->p1.y, occluders[i]->p1.z);
            // printf("transformed_p1: x = %.2f y = %.2f z = %.2f\n",
            //        transformed_p1.x, transformed_p1.y, transformed_p1.z);
            // printf("screen_p1: x = %.2f y = %.2f z = %.2f\n",
            //        transformed_p1.x/transformed_p1.z,
            //        transformed_p1.y/transformed_p1.z,
            //        transformed_p1.z);
        }
    }
}

void
vx::DepthBufferRasterizer::render_pixel(const Point3& p, i32 w0, i32 w1, i32 w2)
{
    u32 index = (p.y * _width) + p.x;
    _buf[index] = p.z;
}

void
vx::DepthBufferRasterizer::draw_triangle(const Point3& v0, const Point3& v1, const Point3& v2)
{
    // NOTE(Leo): A triangle follows the convention that is defined in counter clockwise fashion.
    // This is important by defining the edges of a triangle.
    // For example, a left edge is always an edge that is going down. I.e, the start point is
    // always above the end point. (in the y axis)

    // Compute triangle bounding box
    i32 minx = um::min3(v0.x, v1.x, v2.x);
    i32 miny = um::min3(v0.y, v1.y, v2.y);
    i32 maxx = um::max3(v0.x, v1.x, v2.x);
    i32 maxy = um::max3(v0.y, v1.y, v2.y);

    // Clip against screen bounds
    minx = MAX(minx, 0);
    miny = MAX(miny, 0);
    maxx = MIN(maxx, _width-1);
    maxy = MIN(maxy, _height-1);

    // Triangle setup
    i32 A01 = v0.y - v1.y, B01 = v1.x - v0.x;
    i32 A12 = v1.y - v2.y, B12 = v2.x - v1.x;
    i32 A20 = v2.y - v0.y, B20 = v0.x - v2.x;

    // Barycentric coordinates at minX/minY corner
    Point3 p = { minx, miny, 0 };
    i32 w0_row = orient2d(v1, v2, p);
    i32 w1_row = orient2d(v2, v0, p);
    i32 w2_row = orient2d(v0, v1, p);
    i32 area = w0_row + w1_row + w2_row;

    // printf("AREA IS: %d\n", area);

    // Rasterize
    for (p.y = miny; p.y < maxy; p.y++)
    {
        // Barycentric coordinates at the start of the row
        i32 w0 = w0_row;
        i32 w1 = w1_row;
        i32 w2 = w2_row;
        for (p.x = minx; p.x < maxx; p.x++)
        {
            // If p is on or inside all edges, render pixel
            if ((w0 | w1 | w2) >= 0)
            {
                i32 depth = ((v0.z * w0) + (v1.z * w1) + (v2.z * w2))/area;
                p.z = depth;
                // printf("depth: %d\n", depth);
                render_pixel(p, w0, w1, w2);
            }

            // One step to the right
            w0 += A12;
            w1 += A20;
            w2 += A01;
        }
        // One row step
        w0_row += B12;
        w1_row += B20;
        w2_row += B01;
    }
}

void
vx::DepthBufferRasterizer::set_projection_matrix(const glm::mat4& proj)
{
    _proj = proj;
}

void
vx::DepthBufferRasterizer::set_view_matrix(const glm::mat4& view)
{
    _view = view;
}
