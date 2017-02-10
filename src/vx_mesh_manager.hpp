#ifndef VX_MESH_MANAGER_H
#define VX_MESH_MANAGER_H

#include <stdbool.h>
#include <GL/glew.h>
#include "vx_types.h"
#include "vx_geometry_manager.h"
#include "vx_material.h"
#include "vx_shader_manager.h"

#ifndef MAX_NUM_MESHES
#define MAX_NUM_MESHES 10000
#endif

typedef struct vx_mesh_manager
{
    u64                   numMeshesUsed;
    vx_geometry_handle   *geometryHandles;
    vx_material          *materials;
    vx_shader           **shaders;

    void                 *buffer;
    u64                   bufferSize;
} vx_mesh_manager;

typedef struct vx_mesh_handle
{
    u64                 id;
    vx_shader          *shader;
    vx_geometry_handle  geometryHandle;
    vx_material         material;
} vx_mesh_handle;


vx_mesh_manager *vx_mesh_manager_New           ();
void             vx_mesh_manager_Free          (vx_mesh_manager *manager);
vx_mesh_handle   vx_mesh_manager_CreateMesh    (vx_mesh_manager *manager, vx_geometry_handle handle,
                                                vx_material material, vx_shader *shader);
void             vx_mesh_manager_DestroyMesh   (vx_mesh_manager *manager, vx_mesh_handle handle);

#endif // VX_MESH_MANAGER_H
