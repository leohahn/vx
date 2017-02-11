#include "vx_depth_buffer.hpp"
#include <stdlib.h>
#include <float.h>
#include "um.hpp"
#include "um_image.hpp"

struct Span
{
    f32 depth1, depth2;
    i32 x1, x2;
};

struct Edge
{
    f32 depth1, depth2;
    i32 x1, y1, x2, y2;
};

Span
MakeSpan(f32 depth1, u32 x1, f32 depth2, u32 x2)
{
    Span s;
    if (x1 < x2)
    {
        s.depth1 = depth1;
        s.x1 = x1;
        s.depth2 = depth2;
        s.x2 = x2;
    }
    else
    {
        s.depth1 = depth2;
        s.x1 = x2;
        s.depth2 = depth1;
        s.x2 = x1;
    }
    return s;
}

Edge MakeEdge(const u32 depth1, u32 x1, u32 y1, const u32 depth2, u32 x2, u32 y2)
{
    Edge e;
    if (y1 < y2)
    {
        e.depth1 = depth1;
        e.x1 = x1;
        e.y1 = y1;
        e.depth2 = depth2;
        e.x2 = x2;
        e.y2 = y2;
    }
    else
    {
        e.depth1 = depth2;
        e.x1 = x2;
        e.y1 = y2;
        e.depth2 = depth1;
        e.x2 = x1;
        e.y2 = y1;
    }
    return e;
}

void
ClearBuffer(vx::DepthBuffer* buf)
{
    for (i32 i = 0; i < buf->width * buf->height; i++)
    {
        // Set contents to the maximum float value.
        // Basically means that any value will be rendered on top of it (since it is always closer)
        // 1.0 means the farthest from the camera.
        buf->data[i] = 1.0f;
    }
}

void
DrawSpan(vx::DepthBuffer* buf, Span span, i32 y)
{
    i32 xdiff = span.x2 - span.x1;
    if (xdiff == 0)
        return;

    f32 depthdiff = span.depth2 - span.depth1;

    f32 factor = 0.0f;
    f32 factorStep = 1.0f / xdiff;

    for (i32 x = span.x1; x <= span.x2; x++)
    {
        f32 depth = span.depth1 + (depthdiff * factor);

        //
        // Draw depth to buffer
        //
        u32 index = (y * buf->width) + x;
        buf->data[index] = depth;

        factor += factorStep;
    }
}

void
DrawSpanBetweenEdges(vx::DepthBuffer* buf, Edge e1, Edge e2)
{
    i32 e1ydiff = (e1.y2 - e1.y1);
    if (e1ydiff == 0)
        return;

    i32 e2ydiff = (e2.y2 - e2.y1);
    if (e2ydiff == 0)
        return;

    i32 e1xdiff = e1.x2 - e1.x1;
    i32 e2xdiff = e2.x2 - e2.x1;
    f32 e1depthdiff = e1.depth2 - e1.depth1;
    f32 e2depthdiff = e2.depth2 - e2.depth1;

    f32 factor1 = (f32)(e2.y1 - e1.y1) / e1ydiff;
    f32 factorStep1 = 1.0f / e1ydiff;
    f32 factor2 = 0.0f;
    f32 factorStep2 = 1.0f / e2ydiff;

    for (i32 y = e2.y1; y <= e2.y2; y++)
    {
        Span span = MakeSpan(e1.depth1 + (e1depthdiff * factor1),
                             e1.x1 + (i32)(e1xdiff * factor1),
                             e2.depth1 + (e2depthdiff * factor2),
                             e2.x1 + (i32)(e2xdiff * factor2));

        DrawSpan(buf, span, y);

        factor1 += factorStep1;
        factor2 += factorStep2;
    }
}

void
vx::DepthBuffer::draw_triangle(Vec3f p1, Vec3f p2, Vec3f p3)
{
    // NOTE(leo): The three points received by this function assume that they are in normalized
    // devide coordinates (i.e, all axis between -1 and 1).

    // Transform axis x and y from range -1,1 to the current screen range.
    i32 screenX1 = (i32)(((p1.x + 1) / 2) * this->width);
    i32 screenY1 = (i32)(((p1.y + 1) / 2) * this->height);
    f32 depth1 = p1.z;

    i32 screenX2 = (i32)(((p2.x + 1) / 2) * this->width);
    i32 screenY2 = (i32)(((p2.y + 1) / 2) * this->height);
    f32 depth2 = p2.z;

    i32 screenX3 = (i32)(((p3.x + 1) / 2) * this->width);
    i32 screenY3 = (i32)(((p3.y + 1) / 2) * this->height);
    f32 depth3 = p3.z;

    Edge edges[3] =
    {
        MakeEdge(depth1, screenX1, screenY1, depth2, screenX2, screenY2),
        MakeEdge(depth2, screenX2, screenY2, depth3, screenX3, screenY3),
        MakeEdge(depth3, screenX3, screenY3, depth1, screenX1, screenY1),
    };

    i32 maxLength = 0;
    i32 longEdge = 0;

    // Find the edge with the greatest length on the y axis.
    for (i32 i = 0; i < 3; i++)
    {
        i32 length = edges[i].y2 - edges[i].y1;
        ASSERT(length >= 0);
        if (length > maxLength)
        {
            maxLength = length;
            longEdge = i;
        }
    }

    i32 shortEdge1 = (longEdge + 1) % 3;
    i32 shortEdge2 = (longEdge + 2) % 3;

    DrawSpanBetweenEdges(this, edges[longEdge], edges[shortEdge1]);
    DrawSpanBetweenEdges(this, edges[longEdge], edges[shortEdge2]);
}

vx::DepthBuffer::DepthBuffer(u16 width, u16 height)
{
    this->data = new f32[width * height];
    this->width = width;
    this->height = height;

    ClearBuffer(this);
}

vx::DepthBuffer::~DepthBuffer()
{
    delete[] this->data;
}

void
vx::DepthBuffer::draw_to_file(const char* filepath)
{
    // // This function assumes that the values inside the depth buffers are
    // // ranged between 1 and -1.
    // ltb_tga_grayscale* img = ltb_tga_grayscale_New(buf->width, buf->height);

    // for (i32 y = 0; y < buf->height; y++)
    //     for (i32 x = 0; x < buf->width; x++)
    // {
    //     u32 index = (y * buf->width) + x;
    //     u8 depthNormalized = 1 - (buf->data[index] + 1)/2.0f;
    //     u8 depthGrayscale = (u8)(depthNormalized * 255);
    //     /* printf("DRAWING DEPTH FOR %d %d: %d\n", x, y, depth); */
    //     ltb_tga_grayscale_Fill(img, x, y, depthGrayscale);
    // }

    // ltb_tga_grayscale_WriteToFile(img, filepath);
}

void
vx::DepthBuffer::draw_occluders(Quad3* occluders, u16 numOccluders)
{
    // NOTE(leo): Occluders are assumed to already be in normalized device coordinates.
    // I.e. multiply by the transform matrix before sending them here.

    for (i32 i = 0; i < numOccluders; i++)
    {
        // For each occluder we draw two triangles.
        // draw_triangle(occluders[i].p1, occluders[i].p2, occluders[i].p3);
        // draw_triangle(occluders[i].p3, occluders[i].p4, occluders[i].p1);
    }
}
