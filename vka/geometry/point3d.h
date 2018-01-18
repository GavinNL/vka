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

#ifndef GLA_POINT3D_HPP
#define GLA_POINT3D_HPP

#include <glm/glm.hpp>
#include <cmath>

namespace vka
{

// a simple wrapper for a point class.

template<typename T>
struct point3d
{
    typedef T                                  value_type;
    typedef glm::tvec3<value_type, glm::highp> vec_type;

    point3d(value_type X=0.0f, value_type Y=0.0f, value_type Z=0.0f) : p(X,Y,Z){}
	
    point3d(const vec_type & P) : p(P){}

	point3d(const point3d & P)   : p( P.p ) {}

    point3d & operator==(const vec_type & P)
	{
		p = P;
		return *this;
	}

	explicit operator bool() const 
	{ 
		return (std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z)); 
	}

    vec_type p;
};


// difference of two points results in a vector
template<typename T>
inline typename point3d<T>::vec_type operator-(const point3d<T> & left, const point3d<T> & right )
{
	return left.p-right.p;
}


// point + vector = point
template<typename T>
inline  point3d<T> operator+(const point3d<T> & left, const typename point3d<T>::vec_type & right )
{
    return point3d<T>(left.p+right);
}


// point - vector = point
template<typename T>
inline  point3d<T> operator-(const point3d<T> & left, const typename point3d<T>::vec_type & right )
{
    return point3d<T>(left.p - right);
}


template<typename T>
inline point3d<T> & operator+=(point3d<T> &left, const typename point3d<T>::vec_type & right )
{
	left.p+=right;
	return left;
}

template<typename T>
inline point3d<T> & operator-=(point3d<T> &left, const typename point3d<T>::vec_type  & right )
{
	left.p-=right;
	return left;
}

}

#endif
