/*
 * MIT License
 *
 * Copyright (c) [2017] [Gavin Lobo]
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


/*
 * A Transform class represents a spatial position and an
 * orientation.
 */
#ifndef VKA_TRANSFORM_H
#define VKA_TRANSFORM_H

//#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
//#define GLM_FORCE_RADIANS
#include <glm/gtc/quaternion.hpp>
//#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

namespace vka
{

/**
 * @brief The Transform class represents the scaling, rotation and translation Transform.
 *
 */
class transform
{
    public:



        transform() : m_position(0,0,0) , m_orientation(1,0,0,0) , m_scale(1.0,1.0,1.0)
        {
        }

        transform(const glm::vec3 & position, const glm::quat & rot, const glm::vec3 & scale) : m_position(position), m_orientation(rot), m_scale(scale)
        {
        }

        transform(const glm::vec3 & position, const glm::quat & rot) : m_position(position), m_orientation(rot), m_scale(1.0f,1.0f,1.0f)
        {
        }

        transform(const glm::vec3 & position) : m_position(position), m_orientation(1,0,0,0), m_scale(1,1,1)
        {

        }

        // positional Transforms
        inline void translate(const glm::vec3 & T)  { m_position += T; }
        inline void set_position(const glm::vec3 & P){ m_position  = P; }

        // scaling Transforms
        inline void set_scale(const glm::vec3 & scale){m_scale = scale;}

        // rotational Transform
        inline void set_orientation(const glm::quat & q) { m_orientation = q; }
        inline void rotate(const glm::vec3 & axis, float AngleRadians) { m_orientation = glm::rotate( m_orientation, AngleRadians, axis ); }

        inline void set_euler( const glm::vec3 & PitchYawRoll )
        {
            m_orientation = glm::quat(PitchYawRoll);
        }

        inline glm::mat4 get_matrix() const
        {
            return glm::translate(  glm::mat4(1.0f), m_position) * glm::mat4_cast(m_orientation) * glm::scale( glm::mat4(1.0), m_scale);
        }

        const glm::quat   & get_orientation() const { return m_orientation; }
        const glm::vec3   & get_position   () const { return m_position;    }
        const glm::vec3   & get_scale      () const { return m_scale;       }

        glm::quat reverse() const {  return glm::quat(m_orientation.w, -m_orientation.x,  -m_orientation.y, -m_orientation.z); }

        void lookat(  glm::vec3 const & at, glm::vec3 const & up)
        {
            //m_orientation = glm::conjugate( glm::quat_cast( glm::lookAt(m_transform.get_position(), at, {0,1,0})));
            m_orientation = glm::conjugate( glm::quat_cast( glm::lookAt( m_position, at, up)  ) );
        }

        operator glm::mat4() const
        {
            return get_matrix();
        }
        /**
         * @brief interpolate
         * @param out  a reference to the Transform object that will be the output
         * @param in1  The initial Transform
         * @param in2  The final Transform
         * @param t    scalar paramter between 0 and 1
         *
         * Interpolates between two Transforms.
         */
        static void interpolate( transform & out, transform & in1, transform & in2, float t)
        {
            auto omt = 1.0f-t;
            out.m_position    = omt*in1.m_position + t*in2.m_position;
            out.m_scale       = omt*in1.m_scale    + t*in2.m_scale;
            out.m_orientation = glm::slerp( in1.m_orientation, in2.m_orientation, t);
        }

        static transform interpolate( const transform & in1, const transform & in2, float t)
        {
            return transform(
                               (1.0f-t)*in1.m_position + t*in2.m_position ,
                               glm::slerp( in1.m_orientation, in2.m_orientation, t) ,
                               (1.0f-t)*in1.m_scale    + t*in2.m_scale );
        }


    public:
        glm::vec3    m_position;
        glm::quat    m_orientation;
        glm::vec3    m_scale;
};



inline transform operator * (const transform & ps, const transform & ls)
{
    transform w;
    w.m_position    = ps.m_position  + ps.m_orientation * (ps.m_scale * ls.m_position);
    w.m_orientation = ps.m_orientation * ls.m_orientation;
    //w.Scale       = ps.Scale * (ps.Orientation * ls.Scale);
    w.m_scale       = ps.m_scale * ls.m_scale;

    return w;

}

inline transform& operator *= ( transform & ps, const transform & ls)
{
    ps = ps * ls;

    return ps;

}

inline transform operator/(const transform& ws, const transform& ps)
{
    transform ls;

    const glm::quat psConjugate( ps.m_orientation.w, -ps.m_orientation.x, -ps.m_orientation.y, -ps.m_orientation.z);

    //const glm::quat psConjugate(); ps.Orientation. conjugate(ps.orientation);

    ls.m_position    = (psConjugate * (ws.m_position - ps.m_position)) / ps.m_scale;
    ls.m_orientation = psConjugate * ws.m_orientation;
    ls.m_scale       = psConjugate * (ws.m_scale / ps.m_scale);

    return ls;
}


}

#endif // Transform_H
