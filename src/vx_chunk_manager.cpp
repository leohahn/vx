#include "vx_chunk_manager.hpp"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include "vx_shader_manager.hpp"
#include "vx_camera.hpp"
#include "vx_material.hpp"
#include "open-simplex-noise.h"
#include "vx_debug_counters.hpp"
#include "vx_depth_buffer.hpp"
#include "vx_frustum.hpp"
#include "vx_memory.hpp"
#include "vx_display.hpp"
#include "um.hpp"

void create_chunk_vertex_buffer(vx::Chunk& chunk);
f64 get_noise(f64 x, f64 y, f64 z, f64 startFrequence, u32 octaveCount, f64 persistence, struct osn_context* ctx);
f64 get_noise_2D(f64 x, f64 y, f64 startFrequence, u32 octaveCount, f64 persistence, struct osn_context *ctx);

enum Axis
{
    AXIS_X, AXIS_Y, AXIS_Z,
};

struct SlidingBuffer
{
    i32 data[vx::CHUNK_SIZE][vx::CHUNK_SIZE];
};

bool
find_first_valid_position(um::Pairi& res, const SlidingBuffer& sbuf)
{
    for (i32 i = 0; i < vx::CHUNK_SIZE; i++)
        for (i32 j = 0; j < vx::CHUNK_SIZE; j++)
        {
            if (sbuf.data[i][j] != -1)
            {
                res.a = i;
                res.b = j;
                return true;
            }
            continue;
        }
    return false;
}

Quad3f*
find_largest_quad(const SlidingBuffer& sbuf, vx::Chunk& chunk, vx::Face face)
{
    //@Improvement, @Performance
    // NOTE(leo): This algorithm only tries to expand the first rectangle starting from 0, 0.
    // Consider the cases where the biggest rectangle does not start from this position.
    i32 startI;
    i32 startJ;
    i32 endI;
    i32 endJ;

    // get first index to start iterating from
    um::Pairi startPos;
    if (!find_first_valid_position(startPos, sbuf))
        return nullptr;

    startI = startPos.a;
    startJ = startPos.b;
    endI = startI;
    endJ = startJ;

    // Rectangles of only 1 of width or height are not accepted.
    if (startI+1 >= vx::CHUNK_SIZE || startJ+1 >= vx::CHUNK_SIZE)
        return nullptr;

    // --------------------------------------
    //              Offsets
    // --------------------------------------
    // Now that we have the starting point, we can calculate the directions for i and j.
    i32 offsetDirectionJ;
    i32 offsetDirectionI;
    {
        i32 val = sbuf.data[startI][startJ];
        i32 nextValJ = sbuf.data[startI][startJ+1];
        i32 nextValI = sbuf.data[startI+1][startJ];

        offsetDirectionJ = SIGN(nextValJ - val);
        offsetDirectionI = SIGN(nextValI - val);
    }

    // --------------------------------------
    //        Expand Quad Horizontally
    // --------------------------------------
    // Expand rectangle maximum horizontally
    {
        i32 prevVal = sbuf.data[startI][startJ];
        ASSERT(prevVal != -1);
        i32 offsetJ = sbuf.data[startI][startJ+1] - prevVal;

        while (offsetDirectionJ == SIGN(offsetJ) &&
               abs(offsetJ) <= 1 &&
               sbuf.data[startI][endJ+1] != -1)
        {
            prevVal = sbuf.data[startI][endJ+1];
            endJ++;

            if (endJ+1 >= vx::CHUNK_SIZE)
                break;
            offsetJ = sbuf.data[startI][endJ+1] - prevVal;
        }
    }
    // --------------------------------------
    //        Expand Quad Vertically
    // --------------------------------------
    {
        i32 prevIVal = sbuf.data[startI][startJ];
        i32 offsetI = sbuf.data[startI+1][startJ] - prevIVal;

        while (endI < vx::CHUNK_SIZE &&
               offsetDirectionI == SIGN(offsetI) &&
               abs(offsetI) <= 1 &&
               sbuf.data[startI+1][startJ] != -1)
        {
            for (i32 cj = startJ+1; cj <= endJ; cj++)
            {
                i32 prevJVal = sbuf.data[endI+1][cj-1];
                i32 prevIVal = sbuf.data[endI][cj];
                i32 currVal = sbuf.data[endI+1][cj];

                i32 iOffsetI = currVal - prevIVal;
                i32 iOffsetJ = currVal - prevJVal;

                if (currVal != -1 &&
                    SIGN(iOffsetI) == offsetDirectionI && abs(iOffsetI) <= 1 &&
                    SIGN(iOffsetJ) == offsetDirectionJ && abs(iOffsetJ) <= 1)
                {
                    continue;
                }
                else
                {
                    goto Exit;
                }
            }
            endI++;
        }
    }
Exit:;
    if (startI == endI || startJ == endJ)
        return nullptr;

    // Get world position of each of the indexes.
    Vec3f p1Offset, p2Offset, p3Offset, p4Offset;
    switch (face)
    {
    case vx::FACE_LEFT:
    case vx::FACE_RIGHT:
        // p1
        p1Offset.x = vx::BLOCK_SIZE * sbuf.data[startI][startJ];
        p1Offset.y = vx::BLOCK_SIZE * startI;
        p1Offset.z = vx::BLOCK_SIZE * startJ;
        // p2
        p2Offset.x = vx::BLOCK_SIZE * sbuf.data[endI][endJ];
        p2Offset.y = vx::BLOCK_SIZE * endI;
        p2Offset.z = vx::BLOCK_SIZE * endJ;
        // p3
        p3Offset.x = vx::BLOCK_SIZE * sbuf.data[startI][endJ];
        p3Offset.y = vx::BLOCK_SIZE * startI;
        p3Offset.z = vx::BLOCK_SIZE * endJ;
        // p4
        p4Offset.x = vx::BLOCK_SIZE * sbuf.data[endI][startJ];
        p4Offset.y = vx::BLOCK_SIZE * endI;
        p4Offset.z = vx::BLOCK_SIZE * startJ;
        break;
    case vx::FACE_UP:
    case vx::FACE_DOWN:
        // p1
        p1Offset.x = vx::BLOCK_SIZE * startJ;
        p1Offset.y = vx::BLOCK_SIZE * sbuf.data[startI][startJ];
        p1Offset.z = vx::BLOCK_SIZE * startI;
        // p2
        p2Offset.x = vx::BLOCK_SIZE * endJ;
        p2Offset.y = vx::BLOCK_SIZE * sbuf.data[endI][endJ];
        p2Offset.z = vx::BLOCK_SIZE * endI;
        // p3
        p3Offset.x = vx::BLOCK_SIZE * endJ;
        p3Offset.y = vx::BLOCK_SIZE * sbuf.data[startI][endJ];
        p3Offset.z = vx::BLOCK_SIZE * startI;
        // p4
        p4Offset.x = vx::BLOCK_SIZE * startJ;
        p4Offset.y = vx::BLOCK_SIZE * sbuf.data[endI][startJ];
        p4Offset.z = vx::BLOCK_SIZE * endI;
        break;
    case vx::FACE_FRONT:
    case vx::FACE_BACK:
        // p1
        p1Offset.x = vx::BLOCK_SIZE * startJ;
        p1Offset.y = vx::BLOCK_SIZE * startI;
        p1Offset.z = vx::BLOCK_SIZE * sbuf.data[startI][startJ];
        // p2
        p2Offset.x = vx::BLOCK_SIZE * endJ;
        p2Offset.y = vx::BLOCK_SIZE * endI;
        p2Offset.z = vx::BLOCK_SIZE * sbuf.data[endI][endJ];
        // p3
        p3Offset.x = vx::BLOCK_SIZE * endJ;
        p3Offset.y = vx::BLOCK_SIZE * startI;
        p3Offset.z = vx::BLOCK_SIZE * sbuf.data[startI][endJ];
        // p4
        p4Offset.x = vx::BLOCK_SIZE * startJ;
        p4Offset.y = vx::BLOCK_SIZE * endI;
        p4Offset.z = vx::BLOCK_SIZE * sbuf.data[endI][startJ];
        break;
    default:
        printf("Invalid Face\n");
        ASSERT(false);
    }
    // NOTE(leo): If the function got here, it means at least one quad was foud for the sliding buffer.

    // The origin of each block is always on the smaller coordinates. If we want to return a point
    // int the middle of the block, we have to sum the half size of the block to each of the points.
    f32 halfBlock = (f32)vx::BLOCK_SIZE/2.0f;
    Vec3f halfBlockVec = Vec3f(halfBlock, halfBlock, halfBlock);

    Quad3f* q = &chunk.occluders_data[face];
    q->p1 = chunk.position + p1Offset + halfBlockVec;
    q->p2 = chunk.position + p2Offset + halfBlockVec;
    q->p3 = chunk.position + p3Offset + halfBlockVec;
    q->p4 = chunk.position + p4Offset + halfBlockVec;

    return q;
}

