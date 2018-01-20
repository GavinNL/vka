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



vka::mesh_t vka::sphere_mesh(float radius , uint32_t rings, uint32_t sectors)
{

    using namespace glm;
    vka::mesh_t M;

    using namespace glm;

    M.add_attribute(vka::VertexAttribute::ePosition, vk::Format::eR32G32B32Sfloat, sizeof(glm::vec3));
    M.add_attribute(vka::VertexAttribute::eUV,       vk::Format::eR32G32Sfloat, sizeof(glm::vec2));
    M.add_attribute(vka::VertexAttribute::eNormal,   vk::Format::eR32G32B32Sfloat, sizeof(glm::vec3));

    M.reserve_vertices(36);
    M.reserve_indices(36);


    std::vector<vec3> positions;
    std::vector<vec2> uv;
    std::vector<vec3> normals;

    float const R = 1.0f / (float)(rings-1);
    float const S = 1.0f / (float)(sectors-1);
    unsigned int r, s;


    std::vector<uint16_t> I;

    for(r = 0; r < rings; r++)
        for(s = 0; s < sectors; s++) {
            float const y = std::sin( -3.141592653589f*0.5f + 3.141592653589f * r * R );
            float const x = std::cos(2*3.141592653589f * s * S) * std::sin( 3.141592653589f * r * R );
            float const z = std::sin(2*3.141592653589f * s * S) * std::sin( 3.141592653589f * r * R );

            positions.push_back( { radius*x ,radius*y ,radius*z} );

            uv.push_back( {s*S, r*R} );

            normals.push_back( {x,y,z} );
    }

    for(r = 0 ; r < rings   - 1 ; r++)
    for(s = 0 ; s < sectors - 1 ; s++)
    {
        I.push_back(  (r+1) * sectors + s ); //0
        I.push_back(  (r+1) * sectors + (s+1)  ); //1
        I.push_back(   r * sectors + (s+1) ); //2

        I.push_back(  (r+1) * sectors + s ); //0
        I.push_back(   r * sectors + (s+1) ); //2
        I.push_back(    r * sectors + s ); //3
    }


    //=========================

    M.reserve_indices(I.size());

    auto _I = M.get_index_view<uint16_t>();
    for(uint32_t i=0;i<I.size();i++)
        _I[i] = I[i];



    M.reserve_vertices(positions.size());
    auto _P = M.get_attribute_view<vec3>(vka::VertexAttribute::ePosition);
    auto _U = M.get_attribute_view<vec2>(vka::VertexAttribute::eUV);
    auto _N = M.get_attribute_view<vec3>(vka::VertexAttribute::eNormal);
    for(uint32_t i=0;i<positions.size();i++)
    {
        _P[i] = positions[i];
        _U[i] = uv[i];
        _N[i] = normals[i];
    }
    return M;


    return M;
}
