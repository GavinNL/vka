#ifndef VKA_PRIMATIVES_H
#define VKA_PRIMATIVES_H

#include <vka/core/mesh.h>

namespace vka
{

vka::host_mesh box_mesh(float dx , float dy , float dz );
vka::host_mesh sphere_mesh(float radius , uint32_t rings, uint32_t sectors);
vka::host_mesh plane_mesh(uint32_t Nx, uint32_t Nz);

mesh_t box_mesh_OLD( float dx, float dy, float dz);
mesh_t sphere_mesh_OLD(float radius , uint32_t rings, uint32_t sectors);
mesh_t plane_mesh_OLD(uint32_t Nx, uint32_t Nz);
}
#endif
