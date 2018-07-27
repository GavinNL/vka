#ifndef VKA_CORE2_FRAMEBUFFER_H
#define VKA_CORE2_FRAMEBUFFER_H

#include <vulkan/vulkan.hpp>
#include <vka/core/context_child.h>

#include <vka/core2/TextureMemoryPool.h>

namespace vka
{

class context;
class texture;
class renderpass;

class FrameBuffer : public context_child
{
    public:
        vk::Framebuffer m_framebuffer;
        vk::Image       m_image;
        vk::ImageView   m_image_view;

        FrameBuffer(context * parent) : context_child(parent)
        {

        }
        ~FrameBuffer()
        {

        }

        operator vk::Framebuffer()
        {
            return m_framebuffer;
        }

        FrameBuffer* SetAttachment(uint32_t index, vka::Texture_p  texture)
        {
            if(index >= m_attachments.size() )
                m_attachments.resize( index+1);
            m_attachments.at(index) = texture;
            return this;
        }

        FrameBuffer* SetRenderPass( vk::RenderPass  P)
        {
            m_CreateInfo.renderPass = P;
            //m_renderpass = P;
            return this;
        }

        FrameBuffer* SetExtents(vk::Extent2D e)
        {
            m_CreateInfo.width = e.width;
            m_CreateInfo.height = e.height;
            return this;
        }

        void Create();



protected:
        vk::FramebufferCreateInfo    m_CreateInfo;
        std::vector<vka::Texture_p>  m_attachments;

        friend class context;
        friend class deleter<FrameBuffer>;
};

}

#endif
