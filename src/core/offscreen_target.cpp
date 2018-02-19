#include <vka/core/offscreen_target.h>
#include <vka/core/context.h>
#include <vka/core/texture.h>
#include <vka/core/renderpass.h>
#include <vka/core/framebuffer.h>

vka::offscreen_target::offscreen_target(context * parent) : context_child(parent)
{
    std::string name = get_parent_context()->get_name<vka::offscreen_target>(this);;
    m_renderpass  = get_parent_context()->new_renderpass( name + "_renderpass");
    m_framebuffer = get_parent_context()->new_framebuffer(name + "_framebuffer");
    m_framebuffer->set_renderpass(m_renderpass);
    vk::SubpassDependency S0,S1;
    S0.srcSubpass    = VK_SUBPASS_EXTERNAL;
    S0.dstSubpass    = 0;
    S0.srcStageMask  = vk::PipelineStageFlagBits::eBottomOfPipe;
    S0.dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    S0.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
    S0.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    S0.dependencyFlags = vk::DependencyFlagBits::eByRegion;

    S1.srcSubpass    = 0;
    S1.dstSubpass    = VK_SUBPASS_EXTERNAL;
    S1.srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    S1.dstStageMask  = vk::PipelineStageFlagBits::eBottomOfPipe;
    S1.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
    S1.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    S1.dependencyFlags = vk::DependencyFlagBits::eByRegion;

    m_renderpass->add_subpass_dependency(S0);
    m_renderpass->add_subpass_dependency(S1);
}

vka::offscreen_target* vka::offscreen_target::set_extents( vk::Extent2D size)
{
    m_framebuffer->set_extents(size);
    return this;
}

vka::offscreen_target* vka::offscreen_target::add_color_attachment(vk::Extent2D size, vk::Format format)
{
    std::string name = get_parent_context()->get_name<vka::offscreen_target>(this) + "_texture_" + std::to_string( m_attachments.size() );
    auto * Position_Texture = get_parent_context()->new_texture(name);
    Position_Texture->set_format( format )
                    ->set_size( size.width, size.height, 1 )
                    ->set_usage( vk::ImageUsageFlagBits::eSampled| vk::ImageUsageFlagBits::eColorAttachment )
                    ->set_memory_properties( vk::MemoryPropertyFlagBits::eDeviceLocal)
                    ->set_tiling( vk::ImageTiling::eOptimal)
                    ->set_view_type( vk::ImageViewType::e2D )
                    ->create();
    Position_Texture->create_image_view(vk::ImageAspectFlagBits::eColor);
    m_attachments.push_back(Position_Texture);

    m_renderpass->set_num_color_attachments( m_attachments.size() );
    m_renderpass->set_color_attachment_layout( m_attachments.size()-1, vk::ImageLayout::eColorAttachmentOptimal);
    m_renderpass->get_color_attachment_description(m_attachments.size()-1).finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    m_renderpass->get_color_attachment_description(m_attachments.size()-1).format      = Position_Texture->get_format();

    m_framebuffer->add_attachments(Position_Texture);




    return this;
}

vka::offscreen_target* vka::offscreen_target::add_depth_attachment(vk::Extent2D size, vk::Format format)
{
    std::string name = get_parent_context()->get_name<vka::offscreen_target>(this) + "_texture_" + std::to_string( m_attachments.size() );
    auto * Depth_Texture = get_parent_context()->new_depth_texture(name);
    Depth_Texture->set_size( size.width, size.height, 1)
                 ->set_usage(  vk::ImageUsageFlagBits::eTransferDst |vk::ImageUsageFlagBits::eSampled| vk::ImageUsageFlagBits::eDepthStencilAttachment )
                 ->create();
    Depth_Texture->create_image_view( vk::ImageAspectFlagBits::eDepth);
    m_attachments.push_back(Depth_Texture);

    m_renderpass->set_depth_attachment_layout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
    m_renderpass->get_depth_attachment_description().finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    m_renderpass->get_depth_attachment_description().format       = Depth_Texture->get_format();
    m_framebuffer->add_attachments(Depth_Texture);

    return this;
}

void vka::offscreen_target::create()
{
    m_renderpass->create();
    m_framebuffer->create();
}
