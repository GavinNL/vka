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

#ifndef GLA_PLANE3D_HPP
#define GLA_PLANE3D_HPP

#include <glm/glm.hpp>
#include <limits>
#include "line3d.h"

namespace vka {

template<typename T>
struct plane3d
{
    typedef T                                  value_type;
    typedef glm::tvec3<value_type, glm::highp> vec_type;
    typedef point3d<value_type>                point_type;
    typedef line3d<value_type>                 line_type;

    plane3d(value_type A, value_type B, value_type C, value_type D) : n(A,B,C), d( D ) { }

    plane3d(const point_type & point, const glm::vec3 & normal) : d(-glm::dot(normal,point.p) ), n(normal){}

    plane3d(const point_type & p1, const point_type & p2, const point_type & p3) : plane3d(p1, glm::normalize( glm::cross( p2.p-p1.p, p3.p-p1.p) ) )
	{
	}

    plane3d(const point_type & p1, const line_type & L) : plane3d( p1, glm::cross( p1.p-L.p, L.v) )
	{
	}

	/**
		Gets the line of intersection between two planes.

		@param  P a valid plane

		@return the line of intersection between the two planes. The line is invalid if the planes do not intersect
	*/
    line_type   intersection( const plane3d & P) const // line of intersection between two planes
    {
        assert(false && "No implementation yet");

    }
		
	/**
		Gets the point of intersection between a line and the plane

		@param  r a valid line

		@return the intersection point of the line and plane. The point is invalid if it does not intersect
	*/		
    point_type intersection( const line_type   & r) const // point of intersection between ray and plane
    {
        return point_type(( -d - glm::dot(n,r.p) ) / ( glm::dot(r.v,n )) * r.v + r.p);
    }
	
	/**
		Determines if the line intersects the plane.

		@param  r a valid line

		@return true if the line intersects the plane
	*/
    bool    intersects(const line_type & r ) const
    {
        return !(std::abs( glm::dot( r.v, n)) < std::numeric_limits<value_type>::epsilon());
    }
	/**
		Returns the distance between a Point and the plane

		@param  r a point in 3d space
		@return Distance to the plane. Negative if the point is below the plane
	*/
    value_type      distance(const point_type & r) const
    {
        return ( -d - glm::dot(n, r.p) / glm::dot(n,n) );
    }

	/**
		Returns the displacement between a Point and the plane.
		
		@param  r a point in 3d space
		
		@return vector displacement needed to move the point to the plane
	*/
    glm::vec3  displacement(const point_type & r) const
    {
        return  ((-glm::dot(n,r.p)-d)/glm::length(n))*n ;
    }

	/**
		Determines if a plane is valid and exists
		
		A plane is valid if its normal is non-zero and finite and it's point is finite.
			
		@return true if the plane exists
	*/
	explicit operator bool() const 
	{ 
		return std::isnormal( glm::dot(n,n) ) ;
	}
	
	glm::vec3 n; // normal, does not have to be a unit normal
    value_type     d;
};


}

#endif
