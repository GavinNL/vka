#include <vka/geometry/point3d.h>
#include <vka/geometry/line3d.h>
#include <vka/geometry/plane3d.h>

#include <vka/geometry/boundingbox.h>

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"


SCENARIO("Number Types")
{
    using P3 = vka::point3d<float>;
    using L3 = vka::line3d<float>;

    GIVEN("Points")
    {
        P3 p1(1,2,3);
        P3 p2(-2,0,1);

        REQUIRE( sizeof(P3) == sizeof(glm::vec3) );

        THEN("Difference in two points yields a line")
        {
            auto L = p2-p1;

            REQUIRE( Approx( glm::length(L) ) == sqrt(9+4+4) );

        }


    }

}

SCENARIO("Bounding Box")
{

    using aabb_i = vka::aabb<int>;
    using aabb_f = vka::aabb<float>;

    GIVEN("int bounding box constructed with center and positive size")
    {
        aabb_i a( glm::ivec3(1), glm::ivec3( 2 ));

        THEN("")
        {
            REQUIRE( a.min_x() == 1 );
            REQUIRE( a.max_x() == 3 );

            REQUIRE( a.min_y() == 1 );
            REQUIRE( a.max_y() == 3 );

            REQUIRE( a.min_z() == 1 );
            REQUIRE( a.max_z() == 3 );
        }

    }
    GIVEN("int bounding box constructed with center and negative size")
    {
        aabb_i a( glm::ivec3(3), glm::ivec3( -2 ));

        THEN("")
        {
            REQUIRE( a.min_x() == 1 );
            REQUIRE( a.max_x() == 3 );

            REQUIRE( a.min_y() == 1 );
            REQUIRE( a.max_y() == 3 );

            REQUIRE( a.min_z() == 1 );
            REQUIRE( a.max_z() == 3 );
        }

    }

    GIVEN("Two intersecting boudning boxes")
    {
        aabb_i a( glm::ivec3(0), glm::ivec3( 2 ));

        THEN("Bounding boxes which intersect with a")
        {
            // intersects with a
            aabb_i b( glm::ivec3(1), glm::ivec3(  2 ));
            aabb_i c( glm::ivec3(4), glm::ivec3( -2 )); // intersects at point
            aabb_i d( glm::ivec3(3), glm::ivec3( -2 ));

            REQUIRE( a.intersects(b));
            REQUIRE( a.intersects(c));
            REQUIRE( a.intersects(d));

        }

        THEN("Bounding boxes which do not intersect with a")
        {
            // intersects with a
            aabb_i b( glm::ivec3(3,0,0), glm::ivec3(  2 ) ); // translate to the right
            aabb_i c( glm::ivec3(0,3,0), glm::ivec3(  2 ) ); // intersects at point
            aabb_i d( glm::ivec3(0,0,3), glm::ivec3(  2 ) );

            REQUIRE( !a.intersects(b) );
            REQUIRE( !a.intersects(c) );
            REQUIRE( !a.intersects(d) );

        }
    }

}



