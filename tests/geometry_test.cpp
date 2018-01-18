#include <vka/geometry/point3d.h>
#include <vka/geometry/line3d.h>
#include <vka/geometry/plane3d.h>

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"


SCENARIO("Number Types")
{
    using P3 = vka::point3d<float>;
    using L3 = vka::line3d<float>;

    GIVEN("A JSON constructed with an int")
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
