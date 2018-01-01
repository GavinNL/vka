#include <vka/core/framebuffer.h>
#include <vka/core/context.h>

vka::framebuffer::~framebuffer()
{
    if(m_framebuffer)
    {
        LOG << "Framebuffer destroyed" << ENDL;
        m_parent_context->get_device().destroyFramebuffer(m_framebuffer);
    }
}

void vka::framebuffer::create( vk::RenderPass render_pass,
                               vk::Extent2D extents,
                               vk::ImageView image_view,
                               vk::ImageView depth_image)
{
    vk::FramebufferCreateInfo C;

    std::vector<vk::ImageView> attachments;

    attachments.push_back(image_view);

    if( depth_image)
        attachments.push_back(depth_image);

    C.renderPass      = render_pass;
    C.attachmentCount = static_cast<uint32_t>(attachments.size());
    C.pAttachments    = attachments.data();
    C.width           = extents.width;
    C.height          = extents.height;
    C.layers          = 1;

    m_framebuffer = m_parent_context->get_device().createFramebuffer(C);
    if(!m_framebuffer)
    {
        ERROR << "Error creating frame buffer" << ENDL;
        throw std::runtime_error("Error creating frame buffer");
    }

}
