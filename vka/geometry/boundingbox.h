/*
 * MIT License
 *
 * Copyright (c) [year] [fullname]
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef GLA_BOUNDINGBOX
#define GLA_BOUNDINGBOX

#include <vka/linalg.h>
#include "boundingrect.h"
#include <algorithm>
#include <type_traits>

namespace vka
{

template<typename T>
struct aabb
{
    using vec_type = glm::tvec3<T, glm::defaultp>;
    using value_type = T;

private:
    vec_type m_position; // the corer of the box we are consdering the center
    vec_type m_size;   // the dimensions of the box.

public:
    aabb() : aabb( vec_type(0), vec_type(0) )
    {

    }

    aabb( const vec_type & p, const vec_type & s) : m_position(p), m_size(s)
    {
        m_position.x = s.x<0 ? ( p.x+s.x ) : ( p.x );
        m_position.y = s.y<0 ? ( p.y+s.y ) : ( p.y );
        m_position.z = s.z<0 ? ( p.z+s.z ) : ( p.z );

        m_size.x = std::abs(s.x);
        m_size.y = std::abs(s.y);
        m_size.z = std::abs(s.z);
    }

    aabb(const vec_type & p) : aabb(p, vec_type(0,0,0)) {}

    aabb(value_type v) : aabb( vec_type(0,0,0), vec_type(v,v,v) )
    {
        m_position -= m_size / 2;
    }

    value_type min_x() const { return m_position.x; }
    value_type max_x() const { return m_position.x+m_size.x; }
    value_type min_y() const { return m_position.y; }
    value_type max_y() const { return m_position.y+m_size.y; }
    value_type min_z() const { return m_position.z; }
    value_type max_z() const { return m_position.z+m_size.z; }


    aabb& set_position( vec_type const & v)
    {
        m_position = v;
        return *this;
    }

    aabb& set_size( vec_type const & s)
    {
        m_position.x = s.x<0 ? ( m_position.x+s.x ) : ( m_position.x );
        m_position.y = s.y<0 ? ( m_position.y+s.y ) : ( m_position.y );
        m_position.z = s.z<0 ? ( m_position.z+s.z ) : ( m_position.z );

        m_size.x = std::abs(s.x);
        m_size.y = std::abs(s.y);
        m_size.z = std::abs(s.z);
        return *this;
    }

    vec_type get_size() const
    {
        return m_size;
    }
    vec_type get_position() const
    {
        return m_position;
    }

    aabb& translate( vec_type const & v)
    {
        m_position+=v;
    }
    aabb& scale( vec_type const & v)
    {
        m_size += v;
    }

    value_type volume() const
    {
        return m_size.x*m_size.y*m_size.z;
    }

    vec_type centre() const
    {
        return m_position + m_size/2;
    }

    template<typename F>
    bool intersects( aabb<F> const & other ) const
    {
        if( other.max_x() < min_x() || other.min_x() > max_x() ||
            other.max_y() < min_y() || other.min_y() > max_y() ||
            other.max_z() < min_z() || other.min_z() > max_z()  )
        {
                return false;
        }
        return true;
    }

};

template<typename T=float>
struct bounding_box
{
    using vec_type = glm::tvec3<T, glm::defaultp>;

    vec_type min;
    vec_type max;
	

    bounding_box( const vec_type & pMin) : min(pMin), max(pMin)
    {
    }

    bounding_box( const vec_type & pMin, const vec_type & pMax) : min(pMin), max(pMax)
    {

    }

    bounding_box( const vec_type & pCenter, const T & half_length) : min( pCenter-vec_type(half_length)), max( pCenter+vec_type(half_length) )
    {

    }

    bounding_box()
    {

    }

    vec_type size() const
    {
        return max - min;
    }

    vec_type centre() const
    {
        return (min + max)/static_cast<T>(2);
        //return (min + max) / typename vec_type::value_type(2);
    }

    bounding_box<T>& operator+=(const glm::vec3 & x )
    {
        min += x;
        max += x;
        return *this;
    }
    bounding_box<T>& operator-=(const glm::vec3 & x )
    {
        min -= x;
        max -= x;
        return *this;
    }

    /**
     * @brief transform
     * @param M
     * @return
     *
     * Transforms the bounding box and returns a new bounding box which
     * encompasses the entire transformed box. The returned bounding box
     * is always axis-aligned. In most cases the new bounding box
     * will have a larger volume than the original.
     */
    bounding_box<T> transform(const glm::mat4 & M) const
    {
        auto s = this->Size();

        //========== testing =============
        /*
         * assume affine transformation
         */
#if 0
        auto e1  = M * glm::vec4( s.x , 0  ,    0 ,0);
        auto e2  = M * glm::vec4(    0, s.y,    0 ,0);
        auto e3  = M * glm::vec4(    0, 0  ,  s.z ,0);


        // need to construct all the points using min and the 3 axis vectors
        const auto   p7 =           e3;
        const auto & p6 =      e2     ;
              auto   p5 =      e2 + e3;
        const auto & p4 = e1          ;
              auto   p3 = e1 +      e3;
              auto   p2 = e1 + e2     ;
              auto   p1 = e1 + p5     ;



        #error Dont forget to test this!
#endif
        //===================================

        glm::vec4 p[] =
        {
            M*glm::vec4(min.x, min.y, min.z,1.0f),
            M*glm::vec4(min.x, min.y, max.z,1.0f),
            M*glm::vec4(min.x, max.y, min.z,1.0f),
            M*glm::vec4(min.x, max.y, max.z,1.0f),
            M*glm::vec4(max.x, min.y, min.z,1.0f),
            M*glm::vec4(max.x, min.y, max.z,1.0f),
            M*glm::vec4(max.x, max.y, min.z,1.0f),
            M*glm::vec4(max.x, max.y, max.z,1.0f)
        };

        bounding_box<T> out( glm::vec3(std::numeric_limits<T>::max()),  glm::vec3(std::numeric_limits<T>::lowest()) );

        for(int i=0 ; i < 8 ; i++)
        {
            out.min.x = glm::min( out.min.x, p[i].x );
            out.min.y = glm::min( out.min.y, p[i].y );
            out.min.z = glm::min( out.min.z, p[i].z );

            out.max.x = glm::max( out.max.x, p[i].x );
            out.max.y = glm::max( out.max.y, p[i].y );
            out.max.z = glm::max( out.max.z, p[i].z );
        }

        return out;
    }

    /**
     * @brief Project
     * @param Proj
     * @return BoundingRect of the projection of the
     *
     * Given a matrix that is Proj*View Matrix. Returns a
     * bounding rect that encompases the projection of the boundingbox
     * projected onto the XY plane.
     */
    bounding_rect<T> project(const glm::mat4 & Proj) const
    {
        bounding_rect<T> out;

        glm::vec4 p[] =
        {
            Proj*glm::vec4(min.x, min.y, min.z,1.0f),
            Proj*glm::vec4(min.x, min.y, max.z,1.0f),
            Proj*glm::vec4(min.x, max.y, min.z,1.0f),
            Proj*glm::vec4(min.x, max.y, max.z,1.0f),
            Proj*glm::vec4(max.x, min.y, min.z,1.0f),
            Proj*glm::vec4(max.x, min.y, max.z,1.0f),
            Proj*glm::vec4(max.x, max.y, min.z,1.0f),
            Proj*glm::vec4(max.x, max.y, max.z,1.0f)
        };


        for(int i=0;i<8;i++)
        {
            float im = 1.0f / p[i].w;
            out.min.x = glm::min( out.min.x, p[i].x*im );
            out.min.y = glm::min( out.min.y, p[i].y*im );

            out.max.x = glm::max( out.max.x, p[i].x*im );
            out.max.y = glm::max( out.max.y, p[i].y*im );
        }


        return out;
    }

    // Determines if the other AABB intersects this one
    template<typename F>
    bool intersects(const bounding_box<F> & other) const
    {
            if( other.max.x < min.x || other.min.x > max.x ||
                    other.max.y < min.y || other.min.y > max.y ||
                    other.max.z < min.z || other.min.z > max.z  )
            {
                    return false;
            }
            return true;
    }


    // determines where a point p, is relative to the bounding box.
    // in front, back,
    enum class eLocation :unsigned int {
        INSIDE = 0,
        LEFT   = 1,
        RIGHT  = 2,
        UNDER  = 4,
        ABOVE  = 8,
        BACK   = 16,
        FRONT  = 32,

        LEFT_UNDER=  1 | 4,
        LEFT_ABOVE=  1 | 8,

        RIGHT_UNDER  =  2 | 4 ,
        RIGHT_ABOVE  =  2 | 8 ,

        LEFT_BACK   =  1 | 16,
        LEFT_FRONT  =  1 | 32,

        RIGHT_BACK  =  2 | 16,
        RIGHT_FRONT =  2 | 32,


        UNDER_BACK   =  4 | 16,
        UNDER_FRONT  =  4 | 32,
        ABOVE_BACK   =  8 | 16,
        ABOVE_FRONT  =  8 | 32,


        LEFT_UNDER_BACK   =  1 | 4 | 16,
        LEFT_UNDER_FRONT  =  1 | 4 | 32,
        LEFT_ABOVE_BACK   =  1 | 8 | 16,
        LEFT_ABOVE_FRONT  =  1 | 8 | 32,
        RIGHT_UNDER_BACK  =  2 | 4 | 16,
        RIGHT_UNDER_FRONT =  2 | 4 | 32,
        RIGHT_ABOVE_BACK  =  2 | 8 | 16,
        RIGHT_ABOVE_FRONT =  2 | 8 | 32
    };


    eLocation locate_point( const glm::vec3 & p )
    {
        assert(false && "Not implemented yet");
        unsigned int l=0;

        auto x = p.x < min.x ?  eLocation::LEFT : (p.x > max.x ?  eLocation::RIGHT : eLocation::INSIDE);
        auto y = p.y < min.y ?  eLocation::UNDER: (p.y > max.y ?  eLocation::ABOVE : eLocation::INSIDE);
        auto z = p.z < min.z ?  eLocation::BACK : (p.z > max.z ?  eLocation::FRONT : eLocation::INSIDE);

        l = static_cast<unsigned int>(x) | static_cast<unsigned int>(y) | static_cast<unsigned int>(z);
        return static_cast<eLocation>(l);
    }

    //
    // Returns true if the other aabb is FULLY contained within this box
    //
    bool contains(const bounding_box & other) const
    {
            if( other.max.x < max.x &&
                other.min.x > min.x &&
                other.max.y < max.y &&
                other.min.y > min.y &&
                other.max.z < max.z &&
                other.min.z > min.z  )
            {
                    return true;
            }
            return false;
    }

    T volume() const {
            return (max.x-min.x) * (max.y-min.y) * (max.z-min.z);
    }
	
    float distance(const glm::vec3 & p) const
    {
      float dx = std::max(min.x - p.x, 0, p.x - max.x);
      float dy = std::max(min.y - p.y, 0, p.y - max.y);

      return sqrt(dx*dx + dy*dy);
    }

};

template<typename T>
inline bounding_box<T> operator-(const bounding_box<T> & left, const glm::vec3 & x )
{
    return bounding_box<T>( left.min-x, left.max-x);
}

template<typename T>
inline bounding_box<T> operator+(const bounding_box<T> & left, const glm::vec3 & x )
{
    return bounding_box<T>( left.min+x, left.max+x);
}

template<typename T>
inline bounding_box<T> operator*(const bounding_box<T> & left, const T & x )
{
    auto c = left.centre();


    if( std::is_floating_point<T>::value )
    {
        auto s = left.size()*static_cast<T>(0.5);
        return bounding_box<T>( c-s*x, c+s*x);
    }
    else
    {
        auto s = left.size()/static_cast<T>(2);
        return bounding_box<T>( c - s*x, c+s*x);
    }

}


}

#endif
