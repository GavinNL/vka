#ifndef VKA_PRIMATIVES_H
#define VKA_PRIMATIVES_H

#include <vka/ext/HostMesh.h>

namespace vka
{

vka::host_mesh box_mesh(float dx , float dy , float dz );
vka::host_mesh sphere_mesh(float radius , uint32_t rings, uint32_t sectors);
vka::host_mesh plane_mesh(uint32_t Nx, uint32_t Nz, float sx=1, float sz=1);


}
#endif
