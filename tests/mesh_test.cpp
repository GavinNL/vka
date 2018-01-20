#include <vka/geometry/point3d.h>
#include <vka/geometry/line3d.h>
#include <vka/geometry/plane3d.h>

#include <vka/geometry/boundingbox.h>

#include <vka/utils/buffer_memory_manager.h>

#include <vka/core/mesh.h>

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

SCENARIO("mesh_t")
{

    vka::mesh_t M;

    M.add_attribute(vka::VertexAttribute::ePosition, vk::Format::eR32G32B32Sfloat, 3*4);
    M.add_attribute(vka::VertexAttribute::eNormal  , vk::Format::eR32G32B32Sfloat, 3*4);
    M.add_attribute(vka::VertexAttribute::eUV      , vk::Format::eR32G32Sfloat,    2*4);

    REQUIRE( M.has(vka::VertexAttribute::ePosition ));
    REQUIRE( M.has(vka::VertexAttribute::eNormal ));
    REQUIRE( M.has(vka::VertexAttribute::eUV ));

    REQUIRE( M.vertex_size() == 32);
    REQUIRE( M.num_attributes() == 3);


    REQUIRE( M.size(vka::VertexAttribute::ePosition ) == 3*4);
    REQUIRE( M.size(vka::VertexAttribute::eNormal   ) == 3*4);
    REQUIRE( M.size(vka::VertexAttribute::eUV       ) == 2*4);

    REQUIRE( M.offset(vka::VertexAttribute::ePosition ) == 0);
    REQUIRE( M.offset(vka::VertexAttribute::eUV       ) == 3*4);
    REQUIRE( M.offset(vka::VertexAttribute::eNormal   ) == 5*4);

    M.reserve_vertices(10);
    THEN("array views")
    {
        auto P = M.get_attribute_view<glm::vec3>(vka::VertexAttribute::ePosition);
        auto N = M.get_attribute_view<glm::vec3>(vka::VertexAttribute::eNormal);
        auto U = M.get_attribute_view<glm::vec2>(vka::VertexAttribute::eUV);

        REQUIRE( P.size() == 10);
        REQUIRE( U.size() == 10);
        REQUIRE( N.size() == 10);

        auto c = reinterpret_cast<char*>( &P[1]) - reinterpret_cast<char*>( &P[0]);

        P[0] = glm::vec3(1,1,1);
    }

}
