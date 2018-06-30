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

#ifndef VKA_CAMERA_H
#define VKA_CAMERA_H


#include <vka/linalg.h>
#include <chrono>
#include "transform.h"
#include <functional>

namespace vka
{

class camera
{
    public:

        camera()
        {
            _acc     = 100.0f;
            _drag    = 3.30f;
            mSpeed   = glm::vec3(0.0f);
        }


        void calculate()
        {
            auto currentTime = std::chrono::high_resolution_clock::now();

            double dt = 1.0f * std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() * 0.001;
            startTime = currentTime;

            {
                    float       t = dt;
              const float ACC_MAG = _acc;

              // The acceleration vector in worldspace
              glm::vec3 oAcc  = ACC_MAG*( m_transform.get_orientation() * mAcc );

              // The drag force is proportional to the velocity of the camera
              glm::vec3 dragForce = -_drag * mSpeed;

              // Calculate the change in velocity based on the acceleration and the drag
              glm::vec3 VelocityChange = (oAcc + dragForce ) * t;

              // Make sure the drag force does not cause the camera to move backwards
              mSpeed     +=  VelocityChange; //glm::length(VelocityChange) > 0.0 ? VelocityChange : vec3(0.0f);

              // Clamp the speed
              float speed = glm::length(mSpeed);

              mSpeed      =   speed >  5.0f ? (5.0f * (mSpeed / speed) ) : mSpeed;

              // Set the new position of the camera
              m_transform.translate( (t*mSpeed + 0.0f*oAcc*t*t) );
            }
        }

        void yaw(double dx)
        {
            mEulerAngles.y -= dx;

            mEulerAngles = glm::clamp( mEulerAngles, glm::vec3( -3.14159f/180.0f*89 ,-INFINITY, 0), glm::vec3(3.14159f/180.0f*89, INFINITY, 0) );

            m_transform.set_euler( mEulerAngles);
        }
        void pitch(double dy)
        {
            mEulerAngles.x += dy;

            mEulerAngles = glm::clamp( mEulerAngles, glm::vec3( -3.14159f/180.0f*89 ,-INFINITY, 0), glm::vec3(3.14159f/180.0f*89, INFINITY, 0) );

            m_transform.set_euler( mEulerAngles);
        }

        // void insert_mouse(double x, double y)
        // {
        //     float dx = x - _xMouse;
        //     float dy = y - _yMouse;
        //
        //     _xMouse = x;
        //     _yMouse = y;
        //
        //     if( dx*dx + dy*dy > 30) return;
        //
        //     if( mLooking  )
        //     {
        //         mEulerAngles.x += dy * 0.0025f;
        //         mEulerAngles.y -= dx * 0.0025f;
        //
        //         mEulerAngles = glm::clamp( mEulerAngles, glm::vec3( -3.14159f/180.0f*89 ,-INFINITY, 0), glm::vec3(3.14159f/180.0f*89, INFINITY, 0) );
        //
        //         m_transform.set_euler( mEulerAngles);
        //     }
        // }

        void set_position( glm::vec3 const & p)
        {
            m_transform.set_position(p);
        }
        void lookat(  glm::vec3 const & at, glm::vec3 const & up = glm::vec3(0,1,0) )
        {
           // m_transform.lookat(  m_transform.get_position() - at, up);

            glm::quat orientation = glm::conjugate( glm::quat_cast( glm::lookAt(m_transform.get_position(), at, up)));

            mEulerAngles = glm::eulerAngles(  orientation );
            m_transform.set_orientation(orientation);

        }

        void set_acceleration( glm::vec3 const & a)
        {
            mAcc = a;
        }

        glm::vec3 get_acceleration() const
        {
            return mAcc;
        }

        glm::vec3 get_velocity() const
        {
            return mSpeed;
        }

        void set_fov(float f)
        {
            m_fov = f;
        }

        void set_aspect_ratio(float ar)
        {
            m_aspect_ratio =ar;
        }
        void set_near_plane(float p)
        {
            m_near = p;
        }
        void set_far_plane(float p)
        {
            m_far = p;
        }
       // void insert_key( vka::Key KeyCode, bool Down )
       // {
       //     if( !Down) mAcc *= 0.0f;
       //     switch(KeyCode)
       //     {
       //         case Forward:
       //             mAcc[2] = Down ? -1.0f : 0.0f;
       //             break;
       //         case Back:
       //             mAcc[2] = Down ? 1.0f : 0.0f;
       //             break;
       //         case Left:
       //             mAcc[0] = Down ?  -1.0f : 0.0f;
       //             break;
       //         case Right:
       //             mAcc[0] = Down ? 1.0f : 0.0f;
       //             break;
       //         default:
       //             break;
       //     }
       // }
       //

        glm::mat4 get_view_matrix() const
        {
            return glm::inverse( m_transform.get_matrix() );
        }

        glm::mat4 get_proj_matrix() const
        {
            return glm::perspective(m_fov,m_aspect_ratio,m_near, m_far);
        }
        public:
            float _drag;
            float _acc;

            vka::transform m_transform;

        private:
            //glm::mat4 m_proj;
            //glm::mat4 m_view;

            glm::vec3 mAcc;
            glm::vec3 mSpeed;

            glm::vec3 mEulerAngles;

            float m_fov=45.f;
            float m_aspect_ratio=1.0;
            float m_near=0.1f;
            float m_far=100.0f;

            std::chrono::high_resolution_clock::time_point startTime;
};


} //glre

#endif // EVENTS_H



