#include <vka/core/mesh.h>
#include <vka/core/primatives.h>
#include <glm/glm.hpp>

vka::mesh_t vka::box_mesh(float dx , float dy , float dz )
{

    mesh_t vertices;

    using namespace glm;

    vertices.add_attribute(vka::VertexAttribute::ePosition, vk::Format::eR32G32B32Sfloat, sizeof(glm::vec3));
    vertices.add_attribute(vka::VertexAttribute::eUV,       vk::Format::eR32G32Sfloat, sizeof(glm::vec2));
    vertices.add_attribute(vka::VertexAttribute::eNormal,   vk::Format::eR32G32B32Sfloat, sizeof(glm::vec3));

    vertices.reserve_vertices(36);
    vertices.reserve_indices(36);

    auto P = vertices.get_attribute_view<glm::vec3>(vka::VertexAttribute::ePosition);
    auto N = vertices.get_attribute_view<glm::vec3>(vka::VertexAttribute::eNormal);
    auto U = vertices.get_attribute_view<glm::vec2>(vka::VertexAttribute::eUV);


    uint32 i=0;
    uint32 j=0;
    uint32 k=0;
//       |       Position                           |   UV         |     Normal    |
        P[i++] = glm::vec3(0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz);  U[j++] = glm::vec2(0.0,0.0); N[k++]=glm::vec3(0.0,  0.0,  1.0);
        P[i++] = glm::vec3(dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz);  U[j++] = glm::vec2(1.0,0.0); N[k++]=glm::vec3(0.0,  0.0,  1.0);
        P[i++] = glm::vec3(dx  - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz);  U[j++] = glm::vec2(1.0,1.0); N[k++]=glm::vec3(0.0,  0.0,  1.0);
        P[i++] = glm::vec3(0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz);  U[j++] = glm::vec2(0.0,0.0); N[k++]=glm::vec3(0.0,  0.0,  1.0);
        P[i++] = glm::vec3(dx  - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz);  U[j++] = glm::vec2(1.0,1.0); N[k++]=glm::vec3(0.0,  0.0,  1.0);
        P[i++] = glm::vec3(0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz);  U[j++] = glm::vec2(0.0,1.0); N[k++]=glm::vec3(0.0,  0.0,  1.0);
        P[i++] = glm::vec3(0.0 - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz);  U[j++] = glm::vec2(0.0,1.0); N[k++]=glm::vec3(0.0,  0.0, -1.0);
        P[i++] = glm::vec3(dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz);  U[j++] = glm::vec2(1.0,1.0); N[k++]=glm::vec3(0.0,  0.0, -1.0);
        P[i++] = glm::vec3(dx  - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz);  U[j++] = glm::vec2(1.0,0.0); N[k++]=glm::vec3(0.0,  0.0, -1.0);
        P[i++] = glm::vec3(0.0 - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz);  U[j++] = glm::vec2(0.0,1.0); N[k++]=glm::vec3(0.0,  0.0, -1.0);
        P[i++] = glm::vec3(dx  - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz);  U[j++] = glm::vec2(1.0,0.0); N[k++]=glm::vec3(0.0,  0.0, -1.0);
        P[i++] = glm::vec3(0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz);  U[j++] = glm::vec2(0.0,0.0); N[k++]=glm::vec3(0.0,  0.0, -1.0);
        P[i++] = glm::vec3(0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz);  U[j++] = glm::vec2(0.0,0.0); N[k++]=glm::vec3(1.0f, 0.0,  0.0);
        P[i++] = glm::vec3(0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz);  U[j++] = glm::vec2(1.0,0.0); N[k++]=glm::vec3(1.0f, 0.0,  0.0);
        P[i++] = glm::vec3(0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz);  U[j++] = glm::vec2(1.0,1.0); N[k++]=glm::vec3(1.0f, 0.0,  0.0);
        P[i++] = glm::vec3(0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz);  U[j++] = glm::vec2(0.0,0.0); N[k++]=glm::vec3(1.0f, 0.0,  0.0);
        P[i++] = glm::vec3(0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz);  U[j++] = glm::vec2(1.0,1.0); N[k++]=glm::vec3(1.0f, 0.0,  0.0);
        P[i++] = glm::vec3(0.0 - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz);  U[j++] = glm::vec2(0.0,1.0); N[k++]=glm::vec3(1.0f, 0.0,  0.0);
        P[i++] = glm::vec3(dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz);  U[j++] = glm::vec2(0.0,1.0); N[k++]=glm::vec3(1.0f, 0.0,  0.0);
        P[i++] = glm::vec3(dx  - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz);  U[j++] = glm::vec2(1.0,1.0); N[k++]=glm::vec3(1.0f, 0.0,  0.0);
        P[i++] = glm::vec3(dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz);  U[j++] = glm::vec2(1.0,0.0); N[k++]=glm::vec3(1.0f, 0.0,  0.0);
        P[i++] = glm::vec3(dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz);  U[j++] = glm::vec2(0.0,1.0); N[k++]=glm::vec3(1.0f, 0.0,  0.0);
        P[i++] = glm::vec3(dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz);  U[j++] = glm::vec2(1.0,0.0); N[k++]=glm::vec3(1.0f, 0.0,  0.0);
        P[i++] = glm::vec3(dx  - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz);  U[j++] = glm::vec2(0.0,0.0); N[k++]=glm::vec3(1.0f, 0.0,  0.0);
        P[i++] = glm::vec3(0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz);  U[j++] = glm::vec2(0.0,0.0); N[k++]=glm::vec3(0.0f,-1.0,  0.0);
        P[i++] = glm::vec3(dx  - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz);  U[j++] = glm::vec2(1.0,0.0); N[k++]=glm::vec3(0.0f,-1.0,  0.0);
        P[i++] = glm::vec3(dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz);  U[j++] = glm::vec2(1.0,1.0); N[k++]=glm::vec3(0.0f,-1.0,  0.0);
        P[i++] = glm::vec3(0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz);  U[j++] = glm::vec2(0.0,0.0); N[k++]=glm::vec3(0.0f,-1.0,  0.0);
        P[i++] = glm::vec3(dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz);  U[j++] = glm::vec2(1.0,1.0); N[k++]=glm::vec3(0.0f,-1.0,  0.0);
        P[i++] = glm::vec3(0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz);  U[j++] = glm::vec2(0.0,1.0); N[k++]=glm::vec3(0.0f,-1.0,  0.0);
        P[i++] = glm::vec3(0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz);  U[j++] = glm::vec2(0.0,1.0); N[k++]=glm::vec3(0.0f, 1.0,  0.0);
        P[i++] = glm::vec3(dx  - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz);  U[j++] = glm::vec2(1.0,1.0); N[k++]=glm::vec3(0.0f, 1.0,  0.0);
        P[i++] = glm::vec3(dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz);  U[j++] = glm::vec2(1.0,0.0); N[k++]=glm::vec3(0.0f, 1.0,  0.0);
        P[i++] = glm::vec3(0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz);  U[j++] = glm::vec2(0.0,1.0); N[k++]=glm::vec3(0.0f, 1.0,  0.0);
        P[i++] = glm::vec3(dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz);  U[j++] = glm::vec2(1.0,0.0); N[k++]=glm::vec3(0.0f, 1.0,  0.0);
        P[i++] = glm::vec3(0.0 - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz);  U[j++] = glm::vec2(0.0,0.0); N[k++]=glm::vec3(0.0f, 1.0,  0.0);

    //=========================
    // Edges of the triangle : postion delta


    //=========================
        auto I = vertices.get_index_view<uint16_t>();
    for(uint16_t i=0;i<36;i++)
        I[i] = i;

    return vertices;

}

