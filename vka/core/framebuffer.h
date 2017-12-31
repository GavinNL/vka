#ifndef VKA_FRAMEBUFFER_H
#define VKA_FRAMEBUFFER_H

#include <vulkan/vulkan.hpp>
#include "log.h"

namespace vka
{

class framebuffer
{
    public:
        vk::Framebuffer m_framebuffer;
        vk::Image       m_image;
        vk::ImageView   m_image_view;

        framebuffer()
        {

        }

        framebuffer(const framebuffer & other) = delete;


        framebuffer(framebuffer && other )
        {
            if(&other!=this)
            {
                m_framebuffer = other.m_framebuffer;
                m_image       = other.m_image;
                m_image_view  = other.m_image_view;

                other.m_framebuffer = nullptr;
                other.m_image       = nullptr;
                other.m_image_view  = nullptr;
            }
        }

        framebuffer& operator=(framebuffer && other )
        {
            if(&other!=this)
            {
                m_framebuffer = other.m_framebuffer;
                m_image       = other.m_image;
                m_image_view  = other.m_image_view;

                other.m_framebuffer = nullptr;
                other.m_image       = nullptr;
                other.m_image_view  = nullptr;
            }
            return *this;
        }




        void create( vk::Device device,
                     vk::RenderPass render_pass,
                     vk::Extent2D   extents,
                     vk::ImageView  image_view,
                     vk::ImageView  depth_image=vk::ImageView() );
    private:

};

}

#endif
