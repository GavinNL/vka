#include <vka/core/framebuffer.h>
#include <vka/core/context.h>
#include <vka/core/texture.h>
#include <vka/core/renderpass.h>

vka::framebuffer::~framebuffer()
{
    if(m_framebuffer)
    {
        LOG << "Framebuffer destroyed" << ENDL;
        get_device().destroyFramebuffer(m_framebuffer);
    }
}


void vka::framebuffer::create()
{
    std::vector<vk::ImageView> attachments;

    for(auto * t : m_attachments)
        attachments.push_back( t->get_image_view() );

    if( !m_renderpass )
        throw std::runtime_error("Render pass not set");

    m_CreateInfo.renderPass = m_renderpass->get();
    m_CreateInfo.layers = 1;
    m_CreateInfo.pAttachments = attachments.data();
    m_CreateInfo.attachmentCount = attachments.size();

    m_framebuffer = get_device().createFramebuffer( m_CreateInfo );

    if(!m_framebuffer)
    {
        ERROR << "Error creating frame buffer" << ENDL;
        throw std::runtime_error("Error creating frame buffer");
    }
}


void vka::framebuffer::create( vk::RenderPass render_pass,
                               vk::Extent2D extents,
                               vk::ImageView image_view,
                               vk::ImageView depth_image)
{

    std::vector<vk::ImageView> attachments;

    attachments.push_back(image_view);

    if( depth_image)
        attachments.push_back(depth_image);

    m_CreateInfo.renderPass      = render_pass;
    m_CreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    m_CreateInfo.pAttachments    = attachments.data();
    m_CreateInfo.width           = extents.width;
    m_CreateInfo.height          = extents.height;
    m_CreateInfo.layers          = 1;

    m_framebuffer = get_device().createFramebuffer( m_CreateInfo );

    if(!m_framebuffer)
    {
        ERROR << "Error creating frame buffer" << ENDL;
        throw std::runtime_error("Error creating frame buffer");
    }

}
