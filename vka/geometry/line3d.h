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

#ifndef GLA_LINE3D_HPP
#define GLA_LINE3D_HPP

#include "point3d.h"

namespace vka {

template<typename T>
struct line3d
{
    typedef T                                  value_type;
    typedef glm::tvec3<value_type, glm::highp> vec_type;
    typedef point3d<value_type>                point_type;

    line3d() : p(0.0f), v(1.0f,0.0f,0.0f) {}

    line3d( const point_type & point, const vec_type & dir) : p(point.p), v(dir) { }
	
    line3d( const point_type & p1, const point_type & p2) : p(p1.p), v( p2.p-p1.p ) {}

	/**
		Used to translate the point along the direction vector	
		
		@param  t a scalar parameter to translate the vector by.

		@return A point on the line.
	*/		
    point_type operator()(value_type t) const
	{
        return point_type(p+t*v);
	}

	
	/**
		Gets the shortest distance from the point to the line. 
		
		@param  p a valid point

		@return shortest distance to the line from the point
	*/	
    value_type     distance(const point_type & P) const
    {
        return glm::length( displacement(P) );
    }

	/**
		Gets the displacement of the point to the line

		Gets the translation vector required to move the point to the line
		
		@param  p a valid plane

		@return A vector of the displacement
	*/
    vec_type displacement(const point_type & P) const
    {
        vec_type ap = p - P.p;

        return ap - v * (glm::dot(ap,v) / glm::dot(v,v));
    }
	
	/**
		Gets the shortest distance between two lines

		@param  L a valid line

		@return the shortest distance between the lines
	*/
    value_type     distance(    const line3d & L) const
    {
        return glm::length( displacement(L).v );
    }


	/**
		Gets the line segment between two lines.

		If L1 and L2 are two lines and L3 is the shortest distance between the two lines
		
		then L3(0.0f) is a point on L1
		an   L3(1.0f) is a point on L2
		
		@param  L a valid line

		@return The line segment connecting the two lines at its shortest location
	*/
    line3d displacement(const line3d & P) const
    {
        vec_type n  = glm::cross(v  , P.v);

        vec_type n1 = glm::cross(v  , n);
        vec_type n2 = glm::cross(P.v, n);

        auto t = glm::dot(P.p-p, n1)	/ glm::dot(P.v,n1);
        auto s = glm::dot(p-P.p, n2)	/ glm::dot(  v,n2);

        return line3d(  point_type(p+s*v) ,   P.p+t*P.v - p-s*v );
    }
	
	explicit operator bool() const 
	{ 
		return std::isnormal( glm::dot(v,v) );
	}
	
    vec_type p;
    vec_type v;
};


}

#endif
