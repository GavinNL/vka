#include <vka/ext/HostMesh.h>
#include <vka/math/linalg.h>
#include <iostream>

namespace vka
{

inline host_mesh box_mesh(float dx , float dy , float dz )
{
    host_mesh M;

    M.set_format(vka::VertexAttribute::ePosition, vk::Format::eR32G32B32Sfloat , 3*4);
    M.set_format(vka::VertexAttribute::eUV,       vk::Format::eR32G32Sfloat    , 2*4);
    M.set_format(vka::VertexAttribute::eNormal,   vk::Format::eR32G32B32Sfloat , 3*4);

    M.set_format(vka::VertexAttribute::eIndex,   vk::Format::eR16Uint, 2);
    using namespace glm;

    auto & P = M.get_attribute(vka::VertexAttribute::ePosition);
    auto & U = M.get_attribute(vka::VertexAttribute::eUV      );
    auto & N = M.get_attribute(vka::VertexAttribute::eNormal  );
    auto & I = M.get_attribute(vka::VertexAttribute::eIndex  );

    uint32 i=0;
    uint32 j=0;
    uint32 k=0;
//       |       Position                           |   UV         |     Normal    |
        P.push_back( vec3(0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz)) ;  U.push_back( vec2(0.0,0.0)) ; N.push_back( vec3(0.0,  0.0,  1.0)) ;
        P.push_back( vec3(dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz)) ;  U.push_back( vec2(1.0,0.0)) ; N.push_back( vec3(0.0,  0.0,  1.0)) ;
        P.push_back( vec3(dx  - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz)) ;  U.push_back( vec2(1.0,1.0)) ; N.push_back( vec3(0.0,  0.0,  1.0)) ;
        P.push_back( vec3(0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz)) ;  U.push_back( vec2(0.0,0.0)) ; N.push_back( vec3(0.0,  0.0,  1.0)) ;
        P.push_back( vec3(dx  - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz)) ;  U.push_back( vec2(1.0,1.0)) ; N.push_back( vec3(0.0,  0.0,  1.0)) ;
        P.push_back( vec3(0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz)) ;  U.push_back( vec2(0.0,1.0)) ; N.push_back( vec3(0.0,  0.0,  1.0)) ;
        P.push_back( vec3(0.0 - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz)) ;  U.push_back( vec2(0.0,1.0)) ; N.push_back( vec3(0.0,  0.0, -1.0)) ;
        P.push_back( vec3(dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz)) ;  U.push_back( vec2(1.0,1.0)) ; N.push_back( vec3(0.0,  0.0, -1.0)) ;
        P.push_back( vec3(dx  - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz)) ;  U.push_back( vec2(1.0,0.0)) ; N.push_back( vec3(0.0,  0.0, -1.0)) ;
        P.push_back( vec3(0.0 - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz)) ;  U.push_back( vec2(0.0,1.0)) ; N.push_back( vec3(0.0,  0.0, -1.0)) ;
        P.push_back( vec3(dx  - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz)) ;  U.push_back( vec2(1.0,0.0)) ; N.push_back( vec3(0.0,  0.0, -1.0)) ;
        P.push_back( vec3(0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz)) ;  U.push_back( vec2(0.0,0.0)) ; N.push_back( vec3(0.0,  0.0, -1.0)) ;
        P.push_back( vec3(0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz)) ;  U.push_back( vec2(0.0,0.0)) ; N.push_back( vec3(1.0f, 0.0,  0.0)) ;
        P.push_back( vec3(0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz)) ;  U.push_back( vec2(1.0,0.0)) ; N.push_back( vec3(1.0f, 0.0,  0.0)) ;
        P.push_back( vec3(0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz)) ;  U.push_back( vec2(1.0,1.0)) ; N.push_back( vec3(1.0f, 0.0,  0.0)) ;
        P.push_back( vec3(0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz)) ;  U.push_back( vec2(0.0,0.0)) ; N.push_back( vec3(1.0f, 0.0,  0.0)) ;
        P.push_back( vec3(0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz)) ;  U.push_back( vec2(1.0,1.0)) ; N.push_back( vec3(1.0f, 0.0,  0.0)) ;
        P.push_back( vec3(0.0 - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz)) ;  U.push_back( vec2(0.0,1.0)) ; N.push_back( vec3(1.0f, 0.0,  0.0)) ;
        P.push_back( vec3(dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz)) ;  U.push_back( vec2(0.0,1.0)) ; N.push_back( vec3(1.0f, 0.0,  0.0)) ;
        P.push_back( vec3(dx  - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz)) ;  U.push_back( vec2(1.0,1.0)) ; N.push_back( vec3(1.0f, 0.0,  0.0)) ;
        P.push_back( vec3(dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz)) ;  U.push_back( vec2(1.0,0.0)) ; N.push_back( vec3(1.0f, 0.0,  0.0)) ;
        P.push_back( vec3(dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz)) ;  U.push_back( vec2(0.0,1.0)) ; N.push_back( vec3(1.0f, 0.0,  0.0)) ;
        P.push_back( vec3(dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz)) ;  U.push_back( vec2(1.0,0.0)) ; N.push_back( vec3(1.0f, 0.0,  0.0)) ;
        P.push_back( vec3(dx  - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz)) ;  U.push_back( vec2(0.0,0.0)) ; N.push_back( vec3(1.0f, 0.0,  0.0)) ;
        P.push_back( vec3(0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz)) ;  U.push_back( vec2(0.0,0.0)) ; N.push_back( vec3(0.0f,-1.0,  0.0)) ;
        P.push_back( vec3(dx  - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz)) ;  U.push_back( vec2(1.0,0.0)) ; N.push_back( vec3(0.0f,-1.0,  0.0)) ;
        P.push_back( vec3(dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz)) ;  U.push_back( vec2(1.0,1.0)) ; N.push_back( vec3(0.0f,-1.0,  0.0)) ;
        P.push_back( vec3(0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz)) ;  U.push_back( vec2(0.0,0.0)) ; N.push_back( vec3(0.0f,-1.0,  0.0)) ;
        P.push_back( vec3(dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz)) ;  U.push_back( vec2(1.0,1.0)) ; N.push_back( vec3(0.0f,-1.0,  0.0)) ;
        P.push_back( vec3(0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz)) ;  U.push_back( vec2(0.0,1.0)) ; N.push_back( vec3(0.0f,-1.0,  0.0)) ;
        P.push_back( vec3(0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz)) ;  U.push_back( vec2(0.0,1.0)) ; N.push_back( vec3(0.0f, 1.0,  0.0)) ;
        P.push_back( vec3(dx  - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz)) ;  U.push_back( vec2(1.0,1.0)) ; N.push_back( vec3(0.0f, 1.0,  0.0)) ;
        P.push_back( vec3(dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz)) ;  U.push_back( vec2(1.0,0.0)) ; N.push_back( vec3(0.0f, 1.0,  0.0)) ;
        P.push_back( vec3(0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz)) ;  U.push_back( vec2(0.0,1.0)) ; N.push_back( vec3(0.0f, 1.0,  0.0)) ;
        P.push_back( vec3(dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz)) ;  U.push_back( vec2(1.0,0.0)) ; N.push_back( vec3(0.0f, 1.0,  0.0)) ;
        P.push_back( vec3(0.0 - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz)) ;  U.push_back( vec2(0.0,0.0)) ; N.push_back( vec3(0.0f, 1.0,  0.0)) ;

    //=========================
    // Edges of the triangle : postion delta


    //=========================
    for( j=0;j<36;j++)
        I.push_back( (uint16_t)j );

    return M;

}


inline host_mesh sphere_mesh(float radius , uint32_t rings=20, uint32_t sectors=20)
{

    using namespace glm;
    vka::host_mesh M;

    M.set_format(vka::VertexAttribute::ePosition, vk::Format::eR32G32B32Sfloat , 3*4);
    M.set_format(vka::VertexAttribute::eUV,       vk::Format::eR32G32Sfloat    , 2*4);
    M.set_format(vka::VertexAttribute::eNormal,   vk::Format::eR32G32B32Sfloat , 3*4);
    M.set_format(vka::VertexAttribute::eIndex,    vk::Format::eR16Uint , 2);

    auto & positions = M.get_attribute(vka::VertexAttribute::ePosition);
    auto & uv = M.get_attribute(vka::VertexAttribute::eUV      );
    auto & normals = M.get_attribute(vka::VertexAttribute::eNormal  );
    auto & index = M.get_attribute(vka::VertexAttribute::eIndex);



    float const R = 1.0f / (float)(rings-1);
    float const S = 1.0f / (float)(sectors-1);
    unsigned int r, s;


    for(r = 0; r < rings; r++)
    {
        for(s = 0; s < sectors; s++)
        {
            float const y = std::sin( -3.141592653589f*0.5f + 3.141592653589f * r * R );
            float const x = std::cos(2*3.141592653589f * s * S) * std::sin( 3.141592653589f * r * R );
            float const z = std::sin(2*3.141592653589f * s * S) * std::sin( 3.141592653589f * r * R );

            positions.push_back( vec3( radius*x ,radius*y ,radius*z) );

            uv.push_back( vec2(s*S, r*R) );

            normals.push_back( vec3(x,y,z) );
        }
    }


    for(r = 0 ; r < rings   - 1 ; r++)
    {
        for(s = 0 ; s < sectors - 1 ; s++)
        {
            index.push_back(  (uint16_t)( (r+1) * sectors + s) ); //0
            index.push_back(  (uint16_t)( (r+1) * sectors + (s+1) ) ); //1
            index.push_back(  (uint16_t)(  r * sectors + (s+1) )); //2
            index.push_back(  (uint16_t)( (r+1) * sectors + s )); //0
            index.push_back(  (uint16_t)(  r * sectors + (s+1) )); //2
            index.push_back(  (uint16_t)(   r * sectors + s )); //3
        }
    }


    //=========================


    return M;
}

inline host_mesh plane_mesh(uint32_t Nx, uint32_t Nz, float sx=1.0f, float sz=1.0f)
{
    host_mesh M;

    using namespace glm;

    M.set_format(vka::VertexAttribute::ePosition, vk::Format::eR32G32B32Sfloat , 3*4);
    M.set_format(vka::VertexAttribute::eUV,       vk::Format::eR32G32Sfloat    , 2*4);
    M.set_format(vka::VertexAttribute::eNormal,   vk::Format::eR32G32B32Sfloat , 3*4);
    M.set_format(vka::VertexAttribute::eIndex,    vk::Format::eR16Uint , 2);


    auto & P = M.get_attribute(vka::VertexAttribute::ePosition);
    auto & U = M.get_attribute(vka::VertexAttribute::eUV      );
    auto & N = M.get_attribute(vka::VertexAttribute::eNormal  );
    auto & I = M.get_attribute(vka::VertexAttribute::eIndex  );


    const float Xm = Nx / 2.0;
    const float Zm = Nz / 2.0;
    for(uint32_t z=0 ;z <= Nz ; z++)
    {
        for(uint32_t x=0 ; x <= Nx ; x++)
        {
            P.push_back( glm::vec3(x-Xm,0,z-Zm) * glm::vec3(sx,0,sz));
            N.push_back( glm::vec3(0,1,0));
            U.push_back( glm::vec2(x,z) );
        }
    }

    for(uint32_t z=0 ;z < Nz ; z++)
    {
        for(uint32_t x=0 ; x < Nx ; x++)
        {
            #define to_index(x,z) ((z)*(Nx+1)+(x))

            I.push_back( (uint16_t)to_index(x+1,z+1));
            I.push_back( (uint16_t)to_index(x+1,z));
            I.push_back( (uint16_t)to_index(x,z));

            I.push_back( (uint16_t)to_index(x , z+1));
            I.push_back( (uint16_t)to_index(x+1 , z+1));
            I.push_back( (uint16_t)to_index(x , z));
        }

    }

    return M;

}

}