void
create_chunk_occluders(vx::Chunk& chunk)
{
    // @Improvement, @Speed: At the moment we are calculating occluders for both opposite faces,
    // i.e. FACE_RIGHT and FACE_LEFT, we could maybe calculate the only one for each axis.
    // This is however difficult to predict without a formal algorithm. (HARD)

    // ==================================================
    //
    //     Find occluders for BACK and FRONT faces
    //
    // ==================================================
    SlidingBuffer sbuf_zneg;
    SlidingBuffer sbuf_zpos;
    // Both buffers start with -1 values.
    memset(sbuf_zneg.data, -1, sizeof(sbuf_zneg.data));
    memset(sbuf_zpos.data, -1, sizeof(sbuf_zpos.data));
    for (i32 z = 0; z < vx::CHUNK_SIZE; z++)
    {
        for (i32 y = 0; y < vx::CHUNK_SIZE; y++)
        {
            for (i32 x = 0; x < vx::CHUNK_SIZE; x++)
            {
                i32 invIndex = vx::CHUNK_SIZE - 1 - z;
                // For each slice, find the two 1D voxel buffers.
                // When the iteration over the X axis is over, merge all of the 1D voxel buffers
                // into two 2D voxel buffers. They both should generate one occluder each.

                if (sbuf_zneg.data[y][x] == -1 &&
                    chunk.blocks[x][y][z].exists)
                {
                    sbuf_zneg.data[y][x] = z;
                }
                if (sbuf_zpos.data[y][x] == -1 &&
                    chunk.blocks[x][y][invIndex].exists)
                {
                    sbuf_zpos.data[y][x] = invIndex;
                }
            }
        }
    }
    // ==================================================
    //
    //     Find occluders for LEFT and RIGHT faces
    //
    // ==================================================
    SlidingBuffer sbuf_xneg;
    SlidingBuffer sbuf_xpos;
    // Both buffers start with -1 values.
    memset(sbuf_xneg.data, -1, sizeof(sbuf_xneg.data));
    memset(sbuf_xpos.data, -1, sizeof(sbuf_xpos.data));
    for (i32 x = 0; x < vx::CHUNK_SIZE; x++)
    {
        for (i32 y = 0; y < vx::CHUNK_SIZE; y++)
        {
            for (i32 z = 0; z < vx::CHUNK_SIZE; z++)
            {
                i32 invIndex = vx::CHUNK_SIZE - 1 - x;
                // For each slice, find the two 1D voxel buffers.
                // When the iteration over the X axis is over, merge all of the 1D voxel buffers
                // into two 2D voxel buffers. They both should generate one occluder each.

                if (sbuf_xneg.data[y][z] == -1 &&
                    chunk.blocks[x][y][z].exists)
                {
                    sbuf_xneg.data[y][z] = x;
                }
                if (sbuf_xpos.data[y][z] == -1 &&
                    chunk.blocks[invIndex][y][z].exists)
                {
                    sbuf_xpos.data[y][z] = invIndex;
                }
            }
        }
    }
    // ==================================================
    //
    //     Find occluders for UP and BOTTOM faces
    //
    // ==================================================
    SlidingBuffer sbuf_yneg;
    SlidingBuffer sbuf_ypos;
    // Both buffers start with -1 values.
    memset(sbuf_yneg.data, -1, sizeof(sbuf_yneg.data));
    memset(sbuf_ypos.data, -1, sizeof(sbuf_ypos.data));
    for (i32 y = 0; y < vx::CHUNK_SIZE; y++)
    {
        for (i32 z = 0; z < vx::CHUNK_SIZE; z++)
        {
            for (i32 x = 0; x < vx::CHUNK_SIZE; x++)
            {
                i32 invIndex = vx::CHUNK_SIZE - 1 - y;
                // For each slice, find the two 1D voxel buffers.
                // When the iteration over the X axis is over, merge all of the 1D voxel buffers
                // into two 2D voxel buffers. They both should generate one occluder each.

                if (sbuf_yneg.data[z][x] == -1 &&
                    chunk.blocks[x][y][z].exists)
                {
                    sbuf_yneg.data[z][x] = y;
                }
                if (sbuf_ypos.data[z][x] == -1 &&
                    chunk.blocks[x][invIndex][z].exists)
                {
                    sbuf_ypos.data[z][x] = invIndex;
                }
            }
        }
    }
    // For each sliding buffer, finds an occluder rectangle. Since we have 6 faces, there are 6 occluders.
    // @SPEED(leo): It does not calculate the *biggest* occluder, so there are room for optimizations.

    // NOTE(leo): if no quad is found, NULL is returned
    Quad3f* xposQuad = find_largest_quad(sbuf_xpos, chunk, vx::FACE_RIGHT);
    Quad3f* xnegQuad = find_largest_quad(sbuf_xneg, chunk, vx::FACE_LEFT);
    Quad3f* yposQuad = find_largest_quad(sbuf_ypos, chunk, vx::FACE_UP);
    Quad3f* ynegQuad = find_largest_quad(sbuf_yneg, chunk, vx::FACE_DOWN);
    Quad3f* zposQuad = find_largest_quad(sbuf_zpos, chunk, vx::FACE_FRONT);
    Quad3f* znegQuad = find_largest_quad(sbuf_zneg, chunk, vx::FACE_BACK);

    chunk.occluders[vx::FACE_RIGHT] = xposQuad;
    chunk.occluders[vx::FACE_LEFT] = xnegQuad;
    chunk.occluders[vx::FACE_UP] = yposQuad;
    chunk.occluders[vx::FACE_DOWN] = ynegQuad;
    chunk.occluders[vx::FACE_FRONT] = zposQuad;
    chunk.occluders[vx::FACE_BACK] = znegQuad;
}

