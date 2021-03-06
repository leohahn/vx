#include "vx_depth_buffer_rasterizer.hpp"
#include <algorithm>
#include <math.h>
#include <stdio.h>
#include "glm/glm.hpp"
#include "um_image.hpp"
#include "vx_frustum.hpp"
#include "vx_camera.hpp"
#include <cfloat>

bool
is_top_or_left_edge(const vx::Point3& a, const vx::Point3& b)
{
    // This assumes the triangle is defined in counter clockwise order.
    //       left edge               top edge
    return (a.y > b.y) || ((a.y == b.y) && (a.x > b.x));
}

i32
orient2d(const vx::Point3* a, const vx::Point3* b, const vx::Point3* c)
{
    return (b->x - a->x)*(c->y - a->y) - (b->y - a->y)*(c->x - a->x);
}

vx::Point3
vx::Point3::from_vec(const glm::vec4& v)
{
    return Point3(floor(v.x), floor(v.y), floor(v.z));
}

vx::Point3
vx::Point3::from_vec(const glm::vec3& v)
{
    return Point3(floor(v.x), floor(v.y), floor(v.z));
}

vx::DepthBufferRasterizer::DepthBufferRasterizer(i16 width, i16 height)
    : _width(width)
    , _height(height)
{
    _buf = new f32[width * height];
    clear_buffer();
}

vx::DepthBufferRasterizer::~DepthBufferRasterizer()
{
    delete[] _buf;
}

void
vx::DepthBufferRasterizer::clear_buffer()
{
    u32 num_pixels = _width * _height;
    for (u32 i = 0; i < num_pixels; i++)
        _buf[i] = vx::Camera::ZFAR;
}

void
vx::DepthBufferRasterizer::draw_to_image(const char* filename) const
{
    um::TGAImageGray image(_width, _height);

    constexpr f32 ZNEAR = vx::Camera::ZNEAR;
    constexpr f32 ZFAR = vx::Camera::ZFAR;

    for (u16 j = 0; j < _height; j++)
        for (u16 i = 0; i < _width; i++)
        {
            u32 index = (j * _width) + i;

            ASSERT(_buf[index] <= ZFAR);

            // if (_buf[index] < ZFAR)
            //     printf("Printing depth %.2f\n", _buf[index]);

            f32 normalized_depth = ((ZFAR - _buf[index]) - ZNEAR) / (ZFAR - ZNEAR);

            if (normalized_depth < 1 && normalized_depth > 0)
            {
                printf("Printing normalized depth: %.2f\n", normalized_depth);
            }

            u8 depth = (u8)(normalized_depth * 255.0f);

            ASSERT(depth <= 255);

            image.set(i, j, depth);
        }

    image.write_to_file(filename);

    DEBUG("Depth buffer successfuly printed on %s\n", filename);
}

void
vx::DepthBufferRasterizer::draw_occluders(const Frustum& frustum, Quad3* occluders[vx::FACE_COUNT])
{
    using vec2 = glm::vec2;
    using vec3 = glm::vec3;
    using vec4 = glm::vec4;
    using mat4 = glm::mat4;

    for (u32 i = 0; i < vx::FACE_COUNT; i++)
    {
        if (occluders[i] == nullptr) continue;

        // Apply perspective divide
        // screen_space.x = camera_space.x / camera_space.z
        // screen_space.y = camera_space.y / camera_space.z

        vec4 camera_p1 = _view * vec4(occluders[i]->p1, 1.0f);
        vec4 camera_p2 = _view * vec4(occluders[i]->p2, 1.0f);
        vec4 camera_p3 = _view * vec4(occluders[i]->p3, 1.0f);
        vec4 camera_p4 = _view * vec4(occluders[i]->p4, 1.0f);

        // TODO(Leo): I have to clip the coordinates
        vec4 clip_p1 = _proj * camera_p1;
        vec4 clip_p2 = _proj * camera_p2;
        vec4 clip_p3 = _proj * camera_p3;
        vec4 clip_p4 = _proj * camera_p4;

        ASSERT(clip_p1.w != 0 && clip_p2.w != 0 && clip_p3.w != 0 && clip_p4.w != 0);

        // Transform the coordinates in x y from [-1, 1] to [0, 1]
        vec2 norm_xy_raster_p1 = ((vec2(clip_p1) / clip_p1.w) + vec2(1.0f)) / 2.0f;
        vec2 norm_xy_raster_p2 = ((vec2(clip_p2) / clip_p2.w) + vec2(1.0f)) / 2.0f;
        vec2 norm_xy_raster_p3 = ((vec2(clip_p3) / clip_p3.w) + vec2(1.0f)) / 2.0f;
        vec2 norm_xy_raster_p4 = ((vec2(clip_p4) / clip_p4.w) + vec2(1.0f)) / 2.0f;

        Point3 p1;
        p1.x = floor(_width * norm_xy_raster_p1.x);
        p1.y = floor(_height * norm_xy_raster_p1.y);
        p1.z = -camera_p1.z;
        Point3 p2;
        p2.x = floor(_width * norm_xy_raster_p2.x);
        p2.y = floor(_height * norm_xy_raster_p2.y);
        p2.z = -camera_p2.z;
        Point3 p3;
        p3.x = floor(_width * norm_xy_raster_p3.x);
        p3.y = floor(_height * norm_xy_raster_p3.y);
        p3.z = -camera_p3.z;
        Point3 p4;
        p4.x = floor(_width * norm_xy_raster_p4.x);
        p4.y = floor(_height * norm_xy_raster_p4.y);
        p4.z = -camera_p4.z;

        draw_triangle(p1, p3, p2);
        draw_triangle(p2, p4, p1);
    }
}

