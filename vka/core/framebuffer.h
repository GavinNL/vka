#ifndef VKA_FRAMEBUFFER_H
#define VKA_FRAMEBUFFER_H

#include <vulkan/vulkan.hpp>
#include "log.h"
#include "deleter.h"
#include "context_child.h"

namespace vka
{

class context;

class framebuffer : public context_child
{
    public:
        vk::Framebuffer m_framebuffer;
        vk::Image       m_image;
        vk::ImageView   m_image_view;



        operator vk::Framebuffer()
        {
            return m_framebuffer;
        }

        void create(vk::RenderPass render_pass,
                     vk::Extent2D   extents,
                     vk::ImageView  image_view,
                     vk::ImageView  depth_image=vk::ImageView() );
    private:

        CONTEXT_CHILD_DEFAULT_CONSTRUCTOR(framebuffer)

        ~framebuffer();

        friend class context;
        friend class deleter<framebuffer>;
};

}

#endif