vx::ChunkManager::ChunkManager()
{
    this->position = Vec3f(0.0f, 0.0f, 0.0f);

    for (u32 i = 0; i < WORLD_SIZE; i++)
        for (u32 j = 0; j < WORLD_SIZE; j++)
            for (u32 k = 0; k < WORLD_SIZE; k++)
            {
                vx::Chunk* chunk = &this->chunks[i][j][k];
                memset(chunk, 0, sizeof(*chunk));
            }
}

void
vx::ChunkManager::create_chunk(u32 chunkX, u32 chunkY, u32 chunkZ, vx::Shader* shader, vx::Material material)
{
    ASSERT(chunkX < WORLD_SIZE && chunkY < WORLD_SIZE && chunkZ < WORLD_SIZE);

    // Alias current chunk so it is easier to refer to it.
    vx::Chunk& chunk = this->chunks[chunkX][chunkY][chunkZ];

    // Set the world position of the chunks relative to the world position of the manager itself.
    Vec3f position;
    position.x = this->position.x + (chunkX * CHUNK_SIZE * BLOCK_SIZE);
    position.y = this->position.y + (chunkY * CHUNK_SIZE * BLOCK_SIZE);
    position.z = this->position.z + (chunkZ * CHUNK_SIZE * BLOCK_SIZE);

    chunk.material = material;
    chunk.shader = shader;
    chunk.position.x = position.x;
    chunk.position.y = position.y;
    chunk.position.z = position.z;
    // Fill all the extreme vertices of the chunk.
    // The order of them does not matter, they are used for frustum culling.
    f32 chunkOffset = BLOCK_SIZE * CHUNK_SIZE;
    chunk.max_vertices[0] = position;
    chunk.max_vertices[1] = position + Vec3f(0.0f, 0.0f, chunkOffset);
    chunk.max_vertices[2] = position + Vec3f(0.0f, chunkOffset, 0.0f);
    chunk.max_vertices[3] = position + Vec3f(chunkOffset, 0.0f, 0.0f);
    chunk.max_vertices[4] = position + Vec3f(0.0f, chunkOffset, chunkOffset);
    chunk.max_vertices[5] = position + Vec3f(chunkOffset, chunkOffset, 0.0f);
    chunk.max_vertices[6] = position + Vec3f(chunkOffset, 0.0f, chunkOffset);
    chunk.max_vertices[7] = position + Vec3f(chunkOffset, chunkOffset, chunkOffset);

    // TODO:
    // This method of deciding which block is filled inside a chunk should eventually be refactored.
    //
    struct osn_context *ctx;
    u32 numBlocks = 0;
    ASSERT(open_simplex_noise(0, &ctx) == 0);
    srand(time(NULL)); // @Test
    for (int z = 0; z < CHUNK_SIZE; z++)
    {
        for (int y = 0; y < CHUNK_SIZE; y++)
        {
            for (int x = 0; x < CHUNK_SIZE; x++)
            {
                Vec3f position = this->chunks[chunkX][chunkY][chunkZ].position;
                Vec3f blockPosition;
                blockPosition.x = position.x + (BLOCK_SIZE * x);
                blockPosition.y = position.y + (BLOCK_SIZE * y);
                blockPosition.z = position.z + (BLOCK_SIZE * z);
                f64 noise = get_noise(
                    blockPosition.x,
                    blockPosition.y,
                    blockPosition.z, 0.05, 2.0, 0.25, ctx);
                bool exists = noise > 0.2 ? false : true;

                /* bool exists = (rand() % 20) > 10 ? true : false; */
                /* if (z == CHUNK_SIZE-1 && (x == 0 || x == 1 || x == 3 || x == 2)) */
                /*     exists = true; */
                /* else */
                /*     exists = false; */
                this->chunks[chunkX][chunkY][chunkZ].blocks[x][y][z].exists = exists;

                if (exists)
                {
                    numBlocks++;
                }
            }
        }
    }
    this->chunks[chunkX][chunkY][chunkZ].num_blocks = numBlocks;

    open_simplex_noise_free(ctx);

    //@Performance: This two functions can possibly be merged into one if performance is needed.
    create_chunk_vertex_buffer(chunk);
    //NOTE(leo): at the moment this function is not necessary. First it is best to try to render
    // the triangles for each chunk into the depth buffer.
    create_chunk_occluders(chunk);
}

//@TEMP(leo): this function should be temporary.
static Vec3f buf[vx::FACE_COUNT * 6]; // Each occluder has 6 vertices
void
render_chunk_occluders(const vx::Chunk& chunk)
{
    memset(buf, 0, sizeof(buf));
    u32 v = 0;
    {
        Quad3f* occ = chunk.occluders[vx::FACE_RIGHT];
        if (occ != NULL)
        {
            buf[v++] = occ->p1;
            buf[v++] = occ->p4;
            buf[v++] = occ->p2;
            buf[v++] = occ->p2;
            buf[v++] = occ->p3;
            buf[v++] = occ->p1;
        }
        else
        {
            // OCC IS NULL
        }
    }
    {
        Quad3f* occ = chunk.occluders[vx::FACE_LEFT];
        if (occ != NULL)
        {
            buf[v++] = occ->p1;
            buf[v++] = occ->p3;
            buf[v++] = occ->p2;
            buf[v++] = occ->p2;
            buf[v++] = occ->p4;
            buf[v++] = occ->p1;
        }
        else
        {
            /* printf("OCC NULL\n"); */
        }
    }
    {
        Quad3f* occ = chunk.occluders[vx::FACE_UP];
        if (occ != NULL)
        {
            buf[v++] = occ->p1;
            buf[v++] = occ->p4;
            buf[v++] = occ->p2;
            buf[v++] = occ->p2;
            buf[v++] = occ->p3;
            buf[v++] = occ->p1;
        }
        else
        {
            /* printf("OCC NULL\n"); */
        }
    }
    {
        Quad3f* occ = chunk.occluders[vx::FACE_DOWN];
        if (occ != NULL)
        {
            buf[v++] = occ->p1;
            buf[v++] = occ->p3;
            buf[v++] = occ->p2;
            buf[v++] = occ->p2;
            buf[v++] = occ->p4;
            buf[v++] = occ->p1;
        }
        else
        {
            /* printf("OCC NULL\n"); */
        }
    }
    {
        Quad3f* occ = chunk.occluders[vx::FACE_FRONT];
        if (occ != NULL)
        {
            buf[v++] = occ->p1;
            buf[v++] = occ->p3;
            buf[v++] = occ->p2;
            buf[v++] = occ->p2;
            buf[v++] = occ->p4;
            buf[v++] = occ->p1;
        }
        else
        {
            /* printf("OCC NULL\n"); */
        }
    }
    {
        Quad3f* occ = chunk.occluders[vx::FACE_BACK];
        if (occ != NULL)
        {
            buf[v++] = occ->p1;
            buf[v++] = occ->p4;
            buf[v++] = occ->p2;
            buf[v++] = occ->p2;
            buf[v++] = occ->p3;
            buf[v++] = occ->p1;
        }
        else
        {
            /* printf("OCC NULL\n"); */
        }
    }

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3f) * v, buf, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

    /* glDisable(GL_CULL_FACE); */
    glDrawArrays(GL_TRIANGLES, 0, 36);
    /* glEnable(GL_CULL_FACE); */

    glBindVertexArray(0);
}

