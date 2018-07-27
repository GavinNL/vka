#pragma once
#ifndef VKA_RENDER_TARGET2_H
#define VKA_RENDER_TARGET2_H

#include <vka/core/context_child.h>
#include <vka/core2/TextureMemoryPool.h>

#include <vka/core2/FrameBuffer.h>
#include <vka/core2/RenderPass.h>

namespace vka
{

class renderpass;
class framebuffer;


class RenderTarget : public context_child
{
public:

    RenderTarget(context * parent );

    ~RenderTarget() {}

    RenderTarget( RenderTarget const & other) = delete;


    RenderTarget & operator = ( RenderTarget const & other)
    {
        if( this != & other)
        {

        }
        return *this;
    }

    RenderTarget & operator = ( RenderTarget && other)
    {
        if( this != & other)
        {

        }
        return *this;
    }

    RenderTarget * SetExtents( vk::Extent2D size);
    RenderTarget * SetColorAttachment( uint32_t index, Texture_p colorAttachment);
    RenderTarget * SetDepthAttachment(uint32_t index, Texture_p depthAttachment);
    void           Create();


    //==============
    vk::RenderPass  GetRenderPass() const
    {
        return m_RenderPass.get();
    }

    vka::Texture_p GetImage(uint32_t i);
protected:
    vk::Extent2D m_size;

    vka::RenderPass       m_RenderPass;
    vka::FrameBuffer      m_FrameBuffer;

    std::vector<Texture_p>      m_attachments;
    std::vector<vk::ClearValue> m_clear_values;

    uint32_t m_depth_index; // the index in the above vector that is the depth attachment;

    vk::RenderPassBeginInfo m_renderpass_info;
};

}

#endif
