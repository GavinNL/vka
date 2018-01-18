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
        P3 p1(0,0,0);
        P3 p2(1,1,1);

        auto L = p2-p1;
        THEN("size == 0")
        {
            std::cout << glm::length(L) << std::endl;
            REQUIRE( fabs( glm::length(L) - sqrt(3) ) < 0.001 );
           // REQUIRE( J.size() == 0);
        }


    }
}