void
vx::ChunkManager::render_occluders(const vx::Camera& camera, const vx::Memory& mem, const bool* keyboard) const
{
    Mat4x4f view = camera.view_matrix();
    GLuint viewLoc;
    GLuint ambientColorLoc, diffuseColorLoc, specularColorLoc, shininessLoc;
    GLuint cameraPositionLoc, lightPositionLoc, lightColorLoc;
    vx::Material material;

    for (u32 i = 0; i < WORLD_SIZE; i++)
        for (u32 j = 0; j < WORLD_SIZE; j++)
            for (u32 k = 0; k < WORLD_SIZE; k++)
            {
                if (this->chunks[i][j][k].num_blocks == 0) continue;
                // -----------------
                // Frustum Culling
                // -----------------
                bool chunkInsideFrustum = camera.frustum.chunk_inside(this->chunks[i][j][k]);
                if (!chunkInsideFrustum) continue;

                // =====================================================
                //                Chunk is Visible
                // ======================================================

                vx::Shader* shader = this->chunks[i][j][k].shader;

                glUseProgram(shader->program);
                // Transform matrices locations
                viewLoc           = shader->uniform_location("view");
                // Material uniform locations
                ambientColorLoc   = shader->uniform_location("material.ambientColor");
                diffuseColorLoc   = shader->uniform_location("material.diffuseColor");
                specularColorLoc  = shader->uniform_location("material.specularColor");
                shininessLoc      = shader->uniform_location("material.shininess");
                // Light position and color and camera position
                cameraPositionLoc = shader->uniform_location("cameraPosition");
                lightPositionLoc  = shader->uniform_location("light.position");
                lightColorLoc     = shader->uniform_location("light.color");
                // Sets the corresponding uniforms.
                // Transforms
                glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view.m00);
                // Materials
                material = this->chunks[i][j][k].material;
                glUniform3f(ambientColorLoc, 1.0f, 0.0f, 0.0f);
                glUniform3f(diffuseColorLoc, 1.0f, 0.0f, 0.0f);
                glUniform3f(specularColorLoc,
                            material.specularColor.x,
                            material.specularColor.y,
                            material.specularColor.z);
                glUniform1f(shininessLoc, material.shininess);
                // Light and Camera
                glUniform3f(cameraPositionLoc,
                            camera.frustum.position.x,
                            camera.frustum.position.y,
                            camera.frustum.position.z);

                glUniform3f(lightPositionLoc, 50.0f, 100.0f, 50.0f);
                glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f); // white color
                // ===========================
                //          Render
                // ===========================
                render_chunk_occluders(this->chunks[i][j][k]);

                glBindVertexArray(0);
                glUseProgram(0);

            }
    /* END_TIMED_BLOCK(DebugCycleCount_RenderChunks); */
}

void
vx::ChunkManager::render_chunks(const vx::Camera& camera, const vx::Memory& mem, const bool *keyboard) const
{
    /* BEGIN_TIMED_BLOCK(DebugCycleCount_RenderChunks); */

    Mat4x4f view = camera.view_matrix();
    Mat4x4f transform = camera.frustum.projection * view;

    GLuint viewLoc;
    GLuint ambientColorLoc, diffuseColorLoc, specularColorLoc, shininessLoc;
    GLuint cameraPositionLoc, lightPositionLoc, lightColorLoc;
    vx::Material material;

    for (u32 i = 0; i < WORLD_SIZE; i++)
        for (u32 j = 0; j < WORLD_SIZE; j++)
            for (u32 k = 0; k < WORLD_SIZE; k++)
            {
                // =========================================================
                // Verify if the chunk needs to be rendered or not.
                // Steps:
                //   1. Verify if chunk has blocks
                //   2. Apply frustum culling
                //   3. TODO: Apply occlusion culling
                //          * Implement a depth buffer renderer
                // =========================================================
                if (this->chunks[i][j][k].num_blocks == 0) continue;
                // -----------------
                // Frustum Culling
                // -----------------
                bool chunkInsideFrustum = camera.frustum.chunk_inside(this->chunks[i][j][k]);
                if (!chunkInsideFrustum) continue;
                // -----------------
                // Occlusion Culling
                // -----------------
                // TODO: For each of the chunks on the world, find its occluders.
                // TODO: After the occluders are found, render them on the depth buffer
                // using the scanline algorithm.


                // =====================================================
                //                Chunk is Visible
                // ======================================================

                vx::Shader* shader = this->chunks[i][j][k].shader;

                glUseProgram(shader->program);
                // Transform matrices locations
                viewLoc           = shader->uniform_location("view");
                // Material uniform locations
                ambientColorLoc   = shader->uniform_location("material.ambientColor");
                diffuseColorLoc   = shader->uniform_location("material.diffuseColor");
                specularColorLoc  = shader->uniform_location("material.specularColor");
                shininessLoc      = shader->uniform_location("material.shininess");
                // Light position and color and camera position
                cameraPositionLoc = shader->uniform_location("cameraPosition");
                lightPositionLoc  = shader->uniform_location("light.position");
                lightColorLoc     = shader->uniform_location("light.color");
                // Sets the corresponding uniforms.
                // Transforms
                glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view.m00);
                // Materials
                material = this->chunks[i][j][k].material;
                glUniform3f(ambientColorLoc,
                            material.ambientColor.x,
                            material.ambientColor.y,
                            material.ambientColor.z);
                glUniform3f(diffuseColorLoc,
                            material.diffuseColor.x,
                            material.diffuseColor.y,
                            material.diffuseColor.z);
                glUniform3f(specularColorLoc,
                            material.specularColor.x,
                            material.specularColor.y,
                            material.specularColor.z);
                glUniform1f(shininessLoc, material.shininess);
                // Light and Camera
                glUniform3f(cameraPositionLoc,
                            camera.frustum.position.x,
                            camera.frustum.position.y,
                            camera.frustum.position.z);

                f64 time = glfwGetTime();
                Vec3f lightPosition;
                lightPosition.x = sinf(time) * 50.0f;
                lightPosition.y = 300.0f;
                lightPosition.z = cosf(time) * 50.0f;
                /* vec3_Print(lightPosition); */
                /* glUniform3f(lightPositionLoc, lightPosition.x, lightPosition.y, lightPosition.z); */
                glUniform3f(lightPositionLoc, 50.0f, 100.0f, 50.0f);
                glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f); // white color
                // ===========================
                //          Render
                // ===========================
                glBindVertexArray(this->chunks[i][j][k].vao);

                glDrawArrays(GL_TRIANGLES, 0, this->chunks[i][j][k].num_vertices);

                glBindVertexArray(0);

            }
    /* END_TIMED_BLOCK(DebugCycleCount_RenderChunks); */
}

void
vx::ChunkManager::render_chunks_wireframe(const vx::Camera& camera, const vx::Shader* shader) const
{
    Mat4x4f view = camera.view_matrix();
    GLuint viewLoc;
    for (u32 i = 0; i < WORLD_SIZE; i++)
        for (u32 j = 0; j < WORLD_SIZE; j++)
            for (u32 k = 0; k < WORLD_SIZE; k++)
            {
                glUseProgram(shader->program);
                viewLoc = shader->uniform_location("view");
                glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view.m00);
                // ===========================
                //          Render
                // ===========================
                glBindVertexArray(this->chunks[i][j][k].vao);
                glDrawArrays(GL_TRIANGLES, 0, this->chunks[i][j][k].num_vertices);
                glBindVertexArray(0);
            }
}

