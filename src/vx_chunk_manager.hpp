#ifndef VX_CHUNK_MANAGER_HPP
#define VX_CHUNK_MANAGER_HPP

#include <stdbool.h>
#include <GL/glew.h>
#include "glm/fwd.hpp"
#include "um.hpp"
#include "vx_math.hpp"
#include "vx_material.hpp"

namespace vx
{

struct Chunk;
struct Shader;
struct Camera;
struct Memory;
struct Frustum;

enum Face
{
    FACE_BACK, FACE_FRONT, FACE_RIGHT, FACE_LEFT, FACE_UP, FACE_DOWN, FACE_COUNT
};

struct Block
{
    bool exists;
};

constexpr u8 WORLD_SIZE = 8;
constexpr u8 CHUNK_SIZE = 32;
constexpr u8 BLOCK_SIZE = 1;

struct Chunk
{
    u32                  num_blocks;
    u32                  num_vertices;
    glm::vec3            max_vertices[8];
    /* bool                 empty; */
    GLuint               vao;
    GLuint               vbo;
    Material             material;
    glm::vec3            position;
    Shader*              shader;
    // Specifies the central point of the chunk on the map.
    // Represents a three dimensional cube
    //                           x coord.    y coord.    z coord.
    Block                 blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
    // We have two occluders for each direction of the main axis (x,y,z)
    Quad3                 occluders_data[FACE_COUNT];
    Quad3*                occluders[FACE_COUNT];
};

struct ChunkManager
{
    u32            numUsed;
    Chunk          chunks[WORLD_SIZE][WORLD_SIZE][WORLD_SIZE];
    glm::vec3      position;

    ChunkManager();

    void create_chunk(u32 i, u32 j, u32 k, Shader* shader, Material material);
    void render_chunks(const Frustum& frustum, const glm::mat4& view,
                       const Memory& memory, const bool* keyboard) const;
    void render_chunks_wireframe(const glm::mat4& view, const Shader* shader) const;
    void render_occluders(const Camera& camera, const Memory& mem, const bool* keyboard) const;
};


}
#endif // VX_CHUNK_MANAGER_HPP
