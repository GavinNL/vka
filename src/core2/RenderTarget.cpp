#include <vka/core2/RenderTarget.h>

#include <vka/core/context.h>
#include <vka/core/framebuffer.h>
#include <vka/core/renderpass.h>

#include <vka/core2/FrameBuffer.h>
#include <vka/core2/RenderPass.h>

namespace vka
{


RenderTarget::RenderTarget(context *parent) : context_child(parent),
    m_RenderPass(parent), m_FrameBuffer(parent)
{
    vk::SubpassDependency S0,S1;
    S0.srcSubpass      = VK_SUBPASS_EXTERNAL;
    S0.dstSubpass      = 0;
    S0.srcStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe;
    S0.dstStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    S0.srcAccessMask   = vk::AccessFlagBits::eMemoryRead;
    S0.dstAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    S0.dependencyFlags = vk::DependencyFlagBits::eByRegion;

    S1.srcSubpass      = 0;
    S1.dstSubpass      = VK_SUBPASS_EXTERNAL;
    S1.srcStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    S1.dstStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe;
    S1.srcAccessMask   = vk::AccessFlagBits::eMemoryRead;
    S1.dstAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    S1.dependencyFlags = vk::DependencyFlagBits::eByRegion;

    m_RenderPass.AddSubpassdependency(S0);
    m_RenderPass.AddSubpassdependency(S1);
}

RenderTarget * RenderTarget::SetExtents( vk::Extent2D size)
{
    m_size = size;
    m_renderpass_info.renderArea.extent = size;
    m_renderpass_info.renderArea.offset= vk::Offset2D(0,0);

    m_FrameBuffer.SetExtents( size );
    return this;
}

RenderTarget * RenderTarget::SetColorAttachment(uint32_t index, Texture_p colorAttachment)
{
    // std::string name = get_parent_context()->get_name<vka::offscreen_target>(this) + "_texture_" + std::to_string( m_attachments.size() );
    // auto * Position_Texture = get_parent_context()->new_texture(name);
    // Position_Texture->set_format( format )
    //                 ->set_size( size.width, size.height, 1 )
    //                 ->set_usage( vk::ImageUsageFlagBits::eSampled| vk::ImageUsageFlagBits::eColorAttachment )
    //                 ->set_memory_properties( vk::MemoryPropertyFlagBits::eDeviceLocal)
    //                 ->set_tiling( vk::ImageTiling::eOptimal)
    //                 ->set_view_type( vk::ImageViewType::e2D )
    //                 ->create();
    // Position_Texture->create_image_view(vk::ImageAspectFlagBits::eColor);

    if( index >= m_attachments.size() )
        m_attachments.resize(index+1); //push_back(colorAttachment);

    m_attachments.at(index) = colorAttachment;
    m_RenderPass.SetNumColorAttachments( m_attachments.size() );

    m_RenderPass.SetColorAttachmentLayout( index , vk::ImageLayout::eColorAttachmentOptimal);
    m_RenderPass.GetColorAttachmentDescription( index ).finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
    m_RenderPass.GetColorAttachmentDescription( index ).format      = colorAttachment->GetFormat();

    m_FrameBuffer.SetAttachment(index, colorAttachment);

    m_clear_values.resize( m_attachments.size() );

    m_clear_values.back().color = vk::ClearColorValue( std::array<float,4>( {0.0f, 0.0f, 0.0f, 0.0f} ) );




    return this;
}

RenderTarget * RenderTarget::SetDepthAttachment(uint32_t index, Texture_p depthAttachment)
{
    m_RenderPass.SetDepthAttachmentLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
    m_RenderPass.GetDepthAttachmentDescription().finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    m_RenderPass.GetDepthAttachmentDescription().format      = depthAttachment->GetFormat();
    m_FrameBuffer.SetAttachment( index, depthAttachment);

    m_depth_index = index;
    m_clear_values.resize( m_attachments.size() );
    m_clear_values.back().depthStencil = vk::ClearDepthStencilValue(1.0f, 0.0f);

    return this;
}

void  RenderTarget::Create()
{
    m_RenderPass.Create();
    m_FrameBuffer.SetRenderPass(m_RenderPass.get());
    m_FrameBuffer.Create();
}

vka::Texture_p RenderTarget::GetImage(uint32_t i)
{
    return m_FrameBuffer.GetAttachment(i);
}

}
