#include <vka/utils/buffer_memory_manager.h>

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"


SCENARIO("Buffer Memory manager with offset specification")
{
    using bmm = vka::buffer_memory_manager;

    GIVEN( "LL")
    {
        bmm B(128);


        THEN("When allocating inside a buffer")
        {
        // +----------------------------+
        // |free  |s1    |free          |
        // +----------------------------+
            auto s1 = B.allocate(32,32);

            REQUIRE(s1==32);
            REQUIRE( B.num_blocks()==3);

        }

        THEN("When allocating at an offset with the same size as the buffer")
        {
            // +----------------------------+
            // |s1                          |
            // +----------------------------+
            auto s1 = B.allocate(128,0);
            REQUIRE( B.num_blocks()==1);
        }

        THEN("When allocating at an offset with the same size as the buffer")
        {
            // +----------------------------+
            // |s1                          |
            // +----------------------------+
            auto s1 = B.allocate(128,5);
            REQUIRE( s1 == bmm::error);
        }
    }
}

SCENARIO("Large Buffer")
{
    using bmm = vka::buffer_memory_manager;

    GIVEN( "LL")
    {
        bmm B(1024*1024*10);

        std::vector<size_t> mem;
        size_t n=0;
        while( n != bmm::error )
        {
            n = B.allocate(  (random() % 5+1) * 1024  );
            if( n != bmm::error ) mem.push_back(n);
        }

        B.print(1024);

        THEN("...")
        {
            std::random_device rd;
            std::mt19937 g(rd());

            std::shuffle(mem.begin(), mem.end(),g);

            while(mem.size() )
            {
              B.free( mem.back() );
              mem.pop_back();
            }
            B.print(1024);
        }


    }
}

SCENARIO("Buffer Memory manager")
{
    using bmm = vka::buffer_memory_manager;

    GIVEN( "LL")
    {
        bmm B(128);

        auto s1 = B.allocate(32);
        auto s2 = B.allocate(32);
        auto s3 = B.allocate(32);
        auto s4 = B.allocate(32);

        REQUIRE(s1==0);
        REQUIRE(s2==32);
        REQUIRE(s3==32+32);
        REQUIRE(s4==32+32+32);


        B.print();
        THEN("Freeing")
        {
            // +----------------------------+
            // |s1    |s2    |s3    |s4     |
            // +----------------------------+
            B.free(s3);
            B.free(s2);
            // +----------------------------+
            // |s1    |free         |s4     |
            // +----------------------------+
            REQUIRE( B.num_blocks() == 3 );
        }

        THEN("Forward backward merge")
        {
            // +----------------------------+
            // |s1    |s2    |s3    |s4     |
            // +----------------------------+



            B.free(s2);
            // +----------------------------+
            // |s1    |free  |s3    |s4     |
            // +----------------------------+
            REQUIRE( B.num_blocks() ==  4);



            B.free(s4);
            // +----------------------------+
            // |s1    |free  |s3    |free   |
            // +----------------------------+
            REQUIRE( B.num_blocks() ==  4);


            B.free(s3);
            // +----------------------------+
            // |s1    |free                 |
            // +----------------------------+
            REQUIRE( B.num_blocks() ==  2);
        }
    }
}

