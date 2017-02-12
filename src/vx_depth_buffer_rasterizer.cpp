#include "vx_depth_buffer_rasterizer.hpp"
#include <math.h>
#include <stdio.h>
#include "glm/glm.hpp"
#include "um_image.hpp"
#include "vx_frustum.hpp"
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

void
sort_in_counter_clockwise_order(vx::Point3* v0, vx::Point3* v1, vx::Point3* v2)
{
    // @TODO(Leo): Test to see if this really works as I plan.

    if (orient2d(v0, v1, v2) > 0)
    {
        // Points are already in counter clockwise order.
        return;
    }
    // If the orient2d is negative, it means that they are on clockwise order.
    // Swapping v0 and v1 makes the triangles be on counter clockwise order.
    vx::Point3* temp = v0;
    v0 = v1;
    v1 = temp;
    return;
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
        _buf[i] = FLT_MAX;
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
vx::DepthBufferRasterizer::draw_occluders(const Frustum& frustum, Quad3* occluders[vx::FACE_COUNT])
{
    using vec3 = glm::vec3;
    using vec4 = glm::vec4;
    using mat4 = glm::mat4;

    for (u32 i = 0; i < vx::FACE_COUNT; i++)
    {
        if (occluders[i] == nullptr) continue;

        // Apply perspective divide
        // screen_space.x = camera_space.x / camera_space.z
        // screen_space.y = camera_space.y / camera_space.z

        mat4 proj_view = _proj * _view;

        vec4 raster_p1 = proj_view * vec4(occluders[i]->p1, 1.0f);
        vec4 raster_p2 = proj_view * vec4(occluders[i]->p2, 1.0f);
        vec4 raster_p3 = proj_view * vec4(occluders[i]->p3, 1.0f);
        vec4 raster_p4 = proj_view * vec4(occluders[i]->p4, 1.0f);

        Point3 p1;
        p1.x = floor(_width * raster_p1.x / raster_p1.w);
        p1.y = floor(_height * raster_p1.y / raster_p1.w);
        p1.z = raster_p1.z;
        Point3 p2;
        p2.x = floor(_width * raster_p2.x / raster_p2.w);
        p2.y = floor(_height * raster_p2.y / raster_p2.w);
        p2.z = raster_p2.z;
        Point3 p3;
        p3.x = floor(_width * raster_p3.x / raster_p3.w);
        p3.y = floor(_height * raster_p3.y / raster_p3.w);
        p3.z = raster_p3.z;
        Point3 p4;
        p4.x = floor(_width * raster_p4.x / raster_p4.w);
        p4.y = floor(_height * raster_p4.y / raster_p4.w);
        p4.z = raster_p4.z;

        draw_triangle(p1, p2, p3);
        draw_triangle(p3, p4, p1);
    }
}

void
vx::DepthBufferRasterizer::render_pixel(const Point3& p, i32 w0, i32 w1, i32 w2)
{
    u32 index = (p.y * _width) + p.x;
    _buf[index] = p.z;
}

void
vx::DepthBufferRasterizer::draw_triangle(Point3 unsorted_v0, Point3 unsorted_v1, Point3 unsorted_v2)
{
    // NOTE(Leo): A triangle follows the convention that is defined in counter clockwise fashion.
    // This is important by defining the edges of a triangle.
    // For example, a left edge is always an edge that is going down. I.e, the start point is
    // always above the end point. (in the y axis)

    // We sort the points making v0 always be the smaller point on the x coordinate, and v1 the largest.
    Point3* v0 = &unsorted_v0;
    Point3* v1 = &unsorted_v1;
    Point3* v2 = &unsorted_v2;
    // @Speed: This can be improved by pre-sorting these vertices in a smarter way, and not at
    // the last time.
    sort_in_counter_clockwise_order(v0, v1, v2);


    // Compute triangle bounding box
    i32 minx = um::min3(v0->x, v1->x, v2->x);
    i32 miny = um::min3(v0->y, v1->y, v2->y);
    i32 maxx = um::max3(v0->x, v1->x, v2->x);
    i32 maxy = um::max3(v0->y, v1->y, v2->y);

    // Clip against screen bounds
    minx = MAX(minx, 0);
    miny = MAX(miny, 0);
    maxx = MIN(maxx, _width-1);
    maxy = MIN(maxy, _height-1);

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
                i32 depth = ((v0->z * w0) + (v1->z * w1) + (v2->z * w2))/area;
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