void
create_chunk_vertex_buffer(vx::Chunk& chunk)
{
    /*
      TODO: @Cleanup

      This code can be heavily decreased in line size, since there is a lot of
      code repetition.
    */
    static const u64 SIZE = ((sizeof(Vec3f) * 2 * 8) * vx::CHUNK_SIZE * vx::CHUNK_SIZE * vx::CHUNK_SIZE);
    static Vec3f vertices[SIZE];
    static bool face[vx::CHUNK_SIZE][vx::CHUNK_SIZE];

    /* ===============================================================
     *
     *                       BACK FACES
     *
     * =============================================================== */
    u64 v = 0;
    for (i32 z = 0; z < vx::CHUNK_SIZE; z++)
    {
        memset(face, 0, sizeof(face));

        for (i32 y = 0; y < vx::CHUNK_SIZE; y++)
        {
            // Iterate faster through the x coord
            for (i32 x = 0; x < vx::CHUNK_SIZE; x++)
            {
                if (!chunk.blocks[x][y][z].exists ||
                    face[x][y] == true ||
                    (z > 0 && chunk.blocks[x][y][z-1].exists))
                {
                    face[x][y] = true; //NOTE: maybe not necessary
                    continue;
                }

                const i32 quadBeginX = x;
                face[quadBeginX][y] = true;

                i32 quadEndX = x;
                while (quadEndX+1 < vx::CHUNK_SIZE &&
                       (z == 0 || !chunk.blocks[quadEndX+1][y][z-1].exists) &&
                       chunk.blocks[quadEndX+1][y][z].exists &&
                       face[quadEndX+1][y] == false)
                {
                    quadEndX++;
                    face[quadEndX][y] = true;
                }

                const i32 quadBeginY = y;
                i32 quadEndY = y;
                while (quadEndY+1 < vx::CHUNK_SIZE &&
                       chunk.blocks[quadBeginX][quadEndY+1][z].exists &&
                       (z == 0 || !chunk.blocks[quadBeginX][quadEndY+1][z-1].exists) &&
                       face[quadBeginX][quadEndY+1] == false)
                {
                    for (i32 qi = quadBeginX; qi <= quadEndX; qi++)
                    {
                        if (chunk.blocks[qi][quadEndY+1][z].exists &&
                            (z == 0 || !chunk.blocks[qi][quadEndY+1][z-1].exists) &&
                            face[qi][quadEndY+1] == false)
                            continue;
                        else
                            goto EndBackFaceLoop;
                    }
                    quadEndY++;
                }
            EndBackFaceLoop:
                // Remove back face from blocks
                for (i32 qy = quadBeginY; qy <= quadEndY; qy++)
                    for (i32 qx = quadBeginX; qx <= quadEndX; qx++)
                        face[qx][qy] = true;

                // TODO: Add this to the vertices list to be added to VBO
                // quadBeginX, quadEndX, quadBeginY, quadEndY
                Vec3f leftBottom;
                leftBottom.x = chunk.position.x + (quadBeginX * vx::BLOCK_SIZE);
                leftBottom.y = chunk.position.y + (quadBeginY * vx::BLOCK_SIZE);
                leftBottom.z = chunk.position.z + (z * vx::BLOCK_SIZE);
                Vec3f leftTop;
                leftTop.x = chunk.position.x + (quadBeginX * vx::BLOCK_SIZE);
                leftTop.y = (chunk.position.y + vx::BLOCK_SIZE) + (quadEndY * vx::BLOCK_SIZE);
                leftTop.z = chunk.position.z + (z * vx::BLOCK_SIZE);
                Vec3f rightBottom;
                rightBottom.x = (chunk.position.x + vx::BLOCK_SIZE) + (quadEndX * vx::BLOCK_SIZE);
                rightBottom.y = chunk.position.y + (quadBeginY * vx::BLOCK_SIZE);
                rightBottom.z = chunk.position.z + (z * vx::BLOCK_SIZE);
                Vec3f rightTop;
                rightTop.x = (chunk.position.x + vx::BLOCK_SIZE) + (quadEndX * vx::BLOCK_SIZE);
                rightTop.y = (chunk.position.y + vx::BLOCK_SIZE) + (quadEndY * vx::BLOCK_SIZE);
                rightTop.z = chunk.position.z + (z * vx::BLOCK_SIZE);

                vertices[v++] = leftBottom;
                vertices[v++] = Vec3f(0.0f, 0.0f, -1.0f);
                vertices[v++] = leftTop;
                vertices[v++] = Vec3f(0.0f, 0.0f, -1.0f);
                vertices[v++] = rightTop;
                vertices[v++] = Vec3f(0.0f, 0.0f, -1.0f);
                vertices[v++] = rightTop;
                vertices[v++] = Vec3f(0.0f, 0.0f, -1.0f);
                vertices[v++] = rightBottom;
                vertices[v++] = Vec3f(0.0f, 0.0f, -1.0f);
                vertices[v++] = leftBottom;
                vertices[v++] = Vec3f(0.0f, 0.0f, -1.0f);
            }
        }
    }
    /* ===============================================================
     *
     *                       FRONT FACES
     *
     * =============================================================== */
    for (i32 z = 0; z < vx::CHUNK_SIZE; z++)
    {
        memset(face, 0, sizeof(face));

        for (i32 y = 0; y < vx::CHUNK_SIZE; y++)
        {
            // Iterate faster through the x coord
            for (i32 x = 0; x < vx::CHUNK_SIZE; x++)
            {
                if (!chunk.blocks[x][y][z].exists ||
                    face[x][y] == true ||
                    (z < vx::CHUNK_SIZE-1 && chunk.blocks[x][y][z+1].exists))
                {
                    face[x][y] = true; //NOTE: maybe not necessary
                    continue;
                }

                const i32 quadBeginX = x;
                face[quadBeginX][y] = true;

                i32 quadEndX = x;
                while (quadEndX+1 < vx::CHUNK_SIZE &&
                       (z == vx::CHUNK_SIZE-1 || !chunk.blocks[quadEndX+1][y][z+1].exists) &&
                       chunk.blocks[quadEndX+1][y][z].exists &&
                       face[quadEndX+1][y] == false)
                {
                    quadEndX++;
                    face[quadEndX][y] = true;
                }

                const i32 quadBeginY = y;
                i32 quadEndY = y;
                while (quadEndY+1 < vx::CHUNK_SIZE &&
                       chunk.blocks[quadBeginX][quadEndY+1][z].exists &&
                       (z == vx::CHUNK_SIZE-1 || !chunk.blocks[quadBeginX][quadEndY+1][z+1].exists) &&
                       face[quadBeginX][quadEndY+1] == false)
                {
                    for (i32 qi = quadBeginX; qi <= quadEndX; qi++)
                    {
                        if (chunk.blocks[qi][quadEndY+1][z].exists &&
                            (z == vx::CHUNK_SIZE-1 || !chunk.blocks[qi][quadEndY+1][z+1].exists) &&
                            face[qi][quadEndY+1] == false)
                            continue;
                        else
                            goto EndLoopFrontFace;
                    }
                    quadEndY++;
                }
            EndLoopFrontFace:
                // Remove back face from blocks
                for (i32 qy = quadBeginY; qy <= quadEndY; qy++)
                    for (i32 qx = quadBeginX; qx <= quadEndX; qx++)
                        face[qx][qy] = true;

                Vec3f leftBottom;
                leftBottom.x = chunk.position.x + (quadBeginX * vx::BLOCK_SIZE);
                leftBottom.y = chunk.position.y + (quadBeginY * vx::BLOCK_SIZE);
                leftBottom.z = (chunk.position.z + vx::BLOCK_SIZE) + (z * vx::BLOCK_SIZE);
                Vec3f leftTop;
                leftTop.x = chunk.position.x + (quadBeginX * vx::BLOCK_SIZE);
                leftTop.y = (chunk.position.y + vx::BLOCK_SIZE) + (quadEndY * vx::BLOCK_SIZE);
                leftTop.z = (chunk.position.z + vx::BLOCK_SIZE) + (z * vx::BLOCK_SIZE);
                Vec3f rightBottom;
                rightBottom.x = (chunk.position.x + vx::BLOCK_SIZE) + (quadEndX * vx::BLOCK_SIZE);
                rightBottom.y = chunk.position.y + (quadBeginY * vx::BLOCK_SIZE);
                rightBottom.z = (chunk.position.z + vx::BLOCK_SIZE) + (z * vx::BLOCK_SIZE);
                Vec3f rightTop;
                rightTop.x = (chunk.position.x + vx::BLOCK_SIZE) + (quadEndX * vx::BLOCK_SIZE);
                rightTop.y = (chunk.position.y + vx::BLOCK_SIZE) + (quadEndY * vx::BLOCK_SIZE);
                rightTop.z = (chunk.position.z + vx::BLOCK_SIZE) + (z * vx::BLOCK_SIZE);

                vertices[v++] = leftBottom;
                vertices[v++] = Vec3f(0.0f, 0.0f, 1.0f);
                vertices[v++] = rightBottom;
                vertices[v++] = Vec3f(0.0f, 0.0f, 1.0f);
                vertices[v++] = rightTop;
                vertices[v++] = Vec3f(0.0f, 0.0f, 1.0f);
                vertices[v++] = rightTop;
                vertices[v++] = Vec3f(0.0f, 0.0f, 1.0f);
                vertices[v++] = leftTop;
                vertices[v++] = Vec3f(0.0f, 0.0f, 1.0f);
                vertices[v++] = leftBottom;
                vertices[v++] = Vec3f(0.0f, 0.0f, 1.0f);
            }
        }
    }
    /* ===============================================================
     *
     *                       LEFT FACES
     *
     * =============================================================== */
    for (i32 x = 0; x < vx::CHUNK_SIZE; x++)
    {
        memset(face, 0, sizeof(face));

        for (i32 y = 0; y < vx::CHUNK_SIZE; y++)
        {
            // Iterate faster through the x coord
            for (i32 z = 0; z < vx::CHUNK_SIZE; z++)
            {
                if (!chunk.blocks[x][y][z].exists ||
                    face[z][y] == true ||
                    (x > 0 && chunk.blocks[x-1][y][z].exists))
                {
                    face[z][y] = true; //NOTE: maybe not necessary
                    continue;
                }

                const i32 quadBeginZ = z;
                face[quadBeginZ][y] = true;

                i32 quadEndZ = z;
                while (quadEndZ+1 < vx::CHUNK_SIZE &&
                       (x == 0 || !chunk.blocks[x-1][y][quadEndZ+1].exists) &&
                       chunk.blocks[x][y][quadEndZ+1].exists &&
                       face[quadEndZ+1][y] == false)
                {
                    quadEndZ++;
                    face[quadEndZ][y] = true;
                }

                const i32 quadBeginY = y;
                i32 quadEndY = y;
                while (quadEndY+1 < vx::CHUNK_SIZE &&
                       chunk.blocks[x][quadEndY+1][quadBeginZ].exists &&
                       (x == 0 || !chunk.blocks[x-1][quadEndY+1][quadBeginZ].exists) &&
                       face[quadBeginZ][quadEndY+1] == false)
                {
                    for (i32 qk = quadBeginZ; qk <= quadEndZ; qk++)
                    {
                        if (chunk.blocks[x][quadEndY+1][qk].exists &&
                            (x == 0 || !chunk.blocks[x-1][quadEndY+1][qk].exists) &&
                            face[qk][quadEndY+1] == false)
                            continue;
                        else
                            goto EndLoopLeftFace;
                    }
                    quadEndY++;
                }
            EndLoopLeftFace:
                // Remove back face from blocks
                for (i32 qy = quadBeginY; qy <= quadEndY; qy++)
                    for (i32 qz = quadBeginZ; qz <= quadEndZ; qz++)
                        face[qz][qy] = true;

                Vec3f leftBottom;
                leftBottom.x = chunk.position.x + (x * vx::BLOCK_SIZE);
                leftBottom.y = chunk.position.y + (quadBeginY * vx::BLOCK_SIZE);
                leftBottom.z = chunk.position.z + (quadBeginZ * vx::BLOCK_SIZE);
                Vec3f leftTop;
                leftTop.x = chunk.position.x + (x * vx::BLOCK_SIZE);
                leftTop.y = (chunk.position.y + vx::BLOCK_SIZE) + (quadEndY * vx::BLOCK_SIZE);
                leftTop.z = chunk.position.z + (quadBeginZ * vx::BLOCK_SIZE);
                Vec3f rightBottom;
                rightBottom.x = chunk.position.x + (x * vx::BLOCK_SIZE);
                rightBottom.y = chunk.position.y + (quadBeginY * vx::BLOCK_SIZE);
                rightBottom.z = (chunk.position.z + vx::BLOCK_SIZE) + (quadEndZ * vx::BLOCK_SIZE);
                Vec3f rightTop;
                rightTop.x = chunk.position.x + (x * vx::BLOCK_SIZE);
                rightTop.y = (chunk.position.y + vx::BLOCK_SIZE) + (quadEndY * vx::BLOCK_SIZE);
                rightTop.z = (chunk.position.z + vx::BLOCK_SIZE) + (quadEndZ * vx::BLOCK_SIZE);

                vertices[v++] = leftBottom;
                vertices[v++] = Vec3f(-1.0f, 0.0f, 0.0f);
                vertices[v++] = rightBottom;
                vertices[v++] = Vec3f(-1.0f, 0.0f, 0.0f);
                vertices[v++] = rightTop;
                vertices[v++] = Vec3f(-1.0f, 0.0f, 0.0f);
                vertices[v++] = rightTop;
                vertices[v++] = Vec3f(-1.0f, 0.0f, 0.0f);
                vertices[v++] = leftTop;
                vertices[v++] = Vec3f(-1.0f, 0.0f, 0.0f);
                vertices[v++] = leftBottom;
                vertices[v++] = Vec3f(-1.0f, 0.0f, 0.0f);
            }
        }
    }
    /* ===============================================================
     *
     *                       RIGHT FACES
     *
     * =============================================================== */
    for (i32 x = 0; x < vx::CHUNK_SIZE; x++)
    {
        memset(face, 0, sizeof(face));

        for (i32 y = 0; y < vx::CHUNK_SIZE; y++)
        {
            // Iterate faster through the x coord
            for (i32 z = 0; z < vx::CHUNK_SIZE; z++)
            {
                if (!chunk.blocks[x][y][z].exists ||
                    face[z][y] == true ||
                    (x < vx::CHUNK_SIZE-1 && chunk.blocks[x+1][y][z].exists))
                {
                    face[z][y] = true; //NOTE: maybe not necessary
                    continue;
                }

                const i32 quadBeginZ = z;
                face[quadBeginZ][y] = true;

                i32 quadEndZ = z;
                while (quadEndZ+1 < vx::CHUNK_SIZE &&
                       (x == vx::CHUNK_SIZE-1 || !chunk.blocks[x+1][y][quadEndZ+1].exists) &&
                       chunk.blocks[x][y][quadEndZ+1].exists &&
                       face[quadEndZ+1][y] == false)
                {
                    quadEndZ++;
                    face[quadEndZ][y] = true;
                }

                const i32 quadBeginY = y;
                i32 quadEndY = y;
                while (quadEndY+1 < vx::CHUNK_SIZE &&
                       chunk.blocks[x][quadEndY+1][quadBeginZ].exists &&
                       (x == vx::CHUNK_SIZE-1 || !chunk.blocks[x+1][quadEndY+1][quadBeginZ].exists) &&
                       face[quadBeginZ][quadEndY+1] == false)
                {
                    for (i32 qk = quadBeginZ; qk <= quadEndZ; qk++)
                    {
                        if (chunk.blocks[x][quadEndY+1][qk].exists &&
                            (x == vx::CHUNK_SIZE-1 ||
                             !chunk.blocks[x+1][quadEndY+1][qk].exists) &&
                            face[qk][quadEndY+1] == false)
                            continue;
                        else
                            goto EndLoopRightFace;
                    }
                    quadEndY++;
                }
            EndLoopRightFace:
                // Remove back face from blocks
                for (i32 qy = quadBeginY; qy <= quadEndY; qy++)
                    for (i32 qz = quadBeginZ; qz <= quadEndZ; qz++)
                        face[qz][qy] = true;

                Vec3f rightBottom;
                rightBottom.x = (chunk.position.x + vx::BLOCK_SIZE) + (x * vx::BLOCK_SIZE);
                rightBottom.y = chunk.position.y + (quadBeginY * vx::BLOCK_SIZE);
                rightBottom.z = chunk.position.z + (quadBeginZ * vx::BLOCK_SIZE);
                Vec3f rightTop;
                rightTop.x = (chunk.position.x + vx::BLOCK_SIZE) + (x * vx::BLOCK_SIZE);
                rightTop.y = (chunk.position.y + vx::BLOCK_SIZE) + (quadEndY * vx::BLOCK_SIZE);
                rightTop.z = chunk.position.z + (quadBeginZ * vx::BLOCK_SIZE);
                Vec3f leftBottom;
                leftBottom.x = (chunk.position.x + vx::BLOCK_SIZE) + (x * vx::BLOCK_SIZE);
                leftBottom.y = chunk.position.y + (quadBeginY * vx::BLOCK_SIZE);
                leftBottom.z = (chunk.position.z + vx::BLOCK_SIZE) + (quadEndZ * vx::BLOCK_SIZE);
                Vec3f leftTop;
                leftTop.x = (chunk.position.x + vx::BLOCK_SIZE) + (x * vx::BLOCK_SIZE);
                leftTop.y = (chunk.position.y + vx::BLOCK_SIZE) + (quadEndY * vx::BLOCK_SIZE);
                leftTop.z = (chunk.position.z + vx::BLOCK_SIZE) + (quadEndZ * vx::BLOCK_SIZE);

                vertices[v++] = rightBottom;
                vertices[v++] = Vec3f(1.0f, 0.0f, 0.0f);
                vertices[v++] = rightTop;
                vertices[v++] = Vec3f(1.0f, 0.0f, 0.0f);
                vertices[v++] = leftTop;
                vertices[v++] = Vec3f(1.0f, 0.0f, 0.0f);
                vertices[v++] = leftTop;
                vertices[v++] = Vec3f(1.0f, 0.0f, 0.0f);
                vertices[v++] = leftBottom;
                vertices[v++] = Vec3f(1.0f, 0.0f, 0.0f);
                vertices[v++] = rightBottom;
                vertices[v++] = Vec3f(1.0f, 0.0f, 0.0f);
            }
        }
    }
    /* ===============================================================
     *
     *                       BOTTOM FACES
     *
     * =============================================================== */
    for (i32 y = 0; y < vx::CHUNK_SIZE; y++)
    {
        memset(face, 0, sizeof(face));

        for (i32 z = 0; z < vx::CHUNK_SIZE; z++)
        {
            // Iterate faster through the x coord
            for (i32 x = 0; x < vx::CHUNK_SIZE; x++)
            {
                if (!chunk.blocks[x][y][z].exists ||
                    face[x][z] == true ||
                    (y > 0 && chunk.blocks[x][y-1][z].exists))
                {
                    face[x][z] = true; //NOTE: maybe not necessary
                    continue;
                }

                const i32 quadBeginX = x;
                face[quadBeginX][z] = true;

                i32 quadEndX = x;
                while (quadEndX+1 < vx::CHUNK_SIZE &&
                       (y == 0 || !chunk.blocks[quadEndX+1][y-1][z].exists) &&
                       chunk.blocks[quadEndX+1][y][z].exists &&
                       face[quadEndX+1][z] == false)
                {
                    quadEndX++;
                    face[quadEndX][z] = true;
                }

                const i32 quadBeginZ = z;
                i32 quadEndZ = z;
                while (quadEndZ+1 < vx::CHUNK_SIZE &&
                       chunk.blocks[quadBeginX][y][quadEndZ+1].exists &&
                       (y == 0 || !chunk.blocks[quadBeginX][y-1][quadEndZ+1].exists) &&
                       face[quadBeginX][quadEndZ+1] == false)
                {
                    for (i32 qi = quadBeginX; qi <= quadEndX; qi++)
                    {
                        if (chunk.blocks[qi][y][quadEndZ+1].exists &&
                            (y == 0 || !chunk.blocks[qi][y-1][quadEndZ+1].exists) &&
                            face[qi][quadEndZ+1] == false)
                            continue;
                        else
                            goto EndLoopBottomFace;
                    }
                    quadEndZ++;
                }
            EndLoopBottomFace:
                // Remove face from blocks
                for (i32 qz = quadBeginZ; qz <= quadEndZ; qz++)
                    for (i32 qx = quadBeginX; qx <= quadEndX; qx++)
                        face[qx][qz] = true;

                Vec3f rightBottom;
                rightBottom.x = (chunk.position.x + vx::BLOCK_SIZE) + (quadEndX * vx::BLOCK_SIZE);
                rightBottom.y = chunk.position.y + (y * vx::BLOCK_SIZE);
                rightBottom.z = chunk.position.z + (quadBeginZ * vx::BLOCK_SIZE);
                Vec3f rightTop;
                rightTop.x = (chunk.position.x + vx::BLOCK_SIZE) + (quadEndX * vx::BLOCK_SIZE);
                rightTop.y = chunk.position.y + (y * vx::BLOCK_SIZE);
                rightTop.z = (chunk.position.z + vx::BLOCK_SIZE) + (quadEndZ * vx::BLOCK_SIZE);
                Vec3f leftBottom;
                leftBottom.x = chunk.position.x + (quadBeginX * vx::BLOCK_SIZE);
                leftBottom.y = chunk.position.y + (y * vx::BLOCK_SIZE);
                leftBottom.z = chunk.position.z + (quadBeginZ * vx::BLOCK_SIZE);
                Vec3f leftTop;
                leftTop.x = chunk.position.x + (quadBeginX * vx::BLOCK_SIZE);
                leftTop.y = chunk.position.y + (y * vx::BLOCK_SIZE);
                leftTop.z = (chunk.position.z + vx::BLOCK_SIZE) + (quadEndZ * vx::BLOCK_SIZE);

                vertices[v++] = rightBottom;
                vertices[v++] = Vec3f(0.0f, -1.0f, 0.0f);
                vertices[v++] = rightTop;
                vertices[v++] = Vec3f(0.0f, -1.0f, 0.0f);
                vertices[v++] = leftTop;
                vertices[v++] = Vec3f(0.0f, -1.0f, 0.0f);
                vertices[v++] = leftTop;
                vertices[v++] = Vec3f(0.0f, -1.0f, 0.0f);
                vertices[v++] = leftBottom;
                vertices[v++] = Vec3f(0.0f, -1.0f, 0.0f);
                vertices[v++] = rightBottom;
                vertices[v++] = Vec3f(0.0f, -1.0f, 0.0f);
            }
        }
    }
    /* ===============================================================
     *
     *                       TOP FACES
     *
     * =============================================================== */
    for (i32 y = 0; y < vx::CHUNK_SIZE; y++)
    {
        memset(face, 0, sizeof(face));

        for (i32 z = 0; z < vx::CHUNK_SIZE; z++)
        {
            // Iterate faster through the x coord
            for (i32 x = 0; x < vx::CHUNK_SIZE; x++)
            {
                if (!chunk.blocks[x][y][z].exists ||
                    face[x][z] == true ||
                    (y < vx::CHUNK_SIZE-1 && chunk.blocks[x][y+1][z].exists))
                {
                    face[x][z] = true; //NOTE: maybe not necessary
                    continue;
                }

                const i32 quadBeginX = x;
                face[quadBeginX][z] = true;

                i32 quadEndX = x;
                while (quadEndX+1 < vx::CHUNK_SIZE &&
                       (y == vx::CHUNK_SIZE-1 || !chunk.blocks[quadEndX+1][y+1][z].exists) &&
                       chunk.blocks[quadEndX+1][y][z].exists &&
                       face[quadEndX+1][z] == false)
                {
                    quadEndX++;
                    face[quadEndX][z] = true;
                }
                const i32 quadBeginZ = z;
                i32 quadEndZ = z;
                while (quadEndZ+1 < vx::CHUNK_SIZE &&
                       chunk.blocks[quadBeginX][y][quadEndZ+1].exists &&
                       (y == vx::CHUNK_SIZE-1 || !chunk.blocks[quadBeginX][y+1][quadEndZ+1].exists) &&
                       face[quadBeginX][quadEndZ+1] == false)
                {
                    for (i32 qi = quadBeginX; qi <= quadEndX; qi++)
                    {
                        if (chunk.blocks[qi][y][quadEndZ+1].exists &&
                            (y == vx::CHUNK_SIZE-1 || !chunk.blocks[qi][y+1][quadEndZ+1].exists) &&
                            face[qi][quadEndZ+1] == false)
                            continue;
                        else
                            goto EndLoopTopFace;
                    }
                    quadEndZ++;
                }
            EndLoopTopFace:
                // Remove face from blocks
                for (i32 qz = quadBeginZ; qz <= quadEndZ; qz++)
                    for (i32 qx = quadBeginX; qx <= quadEndX; qx++)
                        face[qx][qz] = true;

                Vec3f rightBottom;
                rightBottom.x = (chunk.position.x + vx::BLOCK_SIZE) + (quadEndX * vx::BLOCK_SIZE);
                rightBottom.y = (chunk.position.y + vx::BLOCK_SIZE) + (y * vx::BLOCK_SIZE);
                rightBottom.z = (chunk.position.z + vx::BLOCK_SIZE) + (quadEndZ * vx::BLOCK_SIZE);
                Vec3f rightTop;
                rightTop.x = (chunk.position.x + vx::BLOCK_SIZE) + (quadEndX * vx::BLOCK_SIZE);
                rightTop.y = (chunk.position.y + vx::BLOCK_SIZE) + (y * vx::BLOCK_SIZE);
                rightTop.z = chunk.position.z + (quadBeginZ * vx::BLOCK_SIZE);
                Vec3f leftBottom;
                leftBottom.x = chunk.position.x + (quadBeginX * vx::BLOCK_SIZE);
                leftBottom.y = (chunk.position.y + vx::BLOCK_SIZE) + (y * vx::BLOCK_SIZE);
                leftBottom.z = (chunk.position.z + vx::BLOCK_SIZE) + (quadEndZ * vx::BLOCK_SIZE);
                Vec3f leftTop;
                leftTop.x = chunk.position.x + (quadBeginX * vx::BLOCK_SIZE);
                leftTop.y = (chunk.position.y + vx::BLOCK_SIZE) + (y * vx::BLOCK_SIZE);
                leftTop.z = chunk.position.z + (quadBeginZ * vx::BLOCK_SIZE);

                vertices[v++] = rightBottom;
                vertices[v++] = Vec3f(0.0f, 1.0f, 0.0f);
                vertices[v++] = rightTop;
                vertices[v++] = Vec3f(0.0f, 1.0f, 0.0f);
                vertices[v++] = leftTop;
                vertices[v++] = Vec3f(0.0f, 1.0f, 0.0f);
                vertices[v++] = leftTop;
                vertices[v++] = Vec3f(0.0f, 1.0f, 0.0f);
                vertices[v++] = leftBottom;
                vertices[v++] = Vec3f(0.0f, 1.0f, 0.0f);
                vertices[v++] = rightBottom;
                vertices[v++] = Vec3f(0.0f, 1.0f, 0.0f);
            }
        }
    }
    chunk.num_vertices = v;

    glGenVertexArrays(1, &chunk.vao);
    glGenBuffers(1, &chunk.vbo);

    glBindVertexArray(chunk.vao);
    glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3f) * v, vertices, GL_DYNAMIC_DRAW);
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

f64
get_noise_2D(f64 x, f64 y, f64 startFrequence, u32 octaveCount,
           f64 persistence, struct osn_context *ctx)
{
    f64 noise = 0;
    f64 normalizeFactor = 0;

    f64 frequence = startFrequence;
    f64 amplitude = 1;

    for (u32 i = 0; i < octaveCount; i++)
    {
        normalizeFactor += amplitude;

        noise += amplitude * open_simplex_noise2(ctx, frequence * x, frequence * y);

        frequence *= 2;

        amplitude *= persistence;
    }
    return noise / normalizeFactor;
}

f64
get_noise(f64 x, f64 y, f64 z, f64 startFrequence, u32 octaveCount,
         f64 persistence, struct osn_context *ctx)
{
    f64 noise = 0;
    f64 normalizeFactor = 0;

    f64 frequence = startFrequence;
    f64 amplitude = 1;

    for (u32 i = 0; i < octaveCount; i++)
    {
        normalizeFactor += amplitude;

        noise += amplitude * open_simplex_noise3(ctx, frequence * x, frequence * y, frequence * z);

        frequence *= 2;

        amplitude *= persistence;
    }
    return noise / normalizeFactor;
}
