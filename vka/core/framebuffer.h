#ifndef VKA_FRAMEBUFFER_H
#define VKA_FRAMEBUFFER_H

#include <vulkan/vulkan.hpp>
#include "log.h"
#include "deleter.h"
#include "context_child.h"

namespace vka
{

class context;
class texture;
class renderpass;

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

        framebuffer* add_attachments(vka::texture * texture)
        {
            m_attachments.push_back(texture);
            return this;
        }

        framebuffer* set_renderpass(vka::renderpass * P)
        {
            m_renderpass = P;
            return this;
        }

        framebuffer* set_extents(vk::Extent2D e)
        {
            m_CreateInfo.width = e.width;
            m_CreateInfo.height = e.height;
            return this;
        }

        void create();

        void create(vk::RenderPass render_pass,
                     vk::Extent2D   extents,
                     vk::ImageView  image_view,
                    vk::ImageView  depth_image=vk::ImageView() );

private:

        CONTEXT_CHILD_DEFAULT_CONSTRUCTOR(framebuffer)

        ~framebuffer();

        vk::FramebufferCreateInfo  m_CreateInfo;

        std::vector<vka::texture*>  m_attachments;
        vka::renderpass            *m_renderpass = nullptr;

        friend class context;
        friend class deleter<framebuffer>;
};

}

#endif