void
vx::DepthBufferRasterizer::render_pixel(const Point3& p, i32 w0, i32 w1, i32 w2)
{
    u32 index = (p.y * _width) + p.x;
    if (_buf[index] < p.z) return;
    _buf[index] = p.z;
}

void
vx::DepthBufferRasterizer::draw_triangle(Point3 unsorted_v0, Point3 unsorted_v1, Point3 unsorted_v2)
{
    // NOTE(Leo): A triangle follows the convention that is defined in counter clockwise fashion.
    // This is important by defining the edges of a triangle.
    // For example, a left edge is always an edge that is going down. I.e, the start point is
    // always above the end point. (in the y axis)

    Point3* v0 = &unsorted_v0;
    Point3* v1 = &unsorted_v1;
    Point3* v2 = &unsorted_v2;

    // We sort the points making v0 always be the smaller point on the x coordinate, and v1 the largest.
    // If the triangle is on clockwise order, swap first 2 vertices.
    if (orient2d(v0, v1, v2) < 0)
    {
        std::swap(v0, v1);
    }

    ASSERT(orient2d(v0, v1, v2) >= 0);

    // printf("Drawing TRIANGLE for points:\n");
    // printf("v0: %d, %d, %.2f\n", v0->x, v0->y, v0->z);
    // printf("v1: %d, %d, %.2f\n", v1->x, v1->y, v1->z);
    // printf("v2: %d, %d, %.2f\n", v2->x, v2->y, v2->z);

    i32 minx = um::min3(v0->x, v1->x, v2->x);
    i32 miny = um::min3(v0->y, v1->y, v2->y);
    i32 maxx = um::max3(v0->x, v1->x, v2->x);
    i32 maxy = um::max3(v0->y, v1->y, v2->y);

    // Clip against screen bounds
    minx = MAX(minx, 0);
    miny = MAX(miny, 0);
    maxx = MIN(maxx, _width-1);
    maxy = MIN(maxy, _height-1);

    // printf("minx, miny = %d, %d\n", minx, miny);
    // printf("maxx, maxy = %d, %d\n", maxx, maxy);

    // Triangle setup
    i32 A01 = v0->y - v1->y, B01 = v1->x - v0->x;
    i32 A12 = v1->y - v2->y, B12 = v2->x - v1->x;
    i32 A20 = v2->y - v0->y, B20 = v0->x - v2->x;

    // Barycentric coordinates at minX/minY corner
    Point3 p = { minx, miny, 0 };
    i32 w0_row = orient2d(v1, v2, &p);
    i32 w1_row = orient2d(v2, v0, &p);
    i32 w2_row = orient2d(v0, v1, &p);
    i32 area = w0_row + w1_row + w2_row;

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
                i32 depth = ((v0->z * w0) + (v1->z * w1) + (v2->z * w2))/area;
                p.z = depth;
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
